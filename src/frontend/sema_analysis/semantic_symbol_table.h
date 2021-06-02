#ifndef SEMANTIC_SYMBOL_TABLE_H
#define SEMANTIC_SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <utility>
#include "chained_map.h"
#include "type.h"

namespace compiler::frontend
{

struct SemanticSymbolTableEntry
{
	define::TypePtr type;
	std::optional<int> initial_val; // Only used for type = "const int".

  public:
	SemanticSymbolTableEntry() = default;
	SemanticSymbolTableEntry(const define::TypePtr &_type)
	  : type(_type), initial_val(std::nullopt) {}
	SemanticSymbolTableEntry(define::TypePtr &&_type)
	  : type(_type), initial_val(std::nullopt) {}
	SemanticSymbolTableEntry(const define::TypePtr &_type, int _initial_val)
	  : type(_type), initial_val(_initial_val) {}
	SemanticSymbolTableEntry(define::TypePtr &&_type, int _initial_val)
	  : type(_type), initial_val(_initial_val) {}
};

class SemanticSymbolTable: public utils::ChainedMap<std::string, SemanticSymbolTableEntry>
{
	static const std::vector<std::pair<std::string, SemanticSymbolTableEntry>> INTERNAL_FUNCTIONS;

  public:
	SemanticSymbolTable();

	template<class... Ts>
	bool insert(const std::string &name, const Ts... ts)
	{
		return basic_insert(name, SemanticSymbolTableEntry(std::move(ts)...)).second;
	}
	
	template<class... Ts>
	bool insert(std::string &&name, const Ts... ts)
	{
		return basic_insert(name, SemanticSymbolTableEntry(std::move(ts)...)).second;
	}
};

} // namespace compiler::frontend

#endif