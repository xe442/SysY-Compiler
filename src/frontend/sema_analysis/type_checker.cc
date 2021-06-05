#include "dbg.h"
#include "exceptions.h"
#include "fstring.h"
#include "semantic_checker.h"

using std::holds_alternative;
using compiler::utils::fstring;

namespace compiler::frontend
{

TypePtr SemanticChecker::TypeChecker::operator() (ConstIntNodePtr &node)
{
	return make_int(/* is_const */ true);
}

TypePtr SemanticChecker::TypeChecker::operator() (IdNodePtr &node)
{
	// Find from symbol table, and get its type.
	auto find_res = table.find(node->name());
	if(!find_res.has_value())
		throw SemanticError(node->location().begin,
			fstring("using undefined variable ", node->name()));
	return find_res.value().type;
}

TypePtr SemanticChecker::TypeChecker::operator() (UnaryOpNodePtr &node)
{
	// for unary operator NOT and NEG, the type of the expression is alway equal
	// to the operand type.
	return std::visit(*this, node->operand_());
}

TypePtr SemanticChecker::TypeChecker::operator() (BinaryOpNodePtr &node)
{
	TypePtr operand1_type = std::visit(*this, node->operand1_());
	TypePtr operand2_type = std::visit(*this, node->operand2_());

	if(node->op() == BinaryOpNode::ACCESS)
	{
		// Check operand1 type, must be an array or pointer.
		if(!holds_alternative<ArrayTypePtr>(operand1_type)
			&& !holds_alternative<PointerTypePtr>(operand1_type))
			throw SemanticError(node->location().begin,
				"operator [] can only work on array types.");
		
		// Check index type, must be int.
		if(!accept_type(make_int(), operand2_type))
			throw SemanticError(node->location().begin,
				fstring("error index type, exptected int, but got ", operand2_type));
		
		if(holds_alternative<ArrayTypePtr>(operand1_type))
			return std::get<ArrayTypePtr>(operand1_type)->element_type();
		else // PointerTypePtr
			return std::get<PointerTypePtr>(operand1_type)->base_type();
	}
	else if(node->op() == BinaryOpNode::ASSIGN)
	{
		// Check lval type, must be non-void basic type.
		if(!is_basic(operand1_type)
			|| std::holds_alternative<VoidTypePtr>(operand1_type))
		{
			throw SemanticError(node->location().begin,
				fstring("error lval type for assign statement, got ", operand1_type));
		}
		
		// Check rval type.
		if(!accept_type(operand1_type, operand2_type))
			throw SemanticError(node->location().begin,
				fstring("error type matching for assign statement, exptected ",
					operand1_type, ", but got ", operand2_type));
		return make_null(); // No ret type for ASSIGN statement.
	}
	else
	{
		// Check if they can operate.
		if(!can_operate(operand1_type, operand2_type))
			throw SemanticError(node->location().begin,
				fstring("error type for binary operator, got operand types: ",
					operand1_type, " and ", operand2_type));
		return common_type(operand1_type, operand2_type);
	}
}

TypePtr SemanticChecker::TypeChecker::operator() (FuncCallNodePtr &node)
{
	if(!holds_alternative<IdNodePtr>(node->id()))
		INTERNAL_ERROR("error type for id node of func call");
	
	// Find the id from the symbol table. Get & check its type.
	const auto &id = std::get<IdNodePtr>(node->id());
	auto find_res = table.find(id->name());
	if(!find_res.has_value())
		throw SemanticError(id->location().begin,
			fstring("use of undefined function ", id->name()));
	if(!holds_alternative<FuncTypePtr>(find_res.value().type))
		throw SemanticError(id->location().begin,
			fstring(id->name(), " is not callable"));
	const auto &id_type = std::get<FuncTypePtr>(find_res.value().type);
	
	// Check paramter count.
	if(!holds_alternative<FuncArgsNodePtr>(node->args()))
		INTERNAL_ERROR(fstring("error parameter type for function ", id->name()));
	auto &args = std::get<FuncArgsNodePtr>(node->args_());
	if(args->children_cnt() != id_type->arg_cnt())
		throw SemanticError(id->location().begin,
			fstring("error parameter count! exptected ", id_type->arg_cnt(),
				", but got ", args->children_cnt()));

	// Match parameters type.
	for(int i = 0; i < id_type->arg_cnt(); i++)
	{
		auto arg_type = std::visit(*this, args->get_(i));
		if(!accept_type(id_type->arg_type(i), arg_type))
			throw SemanticError(id->location().begin,
				fstring("error parameter #", i, ", exptected ", id_type->arg_type(i),
					", but got ", arg_type));
	}
	return id_type->retval_type();
}

template<class ErrorType>
TypePtr SemanticChecker::TypeChecker::operator() (ErrorType &node)
{
	INTERNAL_ERROR(
		fstring("invalid expression type ", typeid(node).name(),
		" for type checker")
	);
}

TypePtr SemanticChecker::TypeChecker::check_type(
	frontend::AstPtr &expr_node, bool optimize_constants)
{
	return std::visit(*this, expr_node);
}

} // namespace compiler::frontend