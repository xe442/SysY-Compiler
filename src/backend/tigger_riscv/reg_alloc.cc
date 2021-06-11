#define DEBUG
#include <algorithm>
#include "dbg.h"
#include "insertion_sort.h"
#include "bitmap.h"
#include "fstring.h"
#include "tigger_gen.h"

namespace compiler::backend::tigger
{

void RegAllocator::_RegisterManager::_return_callee_saved(CalleeSavedReg reg)
{
	utils::insertion_sort(_free_callee_saved_regs, reg, std::less<CalleeSavedReg>());
}
void RegAllocator::_RegisterManager::_return_caller_saved(CallerSavedReg reg)
{
	utils::insertion_sort(_free_caller_saved_regs, reg, std::less<CallerSavedReg>());
}

// ref: https://www.zhihu.com/question/29355187/answer/99413526
//		https://dl.acm.org/doi/pdf/10.1145/330249.330250
//      https://engineering.purdue.edu/~milind/ece573/2014fall/project/step7/step7.html

void RegAllocator::_add_to_active(int interv_id)
{
	utils::insertion_sort(_active, interv_id,
		[this](const int id1, const int id2)
		{
			// Keep _active sorted by increasing order of back_stmt_id.
			return _live_intervals[_curr_func_id][id1].back_stmt_id
				< _live_intervals[_curr_func_id][id2].back_stmt_id;
		}
	);
}

void RegAllocator::_calculate_local_live_sets(ControlFlowGraph &cfg)
{
	const auto &var_to_idx = cfg.defined_var_idx_map();
	DBG(
		std::cout << "var -> idx mapping" << std::endl;
		for(const auto &p : var_to_idx)
			std::cout << p.first << ' ' << p.second << std::endl;
	)

	// pre-calculate the bitmap for all the global variables.
	utils::Bitmap global_var_bitmap(cfg.defined_var().size());
	for(const auto &global_var : cfg.global_vars())
		global_var_bitmap.set(var_to_idx.at(global_var));

	// Calculate live_gen and live_kill for all basic blocks.
	for(auto &v : cfg.all_vertices_())
	{
		EeyoreStmtPtr iter = v.begin;
		int stmt_id = v.begin_stmt_id;
		for( ; stmt_id != v.end_stmt_id; ++iter, ++stmt_id)
		{
			if(std::holds_alternative<eeyore::FuncCallStmt>(*iter))
			{
				// For function call: kill(stmt) = all global vars. Since it may
				// re-define all global variables.
				// gen(stmt) = empty.
				v.live_kill.union_with(global_var_bitmap);
			}
			else
			{
				std::vector<eeyore::Operand> used_vars = eeyore::used_vars(*iter),
											 def_vars = eeyore::defined_vars(*iter);
				for(const auto &var : used_vars)
				{
					int idx = var_to_idx.at(var);
					if(!v.live_kill.get(idx))
						v.live_gen.set(idx);
				}
				for(const auto &var : def_vars)
					v.live_kill.set(var_to_idx.at(var));
			}
		}
	}

	DBG(
		for(const auto &v : cfg.all_vertices())
		{
			std::cout << "block #" << v.id << std::endl;

			std::cout << "live kill: ";
			for(int i = 0; i < v.live_kill.size(); i++)
				if(v.live_kill.get(i))
					std::cout << cfg.defined_var(i) << ' ';
			std::cout << std::endl;

			std::cout << "live gen: ";
			for(int i = 0; i < v.live_gen.size(); i++)
				if(v.live_gen.get(i))
					std::cout << cfg.defined_var(i) << ' ';
			std::cout << std::endl << std::endl;
		}
	);
}

void RegAllocator::_calculate_global_live_sets(ControlFlowGraph &cfg)
{
	const int N = cfg.vertex_cnt();
	while(true)
	{
		bool changed = false;
		for(int i = N - 1; i >= 0; i--)
		{
			auto &v = cfg.vertex_(i);
			
			// v.live_out = union{sux.live_in} for sux in succesor(v)
			auto prev_live_out_cnt = v.live_out.cnt();
			v.live_out.clear();
			for(int bid : cfg.successor_ids(i))
			{
				auto &sux = cfg.vertex(bid);
				v.live_out.union_with(sux.live_in);
			}
			changed = changed || (v.live_out.cnt() != prev_live_out_cnt);

			// v.live_in = (v.live_out - v.live_kill) union v.live_gen
			auto prev_live_in_cnt = v.live_in.cnt();
			v.live_in.clear();
			v.live_in.union_with(v.live_out);
			v.live_in.diff_with(v.live_kill);
			v.live_in.union_with(v.live_gen);
			changed = changed || (v.live_in.cnt() != prev_live_in_cnt);
		}
		if(!changed)
			break;
	}
	DBG(
		for(const auto &v : cfg.all_vertices())
		{
			std::cout << "block #" << v.id << std::endl;

			std::cout << "live in: ";
			for(int i = 0; i < v.live_in.size(); i++)
				if(v.live_in.get(i))
					std::cout << cfg.defined_var(i) << ' ';
			std::cout << std::endl;

			std::cout << "live out: ";
			for(int i = 0; i < v.live_out.size(); i++)
				if(v.live_out.get(i))
					std::cout << cfg.defined_var(i) << ' ';
			std::cout << std::endl << std::endl;
		}
	);
}

void RegAllocator::_build_intervals(const ControlFlowGraph &cfg)
{
	const std::vector<int> &func_starts = cfg.func_start_id();
	
	// Build live interval for each function.
	for(int i = 0; i < func_starts.size(); i++)
	{
		// Find the start and end block idx.
		int func_start = func_starts[i];
		int func_end;
		if(i == func_starts.size() - 1)
			func_end = cfg.vertex_cnt();
		else
			func_end = func_starts[i + 1];
		DBG(std::cout << "block of func #" << i << ": [" << func_start
			<< ", " << func_end - 1 << ']' << std::endl);
		
		// The live interval of all variables in the current function, stored in
		// the hash map temporarily and are further transfered to _live_intervals.
		// Since operands are already stored in the key, the opr field of LiveInterval
		// is not set.
		std::unordered_map<eeyore::Operand, LiveInterval> var_live_interval_map;

		std::vector<int> func_call_id; // All the function call statement id.

		for(int j = func_end - 1; j >= func_start; j--)
		{
			const auto &block = cfg.vertex(j);
			int block_from = block.begin_stmt_id;
			int block_to = block.back_stmt_id();

			for(int k = 0; k < block.live_out.size(); k++)
				if(block.live_out.get(k))
					var_live_interval_map[cfg.defined_var(k)].add_range(block_from, block_to);

			auto iter = block.back();
			int stmt_id = block.back_stmt_id();
			for( ; stmt_id >= block.begin_stmt_id; --iter, --stmt_id)
			{
				if(std::holds_alternative<eeyore::FuncCallStmt>(*iter))
				{
					func_call_id.push_back(stmt_id);
					for(const auto &opr : cfg.global_vars())
					{
						DBG(std::cout << "func call may define " << opr << " at line " << stmt_id + 1 << std::endl);
						var_live_interval_map[opr].set_begin(stmt_id);
					}
				}
				else
				{
					for(const auto &opr : eeyore::defined_vars(*iter))
					{
						DBG(std::cout << "defined " << opr << " at line " << stmt_id + 1 << std::endl);
						var_live_interval_map[opr].set_begin(stmt_id);
					}
					for(const auto &opr : eeyore::used_vars(*iter))
					{
						DBG(std::cout << "used " << opr << " at line " << stmt_id + 1 << std::endl);
						var_live_interval_map[opr].add_range(block_from, stmt_id);
					}
				}
			}
		}

		std::vector<LiveInterval> func_live_intervals;
		for(auto &var_interv_pair : var_live_interval_map)
		{
			var_interv_pair.second.opr = var_interv_pair.first;
			func_live_intervals.push_back(var_interv_pair.second);
		}
		std::sort(func_live_intervals.begin(), func_live_intervals.end(),
			LiveInterval::StartPointLessCmp());
		
		// Check if there is a func_call inside each live interval.
		for(LiveInterval &interv : func_live_intervals)
		{
			auto find_res = std::lower_bound(func_call_id.begin(), func_call_id.end(),
				interv.back_stmt_id, std::greater<int>());
			interv.cross_func_call =
				find_res != func_call_id.end() && *find_res >= interv.begin_stmt_id;
		}
		DBG(
			for(LiveInterval &interv : func_live_intervals)
			{
				std::cout << interv.opr << ": ["
					<< interv.begin_stmt_id + 1 << ' '
					<< interv.back_stmt_id + 1 << "], cross func: "
					<< std::boolalpha << interv.cross_func_call << ", "
					<< "size = " << (std::holds_alternative<eeyore::OrigVar>(interv.opr)? std::get<eeyore::OrigVar>(interv.opr).size : 0) << std::endl;
			}
		);
		_live_intervals.push_back(std::move(func_live_intervals));
	}
	
	DBG(std::cout << "end live interval construction" << std::endl);
}

void RegAllocator::_calculate_live_intervals(
	const EeyoreCode &eeyore_code, std::vector<eeyore::Operand> all_defined_var)
{
	ControlFlowGraph cfg(eeyore_code, std::move(all_defined_var));
	_global_vars.insert(cfg.global_vars().begin(), cfg.global_vars().end());
	_calculate_local_live_sets(cfg);
	_calculate_global_live_sets(cfg);
	_build_intervals(cfg);
}

void RegAllocator::initialize(
	const EeyoreCode &eeyore_code,
	std::vector<eeyore::Operand> all_defined_var,
	std::vector<CalleeSavedReg> callee_saved,
	std::vector<CallerSavedReg> caller_saved)
{
	_curr_func_id = 0;

	DBG(
		std::cout << "configuring reg alloc" << std::endl;
		std::cout << "callee saved: ";
		for(Reg reg : callee_saved)
			std::cout << reg << ' ';
		std::cout << std::endl << "caller saved: ";
		for(Reg reg : caller_saved)
			std::cout << reg << ' ';
		std::cout << std::endl;
	);
	_regs.initialize(
		std::deque<CalleeSavedReg>(callee_saved.begin(), callee_saved.end()),
		std::deque<CallerSavedReg>(caller_saved.begin(), caller_saved.end()));
	_calculate_live_intervals(eeyore_code, std::move(all_defined_var));
}

void RegAllocator::_expire_old_intervals(int stmt_id)
{
	while(!_active.empty())
	{
		const auto &first_interval = _live_intervals[_curr_func_id][_active.front()];
		if(first_interval.back_stmt_id >= stmt_id)
			break;
		_regs.return_reg_of(first_interval);
		_active.pop_front();
	}
}

void RegAllocator::_spill_at_interval(
	LiveInterval &interv, int interv_id,
	std::vector<AllocationChange> &changes)
{
	// Check which operand we will have to spill. We always try to replace
	// the operand with the last interval endpoint.
	auto &spill = _live_intervals[_curr_func_id][_active.back()];
	if(interv.back_stmt_id <= spill.back_stmt_id) // do spill
	{
		INTERNAL_ASSERT(spill.reg.has_value(),
			"found an active variable not assigned a register");
		interv.reg = spill.reg;
		spill.stack_loc = _stack.allocate();

		if(interv.pre_assigned_reg.has_value())
			changes.push_back({interv.pre_assigned_reg.value(), interv.reg.value()});
		changes.push_back({spill.reg.value(), spill.stack_loc.value()});

		_active.pop_back();
		_add_to_active(interv_id);
	}
	else // simply spill the new variable to memory
	{
		interv.stack_loc = _stack.allocate();
		if(interv.pre_assigned_reg.has_value())
			changes.push_back({interv.pre_assigned_reg.value(), interv.stack_loc.value()});
	}
}

// This functions returns all the position changes after allocation, so that
// TiggerGenerator can generate corresponding load stmt to save the save the
// variables.
// Position changes can be:
//   Reg -> Reg, which means a parameter should be saved in a callee saved reg.
//   Reg -> stack, which means a variable is spilled to memory.
std::vector<RegAllocator::AllocationChange>
RegAllocator::allocate_for(const eeyore::EeyoreStatement &stmt, int stmt_id)
{
	static int live_interv_id = 0;

	DBG(std::cout << "allocate register for " << stmt);
	std::vector<AllocationChange> changes;

	_expire_old_intervals(stmt_id);
	if(std::holds_alternative<eeyore::FuncDefStmt>(stmt))
	{
		_active.clear();
		_regs.reset();
		_stack.reset();
		live_interv_id = 0;
	}
	else if(std::holds_alternative<eeyore::EndFuncDefStmt>(stmt))
	{
		_curr_func_id++;
	}
	else
	{
		for( ;
			live_interv_id < _live_intervals.at(_curr_func_id).size()
				&& _live_intervals.at(_curr_func_id).at(live_interv_id).begin_stmt_id <= stmt_id;
			live_interv_id++)
		{
			auto &interv = _live_intervals.at(_curr_func_id).at(live_interv_id);

			// Check if this is an argument.
			if(std::holds_alternative<eeyore::Param>(interv.opr))
			{
				interv.pre_assigned_reg = ArgReg(std::get<eeyore::Param>(interv.opr).id);
				if(!interv.cross_func_call) // No need to assign extra registers.
					continue;
			}

			// Check if this in a global variable. We do not allocate registers
			// for global variables. Also check if this is an empty interval.
			if(is_global_var(interv.opr) || interv.empty())
				continue;

			// Check if this is an array. If so, we have to spill.
			if(std::holds_alternative<eeyore::OrigVar>(interv.opr))
			{
				auto orig_var = std::get<eeyore::OrigVar>(interv.opr);
				if(orig_var.size > sizeof(int))
				{
					interv.stack_loc = _stack.allocate(orig_var.size / sizeof(int));
					continue;
				}
			}

			if(interv.cross_func_call)
			{
				if(!_regs.callee_saved_empty())
				{
					DBG(std::cout << "getting s reg for " << interv.opr << std::endl);
					_regs.get_callee_saved_for(interv);
					_add_to_active(live_interv_id);

					if(interv.pre_assigned_reg.has_value())
						changes.push_back({interv.pre_assigned_reg.value(), interv.reg.value()});
				}
				else
					_spill_at_interval(interv, live_interv_id, changes);
			}
			else
			{
				if(!_regs.caller_saved_empty())
				{
					DBG(std::cout << "getting t reg for " << interv.opr << std::endl);
					_regs.get_caller_saved_for(interv);
					_add_to_active(live_interv_id);

					if(interv.pre_assigned_reg.has_value())
						changes.push_back({interv.pre_assigned_reg.value(), interv.reg.value()});
				}
				else if(!_regs.callee_saved_empty())
				{
					DBG(std::cout << "getting s reg for " << interv.opr << std::endl);
					_regs.get_callee_saved_for(interv);
					_add_to_active(live_interv_id);

					if(interv.pre_assigned_reg.has_value())
						changes.push_back({interv.pre_assigned_reg.value(), interv.reg.value()});
				}
				else
					_spill_at_interval(interv, live_interv_id, changes);
			}
		}

		DBG(
			std::cout << "after allocation: " << std::endl;
			std::cout << "active variables: " << std::endl;
			for(int active_idx : _active)
			{
				const auto &interv = _live_intervals.at(_curr_func_id).at(active_idx);
				std::cout << interv.opr << " -> " << interv.reg.value() << std::endl;
			}
			std::cout << "all variables in current function: " << std::endl;
			for(int i = 0; i < live_interv_id; i++)
			{
				const auto &interv = _live_intervals.at(_curr_func_id).at(i);
				std::cout << interv.opr << ": reg ";
				if(!interv.reg.has_value() && !interv.pre_assigned_reg.has_value())
					std::cout << "not allocated";
				else
				{
					std::cout << "= ";
					if(interv.reg.has_value() && interv.pre_assigned_reg.has_value())
						std::cout << interv.reg.value() << '/' << interv.pre_assigned_reg.value();
					else if(interv.reg.has_value())
						std::cout << interv.reg.value();
					else
						std::cout << interv.pre_assigned_reg.value();
				}
				std::cout << ", stack loc ";
				if(interv.stack_loc.has_value())
					std::cout << "= " << interv.stack_loc.value();
				else
					std::cout << "not allocated";
				std::cout << std::endl;
			}
			std::cout << std::endl;
		);
	}
	return changes;
}

bool RegAllocator::is_global_var(const eeyore::Operand &opr) const
{
	return _global_vars.find(opr) != _global_vars.end();
}

// The actual stored register of an operand.
std::optional<Reg> RegAllocator::reg_of(const eeyore::Operand &opr) const
{
	for(const LiveInterval &interv : _live_intervals.at(_curr_func_id))
		if(interv.opr == opr)
		{
			if(interv.reg.has_value())
				return interv.reg;
			return interv.pre_assigned_reg;
		}
	return std::nullopt;
}

// The stack position of an operand.
std::optional<int> RegAllocator::stack_pos_of(const eeyore::Operand &opr) const
{
	// TODO: This is a O(n) algorithm. Optimize it later.
	for(const LiveInterval &interv : _live_intervals.at(_curr_func_id))
		if(interv.opr == opr)
			return interv.stack_loc;
	return std::nullopt;
}

// The (current) actual position of an operand, can be either in a register or
// in the stack.
std::optional<RegOrNum> RegAllocator::actual_pos_of(const eeyore::Operand &opr) const
{
	const auto &active_intervs = _live_intervals.at(_curr_func_id);
	for(int active_idx : _active)
	{
		const LiveInterval &interv = active_intervs.at(active_idx);
		if(interv.opr == opr)
		{
			INTERNAL_ASSERT(interv.reg.has_value(),
				"found an active operand not assigned a register");
			return interv.reg.value();
		}
	}

	// We have to check if the operand is stored in argument register, since
	// active in an argument register is not considered as "active".
	// This is bad, TODO: change it later.
	auto reg = reg_of(opr);
	if(reg.has_value() && std::holds_alternative<ArgReg>(reg.value()))
		return reg.value();
	
	// The operand must be stored in the stack.
	auto stack_pos = stack_pos_of(opr);
	if(stack_pos.has_value())
		return stack_pos.value();
	else
		return std::nullopt;
}

}