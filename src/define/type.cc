#include "type.h"
#include "dbg.h"
#include "exceptions.h"
#include "visitor_helper.h"

using compiler::utils::LambdaVisitor;

namespace compiler::define
{

bool is_const(const TypePtr &type)
{
	static LambdaVisitor const_checker = {
		[](const IntTypePtr &t) { return t->is_const(); },
		[](const ArrayTypePtr &t) { return t->is_const(); },
		[](const PointerTypePtr &t) { return false; }, // Pointers are always dymatic.
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
		[](const VoidTypePtr &t1, const VoidTypePtr &t2) { return t1->same_as(t2); },
		[](const IntTypePtr &t1, const IntTypePtr &t2) { return t1->same_as(t2); },
		[](const ArrayTypePtr &t1, const ArrayTypePtr &t2) { return t1->same_as(t2); },
		[](const PointerTypePtr &t1, const PointerTypePtr &t2) { return t1->same_as(t2); },
		[](const auto &t1, const auto &t2) { return false; }
	};
	return std::visit(type_checker, type1, type2);
}

bool accept_type(const TypePtr &req_type, const TypePtr &prov_type)
{
	static LambdaVisitor type_checker = {
		[](const IntTypePtr &t1, const IntTypePtr &t2) { return t1->accepts(t2); },
		[](const PointerTypePtr &t1, const ArrayTypePtr &t2) { return t1->accepts(t2); },
		[](const auto &t1, const auto &t2) { return false; }
	};
	return std::visit(type_checker, req_type, prov_type);
}

bool can_operate(const TypePtr &type1, const TypePtr &type2)
{
	static LambdaVisitor operate_checker = {
		[](const IntType &t1, const IntType &t2) { return true; },
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

int ArrayType::element_size() const
{
	static LambdaVisitor size_calculator = {
		[](const IntTypePtr &t) { return t->size(); },
		[](const ArrayTypePtr &t) { return t->size(); },
		[](const auto &t) -> int { INTERNAL_ERROR("invalid array element type"); }
	};
	return std::visit(size_calculator, element_type());
}

bool ArrayType::same_as(ArrayTypePtr other) const
{
	if(len() != other->len() || is_basic(element_type()) != is_basic(other->element_type()))
		return false;
	return std::get<ArrayTypePtr>(element_type())->same_as(
		std::get<ArrayTypePtr>(other->element_type()));
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
	std::visit(*this, t->element_type());
	out << '[' << t->len() << ']';
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