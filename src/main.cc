#include <iostream>
#include "semantic_checker.h"
#include "parser.tab.h"
#include "ast_node_printer.h"

int main()
{
	compiler::define::AstPtr prog_node;
	auto parser = yy::parser(prog_node);
	parser.parse();
	std::cout << prog_node << std::endl;
	return 0;
}