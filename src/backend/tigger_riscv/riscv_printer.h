#include "tigger_printer.h"

namespace compiler::backend::riscv
{

class RiscvPrinter
{
  protected:
	int _stack_size; // Stack size of the current function, in bytes.

  public:
	std::ostream &out;
	static int riscv_xalloc;
	tigger::Reg scratch_reg = tigger::CallerSavedReg(0); // t0

	RiscvPrinter(std::ostream &_out): out(_out) {}

	void operator() (const tigger::GlobalVarDeclStmt &stmt);
	void operator() (const tigger::GlobalArrDeclStmt &stmt);
	void operator() (const tigger::FuncHeaderStmt &stmt);
	void operator() (const tigger::FuncEndStmt &stmt);
	void operator() (const tigger::UnaryOpStmt &stmt);
	void operator() (const tigger::BinaryOpStmt &stmt);
	void operator() (const tigger::MoveStmt &stmt);
	void operator() (const tigger::ReadArrStmt &stmt);
	void operator() (const tigger::WriteArrStmt &stmt);
	void operator() (const tigger::CondGotoStmt &stmt);
	void operator() (const tigger::GotoStmt &stmt);
	void operator() (const tigger::LabelStmt &stmt);
	void operator() (const tigger::FuncCallStmt &stmt);
	void operator() (const tigger::ReturnStmt &stmt);
	void operator() (const tigger::StoreStmt &stmt);
	void operator() (const tigger::LoadStmt &stmt);
	void operator() (const tigger::LoadAddrStmt &stmt);
};

} // namespace compiler::backend::riscv

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::TiggerStatement &stmt);

template<template<class...> class Container>
std::ostream &operator << (std::ostream &out, const Container<compiler::backend::tigger::TiggerStatement> &stmts)
{
	int idx = compiler::backend::riscv::RiscvPrinter::riscv_xalloc;
	if(out.iword(idx) == 1)
	{
		compiler::backend::riscv::RiscvPrinter riscv_printer(out);
		for(auto iter = stmts.begin(); iter != stmts.end(); ++iter)
			std::visit(riscv_printer, *iter);
	}
	else
	{
		compiler::backend::tigger::TiggerPrinter tigger_printer(out);
		for(auto iter = stmts.begin(); iter != stmts.end(); ++iter)
			std::visit(tigger_printer, *iter);
	}
	return out;
}

// Switch output mode between tigger and riscv.
// Usage: std::cout << riscv_mode << stmt;
//     or std::cout << tigger_mode << stmt;
std::ostream &riscv_mode(std::ostream &out);
std::ostream &tigger_mode(std::ostream &out);