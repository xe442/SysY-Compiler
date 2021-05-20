#ifndef EEYORE_PRINTER_H
#define EEYORE_PRINTER_H

#include <iostream>
#include "eeyore.h"
#include "ast_node_printer.h"

namespace compiler::backend::eeyore
{

struct OprPrinter
{
	std::ostream &out;

	void operator() (const OrigVar &var);
	void operator() (const TempVar &var);
	void operator() (const Param &var);
	void operator() (const int &var);
};

struct EeyorePrinter
{
	std::ostream &out;

	void operator() (const DeclStmt &stmt);
	void operator() (const FuncDefStmt &stmt);
	void operator() (const EndFuncDefStmt &stmt);
	void operator() (const ParamStmt &stmt);
	void operator() (const FuncCallStmt &stmt);
	void operator() (const RetStmt &stmt);
	void operator() (const GotoStmt &stmt);
	void operator() (const CondGotoStmt &stmt);
	void operator() (const UnaryOpStmt &stmt);
	void operator() (const BinaryOpStmt &stmt);
	void operator() (const MoveStmt &stmt);
	void operator() (const ReadArrStmt &stmt);
	void operator() (const WriteArrStmt &stmt);
	void operator() (const LabelStmt &stmt);
};

} // namespace compiler::backend::eeyore

std::ostream &operator << (std::ostream &out, const compiler::backend::eeyore::Operand opr);
std::ostream &operator << (std::ostream &out, const compiler::backend::eeyore::EeyoreStatement &stmt);

#endif