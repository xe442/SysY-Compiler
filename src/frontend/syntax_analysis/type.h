#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <memory>
#include <vector>
#include <variant>

namespace compiler::frontend
{

// Type base class. The types of symbols are filled during sematic analysis.
class Type;

// Type implementations.
class VoidType;
class IntType;
class ArrayType;
class PointerType;
class FuncType;

// Type pointers.
// They are std::shared_ptr's of the corresponding type classes.
using VoidTypePtr = std::shared_ptr<VoidType>;
using IntTypePtr = std::shared_ptr<IntType>;
using ArrayTypePtr = std::shared_ptr<ArrayType>;
using PointerTypePtr = std::shared_ptr<PointerType>;
using FuncTypePtr = std::shared_ptr<FuncType>;

// The universal type pointer.
using TypePtr = std::variant
<
	std::monostate, // indicating nullptr
	VoidTypePtr,
	IntTypePtr,
	ArrayTypePtr,
	PointerTypePtr,
	FuncTypePtr
>;
using TypePtrVec = std::vector<TypePtr>;

bool is_const(const TypePtr &type);
bool is_basic(const TypePtr &type);
bool same_type(const TypePtr &type1, const TypePtr &type2);
bool accept_type(const TypePtr &req_type, const TypePtr &prov_type);
bool can_operate(const TypePtr &type1, const TypePtr &type2);
TypePtr common_type(const TypePtr &type1, const TypePtr &type2);
int get_size(const TypePtr &type);

// Handy constructors.
TypePtr make_null();
TypePtr make_void();
TypePtr make_int(bool is_const=false);

class Type
{
  protected:
	int _size;

  public:
	inline int size() const { return _size; }
};

class VoidType: public Type
{
  public:
	VoidType() { _size = 0; }
};

class IntType: public Type
{
  protected:
	bool _is_const;

  public:

	IntType(bool is_const)
	  : _is_const(is_const) { _size = sizeof(int); }

	// Getter functions.
	inline bool is_const() const { return _is_const; }
};

class ArrayType: public Type
{
  protected:
	int _len;
	TypePtr _ele_type;

  public:
	int element_size() const;

	// Build a array type given the size of all dimensions and a base_type.
	ArrayType(TypePtr base_type, std::vector<int> dim_size);
	ArrayType(TypePtr base_type, std::vector<int>::iterator dim_begin,
		std::vector<int>::iterator dim_end);

	// Getters.
	inline const TypePtr &element_type() const { return _ele_type; }
	inline int len() const { return _len; }
	inline bool is_1d_arr() const { return is_basic(element_type()); }

	// Length setter.
	inline void set_len(int len) { _len = len; }
};

/*
 * Although there are no pointers in Sys-Y, we still define and use it for function
 * arguments.
 * The use of pointer type is limited only for function arguments, and as for array
 * [] operators, we would always return (if not int type) an array type instead of
 * a pointer type.
 */
class PointerType: public Type
{
  protected:
	// missing bool _is_const, since const arrays would not be passed to function as
	// its argument.
	TypePtr _base_type; // the type that the pointer points to

  public:
	PointerType(TypePtr base_type)
	  : _base_type(base_type) { _size = sizeof(int); } // _size not used.

	// Getters.
	inline const TypePtr &base_type() const { return _base_type; }
};

class FuncType: public Type
{
  protected:
	TypePtr _retval_type;
	TypePtrVec _arg_types;

  public:
	FuncType(TypePtr retval_type, TypePtrVec arg_types)
	  : _retval_type(retval_type), _arg_types(std::move(arg_types))
	{ _size = 1; } // _size not used.

	// Getters.
	const TypePtr &retval_type() const { return _retval_type; }
	int arg_cnt() const { return _arg_types.size(); }
	const TypePtrVec &arg_types() const { return _arg_types; }
	const TypePtr &arg_type(int idx) const { return _arg_types[idx]; }
};

struct TypePrinter
{
	std::ostream &out;

	void operator() (const std::monostate &t);
	void operator() (const VoidTypePtr &t);
	void operator() (const IntTypePtr &t);
	void operator() (const ArrayTypePtr &t);
	void operator() (const PointerTypePtr &t);
	void operator() (const FuncTypePtr &t);
};

} // namespace compiler::frontend

std::ostream &operator << (std::ostream &out, const compiler::frontend::TypePtr &type);

#endif