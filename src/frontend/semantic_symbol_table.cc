#include <memory>
#include "semantic_symbol_table.h"

using std::make_shared;
using namespace compiler::define;

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
const std::vector<std::pair<std::string, SymbolTableEntry>> SymbolTable::INTERNAL_FUNCTIONS = {
	{ "getint", 	SymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()))													},
	{ "getch", 		SymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()))													},
	{ "getarray",	SymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec{make_shared<PointerType>(make_int())}))				},
	{ "putint", 	SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}))										},
	{ "putch", 		SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}))										},
	{ "putarray",	SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int(), make_shared<PointerType>(make_int())}))	},
	{ "starttime", 	SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec()))													},
	{ "stoptime", 	SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec()))													}
};

SymbolTable::SymbolTable(): ChainedMap<std::string, SymbolTableEntry>()
{
	for(const auto &predef_func : INTERNAL_FUNCTIONS)
		basic_insert(predef_func);
}

std::optional<SymbolTableEntry> SymbolTable::find(const std::string &name) const
{
	auto find_res = basic_find(name);
	if(!find_res.has_value())
		return std::nullopt;
	return find_res.value()->second;
}

} // namespace compiler::frontend