#ifndef TIGGER_H
#define TIGGER_H

#include <iostream>
#include <string>
#include <variant>
#include "eeyore.h"
#include "ast_node.h"

namespace compiler::backend::tigger
{

struct Label
{
	int id;
	Label(int _id): id(_id) {}
	Label(eeyore::Label label): id(label.id) {}
};

// All of these registers has a unique id.
// Operator == are provided so that they can be used as keys of std::unordered_map.
struct RegBase
{
	int id;
	RegBase(int _id): id(_id) {}
};
struct ZeroReg: public RegBase
{
	ZeroReg(int _id): RegBase(_id) {}
	bool operator < (const ZeroReg &reg) const { return id < reg.id; }
	bool operator == (const ZeroReg &reg) const { return id == reg.id; }
};
struct CalleeSavedReg: public RegBase
{
	CalleeSavedReg(int _id): RegBase(_id) {}
	bool operator < (const CalleeSavedReg &reg) const { return id < reg.id; }
	bool operator == (const CalleeSavedReg &reg) const { return id == reg.id; }
};
struct CallerSavedReg: public RegBase
{
	CallerSavedReg(int _id): RegBase(_id) {}
	bool operator < (const CallerSavedReg &reg) const { return id < reg.id; }
	bool operator == (const CallerSavedReg &reg) const { return id == reg.id; }
};
struct ArgReg: public RegBase
{
	ArgReg(int _id): RegBase(_id) {}
	bool operator < (const ArgReg &reg) const { return id < reg.id; }
	bool operator == (const ArgReg &reg) const { return id == reg.id; }
};

const ZeroReg ZERO_REG = {0};
const std::vector<CalleeSavedReg> ALL_CALLEE_SAVED_REG = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
const std::vector<CallerSavedReg> ALL_CALLER_SAVED_REG = {0, 1, 2, 3, 4, 5, 6};
const std::vector<ArgReg> ALL_ARG_REG = {0, 1, 2, 3, 4, 5, 6, 7};

using Reg = std::variant<ZeroReg, CalleeSavedReg, CallerSavedReg, ArgReg>;
using RegOrNum = std::variant<int, Reg>;

struct GlobalVar
{
	int id;

	GlobalVar(int _id): id(_id) {}
};
using GlobalVarOrNum = std::variant<int, GlobalVar>;

struct GlobalVarDeclStmt
{
	GlobalVar var;
	int initial_val;

	GlobalVarDeclStmt(GlobalVar _var, int _initial_val=0)
	  : var(_var), initial_val(_initial_val) {}
};
struct GlobalArrDeclStmt
{
	GlobalVar var;
	int size;

	GlobalArrDeclStmt(GlobalVar _var, int _size)
	  : var(_var), size(_size) {}
};
struct FuncHeaderStmt
{
	std::string func_name;
	int arg_cnt;
	int stack_size;

	FuncHeaderStmt(std::string _func_name, int _arg_cnt, int _stack_size=0)
	  : func_name(_func_name), arg_cnt(_arg_cnt), stack_size(_stack_size) {}
};
struct FuncEndStmt
{
	std::string func_name;

	FuncEndStmt(std::string _func_name): func_name(_func_name) {}
};
struct UnaryOpStmt
{
	using OpType = frontend::UnaryOpNode::OpType;
	OpType op_type;
	Reg opr, opr1;

	UnaryOpStmt(Reg _opr, OpType _op_type, Reg _opr1)
	  : op_type(_op_type), opr(_opr), opr1(_opr1) {}
};
struct BinaryOpStmt
{
	using OpType = frontend::BinaryOpNode::OpType;
	OpType op_type;
	Reg opr, opr1;
	RegOrNum opr2; // operand 2 can be either a oprister or an int

	BinaryOpStmt(Reg _opr, Reg _opr1, OpType _op_type, RegOrNum _opr2)
	  : op_type(_op_type), opr(_opr), opr1(_opr1), opr2(_opr2) {}
};
struct MoveStmt
{
	Reg opr;
	RegOrNum opr1;

	MoveStmt(Reg _opr, RegOrNum _opr1): opr(_opr), opr1(_opr1) {}
};
struct ReadArrStmt
{
	int idx;
	Reg opr, opr1;

	ReadArrStmt(Reg _opr, Reg _opr1, int _idx)
	  : idx(_idx), opr(_opr), opr1(_opr1) {}
};
struct WriteArrStmt
{
	int idx;
	Reg opr, opr1;

	WriteArrStmt(Reg _opr1, int _idx, Reg _opr)
	  : idx(_idx), opr(_opr), opr1(_opr1) {}
};
struct CondGotoStmt
{
	using OpType = frontend::BinaryOpNode::OpType;
	OpType op_type;
	Reg opr1, opr2;
	Label goto_label;

	CondGotoStmt(Reg _opr1, OpType _op_type, Reg _opr2, Label _goto_label)
	  : op_type(_op_type), opr1(_opr1), opr2(_opr2), goto_label(_goto_label) {}
};
struct GotoStmt
{
	Label goto_label;

	GotoStmt(Label _goto_label): goto_label(_goto_label) {}
};
struct LabelStmt
{
	Label label;

	LabelStmt(Label _label): label(_label) {}
};
struct FuncCallStmt
{
	std::string func_name;

	FuncCallStmt(std::string _func_name): func_name(_func_name) {}
};
struct ReturnStmt
{
	ReturnStmt() = default;
};
struct StoreStmt
{
	int stack_offset;
	Reg opr;

	StoreStmt(int _stack_offset, Reg _opr)
	  : stack_offset(_stack_offset), opr(_opr) {}
};
struct LoadStmt
{
	GlobalVarOrNum src;
	Reg opr;

	LoadStmt(Reg _opr, GlobalVarOrNum _src): src(_src), opr(_opr) {}
};
struct LoadAddrStmt
{
	GlobalVarOrNum src;
	Reg opr;

	LoadAddrStmt(Reg _opr, GlobalVarOrNum _src): src(_src), opr(_opr) {}
};

using TiggerStatement = std::variant
<
	GlobalVarDeclStmt,
	GlobalArrDeclStmt,
	FuncHeaderStmt,
	FuncEndStmt,
	UnaryOpStmt,
	BinaryOpStmt,
	MoveStmt,
	ReadArrStmt,
	WriteArrStmt,
	CondGotoStmt,
	GotoStmt,
	LabelStmt,
	FuncCallStmt,
	ReturnStmt,
	StoreStmt,
	LoadStmt,
	LoadAddrStmt
>;

} // namespace compiler::backend::tigger

namespace std
{

template<>
struct hash<compiler::backend::tigger::ZeroReg>
{
	size_t operator() (const compiler::backend::tigger::ZeroReg &reg) const
	{
		return std::hash<int>()(reg.id);
	}
};

template<>
struct hash<compiler::backend::tigger::CalleeSavedReg>
{
	size_t operator() (const compiler::backend::tigger::CalleeSavedReg &reg) const
	{
		return std::hash<int>()(reg.id);
	}
};

template<>
struct hash<compiler::backend::tigger::CallerSavedReg>
{
	size_t operator() (const compiler::backend::tigger::CallerSavedReg &reg) const
	{
		return std::hash<int>()(reg.id);
	}
};

template<>
struct hash<compiler::backend::tigger::ArgReg>
{
	size_t operator() (const compiler::backend::tigger::ArgReg &reg) const
	{
		return std::hash<int>()(reg.id);
	}
};

} // namespace std

#endif