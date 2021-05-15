#include "symbol_table.h"

using std::make_shared;
using std::make_pair;
using namespace compiler::define;

namespace compiler::define
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
	make_pair("getint", SymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()))),
	make_pair("getch", SymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec()))),
	make_pair("getarray", SymbolTableEntry(make_shared<FuncType>(make_int(), TypePtrVec{make_shared<PointerType>(make_int())}))),
	make_pair("putint", SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}))),
	make_pair("putch", SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int()}))),
	make_pair("putarray", SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec{make_int(), make_shared<PointerType>(make_int())}))),
	make_pair("starttime", SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec()))),
	make_pair("stoptime", SymbolTableEntry(make_shared<FuncType>(make_void(), TypePtrVec())))
};

SymbolTable::SymbolTable(std::vector<std::pair<std::string, SymbolTableEntry>> init)
  : ChainedMap<std::string, SymbolTableEntry>()
{
	for(auto &name_entry_pair : init)
		_table.back().insert(name_entry_pair);
}

} // namespace define