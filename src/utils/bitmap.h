#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>
#include <vector>
#include <unordered_map>

namespace compiler::utils
{

class Bitmap
{
  protected:
	std::vector<uint32_t> _bits;
	size_t _size;

	inline static size_t _ceil_div32(size_t x) { return (x + 31) >> 5; }
	inline static size_t _div32(size_t x) { return x >> 5; }
	inline static int _remain_div32(size_t x) { return static_cast<int>(x & 31); }

  public:
	Bitmap() = default;
	Bitmap(int size)
	  : _bits(_ceil_div32(size), 0), _size(size) {}

	inline size_t size() const { return _size; }
	inline void resize(size_t size) // Note: we do not set the newly added bits to 0.
		{ _bits.resize(_ceil_div32(size)); _size = size; }

	// getters & setters
	inline bool get(size_t idx) const
		{ return (_bits.at(_div32(idx)) >> _remain_div32(idx)) & 1; }
	inline void set(size_t idx)
		{ _bits.at(_div32(idx)) |= 1 << _remain_div32(idx); }
	inline void reset(size_t idx)
		{ _bits.at(_div32(idx)) &= ~(1 << _remain_div32(idx)); }
	inline void set(size_t idx, bool val)
		{ val? set(idx): reset(idx); }
	
	// unary operations
	void clear();
	size_t cnt() const;
	inline void flip(size_t idx)
		{ _bits.at(_div32(idx)) ^= 1 << _remain_div32(idx); }
	void flip_all();

	// binary operations
	void union_with(const Bitmap &other);
	void intersect_with(const Bitmap &other);
	void diff_with(const Bitmap &other);
};

// For simplicity, Key cannot be size_t type, otherwise it cannot compile.
template<class Key>
class NamedBitmap: public Bitmap
{
  protected:
	std::vector<Key> _keys;
	std::unordered_map<Key, size_t> _key_to_idx;

  public:
	NamedBitmap(std::vector<Key> keys)
	  : Bitmap(keys.size()), _keys(std::move(keys))
	{
		for(size_t i = 0; i < _size; i++)
			_key_to_idx.try_emplace(_keys[i], i);
	}

	// For simplicity, a NamedBitmap cannot change its size after constructed.
	inline void resize(size_t size) = delete;

	// Getters & setters can be indexed using a key.
	inline bool get(const Key &key) const { return get(_key_to_idx.at(key)); }
	inline void set(const Key &key) { set(_key_to_idx.at(key)); }
	inline void reset(const Key &key) { reset(_key_to_idx.at(key)); }
	inline void set(const Key &key, bool val) { set(_key_to_idx.at(key), val); }
};

}

#endif