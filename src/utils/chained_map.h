#ifndef CHAINED_MAP_H
#define CHAINED_MAP_H

#include <list>
#include <unordered_map>
#include <optional>
#include "exceptions.h"

namespace compiler::utils
{

// Base class of symbol table. Can also be used to hold other chained-block-like elements.
template<class TKey, class TVal>
class ChainedMap
{
  protected:
	std::list<std::unordered_map<TKey, TVal>> _table;

  public:
  	using MapIter = typename std::unordered_map<TKey, TVal>::iterator;
  	using MapConstIter = typename std::unordered_map<TKey, TVal>::const_iterator;
  	
	ChainedMap(): _table() { new_block(); }
	virtual ~ChainedMap() { end_block(); }

	// Empty is an invalid state for ChainedMap, since the insertion operation
	// requires at least one block.
	inline bool empty() const { return _table.empty(); }

	// Adding & removing a block at the end.
	inline void new_block() { _table.emplace_back(); }
	inline void end_block()
		{ if(empty()) INTERNAL_ERROR("no block to end for chained map"); _table.pop_back(); }

	// Searching & inserting operations.
	std::optional<MapIter> basic_find(const TKey &key)
	{
		for(auto riter = _table.rbegin(); riter != _table.rend(); ++riter)
		{
			auto find_pos = riter->find(key);
			if(find_pos != riter->end())
				return find_pos;
		}
		return std::nullopt;
	}
	std::optional<MapConstIter> basic_find(const TKey &key) const
	{
		for(auto riter = _table.crbegin(); riter != _table.crend(); ++riter)
		{
			auto find_pos = riter->find(key);
			if(find_pos != riter->end()) // found
				return find_pos;
		}
		return std::nullopt;
	}

	// Returns: the inserted position + whether the insertion is successful.
	std::pair<MapIter, bool> basic_insert(const std::pair<TKey, TVal> &key_val_pair)
	{
		if(empty())
			INTERNAL_ERROR("cannot insert element to an empty chained map");
		return _table.back().insert(key_val_pair);
	}
	std::pair<MapIter, bool> basic_insert(std::pair<TKey, TVal> &&key_val_pair)
	{
		if(empty())
			INTERNAL_ERROR("cannot insert element to an empty chained map");
		return _table.back().insert(std::move(key_val_pair));
	}
	std::pair<MapIter, bool> basic_insert(const TKey &key, const TVal &val)
	{
		if(empty())
			INTERNAL_ERROR("cannot insert element to an empty chained map");
		return _table.back().insert(std::make_pair(key, val));
	}
	std::pair<MapIter, bool> basic_insert(TKey &&key, TVal &&val)
	{
		if(empty())
			INTERNAL_ERROR("cannot insert element to an empty chained map");
		return _table.back().insert(std::make_pair(std::move(key), std::move(val)));
	}
};

} // namespace compiler::define

#endif