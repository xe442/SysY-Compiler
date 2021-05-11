#include <sstream>

namespace compiler::utils
{

// TODO: change these "private" fuctions to private member functions of a class.
void _fstring_with_strstream(std::ostringstream &oss)
{
	oss << std::ends;
	return;
}

template<class T, class... Ts>
void _fstring_with_strstream(std::ostringstream &oss, const T &t, const Ts&... ts)
{
	oss << t;
	fstring_with_strstream(oss, ts...);
}

// Prints several objects to a string.
// Example usage:
//	string str = fstring(1, "+", 2, "=", 1+2);
template<class... Ts>
std::string fstring(const Ts&... ts)
{
	std::ostringstream oss;
	fstring_with_strstream(oss, ts...);
	return oss.str();
}

} // namespace compiler::utils