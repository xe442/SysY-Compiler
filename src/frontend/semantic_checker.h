#ifndef SEMANTIC_CHECKER_H
#define SEMANTIC_CHECKER_H

#include <variant>
#include <type_traits>
#include "ast_node.h"
#include "semantic_symbol_table.h"

namespace compiler::frontend
{

class SemanticChecker
{
  	// Definition of semantic error.
  protected:
	static const char _SEMANTIC_ERROR_NAME[]; // = "Semantic error"
  public:
	using SemanticError = utils::CompilerErrorBase<_SEMANTIC_ERROR_NAME>;

  protected:
	// Calculates the value of a const expression and replace the node with
	// ConstIntNode. Throws SemanticError if such expression contains
	// non-const ids.
	struct ConstExprReplacer
	{
		const SemanticSymbolTable &table;

		ConstExprReplacer(SemanticSymbolTable &_table)
		  : table(_table) {}

		define::AstPtr operator() (define::ConstIntNodePtr &node);
		define::AstPtr operator() (define::IdNodePtr &node);
		define::AstPtr operator() (define::InitializerNodePtr &node);
		define::AstPtr operator() (define::UnaryOpNodePtr &node);
		define::AstPtr operator() (define::BinaryOpNodePtr &node);
		template<class ErrorType> define::AstPtr operator() (ErrorType &node);

		void replace_expr(define::AstPtr &expr_node);
	};

	// Deduce the type of the IdNode in the declaration part. Also simplifies the
	// array declaration by removing redundent BianryOp ACCESS nodes and replace
	// them with a single IdNode.
	struct DeclTypeDeducer
	{
		ConstExprReplacer &const_expr_replacer;
	  protected:
	  	std::vector<int> _dim_size;
		define::IdNode *_id_node_ptr;
		bool _is_ptr;

	  public:
		DeclTypeDeducer(ConstExprReplacer &expr_replacer)
		  : const_expr_replacer(expr_replacer), _dim_size(),
			_id_node_ptr(nullptr), _is_ptr(false) {}

		// Do deduce only during visiting.
		void operator() (define::IdNodePtr &node);
		void operator() (define::UnaryOpNodePtr &node);
		void operator() (define::BinaryOpNodePtr &node);
		template<class ErrorType> void operator() (ErrorType &node);
		
		// Do simplification if called from here.
		define::TypePtr deduce_type(define::AstPtr &decl_expr,
									const define::TypePtr &_base_type);

		// Sometimes this function is called through an actual ptr & instead of AstPtr &.
		// template<class T>
		// define::TypePtr deduce_type(const T &single_var_decl,
		// 	const define::TypePtr &base_type);
	};

	// Check and set the type of the initializer list. Throw 
	// SemanticError for invalid initializers.
	struct InitializerTypeChecker
	{
		ConstExprReplacer &const_expr_replacer;
	  protected:
		define::InitializerNode *_parent_ptr; // points to the parent initializer.
		int _idx; // the index of the child node
		bool _const_mode;

	  public:
		InitializerTypeChecker(ConstExprReplacer &expr_replacer)
		  : const_expr_replacer(expr_replacer), _parent_ptr(nullptr),
		  	_idx(0), _const_mode(false) {}

		// Return whether the initializer list is exhausted.
		bool operator() (const define::ArrayTypePtr &arr_type);
		bool operator() (const define::IntTypePtr &ele_type);
		template<class ErrorType> bool operator() (const ErrorType &err_type);

		void check_type(define::AstPtr &initializer, const define::TypePtr &arr_type);
	};

	// Checks if there is a type mismatch in an expression. Returns the type of
	// the expression. Does necessary constants optimization when needed.
	struct TypeChecker
	{
		const SemanticSymbolTable &table;
		bool optimize_constants; // TODO: implement optimize_constants.
								 // This is currently not functional.

		TypeChecker(SemanticSymbolTable &_table)
		  : table(_table), optimize_constants(false) {}

		define::TypePtr operator() (define::ConstIntNodePtr &node);
		define::TypePtr operator() (define::IdNodePtr &node);
		define::TypePtr operator() (define::UnaryOpNodePtr &node);
		define::TypePtr operator() (define::BinaryOpNodePtr &node);
		define::TypePtr operator() (define::FuncCallNodePtr &node);
		// Missing "TypePtrVec operator() (FuncArgsNodePtr &node)", since
		// we cannot return more than one type for a visitor class. Simply
		// check this in FuncCallNode.
		template<class ErrorType> define::TypePtr operator() (ErrorType &node);

		define::TypePtr check_type(define::AstPtr &expr_node, bool optimize_constants=false);
	};

  public:
	SemanticSymbolTable table;
	ConstExprReplacer const_expr_replacer;
	DeclTypeDeducer type_deducer;
	TypeChecker type_checker;
	InitializerTypeChecker initilaizer_type_checker;

	// For declaration check.
	define::TypePtr base_type = define::make_null();

	// For break & continue binding. We use raw pointer here to work as a variable
	// reference.
	const define::WhileNode *inner_loop = nullptr;

	// For function return value check.
	const define::FuncDefNode *curr_func = nullptr;
	inline bool is_global() const { return curr_func == nullptr; }

	// Does the semantic checking process.
	SemanticChecker();
	void operator() (define::ProgramNodePtr &node);
	void operator() (define::VarDeclNodePtr &node);
	void operator() (define::SingleVarDeclNodePtr &node);
	void operator() (define::FuncDefNodePtr &node);
	void operator() (define::FuncParamsNodePtr &node);
	void operator() (define::SingleFuncParamNodePtr &node);
	void operator() (define::BlockNodePtr &node);
	void operator() (define::IfNodePtr &node);
	void operator() (define::WhileNodePtr &node);
	void operator() (define::BreakNodePtr &node);
	void operator() (define::ContNodePtr &node);
	void operator() (define::RetNodePtr &node);

	void operator() (define::ConstIntNodePtr &node);
	void operator() (define::IdNodePtr &node);
	void operator() (define::UnaryOpNodePtr &node);
	void operator() (define::BinaryOpNodePtr &node);
	void operator() (define::FuncCallNodePtr &node);
	template<class ErrorNodePtr> void operator() (ErrorNodePtr &node);
};

} // namespace compiler::frontend

#endif