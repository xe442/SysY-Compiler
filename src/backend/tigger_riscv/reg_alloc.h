#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include <vector>
#include <list>
#include <deque>
#include <unordered_set>
#include "exceptions.h"
#include "visitor_helper.h"
#include "eeyore.h"
#include "tigger_printer.h"
#include "live_interval.h"
#include "cfg.h"

namespace compiler::backend::tigger
{

// Linear-scan register allocator.
class RegAllocator
{
  protected:
	using EeyoreCode = std::list<eeyore::EeyoreStatement>;
	using EeyoreStmtPtr = std::list<eeyore::EeyoreStatement>::const_iterator;

	class _RegisterManager
	{
	  protected:
		// All the registers.
		std::deque<CalleeSavedReg> _callee_saved_regs;
		std::deque<CallerSavedReg> _caller_saved_regs;

		// The free registers.
		std::deque<CalleeSavedReg> _free_callee_saved_regs;
		std::deque<CallerSavedReg> _free_caller_saved_regs;

		// Number of registers that must be used in this function.
		int _max_callee_saved_regs_use;
		int _max_caller_saved_regs_use;

	  public:
	  	_RegisterManager() = default;
		_RegisterManager(
			std::deque<CalleeSavedReg> callee_saved_regs,
			std::deque<CallerSavedReg> caller_saved_regs)
		{
			initialize(std::move(callee_saved_regs), std::move(caller_saved_regs));
		}

		inline void initialize(
			std::deque<CalleeSavedReg> callee_saved_regs,
			std::deque<CallerSavedReg> caller_saved_regs)
		{
			_callee_saved_regs = std::move(callee_saved_regs);
			_caller_saved_regs = std::move(caller_saved_regs);
			std::sort(_callee_saved_regs.begin(), _callee_saved_regs.end(),
				std::less<CalleeSavedReg>());
			std::sort(_caller_saved_regs.begin(), _caller_saved_regs.end(),
				std::less<CallerSavedReg>());
		}
	  	inline void reset()
		{
			_free_callee_saved_regs = _callee_saved_regs;
			_free_caller_saved_regs = _caller_saved_regs;
			_max_callee_saved_regs_use = 0;
			_max_caller_saved_regs_use = 0;
		}
		inline CalleeSavedReg callee_saved_reg(int idx) const
			{ return _callee_saved_regs.at(idx); }
		inline bool callee_saved_empty() const { return _free_callee_saved_regs.empty(); }
		inline bool caller_saved_empty() const { return _free_caller_saved_regs.empty();	}
		inline int max_callee_saved_use() const { return _max_callee_saved_regs_use; }
		inline int max_caller_saved_use() const { return _max_caller_saved_regs_use; }

	  protected:
		inline CalleeSavedReg _get_callee_saved()
		{
			INTERNAL_ASSERT(!_free_callee_saved_regs.empty(),
				"try to get a callee saved register from empty register pool");
			auto reg = _free_callee_saved_regs.front();
			_free_callee_saved_regs.pop_front();
			_max_callee_saved_regs_use = std::max<int>(_max_callee_saved_regs_use,
				_callee_saved_regs.size() - _free_callee_saved_regs.size());
			return reg;
		}
		inline CallerSavedReg _get_caller_saved()
		{
			INTERNAL_ASSERT(!_free_caller_saved_regs.empty(),
				"try to get a caller saved register from empty register pool");
			auto reg = _free_caller_saved_regs.front();
			_free_caller_saved_regs.pop_front();
			_max_caller_saved_regs_use = std::max<int>(_max_caller_saved_regs_use,
				_caller_saved_regs.size() - _free_caller_saved_regs.size());
			return reg;
		}

		void _return_callee_saved(CalleeSavedReg reg);
		void _return_caller_saved(CallerSavedReg reg);
		inline void _return_reg(Reg reg)
		{
			if(std::holds_alternative<CalleeSavedReg>(reg))
				_return_callee_saved(std::get<CalleeSavedReg>(reg));
			else if(std::holds_alternative<CallerSavedReg>(reg))
				_return_caller_saved(std::get<CallerSavedReg>(reg));
			else
				INTERNAL_ERROR("returning a register of error type to register pool");
		}

	  public:
		inline void get_callee_saved_for(LiveInterval &interv)
		{
			interv.reg = _get_callee_saved();
		}
		inline void get_caller_saved_for(LiveInterval &interv)
		{
			interv.reg = _get_caller_saved();
		}
		inline void return_reg_of(const LiveInterval &interv)
		{
			if(interv.reg.has_value())
				_return_reg(interv.reg.value());
			// We do not free pre_assigned_reg, since it is always an argument register.
		}
	};

	// Manages the stack size for spilled variables only.
	class _StackSizeManager
	{
	  protected:
		int _spill_stack_size;

	  public:
	  	inline void reset() { _spill_stack_size = 0; }
		inline int allocate(int size=1)
		{
			int res = _spill_stack_size;
			_spill_stack_size += size;
			return res;
		}

		inline int spill_size() const { return _spill_stack_size; }
	};

	_RegisterManager _regs;
	_StackSizeManager _stack;

  public:
	inline CalleeSavedReg callee_saved_reg(int idx) const
	{
		return _regs.callee_saved_reg(idx);
	}
	inline int callee_saved_reg_use_cnt() const
	{
		return _regs.max_callee_saved_use();
	}
	inline int func_stack_size() const
	{
		return _regs.max_callee_saved_use() + _stack.spill_size();
	}

  protected:
	// All the live intervals. For all the live intervals in a function, they
	// are sorted in increasing order of start point.
	std::vector<std::vector<LiveInterval>> _live_intervals;

	// The active intervals of the current function. Always sorted by endpoint.
	int _curr_func_id;
	std::deque<int> _active;

	// All the global variables, do not allocate registers for them.
	std::unordered_set<eeyore::Operand> _global_vars;

	void _add_to_active(int interv_id);

	// Functions used for initializing.
	void _calculate_local_live_sets(ControlFlowGraph &cfg);
	void _calculate_global_live_sets(ControlFlowGraph &cfg);
	void _build_intervals(const ControlFlowGraph &cfg);
	void _calculate_live_intervals(
		const EeyoreCode &eeyore_code, std::vector<eeyore::Operand> all_defined_var);

  public:
	void initialize(
		const EeyoreCode &eeyore_code,
		std::vector<eeyore::Operand> all_defined_var,
		std::vector<CalleeSavedReg> callee_saved,
		std::vector<CallerSavedReg> caller_saved);
	

	struct AllocationChange
	{
		Reg prev_pos;
		RegOrNum new_pos;
	};
  protected:
	void _expire_old_intervals(int stmt_id);
	void _spill_at_interval(LiveInterval &interv, int interv_id,
							std::vector<AllocationChange> &changes);

  public:
	std::vector<AllocationChange>
	allocate_for(const eeyore::EeyoreStatement &stmt, int stmt_id);

	inline const std::vector<std::vector<LiveInterval>> &live_intervals() const
		{ return _live_intervals; }

	// query functions.
	bool is_global_var(const eeyore::Operand &opr) const;
	std::optional<Reg> reg_of(const eeyore::Operand &opr) const;
	std::optional<int> stack_pos_of(const eeyore::Operand &opr) const;
	std::optional<RegOrNum> actual_pos_of(const eeyore::Operand &opr) const;
};

} // namespace compiler::backend::tigger

#endif