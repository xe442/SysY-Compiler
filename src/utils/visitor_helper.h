#ifndef VISITOR_BASE
#define VISITOR_BASE

namespace compiler::utils
{

// A visitor class that uses lambda functions to overload operator ().
// Example usage:
//	LambdaVisitor visitor = {
// 		[](const int &i) {cout << "int" << endl;},
//		[](const double &d) { cout << "double" << endl; },
//		[](const auto &a) { cout << "unrecognized type" << endl; }
//	};
//	Variable<int,double> v = 2.33;
//	visit(visitor, v);
template<class... Ts> struct LambdaVisitor : Ts... { using Ts::operator()...; };
template<class... Ts> LambdaVisitor(Ts...) -> LambdaVisitor<Ts...>;

} // compiler::utils

#endif