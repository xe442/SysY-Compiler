#ifndef EEYORE_H
#define EEYORE_H

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
struct VarBase
{
	int id;
	VarBase(int _id): id(_id) {}
};
struct OrigVar: public VarBase
{
	int size;
	OrigVar(int _id, int _size=sizeof(int)): VarBase(_id), size(_size) {}
	bool operator == (const OrigVar &other) const { return id == other.id; }
};
struct TempVar: public VarBase
{
	TempVar(int _id): VarBase(_id) {}
	bool operator == (const TempVar &other) const { return id == other.id; }
};
struct Param: public VarBase
{
	Param(int _id): VarBase(_id) {}
	bool operator == (const Param &other) const { return id == other.id; }
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
	frontend::BinaryOpNode::OpType op;
	Operand opr1, opr2;
	Label goto_label;

	CondGotoStmt(Operand _opr1, frontend::BinaryOpNode::OpType _op, Operand _opr2, Label _goto_label)
	  : op(_op), opr1(_opr1), opr2(_opr2), goto_label(_goto_label) {}
};

struct UnaryOpStmt
{
	// opr = op_type opr1
	frontend::UnaryOpNode::OpType op_type;
	Operand opr, opr1;

	UnaryOpStmt(Operand _opr, frontend::UnaryOpNode::OpType _op_type, Operand _opr1)
	  : op_type(_op_type), opr(_opr), opr1(_opr1) {}
};
struct BinaryOpStmt
{
	// opr = opr1 op_type opr2
	frontend::BinaryOpNode::OpType op_type;
	Operand opr, opr1, opr2;

	BinaryOpStmt(Operand _opr, Operand _opr1, frontend::BinaryOpNode::OpType _op_type, Operand _opr2)
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

std::vector<Operand> used_vars(const EeyoreStatement &stmt);
std::vector<Operand> defined_vars(const EeyoreStatement &stmt);

} // namespace compiler::backend::eeyore

namespace std
{

template<>
struct hash<class compiler::backend::eeyore::OrigVar>
{
	size_t operator() (const compiler::backend::eeyore::OrigVar &var)
	{
		return hash<int>()(var.id);
	}
};
template<>
struct hash<class compiler::backend::eeyore::TempVar>
{
	size_t operator() (const compiler::backend::eeyore::TempVar &var)
	{
		return hash<int>()(var.id);
	}
};
template<>
struct hash<class compiler::backend::eeyore::Param>
{
	size_t operator() (const compiler::backend::eeyore::Param &var)
	{
		return hash<int>()(var.id);
	}
};

}

#endif