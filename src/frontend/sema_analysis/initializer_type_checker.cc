#include "dbg.h"
#include "semantic_checker.h"
#include "fstring.h"
#include "ast_node_printer.h"

using std::visit;
using compiler::utils::fstring;

namespace compiler::frontend
{

bool SemanticChecker::InitializerTypeChecker::operator() (const ArrayTypePtr &arr_type)
{
	if(_idx == _parent_ptr->children_cnt()) // exhaused
	{
		DBG(std::cout << "exhaused "; TypePrinter{std::cout}(arr_type);
			std::cout << std::endl;);
		return true;
	}
	AstPtr &curr = _parent_ptr->get_(_idx);

	if(std::holds_alternative<InitializerNodePtr>(curr))
	{
		// Match a initializer with an array.
		DBG(std::cout << "match " << curr << " with ";
			TypePrinter{std::cout}(arr_type); std::cout << std::endl);

		// Store _parent_ptr.
		auto prev__parent_ptr = _parent_ptr;
		int prev__idx = _idx;
		_parent_ptr = std::get<InitializerNodePtr>(curr).get();
		_idx = 0;

		for(int i = 0; i < arr_type->len(); i++)
			if(std::visit(*this, arr_type->element_type())) // exhaused, finish the check
				break;
		// Check if there is exceed element.
		if(_idx < _parent_ptr->children_cnt())
			throw SemanticError(_parent_ptr->location().end,
				"too many elements in the initializer list");
		
		// Restore _parent_ptr.
		_parent_ptr = prev__parent_ptr;
		_idx = prev__idx + 1; // move to next match
	}
	else
	{
		// Match expressions with an array.
		if(arr_type->is_1d_arr())
		{
			InitializerNodePtr arr_rebuild = std::make_unique<InitializerNode>();
			int parent_len = _parent_ptr->children_cnt();
			int i;
			for(i = 0; i < arr_type->len() && _idx + i < parent_len; i++)
			{
				AstPtr &ith_expr = _parent_ptr->get_(_idx + i);
				if(std::holds_alternative<InitializerNodePtr>(ith_expr))
					throw SemanticError(
						std::get<InitializerNodePtr>(ith_expr)->location().begin,
						"expected an expression but got an initializer"
					);
				if(_const_mode)
					const_expr_replacer.replace_expr(_parent_ptr->get_(_idx + i));
				arr_rebuild->push_back(std::move(ith_expr));
			}

			// Replace these expressions with a single InitializerNode.
			auto &children = _parent_ptr->children_();
			children.erase(
				children.begin() + _idx + 1,
				children.begin() + _idx + i
			);
			children[_idx] = std::move(arr_rebuild);
			_idx++;
		}
		else
		{
			// Handle them until 1-d array.
			return std::visit(*this, arr_type->element_type());
		}
	}
	return false;
}

bool SemanticChecker::InitializerTypeChecker::operator() (const IntTypePtr &ele_type)
{
	if(_idx == _parent_ptr->children_cnt()) // exhaused
		return true;
	AstPtr &curr = _parent_ptr->get_(_idx);

	if(std::holds_alternative<InitializerNodePtr>(curr))
		throw SemanticError(_parent_ptr->location().begin,
			fstring("expted array type ", ele_type, ", but got an expression"));
	
	DBG(std::cout << "match " << _parent_ptr->get(_idx) << " with expr" << std::endl);
	if(_const_mode)
		const_expr_replacer.replace_expr(_parent_ptr->get_(_idx));
	
	_idx++;
	return false;
}

template<class ErrorType>
bool SemanticChecker::InitializerTypeChecker::operator() (const ErrorType &err_type)
{
	INTERNAL_ERROR("error type for initializer type checker");
}

void SemanticChecker::InitializerTypeChecker::check_type(
	AstPtr &initializer, const TypePtr &arr_type)
{
	if(!std::holds_alternative<InitializerNodePtr>(initializer))
		INTERNAL_ERROR("invalid initializer type for initializer type checker");
	if(!std::holds_alternative<ArrayTypePtr>(arr_type))
		INTERNAL_ERROR("invalid type for initializer type checker");
	
	auto &_initializer = std::get<InitializerNodePtr>(initializer);
	const auto &_arr_type = std::get<ArrayTypePtr>(arr_type);
	_parent_ptr = _initializer.get();
	_idx = 0;
	_const_mode = is_const(arr_type);
	for(int i = 0; i < _arr_type->len(); i++)
		if(std::visit(*this, _arr_type->element_type()))
			break;
	if(_idx < _parent_ptr->children_cnt())
		throw SemanticError(_parent_ptr->location().end,
			"too many elements in the initializer list");
}

} // namespace compiler::frontend