// #include <iostream>
// #include "parser.tab.h"
// #include "semantic_checker.h"
// #include "ast_node_printer.h"

// using namespace compiler;
// using namespace std;

// int main()
// {
// 	define::AstPtr prog_node;
// 	auto parser = yy::parser(prog_node);
// 	parser.parse();
// 	cout << "after syntax analysis: " << endl;
// 	cout << prog_node << endl << endl;

// 	frontend::SemanticChecker checker;
// 	try
// 	{
// 		visit(checker, prog_node);
// 	}
// 	catch(utils::InternalError &e)
// 	{
// 		cout << "error occurred!" << endl;
// 		cout << e.what() << endl;
// 		return 0;
// 	}
// 	catch(frontend::SemanticChecker::SemanticError &e)
// 	{
// 		cout << "semantic error!" << endl;
// 		cout << e.what() << endl;
// 		return 0;
// 	}
// 	cout << "after semantic analysis: " << endl;
// 	cout << prog_node << endl << endl;
// 	return 0;
// }
