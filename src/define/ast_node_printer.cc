#include "ast_node_printer.h"

using std::endl;

namespace compiler::define
{

void AstNodePrinter::operator() (const std::monostate &node)
{
	return;
}

void AstNodePrinter::operator() (const ProgramNodePtr &node)
{
	for(int i = 0; i < node->children_cnt(); i++)
	{
		if(i != 0)
			out << indent;
		std::visit(*this, node->get(i));
		if(i + 1 != node->children_cnt())
			out << endl;
	}
}

void AstNodePrinter::operator() (const VarDeclNodePtr &node)
{
	for(int i = 0; i < node->children_cnt(); i++)
	{
		if(i != 0)
			out << indent;
		std::visit(*this, node->get(i));
		if(i + 1 != node->children_cnt())
			out << endl;
	}
}

void AstNodePrinter::operator() (const SingleVarDeclNodePtr &node)
{
	out << node->type();
	std::visit(*this, node->lval());
	if(!is_null_ast(node->init_val()))
	{
		out << " = ";
		std::visit(*this, node->init_val());
	}
}

void AstNodePrinter::operator() (const ConstIntNodePtr &node)
{
	out << node->val();
}

void AstNodePrinter::operator() (const IdNodePtr &node)
{
	out << node->name();
}

void AstNodePrinter::operator() (const InitializerNodePtr &node)
{
	out << '{';
	for(int i = 0; i < node->children_cnt(); i++)
	{
		if(i > 0)
			out << ", ";
		std::visit(*this, node->get(i));
	}
	out << '}';
}

void AstNodePrinter::operator() (const FuncDefNodePtr &node)
{
	out << node->retval_type() << ' ';
	std::visit(*this, node->name());
	out << '(';
	std::visit(*this, node->params());
	out << "):" << endl;

	indent.incr();
	out << indent;
	std::visit(*this, node->block());
	indent.decr();
	out << "end func def";
}

void AstNodePrinter::operator() (const FuncParamsNodePtr &node)
{
	for(int i = 0; i < node->children_cnt(); i++)
	{
		if(i != 0)
			out << ", ";
		std::visit(*this, node->get(i));
	}
}

void AstNodePrinter::operator() (const SingleFuncParamNodePtr &node)
{
	out << node->type() << ' ';
	std::visit(*this, node->lval());
}

void AstNodePrinter::operator() (const BlockNodePtr &node)
{
	for(int i = 0; i < node->children_cnt(); i++)
	{
		if(i != 0)
			out << indent;
		std::visit(*this, node->get(i));
		if(i + 1 != node->children_cnt())
			out << endl;
	}
}

void AstNodePrinter::operator() (const UnaryOpNodePtr &node)
{
	out << '(' << node->op();
	std::visit(*this, node->operand());
	out << ')';
}

void AstNodePrinter::operator() (const BinaryOpNodePtr &node)
{
	out << '(';
	std::visit(*this, node->operand1());
	out << ' ' << node->op() << ' ';
	std::visit(*this, node->operand2());
	out << ')';
}

void AstNodePrinter::operator() (const IfNodePtr &node)
{
	out << "if ";
	std::visit(*this, node->expr());
	out << ':' << endl;

	indent.incr();
	out << indent;
	std::visit(*this, node->if_body());
	indent.decr();

	if(!is_null_ast(node->else_body()))
	{
		out << endl;
		out << indent << "else:" << endl;
		indent.incr();
		out << indent;
		std::visit(*this, node->else_body());
		indent.decr();
	}
}

void AstNodePrinter::operator() (const WhileNodePtr &node)
{
	out << "while ";
	std::visit(*this, node->expr());
	out << ':' << endl;

	indent.incr();
	out << indent;
	std::visit(*this, node->body());
	indent.decr();
}

void AstNodePrinter::operator() (const BreakNodePtr &node)
{
	out << "break";
}

void AstNodePrinter::operator() (const ContNodePtr &node)
{
	out << "continue";
}

void AstNodePrinter::operator() (const RetNodePtr &node)
{
	out << "return";
	if(!is_null_ast(node->expr()))
	{
		out << ' ';
		std::visit(*this, node->expr());
	}
}

void AstNodePrinter::operator() (const FuncCallNodePtr &node)
{
	std::visit(*this, node->id());
	out << '(';
	std::visit(*this, node->params());
	out << ')';
}

void AstNodePrinter::operator() (const FuncArgsNodePtr &node)
{
	for(int i = 0; i < node->children_cnt(); i++)
	{
		if(i != 0)
			out << ", ";
		std::visit(*this, node->get(i));
	}
}


}// namespace compiler::define

std::ostream &operator << (std::ostream &out, const compiler::define::UnaryOpNode::OpType &op_type)
{
	using OpType = compiler::define::UnaryOpNode;
	switch(op_type)
	{
		case OpType::NEG: out << '-'; break;
		case OpType::NOT: out << '!'; break;
		case OpType::POINTER: out << '&'; break;
		default: out << "?"; break;
	}
	return out;
}

std::ostream &operator << (std::ostream &out, const compiler::define::BinaryOpNode::OpType &op_type)
{
	using OpType = compiler::define::BinaryOpNode;
	switch(op_type)
	{
		case OpType::ADD: out << '+'; break;
		case OpType::SUB: out << '-'; break;
		case OpType::MUL: out << '*'; break;
		case OpType::DIV: out << '/'; break;
		case OpType::MOD: out << '%'; break;
		case OpType::OR: out << '|'; break;
		case OpType::AND: out << '&'; break;
		case OpType::GT: out << '>'; break;
		case OpType::LT: out << '<'; break;
		case OpType::GE: out << ">="; break;
		case OpType::LE: out << "<="; break;
		case OpType::EQ: out << "=="; break;
		case OpType::NE: out << "!="; break;
		case OpType::ASSIGN: out << "="; break;
		case OpType::ACCESS: out << "@"; break;
		default: out << '?'; break;
	}
	return out;
}

std::ostream &operator << (std::ostream &out, const compiler::define::AstNodePrinter::Indent &indent)
{
	return out << std::string(indent.cnt * 4, ' ');
}

std::ostream &operator << (std::ostream &out, const compiler::define::AstPtr &node)
{
	compiler::define::AstNodePrinter printer(out);
	std::visit(printer, node);
	return out;
}