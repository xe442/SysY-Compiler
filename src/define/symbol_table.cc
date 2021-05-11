#include "symbol_table.h"

using namespace compiler::define;

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

bool SymbolTable::insert(const std::string &name, SymbolTableEntry entry)
{
	if(_table.empty())
		INTERNAL_ERROR("inserting an id to an empty symbol table");
	
	auto entry_ptr = _table.back().find(name);
	if(entry_ptr == _table.back().end()) // does not exist, insert it
	{
		_table.back().insert(std::make_pair(name, entry));
		return true;
	}
	else
		return false;
}

// SymbolTableEntry & SymbolTable::operator [](const std::string &name)
// {
// 	auto entry = find(name);
// 	if(entry.has_value())
// 		return const_cast<SymbolTableEntry &>(entry.value());

// 	// if not found, insert a new identifier in the last block of a symbol table.
// 	auto insert_res = _table.back().insert(std::make_pair(name, SymbolTableEntry()));
// 		// type(insert_res) = pair(iterator, bool)
// 	return insert_res.first->second;
// }