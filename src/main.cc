#include <iostream>
#include <fstream>
#include <cstring>
#include "parser.tab.h"
#include "semantic_checker.h"
#include "ast_node_printer.h"
#include "eeyore_gen.h"
#include "eeyore_printer.h"

using namespace compiler;
using namespace std;

// returns the input and output filename
std::pair<char *, char *> parse_args(int argc, char *argv[])
{
	std::pair<char *, char *> ret = {nullptr, nullptr};
	for(int i = 1; i < argc - 1; i++)
		if(strcmp(argv[i], "-e") == 0) // input
		{
			ret.first = argv[i + 1];
			i++;
		}
		else if(strcmp(argv[i], "-o") == 0) // output
		{
			ret.second = argv[i + 1];
			i++;
		}
	return ret;
}

int main(int argc, char *argv[])
{
	pair<char *, char *> io_filename = parse_args(argc, argv);
	if(io_filename.first != nullptr)
		set_input_file(io_filename.first);

	define::AstPtr prog_node;
	auto parser = yy::parser(prog_node);
	parser.parse();

	frontend::SemanticChecker checker;
	try
	{
		visit(checker, prog_node);
	}
	catch(utils::InternalError &e)
	{
		cerr << "error occurred!" << endl;
		cerr << e.what() << endl;
		return 1;
	}
	catch(frontend::SemanticChecker::SemanticError &e)
	{
		cerr << "semantic error!" << endl;
		cerr << e.what() << endl;
		return 1;
	}
	
	// cout << "after semantic analysis: " << endl;
	// cout << prog_node << endl << endl;

	backend::eeyore::EeyoreGenerator eeyore_gen;
	try
	{
		const list<backend::eeyore::EeyoreStatement> &
		code = eeyore_gen.generate_eeyore(prog_node);
		
		if(io_filename.second == nullptr)
		{
			for(const auto &stmt : code)
				cout << stmt;
		}
		else
		{
			fstream outf(io_filename.second, fstream::out);
			for(const auto &stmt : code)
				outf << stmt;
		}
	}
	catch(utils::InternalError &e)
	{
		cerr << e.what() << endl;
	}
	catch(std::bad_optional_access &e)
	{
		cerr << e.what() << endl;
	}

	return 0;
}
