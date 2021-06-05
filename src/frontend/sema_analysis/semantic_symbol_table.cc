#include <memory>
#include "semantic_symbol_table.h"

using std::make_shared;

namespace compiler::frontend
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
const std::vector<std::pair<std::string, SemanticSymbolTableEntry>> SemanticSymbolTable::INTERNAL_FUNCTIONS = {
	{ "getint", 	SemanticSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()))													},
	{ "getch", 		SemanticSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()))													},
	{ "getarray",	SemanticSymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec{make_shared<PointerType>(make_int())}))				},
	{ "putint", 	SemanticSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}))										},
	{ "putch", 		SemanticSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}))										},
	{ "putarray",	SemanticSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int(), make_shared<PointerType>(make_int())}))	},
	{ "starttime", 	SemanticSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec()))													},
	{ "stoptime", 	SemanticSymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec()))													}
};

SemanticSymbolTable::SemanticSymbolTable(): ChainedMap<std::string, SemanticSymbolTableEntry>()
{
	for(const auto &predef_func : INTERNAL_FUNCTIONS)
		basic_insert(predef_func);
}

} // namespace compiler::frontend