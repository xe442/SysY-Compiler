#define DEBUG
#include "dbg.h"
#include "exceptions.h"
#include "visitor_helper.h"
#include "type.h"
#include "ast_node.h"

using compiler::utils::LambdaVisitor;

namespace compiler::define
{

VarDeclNode::VarDeclNode(TypePtr base_type, AstPtrVec &&definitions)
  : XaryAstNodeBase(std::move(definitions)), _base_type(base_type)
{
	DBG(std::cout << "VarDeclNode built!" << std::endl);
}

SingleVarDeclNode::SingleVarDeclNode(AstPtr &&lval)
  : NaryAstNodeBase<2>({std::move(lval), NullAst()}), _type(make_null())
{
	DBG(std::cout << "SingleVarDeclNode built!" << std::endl);
}

SingleVarDeclNode::SingleVarDeclNode(AstPtr &&lval, AstPtr &&init_val)
  : NaryAstNodeBase<2>({std::move(lval), std::move(init_val)}), _type(make_null())
{
	DBG(std::cout << "SingleVarDeclNode built!" << std::endl);
}

ConstIntNode::ConstIntNode(int val)
  : _val(val)
{
	DBG(std::cout << "ConstIntNode built! val = " << _val << std::endl);
}

IdNode::IdNode(std::string name, yy::location loc)
  : _name(name), _loc(loc)
{
	DBG(std::cout << "IdNode built! name = " << _name << std::endl);
}

IdNode::IdNode(TypePtr type, std::string name, yy::location loc)
  : _name(name), _loc(loc)
{
	DBG(std::cout << "IdNode built! name = " << _name << std::endl);
}

InitializerNode::InitializerNode(yy::location loc)
  : XaryAstNodeBase(), _expected_type(make_null())
{
	DBG(std::cout << "InitializerNode built! len = " << children_cnt() << std::endl);
}

InitializerNode::InitializerNode(AstPtrVec &&sub_initializer, yy::location loc)
  : XaryAstNodeBase(std::move(sub_initializer)), _expected_type(make_null()), _loc(loc)
{
	DBG(std::cout << "InitializerNode built! len = " << children_cnt() << std::endl);
}

FuncDefNode::FuncDefNode(TypePtr retval_type, AstPtr &&name, AstPtr &&block)
  // paramless function definition
  : NaryAstNodeBase<3>({std::move(name), std::make_unique<FuncParamsNode>(), std::move(block)}),
	_retval_type(retval_type)
{
	DBG(std::cout << "FuncDefNode built! ret_type = " << _retval_type << std::endl);
}

FuncDefNode::FuncDefNode(TypePtr retval_type, AstPtr &&name, AstPtr &&param_list, AstPtr &&block)
  : NaryAstNodeBase<3>({std::move(name), std::move(param_list), std::move(block)}),
		_retval_type(retval_type)
{
	DBG(std::cout << "FuncDefNode built! ret_type = " << _retval_type << std::endl);
}

FuncParamsNode::FuncParamsNode(AstPtrVec &&params)
  : XaryAstNodeBase(std::move(params))
{
	DBG(std::cout << "FuncParamsNode built! len = " << children_cnt() << std::endl);
}

TypePtrVec FuncParamsNode::get_param_types()
{
	TypePtrVec param_types;
	for(const AstPtr &child : children())
		param_types.push_back(std::get<SingleFuncParamNodePtr>(child)->type());
	return param_types;
}

SingleFuncParamNode::SingleFuncParamNode(TypePtr base_type, AstPtr &&lval)
  : NaryAstNodeBase<1>({std::move(lval)}), _type(base_type)
{
	DBG(std::cout << "SingleFuncParamNode built! type = " << _type << std::endl);
}

UnaryOpNode::UnaryOpNode(OpType op, AstPtr &&operand, yy::location loc)
  : NaryAstNodeBase<1>({std::move(operand)}), _op(op), _loc(loc)
{
	DBG(std::cout << "UnaryOpNode built! op = " << _op << std::endl);
}

BinaryOpNode::BinaryOpNode(OpType op, AstPtr &&operand1, AstPtr &&operand2, yy::location loc)
  : NaryAstNodeBase<2>({std::move(operand1), std::move(operand2)}), _op(op), _loc(loc)
{
	DBG(std::cout << "BinaryOpNode built! op = " << _op << std::endl);
}

IfNode::IfNode(AstPtr &&expr, AstPtr &&if_body)
  : NaryAstNodeBase<3>({std::move(expr), std::move(if_body), NullAst()})
{
	DBG(std::cout << "IfNode built without else!" << std::endl);
}

IfNode::IfNode(AstPtr &&expr, AstPtr &&if_body, AstPtr &&else_body)
  : NaryAstNodeBase<3>({std::move(expr), std::move(if_body), std::move(else_body)})
{
	DBG(std::cout << "IfNode built with else!" << std::endl);
}

WhileNode::WhileNode(AstPtr &&expr, AstPtr &&body)
  : NaryAstNodeBase<2>({std::move(expr), std::move(body)})
{
	DBG(std::cout << "WhileNode built!" << std::endl);
}

BreakNode::BreakNode(yy::location loc): _loc(loc)
{
	DBG(std::cout << "BreakNode built!" << std::endl);
}

ContNode::ContNode(yy::location loc): _loc(loc)
{
	DBG(std::cout << "ContNode built!" << std::endl);
}

RetNode::RetNode(yy::location loc)
  : NaryAstNodeBase({NullAst()}), _loc(loc)
{
	DBG(std::cout << "RetNode built without retval!" << std::endl);
}

RetNode::RetNode(AstPtr &&expr, yy::location loc)
  : NaryAstNodeBase({std::move(expr)}), _loc(loc)
{
	DBG(std::cout << "RetNode built with retval!" << std::endl);
}

FuncCallNode::FuncCallNode(AstPtr &&id)
  : NaryAstNodeBase<2>({std::move(id), std::make_unique<FuncArgsNode>()})
{
	DBG(std::cout << "FuncCallNode built!" << std::endl);
}

FuncCallNode::FuncCallNode(AstPtr &&id, AstPtr &&args)
  : NaryAstNodeBase<2>({std::move(id), std::move(args)})
{
	DBG(std::cout << "FuncCallNode built!" << std::endl);
}

FuncArgsNode::FuncArgsNode(AstPtrVec &&args)
  : XaryAstNodeBase(std::move(args))
{
	DBG(std::cout << "FuncArgsNode built!" << std::endl);
}

} // namespace compiler::define