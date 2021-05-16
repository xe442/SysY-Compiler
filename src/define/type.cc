#include "type.h"
#include "exceptions.h"
#include "visitor_helper.h"

using compiler::utils::LambdaVisitor;

namespace compiler::define
{

bool is_const(const TypePtr &type)
{
	static LambdaVisitor const_checker = {
		[](const IntTypePtr &t) { return t->is_const(); },
		[](const ArrayTypePtr &t) { return is_const(t->element_type()); },
		[](const PointerTypePtr &t) { return false; }, // pointers are alway variable
		[](const auto &t) { return false; }
	};
	return std::visit(const_checker, type);
}

bool is_basic(const TypePtr &type)
{
	static LambdaVisitor basic_checker = {
		[](const VoidTypePtr &t) { return true; },
		[](const IntTypePtr &t) { return true; },
		[](const auto &t) { return false; }
	};
	return std::visit(basic_checker, type);
}

bool same_type(const TypePtr &type1, const TypePtr &type2)
{
	static LambdaVisitor type_checker = {
		[](const VoidTypePtr &t1, const VoidTypePtr &t2) { return true; },
		[](const IntTypePtr &t1, const IntTypePtr &t2)
			{ return t1->is_const() == t2->is_const(); },
		[](const ArrayTypePtr &t1, const ArrayTypePtr &t2)
			{
				if(t1->len() != t2->len())
					return false;
				return same_type(t1->element_type(), t2->element_type());
			},
		[](const PointerTypePtr &t1, const PointerTypePtr &t2)
			{ return same_type(t1->base_type(), t2->base_type()); },
		[](const auto &t1, const auto &t2) { return false; }
	};
	return std::visit(type_checker, type1, type2);
}

bool accept_type(const TypePtr &req_type, const TypePtr &prov_type)
{
	static LambdaVisitor type_checker = {
		[](const IntTypePtr &t1, const IntTypePtr &t2) { return true; },
		[](const PointerTypePtr &t1, const ArrayTypePtr &t2)
			{ return accept_type(t1->base_type(), t2->element_type()); },
		[](const PointerTypePtr &t1, const PointerTypePtr &t2)
			{ return accept_type(t1->base_type(), t2->base_type()); },
		[](const auto &t1, const auto &t2) { return false; }
	};
	return std::visit(type_checker, req_type, prov_type);
}

bool can_operate(const TypePtr &type1, const TypePtr &type2)
{
	static LambdaVisitor operate_checker = {
		[](const IntTypePtr &t1, const IntTypePtr &t2) { return true; },
		[](const auto &t1, const auto &t2) { return false; }
	};
	return std::visit(operate_checker, type1, type2);
}

TypePtr common_type(const TypePtr &type1, const TypePtr &type2)
{
	static LambdaVisitor common_type_getter = {
		[](const IntTypePtr &t1, const IntTypePtr &t2)
			{ return t1->is_const() && t2->is_const()? make_int(true) : make_int(false); },
		[](const auto &t1, const auto &t2) -> TypePtr
			{ INTERNAL_ERROR("invalid type for common type, check can_operate before using this"); }
	};
	return std::visit(common_type_getter, type1, type2);
}

int get_size(const TypePtr &type)
{
	static LambdaVisitor type_calculator = {
		[](const std::monostate &t) { return 0; },
		[](const auto &t)
			{ return std::static_pointer_cast<const Type>(t)->size(); }
	};
	return std::visit(type_calculator, type);
}

int ArrayType::element_size() const
{
	static LambdaVisitor size_calculator = {
		[](const IntTypePtr &t) { return t->size(); },
		[](const ArrayTypePtr &t) { return t->size(); },
		[](const auto &t) -> int { INTERNAL_ERROR("invalid array element type"); }
	};
	return std::visit(size_calculator, element_type());
}

ArrayType::ArrayType(TypePtr base_type, std::vector<int> dim_size)
  : ArrayType(base_type, dim_size.begin(), dim_size.end())
{ _size = _len * element_size(); }

ArrayType::ArrayType(TypePtr base_type, std::vector<int>::iterator dim_begin,
		std::vector<int>::iterator dim_end)
{
	if(dim_begin >= dim_end)
		INTERNAL_ERROR("empty dimension array provided for ArrayType constructor");
	if(!is_basic(base_type))
		INTERNAL_ERROR("non-basic type provided for ArrayType constructor");
	
	_len = *dim_begin;
	++dim_begin;
	if(dim_begin != dim_end)
		_ele_type = std::make_shared<ArrayType>(base_type, dim_begin, dim_end);
	else
		_ele_type = base_type;
	_size = _len * element_size();
}

TypePtr make_null()
{
	static const auto NULL_TYPE_PTR = TypePtr(std::monostate());
	return NULL_TYPE_PTR;
}

TypePtr make_void()
{
	static const auto VOID_TYPE_PTR = TypePtr(std::make_shared<VoidType>());
			// the global void type pointers points to this pointer
	return VOID_TYPE_PTR;
}

TypePtr make_int(bool is_const)
{
	static const auto CONST_INT_TYPE_PTR = TypePtr(std::make_shared<IntType>(true));
	static const auto INT_TYPE_PTR = TypePtr(std::make_shared<IntType>(false));
	return is_const? CONST_INT_TYPE_PTR : INT_TYPE_PTR;
}

void TypePrinter::operator() (const std::monostate &t)
{
	out << "invalid";
}
void TypePrinter::operator() (const VoidTypePtr &t)
{
	out << "void";
}
void TypePrinter::operator() (const IntTypePtr &t)
{
	if(t->is_const())
		out << "const int";
	else
		out << "int";
}
void TypePrinter::operator() (const ArrayTypePtr &t)
{
	out << '[' << t->len() << ']';
	std::visit(*this, t->element_type());
}
void TypePrinter::operator() (const PointerTypePtr &t)
{
	out << "pointer to ";
	std::visit(*this, t->base_type());
}
void TypePrinter::operator() (const FuncTypePtr &t)
{
	out << "func(";
	for(int i = 0; i < t->arg_cnt(); i++)
	{
		if(i != 0)
			out << ", ";
		std::visit(*this, t->arg_type(i));
	}
	out << ") -> ";
	std::visit(*this, t->retval_type());
}

} // compiler::define

std::ostream &operator << (std::ostream &out, const compiler::define::TypePtr &type)
{
	std::visit(compiler::define::TypePrinter{out}, type);
	return out;
}