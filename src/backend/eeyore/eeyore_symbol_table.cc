#include "eeyore_symbol_table.h"

using std::make_shared;
using namespace compiler::frontend;

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
 * void _sysy_starttime(int lineno)
 * void _sysy_stoptime(int lineno)
 */
const std::vector<std::pair<std::string, EeyoreSymbolTableEntry>> EeyoreSymbolTable::INTERNAL_FUNCTIONS = {
	{ "getint", 	EeyoreSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()), std::monostate())													},
	{ "getch", 		EeyoreSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()), std::monostate())													},
	{ "getarray",	EeyoreSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec{make_shared<PointerType>(make_int())}), std::monostate())				},
	{ "putint", 	EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}), std::monostate())										},
	{ "putch", 		EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}), std::monostate())										},
	{ "putarray",	EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int(), make_shared<PointerType>(make_int())}), std::monostate())	},
	{ "_sysy_starttime", 	EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}), std::monostate())								},
	{ "_sysy_stoptime", 	EeyoreSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}), std::monostate())								}
};

EeyoreSymbolTable::EeyoreSymbolTable(): utils::ChainedMap<std::string, EeyoreSymbolTableEntry>()
{
	for(const auto &predef_func : INTERNAL_FUNCTIONS)
		basic_insert(predef_func);
}

}