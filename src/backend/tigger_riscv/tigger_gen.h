#ifndef TIGGER_GEN_H
#define TIGGER_GEN_H

#include "insertion_sort.h"
#include "eeyore_gen.h"
#include "reg_alloc.h"
#include "cfg.h"
#include "live_interval.h"

namespace compiler::backend::tigger
{

class TiggerGenerator
{
	const std::list<eeyore::EeyoreStatement> &_eeyore_code;
	RegAllocator _allocator;
	std::list<TiggerStatement> _tigger_code;
	std::list<TiggerStatement>::iterator _func_start;
	std::vector<std::list<TiggerStatement>::iterator> _return_stmt_pos;

	bool _is_global;
	int _param_id;
	int _func_id;
	int _eeyore_stmt_id;

	class _TempRegManager
	{
	  protected:
		std::deque<CallerSavedReg> _free_temp_regs;
		std::deque<CallerSavedReg> _allocated_temp_regs;

	  public:
		inline void set_temp(std::deque<CallerSavedReg> temp_regs)
		{
			_free_temp_regs = std::move(temp_regs);
			_allocated_temp_regs.clear();
		}

		inline CallerSavedReg get_temp()
		{
			INTERNAL_ASSERT(!_free_temp_regs.empty(), "no free temp register");
			auto reg = _free_temp_regs.front();
			_free_temp_regs.pop_front();
			_allocated_temp_regs.push_back(reg);
			return reg;
		}

		inline void return_temp(CallerSavedReg reg)
		{
			auto find_pos = std::find(_allocated_temp_regs.begin(), _allocated_temp_regs.end(), reg);
			INTERNAL_ASSERT(find_pos != _allocated_temp_regs.end(),
				"returning an invalid register to temp register pool");
			_allocated_temp_regs.erase(find_pos);
			utils::insertion_sort(_free_temp_regs, reg);
		}
		inline bool try_return_temp(Reg reg)
		{
			if(!std::holds_alternative<CallerSavedReg>(reg))
				return false;
			
			auto _reg = std::get<CallerSavedReg>(reg);
			auto find_pos = std::find(_allocated_temp_regs.begin(), _allocated_temp_regs.end(), _reg);
			if(find_pos != _allocated_temp_regs.end())
			{
				_allocated_temp_regs.erase(find_pos);
				utils::insertion_sort(_free_temp_regs, _reg);
				return true;
			}
			return false;
		}

		inline bool is_temp(Reg reg) const
		{
			if(!std::holds_alternative<CallerSavedReg>(reg))
				return false;
			return std::find(_allocated_temp_regs.begin(), _allocated_temp_regs.end(),
				std::get<CallerSavedReg>(reg)) != _allocated_temp_regs.end();
		}
		void reset();
	};
	_TempRegManager _temp_regs;

	inline bool _is_global_var(eeyore::Operand opr) const
		{ return _allocator.is_global_var(opr); }
	bool _is_useless_stmt(const eeyore::EeyoreStatement &stmt) const;
	Reg _read_opr(eeyore::Operand opr);
	Reg _read_opr_addr(eeyore::Operand opr);
	
  public:
	TiggerGenerator(
		const std::list<eeyore::EeyoreStatement> &eeyore_code,
		std::vector<eeyore::Operand> all_vars);
	const std::list<TiggerStatement> &generate_tigger();

	void operator() (const eeyore::DeclStmt &stmt);
	void operator() (const eeyore::FuncDefStmt &stmt);
	void operator() (const eeyore::EndFuncDefStmt &stmt);
	void operator() (const eeyore::ParamStmt &stmt);
	void operator() (const eeyore::FuncCallStmt &stmt);
	void operator() (const eeyore::RetStmt &stmt);
	void operator() (const eeyore::GotoStmt &stmt);
	void operator() (const eeyore::CondGotoStmt &stmt);
	void operator() (const eeyore::UnaryOpStmt &stmt);
	void operator() (const eeyore::BinaryOpStmt &stmt);
	void operator() (const eeyore::MoveStmt &stmt);
	void operator() (const eeyore::ReadArrStmt &stmt);
	void operator() (const eeyore::WriteArrStmt &stmt);
	void operator() (const eeyore::LabelStmt &stmt);
};

} // namespace compiler::backend::tigger

#endif