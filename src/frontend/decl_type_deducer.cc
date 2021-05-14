#define DEBUG
#include "dbg.h"
#include <iostream>
#include "fstring.h"
#include "semantic_checker.h"

using compiler::utils::fstring;
using namespace compiler::define;

namespace compiler::frontend
{

void SemanticChecker::DeclTypeDeducer::operator() (IdNodePtr &node)
{
	_id_node_ptr = node.get();
	return;
}

void SemanticChecker::DeclTypeDeducer::operator() (UnaryOpNodePtr &node)
{
	if(node->op() != UnaryOpNode::POINTER)
		INTERNAL_ERROR(fstring("invalid unary operator ", node->op(), " for decl"));
	if(_is_ptr)
		INTERNAL_ERROR("pointer to pointer is not allowed");
	
	_is_ptr = true;
	std::visit(*this, node->operand_());
}

void SemanticChecker::DeclTypeDeducer::operator() (BinaryOpNodePtr &node)
{
	if(node->op() != BinaryOpNode::ACCESS)
		INTERNAL_ERROR(fstring("invalid binary operator ", node->op(), " for decl"));

	// calculate rval type, it must be a constexpr.
	const_expr_replacer.replace_expr(node->operand2_());
	int size = std::get<ConstIntNodePtr>(node->operand2())->val();
	
	std::visit(*this, node->operand1_());
	_dim_size.push_back(size);
}

template<class ErrorType>
void SemanticChecker::DeclTypeDeducer::operator() (ErrorType &node)
{
	INTERNAL_ERROR(fstring("invalid declaration statement, got ", typeid(node).name()));
}

TypePtr SemanticChecker::DeclTypeDeducer::deduce_type(AstPtr &decl_expr,
													  const TypePtr &base_type)
{
	DBG(std::cout << "decl type deducer called!" << std::endl);
	_dim_size.clear();
	_is_ptr = false;
	std::visit(*this, decl_expr);

	DBG(if(_is_ptr) { std::cout << "pointer of "; });
	if(_dim_size.empty())
	{
		DBG(std::cout << "base type: " << base_type << std::endl);
		if(_is_ptr)
		{
			IdNodePtr simplified_id = std::make_unique<IdNode>(*_id_node_ptr);
			decl_expr = std::move(simplified_id);
			return std::make_shared<PointerType>(base_type);
		}
		else
			return base_type;
	}
	else
	{
		DBG(std::cout << "array type of " << base_type << std::endl);
		// do simplification
		IdNodePtr simplified_id = std::make_unique<IdNode>(*_id_node_ptr);
		decl_expr = std::move(simplified_id);

		TypePtr tp = std::make_shared<ArrayType>(base_type, _dim_size);
		if(_is_ptr)
			tp = std::make_shared<PointerType>(tp);
		return tp;
	}
}

} // namespace compiler::frontend