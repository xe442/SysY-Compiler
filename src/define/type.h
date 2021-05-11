#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <memory>
#include <vector>
#include <variant>

namespace compiler::define
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

// Handy constructors.
TypePtr make_null();
TypePtr make_void();
TypePtr make_int(bool is_const);

class Type
{
  protected:
	int _size;

  public:
	virtual void init_size() = 0;
	inline int size() const { return _size; }
};

class VoidType: public Type
{
  public:
	void init_size() override { _size = 0; }
	VoidType() {}

	// Type checkers.
	bool same_as(VoidTypePtr other) const { return true; }
	// Missing bool accepts: void types can not be argument types.
};

class IntType: public Type
{
  protected:
	bool _is_const;

  public:
	void init_size() override { _size = sizeof(int); }

	IntType(bool is_const)
	  : _is_const(is_const) { init_size(); }

	// Type checkers.
	bool same_as(IntTypePtr other) const { return is_const() == other->is_const(); }
	bool accepts(IntTypePtr arg_type) const { return true; } // any int is acceptible

	// Getter functions.
	inline bool is_const() const { return _is_const; }
};

class ArrayType: public Type
{
  protected:
	bool _is_const;
	int _len;
	TypePtr _ele_type;

  public:
	int element_size() const;
	void init_size() { _size = _len * element_size(); }

	ArrayType(bool is_const, bool is_lval, int len, TypePtr ele_type)
	  : _is_const(is_const), _len(len), _ele_type(std::move(ele_type))
	{ init_size(); }

	// Type checkers.
	bool same_as(ArrayTypePtr other) const;
	// Missing bool accepts: array types cannot be argument types.

	// Getters.
	inline bool is_const() const { return _is_const; }
	inline const TypePtr &element_type() const { return _ele_type; }
	inline int len() const { return _len; }

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
	void init_size() override { _size = sizeof(int); }

	PointerType(TypePtr base_type)
	  : _base_type(base_type) {}

	// Type checkers.
	bool same_as(PointerTypePtr other) const
		{ return same_type(base_type(), other->base_type()); }
	bool accepts(ArrayTypePtr func_arg) const
		// this is speical, a pointer type can only accept an array type as argument.
		{ return same_type(base_type(), func_arg->element_type()); }

	// Getters.
	inline const TypePtr &base_type() const { return _base_type; }
};

class FuncType: public Type
{
  protected:
	TypePtr _retval_type;
	std::vector<TypePtr> _arg_types;

  public:
	void init_size() override { _size = 0; } // TODO: what is the size of a function?

	FuncType(TypePtr retval_type, std::vector<TypePtr> arg_types)
	  : _retval_type(retval_type), _arg_types(std::move(arg_types)) {}

	// No type checkers, since we never compare whether two functions are the same.

	// Getters.
	const TypePtr &retval_type() const { return _retval_type; }
	int arg_cnt() const { return _arg_types.size(); }
	const std::vector<TypePtr> &arg_types() const { return _arg_types; }
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

} // namespace compiler::define

std::ostream &operator << (std::ostream &out, const compiler::define::TypePtr &type);

#endif