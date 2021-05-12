#ifndef AST_NODE_PRINTER_H
#define AST_NODE_PRINTER_H

#include <iostream>
#include <string>
#include "exceptions.h"
#include "ast_node.h"

namespace compiler::define
{

struct AstNodePrinter
{
	struct Indent
	{
		int cnt = 0;

		void incr() { ++cnt; }
		void decr() { if(--cnt < 0) INTERNAL_ERROR("invalid indent decr"); }
	};

	std::ostream &out;
	Indent indent = Indent();

  public:
	AstNodePrinter(std::ostream &_out): out(_out) {}
	void operator() (const std::monostate &node);
	void operator() (const ProgramNodePtr &node);
	void operator() (const VarDeclNodePtr &node);
	void operator() (const SingleVarDeclNodePtr &node);
	void operator() (const ConstIntNodePtr &node);
	void operator() (const IdNodePtr &node);
	void operator() (const InitializerNodePtr &node);
	void operator() (const FuncDefNodePtr &node);
	void operator() (const FuncParamsNodePtr &node);
	void operator() (const SingleFuncParamNodePtr &node);
	void operator() (const BlockNodePtr &node);
	void operator() (const UnaryOpNodePtr &node);
	void operator() (const BinaryOpNodePtr &node);
	void operator() (const IfNodePtr &node);
	void operator() (const WhileNodePtr &node);
	void operator() (const BreakNodePtr &node);
	void operator() (const ContNodePtr &node);
	void operator() (const RetNodePtr &node);
	void operator() (const FuncCallNodePtr &node);
	void operator() (const FuncArgsNodePtr &node);
};

} // namespace compiler::define

std::ostream &operator << (std::ostream &out, const compiler::define::UnaryOpNode::OpType &op_type);
std::ostream &operator << (std::ostream &out, const compiler::define::BinaryOpNode::OpType &op_type);
std::ostream &operator << (std::ostream &out, const compiler::define::AstNodePrinter::Indent &indent);
std::ostream &operator << (std::ostream &out, const compiler::define::AstPtr &node);

#endif