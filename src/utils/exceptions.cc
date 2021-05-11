#include "exceptions.h"

namespace compiler::utils
{

std::string InternalError::what() const
{
	std::ostringstream sout;
	sout << "Internal error at file " << _filename << ", line " << _line
		<< ": " << _what << "." << std::endl << std::ends;
	return sout.str();
}

} // namespace compiler::utils