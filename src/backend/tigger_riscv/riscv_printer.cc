#define DEBUG
#include "dbg.h"
#include "riscv_printer.h"
#include "exceptions.h"
#include "ast_node.h"

using std::endl;
using std::holds_alternative;

namespace
{

inline bool is_12bit(int x)
{
	return -2048 <= x && x < 2048;
}
inline bool is_10bit(int x)
{
	return -512 <= x && x < 512;
}

} // namespace

namespace compiler::backend::riscv
{

int RiscvPrinter::riscv_xalloc = std::ios::xalloc();

void RiscvPrinter::operator() (const tigger::GlobalVarDeclStmt &stmt)
{
	out << "  .global " << stmt.var << endl;
	out << "  .section .sdata" << endl;
	out << "  .align 2" << endl;
	out << "  .type " << stmt.var << ", @object" << endl;
	out << "  .size " << stmt.var << ", 4" << endl;
	out << stmt.var << ':' << endl;
	out << "  .word " << stmt.initial_val << endl;
}

void RiscvPrinter::operator() (const tigger::GlobalArrDeclStmt &stmt)
{
	out << "  .comm " << stmt.var << ", " << stmt.size << ", 4" << endl;
}

void RiscvPrinter::operator() (const tigger::FuncHeaderStmt &stmt)
{
	auto actual_func_name = stmt.func_name.substr(2);
	int stack_size_bytes = (stmt.stack_size / 4 + 1) * 16;
	_stack_size = stack_size_bytes;
	out << "  .text" << endl;
	out << "  .align 2" << endl;
	out << "  .global " << actual_func_name << endl;
	out << "  .type " << actual_func_name << ", @function" << endl;
	out << actual_func_name << ':' << endl;

	if(is_12bit(stack_size_bytes))
	{
		out << "  addi sp, sp, -" << stack_size_bytes << endl;
		out << "  sw ra, " << stack_size_bytes - 4 << "(sp)" << endl;
	}
	else
	{
		out << "  sw ra, -4(sp)" << endl;
		out << "  li " << scratch_reg << ", " << stack_size_bytes << endl;
		out << "  sub sp, sp, " << scratch_reg << endl;
	}
}

void RiscvPrinter::operator() (const tigger::FuncEndStmt &stmt)
{
	auto actual_func_name = stmt.func_name.substr(2);
	out << "  .size   " << actual_func_name << ", .-" << actual_func_name << endl;
	out << endl;
}

void RiscvPrinter::operator() (const tigger::UnaryOpStmt &stmt)
{
	switch(stmt.op_type)
	{
		case frontend::UnaryOpNode::NEG:
			out << "  neg " << stmt.opr << ", " << stmt.opr1 << endl; break;
		case frontend::UnaryOpNode::NOT:
			out << "  seqz " << stmt.opr << ", " << stmt.opr1 << endl; break;
		default:
			INTERNAL_ERROR("invalid unary op type");
	}
}

void RiscvPrinter::operator() (const tigger::BinaryOpStmt &stmt)
{
	// generate the one-statement "Op reg1, reg2, reg3" type instructions,
	// or "Opi reg1, reg2, imm" if needed.
	static auto gen_imm_binary =
	[this](const tigger::BinaryOpStmt &stmt, std::string op_name)
	{
		if(holds_alternative<int>(stmt.opr2))
		{
			int opr2_val = std::get<int>(stmt.opr2);
			if(is_12bit(opr2_val))
				out << "  " << op_name << "i " << stmt.opr << ", " << stmt.opr1
					<< ", " << opr2_val << endl;
			else
			{
				out << "  li " << scratch_reg << ", " << opr2_val << endl;
				out << "  " << op_name << ' ' << stmt.opr << ", " << stmt.opr1
					<< ", " << scratch_reg << endl;
			}
		}
		else
		{
			out << "  " << op_name << ' ' << stmt.opr << ", " << stmt.opr1
				<< ", " << stmt.opr2 << endl;
		}
	};

	// always load the immediate to scratch register, since there is no
	// corresponding I-instructions.
	static auto gen_always_load_binary =
	[this](const tigger::BinaryOpStmt &stmt, std::string op_name)
	{
		if(holds_alternative<int>(stmt.opr2))
		{
			int opr2_val = std::get<int>(stmt.opr2);
			if(opr2_val == 0)
			{
				out << "  " << op_name << ' ' << stmt.opr << ", " << stmt.opr1
					<< ", x0" << endl;
			}
			else
			{
				out << "  li " << scratch_reg << ", " << opr2_val << endl;
				out << "  " << op_name << ' ' << stmt.opr << ", " << stmt.opr1
					<< ", " << scratch_reg << endl;
			}
		}
		else
		{
			out << "  " << op_name << ' ' << stmt.opr << ", " << stmt.opr1
				<< ", " << stmt.opr2 << endl;
		}
	};

	switch(stmt.op_type)
	{
		case frontend::BinaryOpNode::ADD:
			gen_imm_binary(stmt, "add"); break;
		case frontend::BinaryOpNode::SUB: // Sub can make use of addi by adding minus immediate.
			if(holds_alternative<int>(stmt.opr2))
			{
				int opr2_val = -std::get<int>(stmt.opr2);
				if(is_12bit(opr2_val))
					out << "  addi " << stmt.opr << ", " << stmt.opr1
						<< ", " << opr2_val << endl;
				else
				{
					out << "  li " << scratch_reg << ", " << opr2_val << endl;
					out << "  addi " << stmt.opr << ", " << stmt.opr1
						<< ", " << scratch_reg << endl;
				}
			}
			else
			{
				out << "  sub " << stmt.opr << ", " << stmt.opr1
					<< ", " << stmt.opr2 << endl;
			}
			break;
		case frontend::BinaryOpNode::MUL:
			gen_always_load_binary(stmt, "mul"); break;
		case frontend::BinaryOpNode::DIV:
			gen_always_load_binary(stmt, "div"); break;
		case frontend::BinaryOpNode::MOD:
			gen_always_load_binary(stmt, "rem"); break;
		case frontend::BinaryOpNode::OR:
			gen_imm_binary(stmt, "or");
			out << "  snez " << stmt.opr << ", " << stmt.opr << endl;
			break;
		case frontend::BinaryOpNode::AND:
			if(std::holds_alternative<int>(stmt.opr2))
			{
				if(std::get<int>(stmt.opr2) == 0)
					out << "  li " << stmt.opr << ", " << 0 << endl;
				else
					out << "  snez " << stmt.opr << ", " << stmt.opr1 << endl;
			}
			else
			{
				out << "  snez " << stmt.opr << ", " << stmt.opr1 << endl;
				out << "  snez " << scratch_reg << ", " << stmt.opr2 << endl;
				out << "  and " << stmt.opr << ", " << stmt.opr << ", " << scratch_reg << endl;
			}
			break;
		case frontend::BinaryOpNode::GT:
			gen_always_load_binary(stmt, "sgt"); break;
		case frontend::BinaryOpNode::LT:
			gen_imm_binary(stmt, "slt"); break;
		case frontend::BinaryOpNode::GE:
			gen_imm_binary(stmt, "slt");
			out << "  seqz " << stmt.opr << ", " << stmt.opr << endl;
			break;
		case frontend::BinaryOpNode::LE:
			gen_always_load_binary(stmt, "sgt");
			out << "  seqz " << stmt.opr << ", " << stmt.opr << endl;
			break;
		case frontend::BinaryOpNode::EQ:
			gen_imm_binary(stmt, "xor");
			out << "  seqz " << stmt.opr << ", " << stmt.opr << endl;
			break;
		case frontend::BinaryOpNode::NE:
			gen_imm_binary(stmt, "xor");
			out << "  snez " << stmt.opr << ", " << stmt.opr << endl;
			break;
		default:
			INTERNAL_ERROR("invalid binary op type");
	}
}

void RiscvPrinter::operator() (const tigger::MoveStmt &stmt)
{
	if(holds_alternative<tigger::Reg>(stmt.opr1))
		out << "  mv " << stmt.opr << ", " << stmt.opr1 << endl;
	else // int
	{
		int val = std::get<int>(stmt.opr1);
		if(val == 0)
			out << "  mv " << stmt.opr << ", x0" << endl;
		else
			out << "  li " << stmt.opr << ", " << val << endl;
	}
}

void RiscvPrinter::operator() (const tigger::ReadArrStmt &stmt)
{
	if(is_12bit(stmt.idx))
		out << "  lw " << stmt.opr << ", " << stmt.idx << '(' << stmt.opr1 << ')' << endl;
	else // We have to calculate the actual address.
	{
		out << "  li " << scratch_reg << ", " << stmt.idx << endl;
		out << "  add " << scratch_reg << ", " << scratch_reg << ", " << stmt.opr1 << endl;
		out << "  lw " << stmt.opr << ", " << "0(" << scratch_reg << ')' << endl;
	}
}

void RiscvPrinter::operator() (const tigger::WriteArrStmt &stmt)
{
	if(is_12bit(stmt.idx))
		out << "  sw " << stmt.opr << ", " << stmt.idx << '(' << stmt.opr1 << ')' << endl;
	else
	{
		out << "  li " << scratch_reg << ", " << stmt.idx << endl;
		out << "  add " << scratch_reg << ", " << scratch_reg << ", " << stmt.opr1 << endl;
		out << "  sw " << stmt.opr << ", " << "0(" << scratch_reg << ')' << endl;
	}
}

void RiscvPrinter::operator() (const tigger::CondGotoStmt &stmt)
{
	out << "  ";
	switch(stmt.op_type)
	{
		case frontend::BinaryOpNode::LT: out << "blt"; break;
		case frontend::BinaryOpNode::GT: out << "bgt"; break;
		case frontend::BinaryOpNode::LE: out << "ble"; break;
		case frontend::BinaryOpNode::GE: out << "bge"; break;
		case frontend::BinaryOpNode::NE: out << "bne"; break;
		case frontend::BinaryOpNode::EQ: out << "beq"; break;
		default: INTERNAL_ERROR("invalid condition op type");
	}
	out << ' ' << stmt.opr1 << ", " << stmt.opr2 << ", ." << stmt.goto_label << endl;
}

void RiscvPrinter::operator() (const tigger::GotoStmt &stmt)
{
	out << "  j ." << stmt.goto_label << endl;
}

void RiscvPrinter::operator() (const tigger::LabelStmt &stmt)
{
	out << '.' << stmt.label << ':' << endl;
}

void RiscvPrinter::operator() (const tigger::FuncCallStmt &stmt)
{
	auto actual_func_name = stmt.func_name.substr(2);
	out << "  call " << actual_func_name << endl;
}

void RiscvPrinter::operator() (const tigger::ReturnStmt &stmt)
{
	if(is_12bit(_stack_size))
	{
		out << "  lw ra, " << _stack_size-4 << "(sp)" << endl;
		out << "  addi sp, sp, " << _stack_size << endl;
	}
	else
	{
		out << "  li " << scratch_reg << ", " << _stack_size << endl;
		out << "  add sp, sp, " << scratch_reg << endl;
		out << "  lw ra, -4(sp)" << endl;
	}
	out << "  ret" << endl;
}

void RiscvPrinter::operator() (const tigger::StoreStmt &stmt)
{
	if(is_10bit(stmt.stack_offset))
		out << "  sw " << stmt.opr << ", " << stmt.stack_offset*4 << "(sp)" << endl;
	else
	{
		out << "  li " << scratch_reg << ", " << stmt.stack_offset*4 << endl;
		out << "  add " << scratch_reg << ", " << scratch_reg << ", sp" << endl;
		out << "  sw " << stmt.opr << ", " << "0(" << scratch_reg << ')' << endl;
	}
}

void RiscvPrinter::operator() (const tigger::LoadStmt &stmt)
{
	if(holds_alternative<int>(stmt.src))
	{
		int stack_offset = std::get<int>(stmt.src);
		if(is_10bit(stack_offset))
			out << "  lw " << stmt.opr << ", " << stack_offset*4 << "(sp)" << endl;
		else
		{
			out << "  li " << scratch_reg << ", " << stack_offset*4 << endl;
			out << "  add " << scratch_reg << ", " << scratch_reg << ", sp" << endl;
			out << "  lw " << stmt.opr << ", " << "0(" << scratch_reg << ')' << endl;
		}
	}
	else
	{
		auto global_var = std::get<tigger::GlobalVar>(stmt.src);
		out << "  lui " << stmt.opr << ", %hi(" << global_var << ')' << endl;
		out << "  lw " << stmt.opr << ", %lo(" << global_var << ")(" << stmt.opr << ')' << endl;
	}
}

void RiscvPrinter::operator() (const tigger::LoadAddrStmt &stmt)
{
	if(holds_alternative<int>(stmt.src))
	{
		int offset = std::get<int>(stmt.src);
		if(is_10bit(offset))
			out << "  addi " << stmt.opr << ", sp, " << offset*4 << endl;
		else
		{
			out << "  li " << stmt.opr << ", " << offset*4 << endl;
			out << "  add " << stmt.opr << ", " << stmt.opr << ", sp" << endl;
		}
	}
	else
	{
		auto global_var = std::get<tigger::GlobalVar>(stmt.src);
		out << "  la " << stmt.opr << ", " << global_var << endl;
	}
}


} // namespace compiler::backend::riscv

std::ostream &operator << (std::ostream &out, const compiler::backend::tigger::TiggerStatement &stmt)
{
	int idx = compiler::backend::riscv::RiscvPrinter::riscv_xalloc;
	if(out.iword(idx) == 1)
	{
		compiler::backend::riscv::RiscvPrinter riscv_printer(out);
		std::visit(riscv_printer, stmt);
	}
	else
	{
		compiler::backend::tigger::TiggerPrinter tigger_printer(out);
		std::visit(tigger_printer, stmt);
	}
	return out;
}

std::ostream &riscv_mode(std::ostream &out)
{
	static int idx = compiler::backend::riscv::RiscvPrinter::riscv_xalloc;
	out.iword(idx) = 1;
	return out;
}

std::ostream &tigger_mode(std::ostream &out)
{
	static int idx = compiler::backend::riscv::RiscvPrinter::riscv_xalloc;
	out.iword(idx) = 0;
	return out;
}