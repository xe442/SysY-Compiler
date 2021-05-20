#ifndef EEYORE
#define EEYORE

#include <string>
#include <variant>
#include <optional>
#include "ast_node.h"

namespace compiler::backend::eeyore
{

struct Label
{
	int id;
	Label(int _id): id(_id) {}
};
struct OrigVar
{
	int id;
	int size;
	OrigVar(int _id, int _size=sizeof(int)): id(_id), size(_size) {}
};
struct TempVar
{
	int id;
	TempVar(int _id): id(_id) {}
};
struct Param
{
	int id;
	Param(int _id): id(_id) {}
};
using Operand = std::variant<int, OrigVar, TempVar, Param>;

struct DeclStmt
{
	Operand var;

	DeclStmt(Operand _var): var(_var) {}
};
struct FuncDefStmt
{
	std::string func_name;
	int arg_cnt;

	FuncDefStmt(std::string _func_name, int _arg_cnt)
	  : func_name("f_" + _func_name), arg_cnt(_arg_cnt) {}
};
struct EndFuncDefStmt
{
	std::string func_name;

	EndFuncDefStmt(std::string _func_name)
	  : func_name("f_" + _func_name) {}
};

struct ParamStmt
{
	Operand param;

	ParamStmt(Operand _param): param(_param) {}
};

struct FuncCallStmt
{
	std::string func_name;
	std::optional<Operand> retval_receiver;

	FuncCallStmt(std::string _func_name)
	  : func_name("f_" + _func_name), retval_receiver(std::nullopt)
	{}
	FuncCallStmt(std::string _func_name, Operand _retval_receiver)
	  : func_name("f_" + _func_name), retval_receiver(_retval_receiver)
	{}
};
struct RetStmt
{
	std::optional<Operand> retval;

	RetStmt(): retval(std::nullopt) {}
	RetStmt(Operand _retval): retval(_retval) {}
};

struct GotoStmt
{
	Label goto_label;

	GotoStmt(Label _goto_label): goto_label(_goto_label) {}
};
struct CondGotoStmt
{
	define::BinaryOpNode::OpType op;
	Operand opr1, opr2;
	Label goto_label;

	CondGotoStmt(Operand _opr1, define::BinaryOpNode::OpType _op, Operand _opr2, Label _goto_label)
	  : op(_op), opr1(_opr1), opr2(_opr2), goto_label(_goto_label) {}
};

struct UnaryOpStmt
{
	// opr = op_type opr1
	define::UnaryOpNode::OpType op_type;
	Operand opr, opr1;

	UnaryOpStmt(Operand _opr, define::UnaryOpNode::OpType _op_type, Operand _opr1)
	  : op_type(_op_type), opr(_opr), opr1(_opr1) {}
};
struct BinaryOpStmt
{
	// opr = opr1 op_type opr2
	define::BinaryOpNode::OpType op_type;
	Operand opr, opr1, opr2;

	BinaryOpStmt(Operand _opr, Operand _opr1, define::BinaryOpNode::OpType _op_type, Operand _opr2)
	  : op_type(_op_type), opr(_opr), opr1(_opr1), opr2(_opr2) {}
};
struct MoveStmt
{
	Operand opr, opr1;

	MoveStmt(Operand _opr, Operand _opr1)
	  : opr(_opr), opr1(_opr1) {}
};
struct ReadArrStmt
{
	Operand opr, arr_opr, idx_opr;

	ReadArrStmt(Operand _opr, Operand _arr_opr, Operand _idx_opr)
	  : opr(_opr), arr_opr(_arr_opr), idx_opr(_idx_opr) {}
};
struct WriteArrStmt
{
	Operand opr, arr_opr, idx_opr;

	WriteArrStmt(Operand _arr_opr, Operand _idx_opr, Operand _opr)
	  : opr(_opr), arr_opr(_arr_opr), idx_opr(_idx_opr) {}
};

struct LabelStmt
{
	Label label;

	LabelStmt(Label _label): label(_label) {}
};

using EeyoreStatement = std::variant
<
	DeclStmt,
	FuncDefStmt,
	EndFuncDefStmt,
	ParamStmt,
	FuncCallStmt,
	RetStmt,
	GotoStmt,
	CondGotoStmt,
	UnaryOpStmt,
	BinaryOpStmt,
	MoveStmt,
	ReadArrStmt,
	WriteArrStmt,
	LabelStmt
>;

} // namespace compiler::backend::eeyore

#endif