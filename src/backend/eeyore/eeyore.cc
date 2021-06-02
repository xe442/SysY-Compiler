#include "visitor_helper.h"
#include "eeyore.h"

namespace
{

void remove_int_from_vec(std::vector<compiler::backend::eeyore::Operand> &oprs)
{
	for(auto iter = oprs.begin(); iter != oprs.end(); )
	{
		if(std::holds_alternative<int>(*iter))
			iter = oprs.erase(iter);
		else
			++iter;
	}
}

} //  namespace

namespace compiler::backend::eeyore
{

std::vector<Operand> used_vars(const EeyoreStatement &stmt)
{
	using Operands = std::vector<Operand>;
	static utils::LambdaVisitor used_opr_getter = {
		[](const ParamStmt &stmt) { return Operands{stmt.param}; },
		[](const RetStmt &stmt)
		{
			const auto &retval = stmt.retval;
			return retval.has_value()? Operands{retval.value()} : Operands{};
		},
		[](const CondGotoStmt &stmt) { return Operands{stmt.opr1, stmt.opr2}; },
		[](const UnaryOpStmt &stmt) { return Operands{stmt.opr1}; },
		[](const BinaryOpStmt &stmt) { return Operands{stmt.opr1, stmt.opr2}; },
		[](const MoveStmt &stmt) { return Operands{stmt.opr1}; },
		[](const ReadArrStmt &stmt) { return Operands{stmt.arr_opr, stmt.idx_opr}; },
		[](const WriteArrStmt &stmt) { return Operands{stmt.opr, stmt.idx_opr}; },
		[](const auto &stmt) { return Operands{}; }
	};

	Operands used_oprs = std::visit(used_opr_getter, stmt); 
	remove_int_from_vec(used_oprs);
	return used_oprs;
}

std::vector<Operand> defined_vars(const EeyoreStatement &stmt)
{
	using Operands = std::vector<Operand>;
	static utils::LambdaVisitor defined_opr_getter = {
		[](const UnaryOpStmt &stmt) { return Operands{stmt.opr}; },
		[](const BinaryOpStmt &stmt) { return Operands{stmt.opr}; },
		[](const MoveStmt &stmt) { return Operands{stmt.opr}; },
		[](const ReadArrStmt &stmt) { return Operands{stmt.opr}; },
		[](const WriteArrStmt &stmt) { return Operands{stmt.arr_opr}; },
		[](const auto &stmt) { return Operands{}; }
	};

	Operands defined_oprs = std::visit(defined_opr_getter, stmt);
	remove_int_from_vec(defined_oprs);
	return defined_oprs;
}

} // namespace compiler::backend::eeyore