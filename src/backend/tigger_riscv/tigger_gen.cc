#define DEBUG
#include "dbg.h"
#include "fstring.h"
#include "visitor_helper.h"
#include "tigger_gen.h"

using std::holds_alternative;

namespace compiler::backend::tigger
{

void TiggerGenerator::_TempRegManager::reset()
{
	while(!_allocated_temp_regs.empty())
	{
		_free_temp_regs.push_back(_allocated_temp_regs.front());
		_allocated_temp_regs.pop_front();
	}
}

Reg TiggerGenerator::_read_opr(eeyore::Operand opr)
{
	if(std::holds_alternative<int>(opr))
	{
		Reg tmp_reg = _temp_regs.get_temp();
		_tigger_code.emplace_back(MoveStmt(tmp_reg, std::get<int>(opr)));
		return tmp_reg;
	}
	else
	{
		auto actual_pos = _allocator.actual_pos_of(opr);
		if(!actual_pos.has_value()) // global variable
		{
			INTERNAL_ASSERT(_is_global_var(opr), utils::fstring("invalid position of opr ", opr));
			auto var = std::get<eeyore::OrigVar>(opr);
			Reg tmp_reg = _temp_regs.get_temp();
			_tigger_code.emplace_back(LoadStmt(tmp_reg, GlobalVar(var.id)));
			return tmp_reg;
		}
		else if(holds_alternative<Reg>(actual_pos.value()))
		{
			return std::get<Reg>(actual_pos.value());
		}
		else // in stack
		{
			Reg tmp_reg = _temp_regs.get_temp();
			_tigger_code.emplace_back(LoadStmt(tmp_reg, std::get<int>(actual_pos.value())));
			return tmp_reg;
		}
	}
}

Reg TiggerGenerator::_read_opr_addr(eeyore::Operand opr)
{
	INTERNAL_ASSERT(std::holds_alternative<eeyore::OrigVar>(opr),
		"temp variables does not have an address");
	auto var = std::get<eeyore::OrigVar>(opr);
	INTERNAL_ASSERT(var.size > sizeof(int), "expected array type for _read_opr_addr, but got int");
	INTERNAL_ASSERT(_is_global_var(opr), "non-global variables does not have an address");
	
	Reg tmp_reg = _temp_regs.get_temp();
	_tigger_code.emplace_back(LoadAddrStmt(tmp_reg, GlobalVar(var.id)));
	return tmp_reg;
}

TiggerGenerator::TiggerGenerator(
	const std::list<eeyore::EeyoreStatement> &code,
	std::vector<eeyore::Operand> all_vars
) : _eeyore_code(code)
{
	auto _free_callee_saved_reg = ALL_CALLEE_SAVED_REG;

	// Reserve 2 caller saved register to store temp values.
	// t0 is completely not used in tigger, and is used as scratch register
	// in Risc-V.
	auto _free_caller_saved_reg = std::vector<CallerSavedReg>(
		ALL_CALLER_SAVED_REG.begin() + 3, ALL_CALLER_SAVED_REG.end());
	auto _free_temp_regs = std::vector<CallerSavedReg>(
		ALL_CALLER_SAVED_REG.begin() + 1, ALL_CALLER_SAVED_REG.begin() + 3
	);

	_allocator.initialize(
		code, std::move(all_vars),
		std::move(_free_callee_saved_reg), std::move(_free_caller_saved_reg)
	);
	_temp_regs.set_temp(std::deque<CallerSavedReg>(
		_free_temp_regs.begin(), _free_temp_regs.end()
	));

	DBG(std::cout << "end tigger gen construction" << std::endl);
}

const std::list<TiggerStatement> &TiggerGenerator::generate_tigger()
{
	DBG(std::cout << "start allocating" << std::endl);
	_is_global = true;
	_func_id = 0;
	_param_id = 0;
	_eeyore_stmt_id = 0;

	for(const auto &stmt : _eeyore_code)
	{
		auto alloc_changes = _allocator.allocate_for(stmt, _eeyore_stmt_id);
		for(const auto &change : alloc_changes)
		{
			if(holds_alternative<Reg>(change.new_pos))
				_tigger_code.emplace_back(MoveStmt(std::get<Reg>(change.new_pos), change.prev_pos));
			else // holds_alternative<int>(change.new_pos)
				_tigger_code.emplace_back(StoreStmt(std::get<int>(change.new_pos), change.prev_pos));
		}

		std::visit(*this, stmt);
		_temp_regs.reset();
		_eeyore_stmt_id++;
	}
	DBG(std::cout << "end allocating" << std::endl);
	return _tigger_code;
}

// Only global declaration is translated.
// var T0      -->   v0 = 0  // The initial value is always zero.
// var 40 T0   -->   v1 = malloc 40
void TiggerGenerator::operator() (const eeyore::DeclStmt &stmt)
{
	if(_is_global)
	{
		INTERNAL_ASSERT(holds_alternative<eeyore::OrigVar>(stmt.var),
			"Error global definition");
		auto var = std::get<eeyore::OrigVar>(stmt.var);
		if(var.size == sizeof(int))
			_tigger_code.emplace_back(GlobalVarDeclStmt(var.id));
		else
			_tigger_code.emplace_back(GlobalArrDeclStmt(var.id, var.size));
	}
	// Local declaration statements corresponds to no tigger statements.
}

// f_func [2]   -->   f_func [2] [stack_size]
void TiggerGenerator::operator() (const eeyore::FuncDefStmt &stmt)
{
	_is_global = false;
	_tigger_code.emplace_back(FuncHeaderStmt(stmt.func_name, stmt.arg_cnt));
	_func_start = std::prev(_tigger_code.end());
	_return_stmt_pos.clear();
	// Function stack size and register storing is done when the function ends.

	// Stores parameter in callee saved registers.
	for(int i = 0; i < stmt.arg_cnt; i++)
	{
		auto arg_reg = _allocator.reg_of(eeyore::Param(i));
		if(arg_reg.has_value() && !holds_alternative<ArgReg>(arg_reg.value()))
			_tigger_code.emplace_back(MoveStmt(arg_reg.value(), ArgReg(i)));
	}
}

// Same as eeyore.
void TiggerGenerator::operator() (const eeyore::EndFuncDefStmt &stmt)
{
	int stack_size = _allocator.func_stack_size();
	std::get<FuncHeaderStmt>(*_func_start).stack_size = stack_size;

	// Add all the register store statements at the beginning of the function.
	for(int i = 0; i < _allocator.callee_saved_reg_use_cnt(); i++)
		_func_start = _tigger_code.emplace(++_func_start,
			StoreStmt(stack_size - 1 - i, _allocator.callee_saved_reg(i)));
	// Then add restore statments before every return statement.
	for(auto ret_stmt_pos : _return_stmt_pos)
		for(int i = 0; i < _allocator.callee_saved_reg_use_cnt(); i++)
			ret_stmt_pos = _tigger_code.emplace(ret_stmt_pos,
				LoadStmt(_allocator.callee_saved_reg(i), stack_size - 1 - i));

	_tigger_code.emplace_back(FuncEndStmt(stmt.func_name));
	_is_global = true;
}

// If t0 is an immediate number:
// param T0   -->   aX = t0
// or if t0 is a global variable:
//            -->   load var(t0) aX
// or if t0 is a global array:
//            -->   loadaddr var(t0) aX
// or if t0 is in a register:
//            -->   aX = reg(t0)
// or if t0 is a variable in the stack:
//            -->   load loc(t0) aX
// or if t0 is an array in the stack:
//            -->   loadaddr loc(t0) aX
void TiggerGenerator::operator() (const eeyore::ParamStmt &stmt)
{
	if(holds_alternative<int>(stmt.param))
	{
		int param_val = std::get<int>(stmt.param);
		_tigger_code.emplace_back(MoveStmt(ArgReg(_param_id), param_val));
	}
	else if(_is_global_var(stmt.param))
	{
		auto var = std::get<eeyore::OrigVar>(stmt.param);
		if(var.size == sizeof(int))
			_tigger_code.emplace_back(LoadStmt(ArgReg(_param_id), GlobalVar(var.id)));
		else
			_tigger_code.emplace_back(LoadAddrStmt(ArgReg(_param_id), GlobalVar(var.id)));
	}
	else
	{
		auto pos = _allocator.actual_pos_of(stmt.param);
		INTERNAL_ASSERT(pos.has_value(),
			utils::fstring("invalid position of variable ", stmt.param));
		if(holds_alternative<Reg>(pos.value()))
		{
			Reg reg = std::get<Reg>(pos.value());
			_tigger_code.emplace_back(MoveStmt(ArgReg(_param_id), reg));
		}
		else
		{
			int stack_pos = std::get<int>(pos.value());
			if(std::holds_alternative<eeyore::OrigVar>(stmt.param)
				&& std::get<eeyore::OrigVar>(stmt.param).size != sizeof(int))
			{
				_tigger_code.emplace_back(LoadAddrStmt(ArgReg(_param_id), stack_pos));
			}
			else
				_tigger_code.emplace_back(LoadStmt(ArgReg(_param_id), stack_pos));
		}
	}
	_param_id++;
}

// call f_func        -->   call f_func
// If T0 is global variable:
// T0 = call f_func   -->   call f_func
//                          loadaddr var(T0) tmp_reg
//                          tmp_reg[0] = a0
// or if T0 is in a register:
//                          call f_func
//                          reg(T0) = a0
// or if T0 is in stack:
//                    -->   call f_func
//                          store a0 loc(T0)
void TiggerGenerator::operator() (const eeyore::FuncCallStmt &stmt)
{
	_param_id = 0;
	_tigger_code.emplace_back(FuncCallStmt(stmt.func_name));
	if(stmt.retval_receiver.has_value())
	{
		if(_is_global_var(stmt.retval_receiver.value()))
		{
			auto var = std::get<eeyore::OrigVar>(stmt.retval_receiver.value());
			auto tmp_reg = _temp_regs.get_temp();
			_tigger_code.emplace_back(LoadAddrStmt(tmp_reg, GlobalVar(var.id)));
			_tigger_code.emplace_back(WriteArrStmt(tmp_reg, 0, ArgReg(0)));
		}
		else
		{
			auto pos = _allocator.actual_pos_of(stmt.retval_receiver.value());
			if(!pos.has_value()) // The retval is written, but never used.
							// In this case we simply discard the retval.
				return;
			
			if(holds_alternative<Reg>(pos.value())) // in register
			{
				auto reg = std::get<Reg>(pos.value());
				_tigger_code.emplace_back(MoveStmt(reg, ArgReg(0)));
			}
			else // in memory
			{
				auto stack_pos = std::get<int>(pos.value());
				_tigger_code.emplace_back(StoreStmt(stack_pos, ArgReg(0)));
			}
		}
	}
}

// return      -->   return
// If t0 is an immediate number:
// return T0   -->   a0 = T0
//                   return
// or if t0 is a global variable:
//             -->   load var(T0) a0
//                   return
// or if t0 is in a register:
//             -->   a0 = reg(T0)
//                   return
// or if t0 is in stack:
//             -->   load loc(T0) a0
//                   return
void TiggerGenerator::operator() (const eeyore::RetStmt &stmt)
{
	if(stmt.retval.has_value())
	{
		if(holds_alternative<int>(stmt.retval.value()))
		{
			int retval = std::get<int>(stmt.retval.value());
			_tigger_code.emplace_back(MoveStmt(ArgReg(0), retval));
		}
		else if(_is_global_var(stmt.retval.value()))
		{
			auto var = std::get<eeyore::OrigVar>(stmt.retval.value());
			_tigger_code.emplace_back(LoadStmt(ArgReg(0), GlobalVar(var.id)));
		}
		else
		{
			auto pos = _allocator.actual_pos_of(stmt.retval.value());
			INTERNAL_ASSERT(pos.has_value(),
				utils::fstring("invalid opr position in stmt ", stmt));
			if(holds_alternative<Reg>(pos.value()))
			{
				Reg reg = std::get<Reg>(pos.value());
				_tigger_code.emplace_back(MoveStmt(ArgReg(0), reg));
			}
			else
			{
				int stack_pos = std::get<int>(pos.value());
				_tigger_code.emplace_back(LoadStmt(ArgReg(0), stack_pos));
			}
		}
	}
	_tigger_code.emplace_back(ReturnStmt());
	_return_stmt_pos.push_back(std::prev(_tigger_code.end()));
}

// Same as eeyore.
void TiggerGenerator::operator() (const eeyore::GotoStmt &stmt)
{
	_tigger_code.emplace_back(GotoStmt(stmt.goto_label));
}

// if T1 BinOp T2 goto l1   -->   (load T1 if T1 is not in a register)
//                                (load T2 if T2 is not in a register)
//                                if reg(T1) BinOp reg(T2) goto l1
void TiggerGenerator::operator() (const eeyore::CondGotoStmt &stmt)
{
	Reg reg1 = _read_opr(stmt.opr1);
	Reg reg2 = _read_opr(stmt.opr2);
	_tigger_code.emplace_back(CondGotoStmt(reg1, stmt.op, reg2, stmt.goto_label));
}

// If T0 is a global variable:
// T0 = UniOp T1   -->   (load T1 if T1 is not in a register)
//                       tmp_reg1 = UniOp reg(T1)
//                       loadaddr var(T0) tmp_reg2
//                       tmp_reg2[0] = tmp_reg1
// or if T0 is in a register:
//                 -->   (load T1 if T1 is not in a register)
//                       reg(T0) = UniOp reg(T1)
// or if T0 is in stack:
//                 -->   (load T1 if T1 is not in a register)
//                       tmp_reg = UniOp reg(T1)
//                       store tmp_reg loc(T0)
// Maximum temporary register used: 2.
void TiggerGenerator::operator() (const eeyore::UnaryOpStmt &stmt)
{
	Reg reg1 = _read_opr(stmt.opr1);
	if(_is_global_var(stmt.opr))
	{
		auto var = std::get<eeyore::OrigVar>(stmt.opr);
		auto tmp_reg = _temp_regs.get_temp();
		auto tmp_reg2 = _temp_regs.get_temp();
		_tigger_code.emplace_back(UnaryOpStmt(tmp_reg, stmt.op_type, reg1));
		_tigger_code.emplace_back(LoadAddrStmt(tmp_reg2, GlobalVar(var.id)));
		_tigger_code.emplace_back(WriteArrStmt(tmp_reg2, 0, tmp_reg));
	}
	else
	{
		auto pos = _allocator.actual_pos_of(stmt.opr);
		if(!pos.has_value()) // The calculation is performed, but its result is
						// never used. In this case we do not generate code for it.
			return;
		if(holds_alternative<Reg>(pos.value()))
		{
			auto reg = std::get<Reg>(pos.value());
			_tigger_code.emplace_back(UnaryOpStmt(reg, stmt.op_type, reg1));
		}
		else
		{
			auto stack_pos = std::get<int>(pos.value());
			auto tmp_reg = _temp_regs.get_temp();
			_tigger_code.emplace_back(UnaryOpStmt(tmp_reg, stmt.op_type, reg1));
			_tigger_code.emplace_back(StoreStmt(stack_pos, tmp_reg));
		}
	}
}

// If T0 is a global variable:
// T0 = T1 BinOp T2   -->   (load T1 if T1 is not in a register)
//                          (load T2 if T2 is not in a register)
//                          tmp_reg = reg(T1) BinOp reg(T2) // We have to reuse one of the registers
//                                                 // if both reg(T1) and reg(T2) are temp registers.
//                          (free the unused T1 or T2 if it is an unused temp register)
//                          loadaddr T0 tmp_reg2
//                          tmp_reg2[0] = tmp_reg
// or if T0 is in a register:
//                    -->   (load T1 if T1 is not in a register)
//                          (load T2 if T2 is not in a register)
//                          reg(T0) = reg(T1) BinOp reg(T2)
// or if T0 is in stack:
//                    -->   (load T1 if T1 is not in a register)
//                          (load T2 if T2 is not in a register)
//                          tmp_reg = reg(T1) BinOp reg(T2) // We have to reuse one of the registers
//                                                 // if both reg(T1) and reg(T2) are temp registers.
//                          store tmp_reg loc(T0)
//
// Note: T2 allocation part can be optimized if T2 is an immediate number.
// Maximum temporary register used: 2
void TiggerGenerator::operator() (const eeyore::BinaryOpStmt &stmt)
{
	Reg reg1 = _read_opr(stmt.opr1);
	
	if(_is_global_var(stmt.opr))
	{
		auto var = std::get<eeyore::OrigVar>(stmt.opr);
		if(!holds_alternative<int>(stmt.opr2))
		{
			Reg reg2 = _read_opr(stmt.opr2);

			// If both of reg1 and reg2 are temp variables, we would have to
			// reuse one of them (here we use reg1) to store the calculation
			// result.
			Reg res_reg = reg1;
			if(_temp_regs.is_temp(reg1) && _temp_regs.is_temp(reg2))
			{
				_tigger_code.emplace_back(BinaryOpStmt(reg1, reg1, stmt.op_type, reg2));
				INTERNAL_ASSERT(_temp_regs.try_return_temp(reg2),
					"fail to return temp register");
			}
			else
			{
				res_reg = _temp_regs.get_temp();
				_tigger_code.emplace_back(BinaryOpStmt(res_reg, reg1, stmt.op_type, reg2));
				_temp_regs.try_return_temp(reg1);
				_temp_regs.try_return_temp(reg2);
			}

			auto tmp_reg2 = _temp_regs.get_temp();
			_tigger_code.emplace_back(LoadAddrStmt(tmp_reg2, GlobalVar(var.id)));
			_tigger_code.emplace_back(WriteArrStmt(tmp_reg2, 0, res_reg));
		}
		else
		{
			int reg2_val = std::get<int>(stmt.opr2);
			auto res_reg = _temp_regs.get_temp();
			_tigger_code.emplace_back(BinaryOpStmt(res_reg, reg1, stmt.op_type, reg2_val));
			_temp_regs.try_return_temp(reg1);
			
			auto tmp_reg2 = _temp_regs.get_temp();
			_tigger_code.emplace_back(LoadAddrStmt(tmp_reg2, GlobalVar(var.id)));
			_tigger_code.emplace_back(WriteArrStmt(tmp_reg2, 0, res_reg));
		}
	}
	else
	{
		auto pos = _allocator.actual_pos_of(stmt.opr);
		if(!pos.has_value()) // The calculation is performed, but its result is
						// never used. In this case we do not generate code for it.
			return;
		if(!holds_alternative<int>(stmt.opr2))
		{
			Reg reg2 = _read_opr(stmt.opr2);
			if(holds_alternative<Reg>(pos.value()))
			{
				auto reg = std::get<Reg>(pos.value());
				_tigger_code.emplace_back(BinaryOpStmt(reg, reg1, stmt.op_type, reg2));
			}
			else
			{
				auto stack_pos = std::get<int>(pos.value());

				// If both of reg1 and reg2 are temp variables, we would have to
				// reuse one of them (here we use reg1) to store the calculation
				// result.
				if(_temp_regs.is_temp(reg1) && _temp_regs.is_temp(reg2))
				{
					_tigger_code.emplace_back(BinaryOpStmt(reg1, reg1, stmt.op_type, reg2));
					_tigger_code.emplace_back(StoreStmt(stack_pos, reg1));
					_temp_regs.try_return_temp(reg2);
				}
				else
				{
					auto tmp_reg = _temp_regs.get_temp();
					_tigger_code.emplace_back(BinaryOpStmt(tmp_reg, reg1, stmt.op_type, reg2));
					_tigger_code.emplace_back(StoreStmt(stack_pos, tmp_reg));
				}
			}
		}
		else // Generate T0 = T1 BinOp Num style.
		{
			int reg2_val = std::get<int>(stmt.opr2);
			if(holds_alternative<Reg>(pos.value()))
			{
				auto reg = std::get<Reg>(pos.value());
				_tigger_code.emplace_back(BinaryOpStmt(reg, reg1, stmt.op_type, reg2_val));
			}
			else
			{
				auto stack_pos = std::get<int>(pos.value());
				auto tmp_reg = _temp_regs.get_temp();
				_tigger_code.emplace_back(BinaryOpStmt(tmp_reg, reg1, stmt.op_type, reg2_val));
				_tigger_code.emplace_back(StoreStmt(stack_pos, tmp_reg));
			}
		}
	}
}

// If T0 is a global var:
// T0 = T1   -->   (load T1 if T1 is not in a register)
//                 loadaddr T0 tmp_reg
//                 tmp_reg[0] = reg(T1)
// or if T0 is in a register:
//           -->   (load T1 if T1 is not in a register)
//                 reg(T0) = reg(T1)
// or if T0 is in stack:
//           -->   (load T1 if T1 is not in a register)
//                 store tmp_reg loc(T1)
// or T1 allocation can be optimized if T1 is an immediate number.
void TiggerGenerator::operator() (const eeyore::MoveStmt &stmt)
{
	if(_is_global_var(stmt.opr))
	{
		auto var = std::get<eeyore::OrigVar>(stmt.opr);
		Reg reg1 = _read_opr(stmt.opr1);
		auto tmp_reg = _temp_regs.get_temp();
		_tigger_code.emplace_back(LoadAddrStmt(tmp_reg, GlobalVar(var.id)));
		_tigger_code.emplace_back(WriteArrStmt(tmp_reg, 0, reg1));
	}
	else
	{
		auto pos = _allocator.actual_pos_of(stmt.opr);
		if(!pos.has_value()) // The operation is performed, but its result is
						// never used. In this case we do not generate code for it.
			return;
		if(!std::holds_alternative<int>(stmt.opr1))
		{
			Reg reg1 = _read_opr(stmt.opr1);
			if(holds_alternative<Reg>(pos.value()))
			{
				auto reg = std::get<Reg>(pos.value());
				_tigger_code.emplace_back(MoveStmt(reg, reg1));
			}
			else
			{
				auto stack_pos = std::get<int>(pos.value());
				_tigger_code.emplace_back(StoreStmt(stack_pos, reg1));
			}
		}
		else
		{
			int reg1_val = std::get<int>(stmt.opr1);
			if(holds_alternative<Reg>(pos.value()))
			{
				auto reg = std::get<Reg>(pos.value());
				_tigger_code.emplace_back(MoveStmt(reg, reg1_val));
			}
			else
			{
				auto stack_pos = std::get<int>(pos.value());
				auto tmp_reg = _temp_regs.get_temp();
				_tigger_code.emplace_back(MoveStmt(tmp_reg, reg1_val));
				_tigger_code.emplace_back(StoreStmt(stack_pos, tmp_reg));
			}
		}
	}
}

/*
   For global arrays, if T2 is not an immediate:
   T0 = T1[T2]   -->   load addr of T1 to a register tmp_reg1
                       (load T2 if T2 is not in a register)
                       tmp_reg1 = tmp_reg1 + reg(T2)
                       reg(T0) = tmp_reg1[0]
              or -->   load addr of T1 to a register tmp_reg1
                       (load T2 if T2 is not in a register)
                       tmp_reg1 = tmp_reg1 + reg(T2)
                       tmp_reg1 = tmp_reg1[0]
                       store tmp_reg1 loc(T0)
   or if T2 is an immediate number:
                 -->   load addr of T1 to a register tmp_reg1
                       reg(T0) = tmp_reg1[T2]
              or -->   load addr of T1 to a register tmp_reg1
                       tmp_reg1 = tmp_reg1[T2]
                       store tmp_reg1 loc(T0)
   
   For local arrays, if T2 is not an immediate, the code is almost the same as
   the case where T2 is an immediate above.
   If T2 is an immediate number:
    T0 = T1[T2]   -->   load loc(T1)+T2 reg(T0)
               or -->   load loc(T1)+T2 tmp_reg1
                       store tmp_reg1 loc(T0)
	
   For pointer arrays, if T2 is not an immediate:
    T0 = p0[T2]   -->   (load T2 if T2 is not in a register)
                        tmp_reg = reg(p0) + reg(T2)
                        reg(T0) = tmp_reg[0]
               or -->   (load T2 if T2 is not in a register)
                        tmp_reg = reg(p0) + reg(T2)
                        tmp_reg = tmp_reg[0]
                        store tmp_reg loc(T0)
    or if T2 is an immediate:
                  -->   reg(T0) = p0[T2]
               or -->   tmp_reg = p0[T2]
                        store tmp_reg loc(T0)
   
   Maximum temporary register used: 2
 */
void TiggerGenerator::operator() (const eeyore::ReadArrStmt &stmt)
{
	// value read but never used.
	if(!_is_global_var(stmt.opr) && !_allocator.actual_pos_of(stmt.opr).has_value())
		return;

	auto arr_pos = _allocator.actual_pos_of(stmt.arr_opr);
	if(!arr_pos.has_value()) // global array
	{
		INTERNAL_ASSERT(_is_global_var(stmt.arr_opr),
			utils::fstring("error position of ", stmt.arr_opr));
		Reg reg_idx = _read_opr(stmt.idx_opr);
		Reg tmp_reg = _read_opr_addr(stmt.arr_opr);

		_tigger_code.emplace_back(BinaryOpStmt(
			tmp_reg, tmp_reg, frontend::BinaryOpNode::ADD, reg_idx
		));
		// Now the address of the array is stored in tmp_reg, and we are only
		// using this temp register.

		if(_is_global_var(stmt.opr))
		{
			auto var = std::get<eeyore::OrigVar>(stmt.opr);
			auto tmp_reg2 = _temp_regs.get_temp();
			_tigger_code.emplace_back(ReadArrStmt(tmp_reg, tmp_reg, 0));
			_tigger_code.emplace_back(LoadAddrStmt(tmp_reg2, GlobalVar(var.id)));
			_tigger_code.emplace_back(WriteArrStmt(tmp_reg2, 0, tmp_reg));
		}
		else
		{
			auto opr_pos = _allocator.actual_pos_of(stmt.opr);
			INTERNAL_ASSERT(opr_pos.has_value(), utils::fstring("error at statement ", stmt));
			if(holds_alternative<Reg>(opr_pos.value()))
			{
				auto reg = std::get<Reg>(opr_pos.value());
				_tigger_code.emplace_back(ReadArrStmt(reg, tmp_reg, 0));
			}
			else
			{
				int reg_pos = std::get<int>(opr_pos.value());
				_tigger_code.emplace_back(ReadArrStmt(tmp_reg, tmp_reg, 0));
				_tigger_code.emplace_back(StoreStmt(reg_pos, tmp_reg));
			}
		}

	}
	else if(holds_alternative<int>(arr_pos.value())) // local array
	{
		int arr_stack_pos = std::get<int>(arr_pos.value());

		if(!std::holds_alternative<int>(stmt.idx_opr))
		{
			Reg reg_idx = _read_opr(stmt.idx_opr);
			auto tmp_reg = _temp_regs.get_temp();
			_tigger_code.emplace_back(LoadAddrStmt(tmp_reg, arr_stack_pos));
			_tigger_code.emplace_back(BinaryOpStmt(
				tmp_reg, tmp_reg, frontend::BinaryOpNode::ADD, reg_idx
			));
			_temp_regs.return_temp(tmp_reg);

			if(_is_global_var(stmt.opr))
			{
				auto var = std::get<eeyore::OrigVar>(stmt.opr);
				auto tmp_reg2 = _temp_regs.get_temp();
				_tigger_code.emplace_back(ReadArrStmt(tmp_reg, tmp_reg, 0));
				_tigger_code.emplace_back(LoadAddrStmt(tmp_reg2, GlobalVar(var.id)));
				_tigger_code.emplace_back(WriteArrStmt(tmp_reg2, 0, tmp_reg));
			}
			else
			{
				auto opr_pos = _allocator.actual_pos_of(stmt.opr);
				INTERNAL_ASSERT(opr_pos.has_value(), utils::fstring("error at statement ", stmt));
				if(holds_alternative<Reg>(opr_pos.value()))
				{
					auto reg = std::get<Reg>(opr_pos.value());
					_tigger_code.emplace_back(ReadArrStmt(reg, tmp_reg, 0));
				}
				else
				{
					int reg_pos = std::get<int>(opr_pos.value());
					_tigger_code.emplace_back(ReadArrStmt(tmp_reg, tmp_reg, 0));
					_tigger_code.emplace_back(StoreStmt(reg_pos, tmp_reg));
				}
			}
		}
		else
		{
			int idx_val = std::get<int>(stmt.idx_opr);
			int ele_stack_pos = arr_stack_pos + idx_val / sizeof(int);
			if(_is_global_var(stmt.opr))
			{
				auto var = std::get<eeyore::OrigVar>(stmt.opr);
				auto tmp_reg = _temp_regs.get_temp();
				auto tmp_reg2 = _temp_regs.get_temp();
				_tigger_code.emplace_back(LoadStmt(tmp_reg, ele_stack_pos));
				_tigger_code.emplace_back(LoadAddrStmt(tmp_reg2, GlobalVar(var.id)));
				_tigger_code.emplace_back(WriteArrStmt(tmp_reg2, 0, tmp_reg));
			}
			else
			{
				auto opr_pos = _allocator.actual_pos_of(stmt.opr);
				INTERNAL_ASSERT(opr_pos.has_value(), utils::fstring("error at statement ", stmt));
				if(holds_alternative<Reg>(opr_pos.value()))
				{
					auto reg = std::get<Reg>(opr_pos.value());
					_tigger_code.emplace_back(LoadStmt(reg, ele_stack_pos));
				}
				else
				{
					int reg_pos = std::get<int>(opr_pos.value());
					auto tmp_reg = _temp_regs.get_temp();
					_tigger_code.emplace_back(LoadStmt(tmp_reg, ele_stack_pos));
					_tigger_code.emplace_back(StoreStmt(reg_pos, tmp_reg));
				}
			}
		}
	}
	else // pointer array
	{
		Reg reg_arr = std::get<Reg>(arr_pos.value());
		if(!holds_alternative<int>(stmt.idx_opr))
		{
			Reg reg_idx = _read_opr(stmt.idx_opr);
			auto tmp_reg = _temp_regs.get_temp();
			_tigger_code.emplace_back(BinaryOpStmt(
				tmp_reg, reg_arr, frontend::BinaryOpNode::ADD, reg_idx
			));

			if(_is_global_var(stmt.opr))
			{
				auto var = std::get<eeyore::OrigVar>(stmt.opr);
				auto tmp_reg2 = _temp_regs.get_temp();
				_tigger_code.emplace_back(ReadArrStmt(tmp_reg, tmp_reg, 0));
				_tigger_code.emplace_back(LoadAddrStmt(tmp_reg2, GlobalVar(var.id)));
				_tigger_code.emplace_back(WriteArrStmt(tmp_reg2, 0, tmp_reg));
			}
			else
			{
				auto opr_pos = _allocator.actual_pos_of(stmt.opr);
				INTERNAL_ASSERT(opr_pos.has_value(), utils::fstring("error at statement ", stmt));
				if(holds_alternative<Reg>(opr_pos.value()))
				{
					auto reg = std::get<Reg>(opr_pos.value());
					_tigger_code.emplace_back(ReadArrStmt(reg, tmp_reg, 0));
				}
				else
				{
					int reg_pos = std::get<int>(opr_pos.value());
					_tigger_code.emplace_back(ReadArrStmt(tmp_reg, tmp_reg, 0));
					_tigger_code.emplace_back(StoreStmt(reg_pos, tmp_reg));
				}
			}
		}
		else
		{
			int idx_val = std::get<int>(stmt.idx_opr);
			if(_is_global_var(stmt.opr))
			{
				auto var = std::get<eeyore::OrigVar>(stmt.opr);
				auto tmp_reg = _temp_regs.get_temp();
				auto tmp_reg2 = _temp_regs.get_temp();
				_tigger_code.emplace_back(ReadArrStmt(tmp_reg, reg_arr, idx_val));
				_tigger_code.emplace_back(LoadAddrStmt(tmp_reg2, GlobalVar(var.id)));
				_tigger_code.emplace_back(WriteArrStmt(tmp_reg2, 0, tmp_reg));
			}
			else
			{
				auto opr_pos = _allocator.actual_pos_of(stmt.opr);
				INTERNAL_ASSERT(opr_pos.has_value(), utils::fstring("error at statement ", stmt));
				if(holds_alternative<Reg>(opr_pos.value()))
				{
					auto reg = std::get<Reg>(opr_pos.value());
					_tigger_code.emplace_back(ReadArrStmt(reg, reg_arr, idx_val));
				}
				else
				{
					int reg_pos = std::get<int>(opr_pos.value());
					auto tmp_reg = _temp_regs.get_temp();
					_tigger_code.emplace_back(ReadArrStmt(tmp_reg, reg_arr, idx_val));
					_tigger_code.emplace_back(StoreStmt(reg_pos, tmp_reg));
				}
			}
		}
	}
}

/*
   For global arrays, if T2 is not an immediate:
   T1[T2] = T0   -->   loadaddr var(T1) tmp_reg
                       (load T2 if T2 is not in a register)
                       tmp_reg = tmp_reg + reg(T2)
                       (free the temp register of T2)
                       (load T0 if T0 is not in a register)
                       tmp_reg[0] = reg(T0)
   or if T2 is an immediate number, the allocation of T2 can be optimized.
   
   For local arrays, if T2 is not an immediate, it is almost the same as the
   case of global arrays.
   or if T2 is an immediate number:
                 -->   (load T0 if T0 is not in a register)
                 -->   store reg(T0) loc(T1)+T2
   
   For pointer arrays, almost the same as global arrays.
   Maximum temporary register used: 2
*/
void TiggerGenerator::operator() (const eeyore::WriteArrStmt &stmt)
{
	auto arr_pos = _allocator.actual_pos_of(stmt.arr_opr);
	if(!arr_pos.has_value()) // global array
	{
		if(!_is_global_var(stmt.arr_opr)) // Write an array that is never used.
			return;
		Reg tmp_reg = _read_opr_addr(stmt.arr_opr);

		if(!holds_alternative<int>(stmt.idx_opr))
		{
			Reg reg_idx = _read_opr(stmt.idx_opr);
			_tigger_code.emplace_back(BinaryOpStmt(
				tmp_reg, tmp_reg, frontend::BinaryOpNode::ADD, reg_idx
			));
			_temp_regs.try_return_temp(reg_idx);

			Reg reg_opr = _read_opr(stmt.opr);
			_tigger_code.emplace_back(WriteArrStmt(tmp_reg, 0, reg_opr));
		}
		else
		{
			int reg_idx_val = std::get<int>(stmt.idx_opr);
			Reg reg_opr = _read_opr(stmt.opr);
			_tigger_code.emplace_back(WriteArrStmt(tmp_reg, reg_idx_val, reg_opr));
		}
	}
	else if(holds_alternative<int>(arr_pos.value())) // local array
	{
		int arr_stack_pos = std::get<int>(arr_pos.value());

		if(!holds_alternative<int>(stmt.idx_opr))
		{
			Reg reg_idx = _read_opr(stmt.idx_opr);
			auto tmp_reg = _temp_regs.get_temp();
			_tigger_code.emplace_back(LoadAddrStmt(tmp_reg, arr_stack_pos));
			_tigger_code.emplace_back(BinaryOpStmt(
				tmp_reg, tmp_reg, frontend::BinaryOpNode::ADD, reg_idx
			));
			_temp_regs.try_return_temp(reg_idx);

			Reg reg_opr = _read_opr(stmt.opr);
			_tigger_code.emplace_back(WriteArrStmt(tmp_reg, 0, reg_opr));
		}
		else
		{
			Reg reg_opr = _read_opr(stmt.opr);
			int idx_val = std::get<int>(stmt.idx_opr);
			int ele_pos = arr_stack_pos + idx_val / sizeof(int);
			_tigger_code.emplace_back(StoreStmt(ele_pos, reg_opr));
		}
	}
	else
	{
		Reg reg_arr = std::get<Reg>(arr_pos.value());

		if(!holds_alternative<int>(stmt.idx_opr))
		{
			auto tmp_reg = _temp_regs.get_temp();
			Reg reg_idx = _read_opr(stmt.idx_opr);
			_tigger_code.emplace_back(BinaryOpStmt(
				tmp_reg, reg_arr, frontend::BinaryOpNode::ADD, reg_idx
			));
			_temp_regs.try_return_temp(reg_idx);

			Reg reg_opr = _read_opr(stmt.opr);
			_tigger_code.emplace_back(WriteArrStmt(tmp_reg, 0, reg_opr));
		}
		else
		{
			int reg_idx_val = std::get<int>(stmt.idx_opr);
			Reg reg_opr = _read_opr(stmt.opr);
			_tigger_code.emplace_back(WriteArrStmt(reg_arr, reg_idx_val, reg_opr));
		}
	}
}

// Same as eeyore.
void TiggerGenerator::operator() (const eeyore::LabelStmt &stmt)
{
	_tigger_code.emplace_back(LabelStmt(stmt.label));
}


} // namespace compiler::backend::tigger