#ifndef FSTRING_H
#define FSTRING_H

#include <sstream>

namespace compiler::utils
{

class Fstring
{
	std::ostringstream oss;

  public:
	Fstring(): oss() {}
	std::string str() { return oss.str(); }

	void print() { oss << std::ends; }

	template<class T, class... Ts>
	void print(const T &t, const Ts&... ts)
	{
		oss << t;
		print(ts...);
	}
};

// Prints several objects to a string.
// Example usage:
//	string str = fstring(1, "+", 2, "=", 1+2);
template<class... Ts>
std::string fstring(const Ts&... ts)
{
	Fstring fstring;
	fstring.print(ts...);
	return fstring.str();
}

} // namespace compiler::utils

#endif