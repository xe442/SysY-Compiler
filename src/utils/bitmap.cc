#include "bitmap.h"
#include "exceptions.h"

namespace compiler::utils
{

void Bitmap::clear()
{
	for(auto &data : _bits)
		data = 0;
}

size_t Bitmap::cnt() const
{
	size_t res = 0;
	for(int i = 0; i < _size; i++)
		res += get(i);
	return res;
}

void Bitmap::flip_all()
{
	for(auto &data : _bits)
		data = ~data;
}

void Bitmap::union_with(const Bitmap &other)
{
	INTERNAL_ASSERT(other.size() == size(), "invalid union operation on different sized bitmap");
	size_t data_cnt = _bits.size();
	for(size_t i = 0; i < data_cnt; i++)
		_bits.at(i) |= other._bits.at(i);
}

void Bitmap::intersect_with(const Bitmap &other)
{
	INTERNAL_ASSERT(other.size() == size(), "invalid union operation on different sized bitmap");
	size_t data_cnt = _bits.size();
	for(size_t i = 0; i < data_cnt; i++)
		_bits.at(i) &= other._bits.at(i);
}

void Bitmap::diff_with(const Bitmap &other)
{
	INTERNAL_ASSERT(other.size() == size(), "invalid union operation on different sized bitmap");
	size_t data_cnt = _bits.size();
	for(size_t i = 0; i < data_cnt; i++)
		_bits.at(i) &= ~(other._bits.at(i));
}

}