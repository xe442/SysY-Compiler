#ifndef AST_NODE_H
#define AST_NODE_H

#include <iostream>
#include <memory>
#include <variant>
#include <vector>
#include "type.h"
#include "location.h"

namespace compiler::frontend
{

// the abstract basic classes
class AstNodeBase;
class AstLeafNodeBase;
template<int N> class NaryAstNodeBase;
class XaryAstNodeBase;

// all the node classes of AST
class ProgramNode;
class VarDeclNode;
class SingleVarDeclNode;
class ConstIntNode;
class IdNode;
class InitializerNode;
class FuncDefNode;
class FuncParamsNode;
class SingleFuncParamNode;
class BlockNode;
class UnaryOpNode;
class BinaryOpNode;
class IfNode;
class WhileNode;
class BreakNode;
class ContNode;
class RetNode;
class FuncCallNode;
class FuncArgsNode;

// all the node pointer classes of AST
// They are all std::unqiue_ptr's of the corresponding node class.
using ProgramNodePtr = std::unique_ptr<ProgramNode>;
using VarDeclNodePtr = std::unique_ptr<VarDeclNode>;
using SingleVarDeclNodePtr = std::unique_ptr<SingleVarDeclNode>;
using ConstIntNodePtr = std::unique_ptr<ConstIntNode>;
using IdNodePtr = std::unique_ptr<IdNode>;
using InitializerNodePtr = std::unique_ptr<InitializerNode>;
using FuncDefNodePtr = std::unique_ptr<FuncDefNode>;
using FuncParamsNodePtr = std::unique_ptr<FuncParamsNode>;
using SingleFuncParamNodePtr = std::unique_ptr<SingleFuncParamNode>;
using BlockNodePtr = std::unique_ptr<BlockNode>;
using UnaryOpNodePtr = std::unique_ptr<UnaryOpNode>;
using BinaryOpNodePtr = std::unique_ptr<BinaryOpNode>;
using IfNodePtr = std::unique_ptr<IfNode>;
using WhileNodePtr = std::unique_ptr<WhileNode>;
using BreakNodePtr = std::unique_ptr<BreakNode>;
using ContNodePtr = std::unique_ptr<ContNode>;
using RetNodePtr = std::unique_ptr<RetNode>;
using FuncCallNodePtr = std::unique_ptr<FuncCallNode>;
using FuncArgsNodePtr = std::unique_ptr<FuncArgsNode>;


// The universal pointer class.
// We would always assume that AstPtr is passed as rvalue's, since they are
// unique_ptr's and they have to be passed by std::move.
using AstPtr = std::variant
<
	std::monostate, // indicating nullptrs
	ProgramNodePtr,
	VarDeclNodePtr,
	SingleVarDeclNodePtr,
	ConstIntNodePtr,
	IdNodePtr,
	InitializerNodePtr,
	FuncDefNodePtr,
	FuncParamsNodePtr,
	SingleFuncParamNodePtr,
	BlockNodePtr,
	UnaryOpNodePtr,
	BinaryOpNodePtr,
	IfNodePtr,
	WhileNodePtr,
	BreakNodePtr,
	ContNodePtr,
	RetNodePtr,
	FuncCallNodePtr,
	FuncArgsNodePtr
>;
template<int N> using AstPtrArr = std::array<AstPtr, N>;
using AstPtrVec = std::vector<AstPtr>;
using NullAst = std::monostate;

inline bool is_null_ast(const AstPtr &node)
	{ return std::holds_alternative<std::monostate>(node); }

class AstNodeBase
{
};

class AstLeafNodeBase: public AstNodeBase
{
};

// an AST node with known children count
// implemented using std::array
template<int N>
class NaryAstNodeBase: public AstNodeBase
{
  protected:
	AstPtrArr<N> _children;

  public:
  	NaryAstNodeBase() = default;
  	NaryAstNodeBase(AstPtrArr<N> &&children)
  	  : _children(std::move(children))
  	{}

  	inline std::size_t children_cnt() const { return _children.size(); }

