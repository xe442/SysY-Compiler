#include "visitor_helper.h"
#include "tigger_printer.h"
#include "ast_node_printer.h"

using std::endl;

namespace compiler::backend::tigger
{

void RegPrinter::operator() (const ZeroReg &reg)
{
	out << 'x' << reg.id;
}

void RegPrinter::operator() (const CalleeSavedReg &reg)
{
	out << 's' << reg.id;
}

void RegPrinter::operator() (const CallerSavedReg &reg)
{
	out << 't' << reg.id;
}

void RegPrinter::operator() (const ArgReg &reg)
{
	out << 'a' << reg.id;
}


void TiggerPrinter::operator() (const GlobalVarDeclStmt &stmt)
{
	out << stmt.var << " = " << stmt.initial_val << endl;
}

void TiggerPrinter::operator() (const GlobalArrDeclStmt &stmt)
{
	out << stmt.var << " = malloc " << stmt.size << endl;
}

void TiggerPrinter::operator() (const FuncHeaderStmt &stmt)
{
	out << stmt.func_name << " [" << stmt.arg_cnt << "] ["
		<< stmt.stack_size << ']' << endl;
}

void TiggerPrinter::operator() (const FuncEndStmt &stmt)
{
	out << "end " << stmt.func_name << endl;
}

void TiggerPrinter::operator() (const UnaryOpStmt &stmt)
{
	out << "  " << stmt.opr << " = " << stmt.op_type << stmt.opr1 << endl;
}

void TiggerPrinter::operator() (const BinaryOpStmt &stmt)
{
	out << "  " << stmt.opr << " = " << stmt.opr1 << ' ' << stmt.op_type
		<< ' ' << stmt.opr2 << endl;
}

void TiggerPrinter::operator() (const MoveStmt &stmt)
{
	out << "  " << stmt.opr << " = " << stmt.opr1 << endl;
}

void TiggerPrinter::operator() (const ReadArrStmt &stmt)
{
	out << "  " << stmt.opr << " = " << stmt.opr1 << '[' << stmt.idx << ']' << endl;
}

void TiggerPrinter::operator() (const WriteArrStmt &stmt)
{
	out << "  " << stmt.opr1 << '[' << stmt.idx << "] = " << stmt.opr << endl;
}

void TiggerPrinter::operator() (const CondGotoStmt &stmt)
{
	out << "  if " << stmt.opr1 << ' ' << stmt.op_type << ' ' << stmt.opr2
		<< " goto " << stmt.goto_label << endl;
}

void TiggerPrinter::operator() (const GotoStmt &stmt)
{
	out << "  goto " << stmt.goto_label << endl;
}

void TiggerPrinter::operator() (const LabelStmt &stmt)
{
	out << stmt.label << ':' << endl;
}

void TiggerPrinter::operator() (const FuncCallStmt &stmt)
{
	out << "  call " << stmt.func_name << endl;
}

void TiggerPrinter::operator() (const ReturnStmt &stmt)
{
	out << "  return" << endl;
}

void TiggerPrinter::operator() (const StoreStmt &stmt)
{
	out << "  store " << stmt.opr << ' ' << stmt.stack_offset << endl;
}

void TiggerPrinter::operator() (const LoadStmt &stmt)
{
	out << "  load " << stmt.src << ' ' << stmt.opr << endl;
}

void TiggerPrinter::operator() (const LoadAddrStmt &stmt)
{
	out << "  loadaddr " << stmt.src << ' ' << stmt.opr << endl;
}

} // namespace compiler::backend::tigger

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::Label &label)
{
	return out << 'l' << label.id;
}

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::Reg &reg)
{
	compiler::backend::tigger::RegPrinter printer{out};
	std::visit(printer, reg);
	return out;
}

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::RegOrNum &reg_or_num)
{
	// Decompose RegOrNum to a Reg or an int. Both of them are printable
	// using std::ostream.
	compiler::utils::LambdaVisitor printer = {
		[&out](const auto &reg_or_num) { out << reg_or_num; }
	};
	std::visit(printer, reg_or_num);
	return out;
}

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::GlobalVar &global_var)
{
	return out << 'v' << global_var.id;
}

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::GlobalVarOrNum &global_var_or_num)
{
	compiler::utils::LambdaVisitor printer = {
		[&out](const auto &global_var_or_num) { out << global_var_or_num; }
	};
	std::visit(printer, global_var_or_num);
	return out;
}

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::TiggerStatement &stmt)
{
	compiler::backend::tigger::TiggerPrinter printer{out};
	std::visit(printer, stmt);
	return out;
}