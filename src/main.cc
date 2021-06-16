#include <iostream>
#include <fstream>
#include <cstring>
#include "parser.tab.h"
#include "semantic_checker.h"
#include "ast_node_printer.h"
#include "eeyore_gen.h"
#include "eeyore_printer.h"
#include "tigger_gen.h"
#include "tigger_printer.h"
#include "riscv_printer.h"

using namespace compiler;
using namespace std;

// returns the input and output filename

enum class GenType
{
	EEYORE, TIGGER, RISCV
};
struct MainArg
{
	GenType type;
	char *input_filename;
	char *output_filename;
};

MainArg parse_args(int argc, char *argv[])
{
	MainArg ret = {GenType::RISCV, nullptr, nullptr};
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-e") == 0) // eeyore mode
			ret.type = GenType::EEYORE;
		else if(strcmp(argv[i], "-t") == 0) // tigger mode
			ret.type = GenType::TIGGER;
		else if(strcmp(argv[i], "-o") == 0 && i < argc - 1) // set output file
		{
			ret.output_filename = argv[i + 1];
			i++;
		}
		else // set input file
			ret.input_filename = argv[i];
	}
	return ret;
}

int main(int argc, char *argv[])
{
	MainArg args = parse_args(argc, argv);
	if(args.input_filename != nullptr)
		set_input_file(args.input_filename);

	try
	{
		// Syntax analysis.
		frontend::AstPtr prog_node;
		auto parser = yy::parser(prog_node);
		parser.parse();

		// Semantic analysis.
		frontend::SemanticChecker checker;
		visit(checker, prog_node);
		// cout << "after semantic analysis: " << endl;
		// cout << prog_node << endl << endl;

		// Eeyore generation.
		backend::eeyore::EeyoreGenerator eeyore_gen;
		const auto &eeyore_code = eeyore_gen.generate_eeyore(prog_node);

		if(args.type == GenType::EEYORE)
		{
			if(args.output_filename == nullptr)
				cout << eeyore_code;
			else
			{
				fstream outf(args.output_filename, fstream::out);
				outf << eeyore_code;
			}
			return 0;
		}

		backend::tigger::TiggerGenerator tigger_gen(eeyore_code, std::move(eeyore_gen.all_defined_vars()));
		const auto &tigger_code = tigger_gen.generate_tigger();

		if(args.output_filename == nullptr)
		{
			if(args.type == GenType::RISCV)
				cout << riscv_mode;
			else
				cout << tigger_mode;
			cout << tigger_code;
		}
		else
		{
			fstream outf(args.output_filename, fstream::out);
			if(args.type == GenType::RISCV)
				outf << riscv_mode;
			else
				outf << tigger_mode;
			outf << tigger_code;
		}
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
	catch(std::bad_optional_access &e)
	{
		cerr << e.what() << endl;
		return 1;
	}
	return 0;
}