  	// getter & setter
	inline const AstPtrArr<N> &children() const { return _children; }
	inline AstPtrArr<N> &children_() { return _children; }
	inline const AstPtr &get(int idx) const { return _children[idx]; }
	inline AstPtr &get_(int idx) { return _children[idx]; }
	inline void set(int idx, AstPtr &&val) { _children[idx] = std::move(val); }
};

// an AST node with unknown children count
// implemented using std::vector
class XaryAstNodeBase
{
  protected:
  	AstPtrVec _children;

  public:
  	XaryAstNodeBase() = default;
  	XaryAstNodeBase(AstPtrVec &&children)
  	  : _children(std::move(children))
  	{}

  	inline std::size_t children_cnt() const { return _children.size(); }

	// Getters & setters.
	inline const AstPtrVec &children() const { return _children; }
	inline AstPtrVec &children_() { return _children; }
	inline const AstPtr &get(int idx) const { return _children[idx]; }
	inline const AstPtr &back() const { return _children.back(); }
	inline AstPtr &get_(int idx) { return _children[idx]; }
	inline AstPtr &back_() { return _children.back(); }
	inline void set(int idx, AstPtr &&val) { _children[idx] = std::move(val); }
	inline void push_back(AstPtr &&ele) { _children.push_back(std::move(ele)); }
};


// ProgramNode
//  -> (VarDeclNode|FuncDefNode)*
class ProgramNode: public XaryAstNodeBase
{
  public:
  	ProgramNode() = default;
  	ProgramNode(AstPtrVec &&decls)
  	  : XaryAstNodeBase(std::move(decls)) {}
};

// VarDeclNode
//  -> SingleVarDeclNode+
class VarDeclNode: public XaryAstNodeBase
{
	TypePtr _base_type;
  public:
  	VarDeclNode() = default;
  	VarDeclNode(TypePtr base_type, AstPtrVec &&definitions);

	TypePtr base_type() { return _base_type; }
};

// SingleVarDeclNode
//  -> BinaryOpNode(ACCESS only)|IdNode, ExprNode|InitializerNode|nullptr
// ExprNode = IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode
class SingleVarDeclNode: public NaryAstNodeBase<2>
{
	TypePtr _type; // The type of the declared variable. Set during semantic analysis.
  public:
	SingleVarDeclNode() = default;
	SingleVarDeclNode(AstPtr &&lval);
	SingleVarDeclNode(AstPtr &&lval, AstPtr &&init_val);
	
	// Getters & setters.
	inline TypePtr type() const { return _type; }
	inline const AstPtr &lval() const { return get(0); }
	inline const AstPtr &init_val() const { return get(1); }
	inline AstPtr &lval_() { return get_(0); }
	inline AstPtr &init_val_() { return get_(1); }
	inline void set_type(TypePtr type) { _type = type; }
	inline void set_init_val(AstPtr &&init_val) { set(1, std::move(init_val)); }
};

class ConstIntNode: public AstLeafNodeBase
{
  protected:
	int _val;
  
  public:
	ConstIntNode() = default;
	ConstIntNode(int val);

	inline int val() const { return _val; }
};

class IdNode: public AstLeafNodeBase
{
  protected:
	std::string _name;
	yy::position _pos;
	yy::location _loc;

  public:
	IdNode() = default;
	IdNode(TypePtr type, std::string name, yy::location loc);

	// Typeless id, used during syntax analysis of vairable declaration. Their types are set later
	// during sematic analysis.
	IdNode(std::string name, yy::location loc);

	// Getters & setters.
	inline const std::string &name() { return _name; }
	inline yy::location location() const { return _loc; }
};

// InitializerNode
//  -> (InitializerNode|ExprNode)*
// ExprNode = IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode
class InitializerNode: public XaryAstNodeBase
{
  protected:
	yy::location _loc;

  public:
	InitializerNode() = default;

	// typeless InitializerNode
	InitializerNode(yy::location loc);
	InitializerNode(AstPtrVec &&sub_initializer, yy::location loc);
	
