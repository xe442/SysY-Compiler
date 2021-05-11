#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <sstream>
#include <string>
#include "location.h"

// A handy internal error thrower.
#define INTERNAL_ERROR(what_arg) throw compiler::utils::InternalError(__FILE__, __LINE__, what_arg)

namespace compiler::utils
{

// compiler internal logic error
class InternalError
{
  protected:
	std::string _filename;
	int _line;
	std::string _what;

  public:
	InternalError(const std::string &filename, int line, const std::string &what_arg)
	  : _filename(filename), _line(line), _what(what_arg)
	{}
	std::string what() const;
};

template<const char *error_name>
class CompilerErrorBase
{
  protected:
	yy::position _error_pos;
	std::string _what;

  public:
  	CompilerErrorBase(yy::position error_pos, const std::string &what_arg)
  	  :  _error_pos(error_pos), _what(what_arg)
  	{}

  	std::string what() const
	{
		std::ostringstream sout;
		sout << error_name << " at line " << _error_pos.line << ", col " << _error_pos.column
			<< ": " << _what << "." << std::endl << std::ends;
		return sout.str();
	}
};

// const char _LEXICAL_ERROR_NAME[] = "Lexical error";
// const char _SYNTAX_ERROR_NAME[] = "Syntax error";

// using LexicalError = CompilerErrorBase<_LEXICAL_ERROR_NAME>; // lexer error
// using SyntaxError = CompilerErrorBase<_SYNTAX_ERROR_NAME>; // parser error

} // namespace compiler::utils

#endif