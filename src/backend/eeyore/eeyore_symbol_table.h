#ifndef EEYORE_SYMBOL_TABLE_H
#define EEYORE_SYMBOL_TABLE_H

#include "chained_map.h"
#include "type.h"
#include "eeyore.h"

namespace compiler::backend::eeyore
{

struct EeyoreSymbolTableEntry
{
	using VarType = std::variant<std::monostate, OrigVar, Param>;

	define::TypePtr type;
	VarType var;

	EeyoreSymbolTableEntry(const define::TypePtr &_type, const VarType &_var=std::monostate())
	  : type(_type), var(_var) {}
	EeyoreSymbolTableEntry(define::TypePtr &&_type, VarType &&_var=std::monostate())
	  : type(_type), var(_var) {}
};

class EeyoreSymbolTable: public utils::ChainedMap<std::string, EeyoreSymbolTableEntry>
{
	static const std::vector<std::pair<std::string, EeyoreSymbolTableEntry>>
		INTERNAL_FUNCTIONS;
  public:
	EeyoreSymbolTable();

	inline std::optional<EeyoreSymbolTableEntry> find(const std::string &name)
	{
		auto find_res = basic_find(name);
		if(find_res.has_value())
			return find_res.value()->second;
		else
			return std::nullopt;
	}
	
	template<class... Ts>
	bool insert(const std::string &name, const Ts&... ts)
	{
		return basic_insert(name, EeyoreSymbolTableEntry(ts...)).second;
	}

	template<class... Ts>
	bool insert(std::string &&name, Ts&&... ts)
	{
		return basic_insert(std::move(name), EeyoreSymbolTableEntry(std::move(ts)...)).second;
	}
};

} // namespace compiler::backend::eeyore

#endif