	// Getters & setters
	yy::location location() const { return _loc; }
};

// FuncDefNode
//  -> IdNode, FuncParamsNode, BlockNode
class FuncDefNode: public NaryAstNodeBase<3>
{
	TypePtr _retval_type;
	TypePtr _func_type;

  public:
	FuncDefNode() = default;
	FuncDefNode(TypePtr retval_type, AstPtr &&name, AstPtr &&block);
	FuncDefNode(TypePtr retval_type, AstPtr &&name, AstPtr &&param_list, AstPtr &&block);

	// Getters & setters.
	inline const TypePtr &retval_type() const { return _retval_type; }
	inline const TypePtr &type() const { return _func_type; }
	inline const AstPtr &name() const { return get(0); }
	inline const AstPtr &params() const { return get(1); }
	inline const AstPtr &block() const { return get(2); }
	inline const IdNodePtr &actual_name() const
		{ return std::get<IdNodePtr>(get(0)); }
	inline const FuncParamsNodePtr &actual_params() const
		{ return std::get<FuncParamsNodePtr>(get(1)); }
	inline const BlockNodePtr &actual_block() const
		{ return std::get<BlockNodePtr>(get(2)); }
	inline AstPtr &name_() { return get_(0); }
	inline AstPtr &params_() { return get_(1); }
	inline AstPtr &block_() { return get_(2); }
	inline void set_type(const TypePtr &type) { _func_type = type; }
};

// FuncParamsNode
//  -> SingleFuncParamNode*
class FuncParamsNode: public XaryAstNodeBase
{
  public:
	FuncParamsNode() = default;
	FuncParamsNode(AstPtrVec &&params);
};

// SingleFuncParamNode
//  -> BinaryOpNode(ACCESS only)|UnaryOpNode(POINTER only)|IdNode
class SingleFuncParamNode: public NaryAstNodeBase<1>
{
	TypePtr _type;  // This is set only to basic type during syntax analysis.
  public:
	SingleFuncParamNode() = default;
	SingleFuncParamNode(TypePtr base_type, AstPtr &&lval);

	// Getters & setters.
	inline const TypePtr &type() const { return _type; }
	inline const AstPtr &lval() const { return get(0); }
	inline AstPtr &lval_() { return get_(0); }
	inline void set_type(const TypePtr &type) { _type = type; }
};

// BlockNode
//  -> BlockItemNode*
// BlockItemNode = VarDeclNode|StatementNode
// StatementNode = UnaryOpNode|BinaryOpNode|IfNode|WhileNode|ControlNode|ReturnNode|BlockNode
class BlockNode: public XaryAstNodeBase
{
  public:
  	BlockNode() = default;
  	BlockNode(AstPtrVec &&statements)
  	  : XaryAstNodeBase(std::move(statements)) {}
};

// UnaryOpNode
//  -> ExprNode
// ExprNode = IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode
class UnaryOpNode: public NaryAstNodeBase<1>
{
  public:
	enum OpType
	{
		NEG, NOT, POINTER // POINTER refers to the operator [] in "int arr[]" as
						  // a function parameter.
	};

  protected:
	OpType _op;
	yy::location _loc;

  public:
	UnaryOpNode() = default;
	UnaryOpNode(OpType op, AstPtr &&operand, yy::location loc);
	
	// Getters & setters.
	OpType op() const { return _op; }
	inline const AstPtr &operand() const { return get(0); }
	inline AstPtr &operand_() { return get_(0); }
	inline yy::location location() const { return _loc; }
	inline void set_operand(AstPtr &&operand) { set(0, std::move(operand)); }
};

// BinaryOpNode
//  -> ExprNode, ExprNode
// ExprNode = IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode
class BinaryOpNode: public NaryAstNodeBase<2>
{
  public:
	enum OpType
	{
		ADD, SUB, MUL, DIV, MOD, OR, AND, GT, LT, GE, LE, EQ, NE, ASSIGN, ACCESS
	};

