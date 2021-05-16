#ifndef SEMANTIC_SYMBOL_TABLE_H
#define SEMANTIC_SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <utility>
#include "chained_map.h"
#include "type.h"

namespace compiler::frontend
{

struct SymbolTableEntry
{
	define::TypePtr type;
	std::optional<int> initial_val; // Only used for type = "const int".

  public:
	SymbolTableEntry() = default;
	SymbolTableEntry(const define::TypePtr &_type)
	  : type(_type), initial_val(std::nullopt) {}
	SymbolTableEntry(define::TypePtr &&_type)
	  : type(_type), initial_val(std::nullopt) {}
	SymbolTableEntry(const define::TypePtr &_type, int _initial_val)
	  : type(_type), initial_val(_initial_val) {}
	SymbolTableEntry(define::TypePtr &&_type, int _initial_val)
	  : type(_type), initial_val(_initial_val) {}
};

class SymbolTable: public utils::ChainedMap<std::string, SymbolTableEntry>
{
	static const std::vector<std::pair<std::string, SymbolTableEntry>> INTERNAL_FUNCTIONS;

  public:
	SymbolTable();

	std::optional<SymbolTableEntry> find(const std::string &name) const;
	
	template<class... Ts>
	bool insert(const std::string &name, const Ts... ts)
	{
		return basic_insert(name, SymbolTableEntry(std::move(ts)...)).second;
	}
	
	template<class... Ts>
	bool insert(std::string &&name, const Ts... ts)
	{
		return basic_insert(name, SymbolTableEntry(std::move(ts)...)).second;
	}
};

} // namespace compiler::frontend

#endif