#include "dbg.h"
#include <iostream>
#include "exceptions.h"
#include "visitor_helper.h"
#include "eeyore_gen.h"

using std::optional;
using std::holds_alternative;
using namespace compiler::frontend;

namespace compiler::backend::eeyore
{

void EeyoreGenerator::EeyoreRearranger::rearrange(std::list<EeyoreStatement> &eeyore_code)
{
	DBG(std::cout << std::endl << "rearrangement begin" << std::endl);
	std::list<EeyoreStatement> global_assignments;
	auto func_begin = eeyore_code.end(),
		 main_begin = eeyore_code.end(),
		 global_def_end = eeyore_code.end();
	
	auto iter = eeyore_code.begin();

	// Find the end of the global definition part. All the global definitions
	// that occurs later are moved to here.
	if(holds_alternative<DeclStmt>(*(eeyore_code.begin())))
	{
		for(++iter; iter != eeyore_code.end(); ++iter)
		{
			DBG(std::cout << "pre-analyzing: " << *iter);
			if(!holds_alternative<DeclStmt>(*iter))
			{
				global_def_end = iter;
				--global_def_end;
				break;
			}
		}
	}

	while(iter != eeyore_code.end())
	{
		DBG(std::cout << "analyzing: " << *iter);

		const auto &stmt = *iter;
		if(holds_alternative<FuncDefStmt>(stmt))
		{
			DBG(std::cout << "a func def statement" << std::endl);
			if(std::get<FuncDefStmt>(stmt).func_name == "f_main") // main function
				main_begin = iter;
			func_begin = iter;
		}
		else if(holds_alternative<EndFuncDefStmt>(stmt))
		{
			DBG(std::cout << "a end func def statement" << std::endl);
			func_begin = eeyore_code.end(); // reset func_begin
		}
		else if(holds_alternative<DeclStmt>(stmt))
		{
			DBG(std::cout << "a decl statement" << std::endl);
			if(func_begin != eeyore_code.end())
			{
				func_begin = eeyore_code.insert(++func_begin, stmt);
					// We would insert DeclStmts after func_begin, but list::insert
					// inserts an element `before` a certain position, so increase
					// the position by 1, then insert it, and finally reset func_begin
					// to be the statement inserted.
				iter = eeyore_code.erase(iter); // erase the stmt and forward iter
			}
			else // A global DeclStmt, move it before global_def_end. 
			{
				if(global_def_end == eeyore_code.end()) // No global declaration at the beginning.
					global_def_end = eeyore_code.insert(eeyore_code.begin(), stmt);
				else
					global_def_end = eeyore_code.insert(++global_def_end, stmt);
				iter = eeyore_code.erase(iter);
			}
			continue;
		}
		else if(func_begin == eeyore_code.end()) // non-decl statement in global, it must be a global assignment statement.
		{
			DBG(std::cout << "a global assignment" << std::endl);
			global_assignments.push_back(stmt);
			iter = eeyore_code.erase(iter);
			continue;
		}
		++iter;
	}
	INTERNAL_ASSERT(main_begin != eeyore_code.end(), "no main function");
	
	// finally, add all the global assignments to the beginning of main
	// after DeclStmt's
	for(++main_begin; main_begin != eeyore_code.end(); ++main_begin)
		if(!holds_alternative<DeclStmt>(*main_begin))
			break;
	eeyore_code.splice(main_begin, global_assignments);
}

void EeyoreGenerator::GeneratorState::reset_all()
{
	cont_break_label.reset();
	true_false_label.reset();
	lval_mode = false;
	write_operand.reset();
	array_offset.reset();
	access_index.clear();
	curr_type.reset();
}

optional<Operand> EeyoreGenerator::operator() (const ProgramNodePtr &node)
{
	for(const AstPtr &child : node->children())
		std::visit(*this, child);
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const VarDeclNodePtr &node)
{
	for(const auto &child : node->children())
		visit(*this, child);
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const SingleVarDeclNodePtr &node)
{
	DBG(std::cout << "start decl" << std::endl);
	INTERNAL_ASSERT(holds_alternative<IdNodePtr>(node->lval()),
		"expected an IdNode as lval of SingleVarDeclNodePtr");
	const auto &id = std::get<IdNodePtr>(node->lval());
	const std::string &id_name = id->name();
	const TypePtr &id_type = node->type();

	// Generate decl code.
	auto orig_var = resources.get_original_var(get_size(id_type));
	eeyore_code.emplace_back(DeclStmt(orig_var));
	table.insert(id_name, id_type, orig_var);

	// Generate initial value assignment code.
	if(!is_null_ast(node->init_val()))
	{
		state.set_write_opr(orig_var); // The val calculated would write to this opr directly.
									   // Here we do not save the previous write_opr, since we
									   // are alway in read mode when calling visitor method of
									   // VarDeclNodePtr.
		if(!is_basic(id_type)) // initialize an array with initializer list.
			state.set_initializer_write(id_type);
		std::visit(*this, node->init_val());

		// Reset to read mode.
		state.reset_write_opr();
		state.reset_initializer_write();
	}
	DBG(std::cout << "end decl" << std::endl);
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const ConstIntNodePtr &node)
{
	INTERNAL_ASSERT(state.is_rval_mode(), "const ints cannot be lvals");

	DBG(
		if(state.is_lval_mode())
			std::cout << "in lval mode" << std::endl;
		else if(state.is_write_array())
			std::cout << "in write array mode" << std::endl;
		else if(state.is_write_mode())
			std::cout << "in write var mode" << std::endl;
		else
			std::cout << "in read mode" << std::endl;
		
		std::cout << "visiting const int = " << node->val() << std::endl;
	);

	if(state.is_write_mode())
	{
		if(state.is_write_array())
			eeyore_code.emplace_back(WriteArrStmt(
				state.write_opr(), state.arr_offset(), node->val()
			));
		else
			eeyore_code.emplace_back(MoveStmt(
				state.write_opr(), node->val()
			));
		DBG(std::cout << "exit from const int" << std::endl);
		return std::nullopt;
	}
	else
	{
		DBG(std::cout << "exit from const int" << std::endl);
		return node->val();
	}
}

optional<Operand> EeyoreGenerator::operator() (const IdNodePtr &node)
{
	auto find_res = table.find(node->name());
	INTERNAL_ASSERT(find_res.has_value(), "use of undefined variable");
	
	// Wrap a narrow variant to a wider variant.
	static utils::LambdaVisitor OperandWrapper = {
		[](const std::monostate &t) -> Operand
			{ INTERNAL_ERROR("expected a variable here"); },
		[](const auto &t) { return Operand(t); }
	};
	Operand id_opr = visit(OperandWrapper, find_res.value().var);

	DBG(
		if(state.is_lval_mode())
			std::cout << "in lval mode" << std::endl;
		else if(state.is_write_array())
			std::cout << "in write array mode" << std::endl;
		else if(state.is_write_mode())
			std::cout << "in write var mode" << std::endl;
		else
			std::cout << "in read mode" << std::endl;
		
		std::cout << "visiting id = " << node->name();
		if(state.is_arr_access())
			std::cout << ", array access with " << state.access_idx().size() << " indices" << std::endl;
		else
			std::cout << std::endl;
	);

	if(state.is_arr_access()) // calculate access offset.
	{
		Operand offset_opr = resources.get_temp_var();
		eeyore_code.emplace_back(DeclStmt(offset_opr));

		TypePtr type = find_res.value().type;
		int const_offset = 0; // all the const offset are accumulated here.
		bool idx_is_const = true;

		const auto &access_idx = state.access_idx();
		for(auto iter = access_idx.rbegin(); iter != access_idx.rend(); ++iter)
			// The first index is added to access_idx last, so we visit the access_idx
			// in reversed order.
		{
			Operand idx = *iter;

			// Calculate the type and size of the element of array.
			TypePtr element_type = make_null();
			int element_size;

			if(holds_alternative<ArrayTypePtr>(type))
			{
				auto actual_type = std::get<ArrayTypePtr>(type);
				element_type = actual_type->element_type();
				element_size = actual_type->element_size();
			}
			else if(holds_alternative<PointerTypePtr>(type))
			{
				auto acutal_type = std::get<PointerTypePtr>(type);
				element_type = acutal_type->base_type();
				element_size = get_size(element_type);
			}
			else
				INTERNAL_ERROR("invaild type for array access");

			// Accumulate the offset.
			if(holds_alternative<int>(idx))
				const_offset += std::get<int>(idx) * element_size;
			else
			{
				if(idx_is_const) // Generate direct move stmt instead of add for the
								 // first offset term.
				{
					eeyore_code.emplace_back(BinaryOpStmt(
						offset_opr, idx, BinaryOpNode::MUL, element_size
					));
					idx_is_const = false;
				}
				else // accumulate the offset.
				{
					TempVar tmp = resources.get_temp_var();
					eeyore_code.emplace_back(DeclStmt(tmp));
					eeyore_code.emplace_back(BinaryOpStmt(
						tmp, idx, BinaryOpNode::MUL, element_size
					));
					eeyore_code.emplace_back(BinaryOpStmt(
						offset_opr, offset_opr, BinaryOpNode::ADD, tmp
					));
				}
			}

			type = element_type;
		}
		if(idx_is_const)
		{
			offset_opr = const_offset; // the index is a constant, simply set
									// offset_opr to the constant.
		}
		else if(const_offset != 0)
		{
			eeyore_code.emplace_back(BinaryOpStmt(
				offset_opr, offset_opr, BinaryOpNode::ADD, const_offset
			));
		}

		// We have now got two Vars: id_opr (the array) and offset_opr (the idx).
		// There are two cases to handle: access an element or get a pointer.
		if(is_basic(type)) // element access
		{
			if(state.is_lval_mode())
			{
				state.set_write_opr(id_opr);
				state.set_arr_offset(offset_opr);
				DBG(std::cout << "lval exit from id(arr)" << std::endl);
				return std::nullopt;
			}
			else
			{
				if(state.is_write_mode() && !state.is_write_array())
				{
					// Assigning an array element to idx, we need only one memory fetch.
					eeyore_code.emplace_back(ReadArrStmt(
						state.write_opr(), id_opr, offset_opr
					));
					return std::nullopt;
				}
				else
				{
					// We have to store the fetched element.
					TempVar element = resources.get_temp_var();
					eeyore_code.emplace_back(DeclStmt(element));
					eeyore_code.emplace_back(ReadArrStmt(
						element, id_opr, offset_opr
					));

					if(state.is_write_mode())
					{
						// write array
						eeyore_code.emplace_back(WriteArrStmt(
							state.write_opr(), state.arr_offset(), element
						));
						return std::nullopt;
					}
					else // read mode
					{
						return element;
					}
				}
			}
		}
		else // get a pointer.
		{
			INTERNAL_ASSERT(state.is_rval_mode(), "pointers cannot be lvals");
			if(state.is_write_mode())
			{
				INTERNAL_ASSERT(!state.is_write_array(),
					"pointer cannot be array elements");
				eeyore_code.emplace_back(BinaryOpStmt(
					state.write_opr(), id_opr, BinaryOpNode::ADD, offset_opr
				));
				return std::nullopt;
			}
			else // read mode
			{
				TempVar tmp = resources.get_temp_var();
				eeyore_code.emplace_back(DeclStmt(tmp));
				eeyore_code.emplace_back(BinaryOpStmt(
					tmp, id_opr, BinaryOpNode::ADD, offset_opr
				));
				return tmp;
			}
		}
	}
	else // just a simple id, no array access.
	{
		if(state.is_lval_mode())
		{
			state.set_write_opr(id_opr);
			state.reset_arr_offset();
			DBG(std::cout << "exit from lval" << std::endl);
			return std::nullopt;
		}
		else if(state.is_write_mode())
		{
			if(state.is_write_array())
			{
				eeyore_code.emplace_back(WriteArrStmt(
					state.write_opr(), state.arr_offset(), id_opr
				));
			}
			else // write Var
			{
				eeyore_code.emplace_back(MoveStmt(
					state.write_opr(), id_opr
				));
			}
			return std::nullopt;
		}
		else // read mode
		{
			return id_opr;
		}
	}
}

optional<Operand> EeyoreGenerator::operator() (const InitializerNodePtr &node)
{
	INTERNAL_ASSERT(holds_alternative<int>(state.arr_offset()), "the offset of the initializer must be a int");
	INTERNAL_ASSERT(holds_alternative<ArrayTypePtr>(state.arr_type()), "initializer current type should be ArrayTypePtr");
	int base_offset = std::get<int>(state.arr_offset());
	ArrayTypePtr curr_type = std::get<ArrayTypePtr>(state.arr_type());
	TypePtr element_type = curr_type->element_type();
	int element_size = curr_type->element_size();
	DBG(std::cout << "matching array with element " << element_type << std::endl);
	DBG(std::cout << "base_offset = " << base_offset << std::endl);

	auto prev_arr_type = state.arr_type();
	state.set_arr_type(element_type);
	for(int i = 0; i < node->children_cnt(); i++)
	{
		state.set_arr_offset(base_offset + i * element_size);
		visit(*this, node->get(i));
	}
	state.set_arr_type(prev_arr_type);
	state.set_arr_offset(base_offset);
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const FuncDefNodePtr &node)
{
	const IdNodePtr &id = node->actual_name();
	int arg_cnt = node->actual_params()->children_cnt();
	const TypePtr &id_type = node->type();
	table.insert(id->name(), node->type());
	eeyore_code.emplace_back(FuncDefStmt(id->name(), arg_cnt));

	// create a new block for parameters.
	table.new_block();
	for(int i = 0; i < arg_cnt; i++)
	{
		const AstPtr &arg_v = node->actual_params()->get(i);
		if(!holds_alternative<SingleFuncParamNodePtr>(arg_v))
			INTERNAL_ERROR("expected SingleFuncParamNode as children of FuncParamsNode");
		const auto &arg = std::get<SingleFuncParamNodePtr>(arg_v);

		if(!holds_alternative<IdNodePtr>(arg->lval()))
			INTERNAL_ERROR("exptected IdNode as lval of SingleFuncParamNode after semantic analysis");
		table.insert(
			std::get<IdNodePtr>(arg->lval())->name(), arg->type(), Param(i)
		);
	}

	std::visit(*this, node->block());
	// We do not check if all the branches reach the return statement.
	// Simply add an extra return statement if the last statement is not return.
	if(!holds_alternative<RetStmt>(eeyore_code.back()))
	{
		if(!holds_alternative<FuncTypePtr>(node->type()))
			INTERNAL_ERROR("expected FuncType for type of FuncDefNode");
		TypePtr retval_type = std::get<FuncTypePtr>(node->type())->retval_type();
		if(holds_alternative<VoidTypePtr>(retval_type))
			eeyore_code.emplace_back(RetStmt());
		else // return int
			eeyore_code.emplace_back(RetStmt(0));
	}

	// end function
	table.end_block(); // clear parameters.
	eeyore_code.emplace_back(EndFuncDefStmt(id->name()));
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const BlockNodePtr &node)
{
	table.new_block();
	for(const auto &child : node->children())
		visit(*this, child);
	table.end_block();
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const UnaryOpNodePtr &node)
{
	INTERNAL_ASSERT(state.is_rval_mode(), "unary op cannot occur in lvals");
	bool write_mode = state.is_write_mode();
	optional<Operand> prev_write_opr, prev_arr_offset;
	if(write_mode)
	{
		// store write mode.
		prev_write_opr = state.store_write_opr();
		prev_arr_offset = state.store_arr_offset();
		state.reset_write_opr();
		state.reset_arr_offset();
	}

	auto ret1 = std::visit(*this, node->operand());
	INTERNAL_ASSERT(ret1.has_value(), "unary operand should return a type after visiting");
	Operand opr1 = ret1.value();

	if(write_mode)
	{
		// Restore write mode & generate write code
		state.restore_write_opr(prev_write_opr);
		state.restore_arr_offset(prev_arr_offset);

		if(state.is_write_array())
		{
			TempVar tmp = resources.get_temp_var();
			eeyore_code.emplace_back(DeclStmt(tmp));
			eeyore_code.emplace_back(UnaryOpStmt(
				tmp, node->op(), opr1
			));
			eeyore_code.emplace_back(WriteArrStmt(
				state.write_opr(), state.arr_offset(), tmp
			));
		}
		else // write var
		{
			eeyore_code.emplace_back(UnaryOpStmt(
				state.write_opr(), node->op(), opr1
			));
		}
		return std::nullopt;
	}
	else
	{
		TempVar tmp = resources.get_temp_var();
		eeyore_code.emplace_back(DeclStmt(tmp));
		eeyore_code.emplace_back(UnaryOpStmt(
			tmp, node->op(), opr1
		));
		return tmp;
	}
}

optional<Operand> EeyoreGenerator::operator() (const BinaryOpNodePtr &node)
{
	DBG(
		if(state.is_lval_mode())
			std::cout << "in lval mode" << std::endl;
		else if(state.is_write_array())
			std::cout << "in write array mode" << std::endl;
		else if(state.is_write_mode())
			std::cout << "in write var mode" << std::endl;
		else
			std::cout << "in read mode" << std::endl;
		
		std::cout << "op node " << node->op();
		if(state.is_arr_access())
			std::cout << ", array access with " << state.access_idx().size() << " indices" << std::endl;
		else
			std::cout << std::endl;
	);

	if(node->op() == BinaryOpNode::ACCESS)
	{
		// 1. visit index in read mode.
		// 2. visit base in the previous mode.

		// store the previous mode.
		bool lval_mode = state.is_lval_mode();
		bool write_mode = state.is_write_mode();
		optional<Operand> prev_write_opr, prev_arr_offset;
		if(lval_mode)
		{
			state.set_rval_mode();
		}
		else if(write_mode)
		{
			prev_write_opr = state.store_write_opr();
			prev_arr_offset = state.store_arr_offset();
			state.reset_write_opr();
			state.reset_arr_offset();
		}
		std::vector<Operand> stored_idx = state.store_access_idx();
			// Also stores previous access idx, since there could be array
			// recursive visit like "a[b[0]]".
		state.clear_access_idx();

		auto idx_ret = std::visit(*this, node->operand2());
		INTERNAL_ASSERT(idx_ret.has_value(), "expected a return value for lval array index");

		// restore the previous state & return
		state.restore_access_idx(std::move(stored_idx));
		state.add_access_idx(idx_ret.value());
		if(lval_mode)
		{
			state.set_lval_mode();
		}
		else
		{
			state.restore_write_opr(prev_write_opr);
			state.restore_arr_offset(prev_arr_offset);
		}
		auto retval = visit(*this, node->operand1()); // for ACCESS, the actual value is
													  // returned until IdNode is reached.
		state.pop_access_idx();
		return retval;
	}
	else
	{
		if(state.is_lval_mode())
			INTERNAL_ERROR("non access binary op should not occur in lvals");
		if(node->op() == BinaryOpNode::ASSIGN)
		{
			// 1. visit lval in lval mode
			// 2. visit rval in write mode

			// does not save previous mode, since it is always read mode.
			state.set_lval_mode();
			std::visit(*this, node->operand1());
			// write_opr and arr_offset are set during visiting

			state.set_rval_mode();
			std::visit(*this, node->operand2());
			
			// Set to read mode & return.
			state.reset_write_opr();
			state.reset_arr_offset();
			return std::nullopt;
		}
		else
		{
			// Actually there is one more state: the branching state.
			// Check if it is a RelOp && branching is work, if so,
			// generate branching statements instead of returning Var.
			// It is guaranteed that branching state => read mode.
			if((node->op() == BinaryOpNode::AND || node->op() == BinaryOpNode::OR)
				&& state.is_cond_expr())
			{
				// 1. check op, set the jumps.
				// 2. visit operand1.
				// 3. output labels.
				// 4. same for operand2.
				
				if(node->op() == BinaryOpNode::AND)
				{
					Label first_true = resources.get_label();
					Label prev_true_label = state.true_label();
					state.set_true_label(first_true);
					auto opr1_ret = visit(*this, node->operand1());
					if(opr1_ret.has_value()) // opr1 is an expression, generate jump instruction for it.
					{
						Operand opr1_opr = opr1_ret.value();
						eeyore_code.emplace_back(CondGotoStmt(
							opr1_opr, BinaryOpNode::EQ, 0, state.false_label()
						));
					}

					eeyore_code.emplace_back(LabelStmt(first_true));

					state.set_true_label(prev_true_label);
					auto opr2_ret = visit(*this, node->operand2());
					if(opr2_ret.has_value())
					{
						Operand opr2_opr = opr2_ret.value();
						eeyore_code.emplace_back(CondGotoStmt(
							opr2_opr, BinaryOpNode::EQ, 0, state.false_label()
						));
					}

					eeyore_code.emplace_back(GotoStmt(state.true_label()));
				}
				else if(node->op() == BinaryOpNode::OR)
				{
					Label first_false = resources.get_label();
					Label prev_false_label = state.false_label();
					state.set_false_label(first_false);
					auto opr1_ret = visit(*this, node->operand1());
					if(opr1_ret.has_value())
					{
						Operand opr1_opr = opr1_ret.value();
						eeyore_code.emplace_back(CondGotoStmt(
							opr1_opr, BinaryOpNode::NE, 0, state.true_label()
						));
					}

					eeyore_code.emplace_back(LabelStmt(first_false));

					state.set_false_label(prev_false_label);
					auto opr2_ret = visit(*this, node->operand2());
					if(opr2_ret.has_value())
					{
						Operand opr2_opr = opr2_ret.value();
						eeyore_code.emplace_back(CondGotoStmt(
							opr2_opr, BinaryOpNode::NE, 0, state.true_label()
						));
					}

					eeyore_code.emplace_back(GotoStmt(state.false_label()));
				}
				return std::nullopt;
			}
			else // generate normal Var assignment statements.
			{
				// 1. visit operand1 and operand2 in rval mode
				// 2. do return according to the mode given at first

				bool write_mode = state.is_write_mode();
				optional<Operand> prev_write_opr, prev_arr_offset;
				if(write_mode)
				{
					prev_write_opr = state.store_write_opr();
					prev_arr_offset = state.store_arr_offset();
				}

				state.reset_write_opr();
				state.reset_arr_offset();
				auto opr1_ret = std::visit(*this, node->operand1());
				auto opr2_ret = std::visit(*this, node->operand2());

				INTERNAL_ASSERT(opr1_ret.has_value() && opr2_ret.has_value(),
					"expected return value for operand of binary operator");
				Operand opr1_opr = opr1_ret.value(), opr2_opr = opr2_ret.value();

				if(write_mode)
				{
					state.restore_write_opr(prev_write_opr);
					state.restore_arr_offset(prev_arr_offset);
					if(state.is_write_array())
					{
						TempVar tmp = resources.get_temp_var();
						eeyore_code.emplace_back(DeclStmt(tmp));
						eeyore_code.emplace_back(BinaryOpStmt(
							tmp, opr1_opr, node->op(), opr2_opr
						));
						eeyore_code.emplace_back(WriteArrStmt(
							state.write_opr(), state.arr_offset(), tmp
						));
					}
					else
					{
						eeyore_code.emplace_back(BinaryOpStmt(
							state.write_opr(), opr1_opr, node->op(), opr2_opr
						));
					}
					return std::nullopt;
				}
				else // read mode
				{
					TempVar tmp = resources.get_temp_var();
					eeyore_code.emplace_back(DeclStmt(tmp));
					eeyore_code.emplace_back(BinaryOpStmt(
						tmp, opr1_opr, node->op(), opr2_opr
					));
					return tmp;
				}
			}
		}
	}
}

// if structure:
//
// 	 expr -> L1 or L2
// L1:
// 	 body
// L2:

// if-else structure:
//
//   expr -> L1 or L2
// L1:
//   if body
//   goto L3
// L2:
//   else body
// L3:
optional<Operand> EeyoreGenerator::operator() (const IfNodePtr &node)
{
	Label ltrue = resources.get_label(), lfalse = resources.get_label();
	auto prev_true_false_label = state.store_true_false_label();
	state.set_true_false_label(ltrue, lfalse);
	auto expr_ret = visit(*this, node->expr());
	if(expr_ret.has_value())
	{
		eeyore_code.emplace_back(CondGotoStmt(
			expr_ret.value(), BinaryOpNode::EQ, 0, lfalse
		));
	}
	state.restore_true_false_label(prev_true_false_label);
	eeyore_code.emplace_back(LabelStmt(ltrue));
	std::visit(*this, node->if_body());

	if(is_null_ast(node->else_body()))
	{
		eeyore_code.emplace_back(LabelStmt(lfalse));
	}
	else
	{
		Label lend = resources.get_label();
		eeyore_code.emplace_back(GotoStmt(lend));
		eeyore_code.emplace_back(LabelStmt(lfalse));
		std::visit(*this, node->else_body());
		eeyore_code.emplace_back(LabelStmt(lend));
	}
	return std::nullopt;
}

// While structure:
// L1:
// 	 expr...
// L2:
// 	 body
//   goto L1
// L3:
optional<Operand> EeyoreGenerator::operator() (const WhileNodePtr &node)
{
	Label lbegin = resources.get_label();
	Label ltrue = resources.get_label(), lfalse = resources.get_label();
	
	eeyore_code.emplace_back(LabelStmt(lbegin));

	auto prev_true_false_label = state.store_true_false_label();
	auto prev_cont_break_label = state.store_cont_break_label();
	state.set_true_false_label(ltrue, lfalse);
	state.set_cont_break_label(lbegin, lfalse); // this statement is moved before visiting expr
									// to correctly set the temp variables used in expr to 0
	auto expr_ret = visit(*this, node->expr());
	if(expr_ret.has_value())
	{
		eeyore_code.emplace_back(CondGotoStmt(
			expr_ret.value(), BinaryOpNode::EQ, 0, lfalse
		));
	}
	state.restore_true_false_label(prev_true_false_label);
	
	eeyore_code.emplace_back(LabelStmt(ltrue));
	visit(*this, node->body());
	eeyore_code.emplace_back(GotoStmt(lbegin));
	eeyore_code.emplace_back(LabelStmt(lfalse));

	state.restore_cont_break_label(prev_cont_break_label);
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const BreakNodePtr &node)
{
	INTERNAL_ASSERT(state.in_loop(), "break statement should be placed in a loop");
	eeyore_code.emplace_back(GotoStmt(state.break_label()));
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const ContNodePtr &node)
{
	INTERNAL_ASSERT(state.in_loop(), "continue statement should be placed in a loop");
	eeyore_code.emplace_back(GotoStmt(state.cont_label()));
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const RetNodePtr &node)
{
	if(is_null_ast(node->expr()))
		eeyore_code.emplace_back(RetStmt());
	else
	{
		// we are in read mode, so call visit method directly and get the return val.
		auto expr = visit(*this, node->expr());
		INTERNAL_ASSERT(expr.has_value(), "exptected a return value for return statement");
		eeyore_code.emplace_back(RetStmt(expr.value()));
	}
	return std::nullopt;
}

optional<Operand> EeyoreGenerator::operator() (const FuncCallNodePtr &node)
{
	const std::string &id_name = node->actual_id()->name();
	auto find_res = table.find(id_name);
	INTERNAL_ASSERT(find_res.has_value(), "use of undefined function");
	TypePtr func_type = find_res.value().type;
	INTERNAL_ASSERT(holds_alternative<FuncTypePtr>(func_type), "call of non-function type");
	TypePtr retval_type = std::get<FuncTypePtr>(func_type)->retval_type();

	INTERNAL_ASSERT(state.is_rval_mode(), "func call should not occur in lval");
	bool write_mode = state.is_write_mode();
	optional<Operand> prev_write_opr, prev_arr_offset;
	if(write_mode)
	{
		INTERNAL_ASSERT(!holds_alternative<VoidTypePtr>(retval_type), "write mode calling void function");
		prev_write_opr = state.store_write_opr();
		prev_arr_offset = state.store_arr_offset();
		state.reset_write_opr();
		state.reset_arr_offset();
	}

	std::vector<Operand> param_oprs;
	const FuncArgsNodePtr &args = node->actual_args();
	for(int i = 0; i < args->children_cnt(); i++)
	{
		auto param = visit(*this, args->get(i));
		INTERNAL_ASSERT(param.has_value(), "expected value for function parameter");
		param_oprs.push_back(param.value());
	}
	for(const Operand &param_opr : param_oprs)
		eeyore_code.emplace_back(ParamStmt(param_opr));
	
	if(write_mode)
	{
		state.restore_write_opr(prev_write_opr);
		state.restore_arr_offset(prev_arr_offset);
		if(state.is_write_array())
		{
			TempVar tmp = resources.get_temp_var();
			eeyore_code.emplace_back(DeclStmt(tmp));
			eeyore_code.emplace_back(FuncCallStmt(id_name, tmp));
			eeyore_code.emplace_back(WriteArrStmt(
				state.write_opr(), state.arr_offset(), tmp
			));
		}
		else
		{
			eeyore_code.emplace_back(FuncCallStmt(id_name, state.write_opr()));
		}
		return std::nullopt;
	}
	else // read mode
	{
		if(holds_alternative<VoidTypePtr>(retval_type))
		{
			eeyore_code.emplace_back(FuncCallStmt(id_name));
			return std::nullopt;
		}
		else // retval_type holds IntTypePtr
		{
			TempVar tmp = resources.get_temp_var();
			eeyore_code.emplace_back(DeclStmt(tmp));
			eeyore_code.emplace_back(FuncCallStmt(id_name, tmp));
			return tmp;
		}
	}
}

template<class T>
optional<Operand> EeyoreGenerator::operator() (const T &node)
{
	return std::nullopt;
}

const std::list<EeyoreStatement> &EeyoreGenerator::generate_eeyore(const AstPtr &ast)
{
	eeyore_code.clear();
	state.reset_all();

	std::visit(*this, ast);
	DBG(std::cout << "end visiting" << std::endl);

	rearranger.rearrange(eeyore_code);
	optimizer.optimize(eeyore_code);
	return eeyore_code;
}

std::vector<Operand> EeyoreGenerator::all_defined_vars()
{
	std::vector<Operand> vars;

	int max_param_cnt = 0; // Find the largest function argument to determine
						   // used parameter count.
	for(const auto &stmt : eeyore_code)
	{
		if(holds_alternative<DeclStmt>(stmt))
			vars.emplace_back(std::get<DeclStmt>(stmt).var);
		else if(holds_alternative<FuncDefStmt>(stmt))
		{
			max_param_cnt = std::max(max_param_cnt,
				std::get<FuncDefStmt>(stmt).arg_cnt);
		}
	}
	for(int i = 0; i < max_param_cnt; i++)
		vars.emplace_back(Param(i));
	return vars;
}

} // namespace compiler::backend::eeyore