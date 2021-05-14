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
{
	new_block();
	for(auto &name_entry_pair : init)
		_table.back().insert(name_entry_pair);
}

void SymbolTable::new_block()
{
	_table.push_back(std::unordered_map<std::string, SymbolTableEntry>());
}

void SymbolTable::end_block()
{
	if(_table.empty())
		INTERNAL_ERROR("trying to remove a block from an empty symbol table");
	_table.pop_back();
}

std::optional<SymbolTableEntry> SymbolTable::find(const std::string &name) const
{
	if(_table.empty())
		INTERNAL_ERROR("finding an id from an empty symbol table");

	for(auto table_ptr = _table.rbegin(); table_ptr != _table.rend(); ++table_ptr)
	{
		auto entry_ptr = table_ptr->find(name);
		if(entry_ptr != table_ptr->end()) // found
			return table_ptr->at(name);
	}
	return std::nullopt;
}

bool SymbolTable::insert(const std::string &name, const TypePtr &type)
{
	if(_table.empty())
		INTERNAL_ERROR("inserting an id to an empty symbol table");
	
	auto entry_ptr = _table.back().find(name);
	if(entry_ptr == _table.back().end()) // does not exist, insert it
	{
		_table.back()[name] = SymbolTableEntry(type);
		return true;
	}
	else
		return false;
}

bool SymbolTable::insert(const std::string &name, const TypePtr &type, int initial_val)
{
	if(_table.empty())
		INTERNAL_ERROR("inserting an id to an empty symbol table");
	
	auto entry_ptr = _table.back().find(name);
	if(entry_ptr == _table.back().end())
	{
		_table.back()[name] = SymbolTableEntry(type, initial_val);
		return true;
	}
	else
		return false;
}

} // namespace define