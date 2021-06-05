#include "dbg.h"
#include "exceptions.h"
#include "fstring.h"
#include "semantic_checker.h"

using std::visit;
using std::holds_alternative;
using compiler::utils::fstring;

namespace compiler::frontend
{

const char SemanticChecker::_SEMANTIC_ERROR_NAME[] = "Semantic error";

SemanticChecker::SemanticChecker()
  : table(),
  	const_expr_replacer(table),
	type_deducer(const_expr_replacer),
	type_checker(table),
	initilaizer_type_checker(const_expr_replacer)
{}

void SemanticChecker::operator() (ProgramNodePtr &node)
{
	for(auto &child : node->children_())
		visit(*this, child);
}

void SemanticChecker::operator() (VarDeclNodePtr &node)
{
	base_type = node->base_type();
	for(auto &child : node->children_())
		visit(*this, child);
	base_type = make_null();
}

void SemanticChecker::operator() (SingleVarDeclNodePtr &node)
{
	// Get id & id_type.
	TypePtr id_type = type_deducer.deduce_type(node->lval_(), base_type);
	DBG(std::cout << "deduced " << id_type << std::endl);
	
	if(!holds_alternative<IdNodePtr>(node->lval()))
		INTERNAL_ERROR("type deducer crashed!");
	const auto &id = std::get<IdNodePtr>(node->lval());
	node->set_type(id_type);
	const auto &id_name = id->name();

	// Add the id to symbol table. We add it before rval analysis because there
	// are cases where rval depends on the id itself, such that
	// "int a[2] = {0, a[1]}".
	if(is_const(id_type) && is_basic(id_type))
	{
		// For const basic type, we need its initial value before adding it to
		// the symbol table.
		if(is_null_ast(node->init_val()))
		{
			if(!is_global())
				throw SemanticError(id->location().begin,
					"local const basic variable must have a initial value"
				);
			node->set_init_val(std::make_unique<ConstIntNode>(0));
		}
		const_expr_replacer.replace_expr(node->init_val_());
		int id_init_val = std::get<ConstIntNodePtr>(node->init_val())->val();
		if(!table.insert(id_name, id_type, id_init_val))
			throw SemanticError(id->location().begin,
				fstring("redefinition of variable ", id_name)
			);
		DBG(std::cout << "added const int type variable " << id_name << " to symbol table" << std::endl);
		return; // We are done for const int type.
	}
	if(!table.insert(id->name(), id_type))
		throw SemanticError(id->location().begin,
			fstring("redefinition of variable ", id_name)
		);
	DBG(std::cout << "added " << id_type << " type variable " << id_name << " to symbol table" << std::endl);

	// Check if id_type accepts rval type.
	if(is_basic(id_type))
	{
		if(!is_null_ast(node->init_val()))
		{
			auto rval_type = type_checker.check_type(node->init_val_(), is_const(id_type));
			if(!accept_type(id_type, rval_type))
			{
				const auto &pos = id->location().end;
				throw SemanticError(pos, 
					fstring("invalid initial value type for id ", id_name, ", expected ",
						id_type, ", but got ", rval_type)
				);
			}
		}
		else if(is_global())
			// We are declaring a global variable, set its initial value to zero.
			node->set_init_val(std::make_unique<ConstIntNode>(0));
	}
	else // array types
	{
		if(is_global() && is_null_ast(node->init_val()))
			node->set_init_val(std::make_unique<InitializerNode>());
		if(!is_null_ast(node->init_val()))
			initilaizer_type_checker.check_type(node->init_val_(), id_type);
	}
}

void SemanticChecker::operator() (FuncDefNodePtr &node)
{
	// Save the previous curr_func.
	const auto *prev_func = curr_func;
	curr_func = node.get();

	// Check function params.
	visit(*this, node->params_());
	DBG(std::cout << "func types checked" << std::endl);

	// Get the param types.
	TypePtrVec param_types;
	for(const AstPtr &param_v : node->actual_params()->children())
	{
		const auto &param = std::get<SingleFuncParamNodePtr>(param_v);
		param_types.push_back(param->type());
	}
	
	// Add function name and type to symbol table.
	const TypePtr &retval_type = node->retval_type();
	const auto &id = std::get<IdNodePtr>(node->name());
	node->set_type(std::make_shared<FuncType>(retval_type, std::move(param_types)));
	if(!table.insert(id->name(), node->type()))
		throw SemanticError(id->location().begin,
			fstring("conflict definition of id ", id->name()));

	// Add parameter names & types to local block.
	table.new_block();
	for(const AstPtr &param_v : node->actual_params()->children())
	{
		const auto &param = std::get<SingleFuncParamNodePtr>(param_v);
		const auto &id = std::get<IdNodePtr>(param->lval());
		table.insert(id->name(), param->type());
	}

	// Check function body.
	std::visit(*this, node->block_());

	table.end_block(); // clear function param names.
	curr_func = prev_func; // Restore the previous curr_func.
}

void SemanticChecker::operator() (FuncParamsNodePtr &node)
{
	for(auto &child : node->children_())
		visit(*this, child);
}

void SemanticChecker::operator() (SingleFuncParamNodePtr &node)
{
	TypePtr arg_type = type_deducer.deduce_type(node->lval_(), node->type());
		// now node->lval_() is an IdNodePtr
	node->set_type(arg_type);
}

void SemanticChecker::operator() (BlockNodePtr &node)
{
	table.new_block();
	for(auto &child : node->children_())
		visit(*this, child);
	table.end_block();
}

void SemanticChecker::operator() (IfNodePtr &node)
{
	DBG(std::cout << "in if" << std::endl);
	visit(*this, node->expr_());
	visit(*this, node->if_body_());
	if(!is_null_ast(node->else_body()))
		visit(*this, node->else_body_());
	DBG(std::cout << "end if" << std::endl);
}

void SemanticChecker::operator() (WhileNodePtr &node)
{
	const auto prev_inner_loop = inner_loop; // Store the previous inner_loop on the stack.
	inner_loop = node.get();
	DBG(std::cout << "in while loop" << std::endl);
	for(auto &child : node->children_())
		visit(*this, child);
	DBG(std::cout << "end while loop" << std::endl);
	inner_loop = prev_inner_loop; // Restore the previous inner_loop.
}

void SemanticChecker::operator() (BreakNodePtr &node)
{
	if(inner_loop == nullptr)
		throw SemanticError(node->location().begin, "break statement not placed inside a loop");
}

void SemanticChecker::operator() (ContNodePtr &node)
{
	if(inner_loop == nullptr)
		throw SemanticError(node->location().begin, "continue statement not placed inside a loop");
}

void SemanticChecker::operator() (RetNodePtr &node)
{
	if(is_global())
		INTERNAL_ERROR("return statment cannot be outside a function!");
	
	const TypePtr &expected_retval_type = curr_func->retval_type();

	if(is_null_ast(node->expr()))
	{
		if(!holds_alternative<VoidTypePtr>(expected_retval_type))
			throw SemanticError(node->location().begin,
				"return void in non void-returning function");
	}
	else
	{
		TypePtr retval_type = type_checker.check_type(node->expr_());
		if(!accept_type(expected_retval_type, retval_type))
		{
			throw SemanticError(node->location().end,
				fstring("return type does not match, exptected ", expected_retval_type,
					", but got ", retval_type)
			);
		}
	}
}

void SemanticChecker::operator() (ConstIntNodePtr &node)
{
	// type_checker.optimize_constants = false;
	type_checker(node);
}
void SemanticChecker::operator() (IdNodePtr &node)
{
	// type_checker.optimize_constants = false;
	type_checker(node);
}

void SemanticChecker::operator() (UnaryOpNodePtr &node)
{
	// type_checker.optimize_constants = false;
	type_checker(node);
}

void SemanticChecker::operator() (BinaryOpNodePtr &node)
{
	// type_checker.optimize_constants = false;
	type_checker(node);
}

void SemanticChecker::operator() (FuncCallNodePtr &node)
{
	// type_checker.optimize_constants = false;
	type_checker(node);
}

template<class ErrorNodePtr>
void SemanticChecker::operator() (ErrorNodePtr &node)
{
	INTERNAL_ERROR("invalid call to semantic checker");
}

} // namespace compiler::frontend