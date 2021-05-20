#include "eeyore_printer.h"
#include "visitor_helper.h"

using std::endl;

namespace compiler::backend::eeyore
{

void OprPrinter::operator() (const OrigVar &var)
{
	out << 'T' << var.id;
}

void OprPrinter::operator() (const TempVar &var)
{
	out << 't' << var.id;
}

void OprPrinter::operator() (const Param &var)
{
	out << 'p' << var.id;
}

void OprPrinter::operator() (const int &var)
{
	out << var;
}

void EeyorePrinter::operator() (const DeclStmt &stmt)
{
	out << "var " << stmt.var << endl;
}

void EeyorePrinter::operator() (const FuncDefStmt &stmt)
{
	out << stmt.func_name << " [" << stmt.arg_cnt << ']' << endl;
}

void EeyorePrinter::operator() (const EndFuncDefStmt &stmt)
{
	out << "end " << stmt.func_name << endl;
}

void EeyorePrinter::operator() (const ParamStmt &stmt)
{
	out << "param " << stmt.param << endl;
}

void EeyorePrinter::operator() (const FuncCallStmt &stmt)
{
	if(stmt.retval_receiver.has_value())
		out << stmt.retval_receiver.value() << " = ";
	out << "call " << stmt.func_name << endl;
}

void EeyorePrinter::operator() (const RetStmt &stmt)
{
	out << "return";
	if(stmt.retval.has_value())
		out << ' ' << stmt.retval.value();
	out << endl;
}

void EeyorePrinter::operator() (const GotoStmt &stmt)
{
	out << "goto l" << stmt.goto_label.id << endl;
}

void EeyorePrinter::operator() (const CondGotoStmt &stmt)
{
	out << "if " << stmt.opr1 << ' ' << stmt.op << " goto l" << stmt.goto_label.id << endl;
}

void EeyorePrinter::operator() (const UnaryOpStmt &stmt)
{
	out << stmt.opr << " = " << stmt.op_type << stmt.opr1 << endl;
}

void EeyorePrinter::operator() (const BinaryOpStmt &stmt)
{
	out << stmt.opr << " = "
		<< stmt.opr1 << ' ' << stmt.op_type << ' ' << stmt.opr2 << endl;
}

void EeyorePrinter::operator() (const MoveStmt &stmt)
{
	out << stmt.opr << " = " << stmt.opr1 << endl;
}

void EeyorePrinter::operator() (const ReadArrStmt &stmt)
{
	out << stmt.opr << " = " << stmt.arr_opr << '[' << stmt.idx_opr << ']' << endl;
}

void EeyorePrinter::operator() (const WriteArrStmt &stmt)
{
	out << stmt.arr_opr << '[' << stmt.idx_opr << ']' << " = " << stmt.opr << endl;
}

void EeyorePrinter::operator() (const LabelStmt &stmt)
{
	out << 'l' << stmt.label.id << ':' << endl;
}

} // namespace compiler::backend::eeyore

std::ostream &operator << (std::ostream &out, const compiler::backend::eeyore::Operand opr)
{
	compiler::backend::eeyore::OprPrinter printer{out};
	std::visit(printer, opr);
	return out;
}

std::ostream &operator << (std::ostream &out, const compiler::backend::eeyore::EeyoreStatement &stmt)
{
	compiler::backend::eeyore::EeyorePrinter printer{out};
	std::visit(printer, stmt);
	return out;
}