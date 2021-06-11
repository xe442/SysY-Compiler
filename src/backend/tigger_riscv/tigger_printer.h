#ifndef TIGGER_PRINTER_H
#define TIGGER_PRINTER_H

#include <iostream>
#include "tigger.h"

namespace compiler::backend::tigger
{

struct RegPrinter
{
	std::ostream &out;

	void operator() (const ZeroReg &reg);
	void operator() (const CalleeSavedReg &reg);
	void operator() (const CallerSavedReg &reg);
	void operator() (const ArgReg &reg);
};

struct TiggerPrinter
{
  protected:
	static int _riscv_xalloc;

  public:
	std::ostream &out;

	void operator() (const GlobalVarDeclStmt &stmt);
	void operator() (const GlobalArrDeclStmt &stmt);
	void operator() (const FuncHeaderStmt &stmt);
	void operator() (const FuncEndStmt &stmt);
	void operator() (const UnaryOpStmt &stmt);
	void operator() (const BinaryOpStmt &stmt);
	void operator() (const MoveStmt &stmt);
	void operator() (const ReadArrStmt &stmt);
	void operator() (const WriteArrStmt &stmt);
	void operator() (const CondGotoStmt &stmt);
	void operator() (const GotoStmt &stmt);
	void operator() (const LabelStmt &stmt);
	void operator() (const FuncCallStmt &stmt);
	void operator() (const ReturnStmt &stmt);
	void operator() (const StoreStmt &stmt);
	void operator() (const LoadStmt &stmt);
	void operator() (const LoadAddrStmt &stmt);
};

} // namespace compiler::backend::tigger

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::Label &label);
std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::Reg &reg);
std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::RegOrNum &reg_or_num);
std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::GlobalVar &global_var);
std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::GlobalVarOrNum &global_var_or_num);
std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::TiggerStatement &stmt);

// Switch output mode between tigger and riscv.
std::ostream &riscv(std::ostream &out);
std::ostream &tigger(std::ostream &out);

#endif