  protected:
	OpType _op;
	yy::location _loc;

  public:
	BinaryOpNode() = default;
	BinaryOpNode(OpType op, AstPtr &&operand1, AstPtr &&operand2, yy::location loc);
	
	// Getters.
	OpType op() const { return _op; }
	inline const AstPtr &operand1() const { return get(0); }
	inline const AstPtr &operand2() const { return get(1); }
	inline AstPtr &operand1_() { return get_(0); }
	inline AstPtr &operand2_() { return get_(1); }
	inline yy::location location() const { return _loc; }
	inline void set_operand1(AstPtr &&operand1) { set(0, std::move(operand1)); }
	inline void set_operand2(AstPtr &&operand2) { set(1, std::move(operand2)); }
};

// IfNode
//  -> ExprNode, StatementNode, StatementNode|nullptr
// StatementNode = UnaryOpNode|BinaryOpNode|IfNode|WhileNode|ControlNode|ReturnNode|BlockNode
class IfNode: public NaryAstNodeBase<3>
{
  public:
	IfNode() = default;
	IfNode(AstPtr &&expr, AstPtr &&if_body);
	IfNode(AstPtr &&expr, AstPtr &&if_body, AstPtr &&else_body);

	// Getters.
	const AstPtr &expr() const { return get(0); }
	const AstPtr &if_body() const { return get(1); }
	const AstPtr &else_body() const { return get(2); }
	AstPtr &expr_() { return get_(0); }
	AstPtr &if_body_() { return get_(1); }
	AstPtr &else_body_() { return get_(2); }
};

// WhileNode
//  -> ExprNode, StatementNode
// StatementNode = UnaryOpNode|BinaryOpNode|IfNode|WhileNode|ControlNode|ReturnNode|BlockNode
class WhileNode: public NaryAstNodeBase<2>
{
  public:
	WhileNode() = default;
	WhileNode(AstPtr &&expr, AstPtr &&body);

	// Getters.
	const AstPtr &expr() const { return get(0); }
	const AstPtr &body() const { return get(1); }
};

class BreakNode: public AstLeafNodeBase
{
  protected:
	yy::location _loc;

  public:
	BreakNode() = default;
	BreakNode(yy::location loc);

	yy::location location() const { return _loc; }
};

class ContNode: public AstLeafNodeBase
{
  protected:
	yy::location _loc;

  public:
	ContNode() = default;
	ContNode(yy::location loc);

	yy::location location() const { return _loc; }
};

// RetNode
//  -> ExprNode|nullptr
// ExprNode = IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode
class RetNode: public NaryAstNodeBase<1>
{
  protected:
	yy::location _loc;

  public:
	RetNode() = default;
	RetNode(yy::location loc);
	RetNode(AstPtr &&expr, yy::location loc);

	// Getters.
	inline const AstPtr &expr() const { return get(0); }
	inline AstPtr &expr_() { return get_(0); }
	inline yy::location location() const { return _loc; }
};

// FuncCallNode
//  -> IdNode, FuncArgsNode
class FuncCallNode: public NaryAstNodeBase<2>
{
  public:
	FuncCallNode() = default;
	FuncCallNode(AstPtr &&id);
	FuncCallNode(AstPtr &&id, AstPtr &&args);

	inline const AstPtr &id() const { return get(0); }
	inline AstPtr &id_() { return get_(0); }
	inline const AstPtr &args() const { return get(1); }
	inline AstPtr &args_() { return get_(1); }
	inline const IdNodePtr &actual_id() const
		{ return std::get<IdNodePtr>(get(0)); }
	inline const FuncArgsNodePtr &actual_args() const
		{ return std::get<FuncArgsNodePtr>(get(1)); }
};

// FuncArgsNode
//  -> ExprNode*
// ExprNode = IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode
class FuncArgsNode: public XaryAstNodeBase
{
  public:
	FuncArgsNode() = default;
	FuncArgsNode(AstPtrVec &&args);
};


} // namespace compiler::frontend

#endif