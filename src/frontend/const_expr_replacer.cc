#define DEBUG
#include "dbg.h"
#include "ast_node_printer.h"
#include <typeinfo>
#include "exceptions.h"
#include "fstring.h"
#include "semantic_checker.h"

using std::visit;
using std::holds_alternative;
using std::make_unique;
using compiler::utils::fstring;
using namespace compiler::define;

namespace compiler::frontend
{

void SemanticChecker::ConstExprReplacer::replace_expr(AstPtr &node)
{
	node = visit(*this, node);
}

AstPtr SemanticChecker::ConstExprReplacer::operator() (ConstIntNodePtr &node)
{
	DEBUG(std::cout << "literal " << node->val() << std::endl);
	return std::move(node);
}
AstPtr SemanticChecker::ConstExprReplacer::operator() (IdNodePtr &node)
{
	auto find_res = table.find(node->name());
	if(!find_res.has_value())
		throw SemanticError(node->location().begin, "using undefined variable in constexpr");
	
	const TypePtr &type = find_res.value().type;
	if(!is_const(type))
		throw SemanticError(node->location().begin, "using non-const variable in constexpr");
	
	std::optional<int> init_val = find_res.value().initial_val;
	if(!init_val.has_value())
		INTERNAL_ERROR(fstring("non-initialized const variable ", node->name()));
	DBG(std::cout << "id " << node->name() << " = " << init_val.value() << std::endl);
	return make_unique<ConstIntNode>(init_val.value());
}
AstPtr SemanticChecker::ConstExprReplacer::operator() (InitializerNodePtr &node)
{
	for(auto &child : node->children_())
		child = visit(*this, child);
	return std::move(node);
}
AstPtr SemanticChecker::ConstExprReplacer::operator() (UnaryOpNodePtr &node)
{
	AstPtr operand = std::visit(*this, node->operand_());
	if(!holds_alternative<ConstIntNodePtr>(operand))
		INTERNAL_ERROR("error operand type returned for constexpr unary operator");
	
	int val = std::get<ConstIntNodePtr>(operand)->val();
	int target_val;
	switch(node->op())
	{
		case UnaryOpNode::NEG:
			target_val = -val; break;
		case UnaryOpNode::NOT:
			target_val = !val; break;
		case UnaryOpNode::POINTER:
			throw SemanticError(node->location().begin, "pointer types should not occur in constexpr");
	}
	DBG(std::cout << node->op() << val << '=' << target_val << std::endl);
	return make_unique<ConstIntNode>(target_val);
}
AstPtr SemanticChecker::ConstExprReplacer::operator() (BinaryOpNodePtr &node)
{
	AstPtr operand1 = std::visit(*this, node->operand1_()),
		   operand2 = std::visit(*this, node->operand2_());
	if(!holds_alternative<ConstIntNodePtr>(operand1)
		|| !holds_alternative<ConstIntNodePtr>(operand2))
	{
		INTERNAL_ERROR("error operand type returned for constexpr bianry operator");
	}

	int val1 = std::get<ConstIntNodePtr>(operand1)->val();
	int val2 = std::get<ConstIntNodePtr>(operand2)->val();
	int target_val;
	switch(node->op())
	{
		case BinaryOpNode::ADD:
			target_val = val1 + val2; break;
		case BinaryOpNode::SUB:
			target_val = val1 - val2; break;
		case BinaryOpNode::MUL:
			target_val = val1 * val2; break;
		case BinaryOpNode::DIV:
			target_val = val1 / val2; break;
		case BinaryOpNode::MOD:
			target_val = val1 % val2; break;
		case BinaryOpNode::OR:
			target_val = val1 || val2; break;
		case BinaryOpNode::AND:
			target_val = val1 && val2; break;
		case BinaryOpNode::GT:
			target_val = val1 > val2; break;
		case BinaryOpNode::LT:
			target_val = val1 < val2; break;
		case BinaryOpNode::GE:
			target_val = val1 >= val2; break;
		case BinaryOpNode::LE:
			target_val = val1 <= val2; break;
		case BinaryOpNode::EQ:
			target_val = val1 == val2; break;
		case BinaryOpNode::NE:
			target_val = val1 != val2; break;
		case BinaryOpNode::ASSIGN:
			throw SemanticError(node->location().begin, "assign statements should not occur in constexpr");
		case BinaryOpNode::ACCESS:
			throw SemanticError(node->location().begin, "access statements should not occur in constexpr");
	}
	DBG(std::cout << val1 << node->op() << val2 << '=' << target_val << std::endl);
	return make_unique<ConstIntNode>(target_val);
}

template<class ErrorType>
AstPtr SemanticChecker::ConstExprReplacer::operator() (ErrorType &tp)
{
	INTERNAL_ERROR(utils::fstring("error ast type for const expr, got ", typeid(ErrorType).name()));
}

} // namespace compiler::frontend