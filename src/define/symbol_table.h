#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "type.h"
#include "exceptions.h"


namespace compiler::define
{

struct SymbolTableEntry
{
	TypePtr type;
	std::optional<int> initial_val; // Only used for type = "const int".

  public:
	SymbolTableEntry() = default;
	SymbolTableEntry(const TypePtr &_type)
	  : type(_type), initial_val(std::nullopt) {}
	SymbolTableEntry(const TypePtr &_type, int _initial_val)
	  : type(_type), initial_val(_initial_val) {}
};

// A chained symbol table.
class SymbolTable
{
	std::vector<std::unordered_map<std::string, SymbolTableEntry>> _table;
	static const std::vector<std::pair<std::string, SymbolTableEntry>> INTERNAL_FUNCTIONS;
  
  public:
	SymbolTable(std::vector<std::pair<std::string, SymbolTableEntry>> init=INTERNAL_FUNCTIONS);
	~SymbolTable() { end_block(); }
	inline bool empty() const { return _table.empty(); }
	void new_block();
	void end_block();

	std::optional<SymbolTableEntry> find(const std::string &name) const;
	bool insert(const std::string &name, const TypePtr &type);
	bool insert(const std::string &name, const TypePtr &type, int initial_val);
		// return whether the insertion is successful.
};

} // namespace compiler::define

#endif