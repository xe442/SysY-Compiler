#include "eeyore_symbol_table.h"

using std::make_shared;
using namespace compiler::define;

namespace compiler::backend::eeyore
{

/* 
 * default internal functions: 
 * int getint()
 * int getch()
 * int getarray(int a[])
 * void putint(int a)
 * void putch(int a)
 * void putarray(int n, int a[])
 * void starttime()
 * void stoptime()
 */
const std::vector<std::pair<std::string, EeyoreSymbolTableEntry>> EeyoreSymbolTable::INTERNAL_FUNCTIONS = {
	{ "getint", 	EeyoreSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()), std::monostate())													},
	{ "getch", 		EeyoreSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()), std::monostate())													},
	{ "getarray",	EeyoreSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec{make_shared<PointerType>(make_int())}), std::monostate())				},
	{ "putint", 	EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}), std::monostate())										},
	{ "putch", 		EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}), std::monostate())										},
	{ "putarray",	EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int(), make_shared<PointerType>(make_int())}), std::monostate())	},
	{ "starttime", 	EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec()), std::monostate())													},
	{ "stoptime", 	EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec()), std::monostate())													}
};

EeyoreSymbolTable::EeyoreSymbolTable(): utils::ChainedMap<std::string, EeyoreSymbolTableEntry>()
{
	for(const auto &predef_func : INTERNAL_FUNCTIONS)
		basic_insert(predef_func);
}

}