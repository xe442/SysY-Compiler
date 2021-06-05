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

		AstPtr operator() (ConstIntNodePtr &node);
		AstPtr operator() (IdNodePtr &node);
		AstPtr operator() (InitializerNodePtr &node);
		AstPtr operator() (UnaryOpNodePtr &node);
		AstPtr operator() (BinaryOpNodePtr &node);
		template<class ErrorType> AstPtr operator() (ErrorType &node);

		void replace_expr(AstPtr &expr_node);
	};

	// Deduce the type of the IdNode in the declaration part. Also simplifies the
	// array declaration by removing redundent BianryOp ACCESS nodes and replace
	// them with a single IdNode.
	struct DeclTypeDeducer
	{
		ConstExprReplacer &const_expr_replacer;
	  protected:
	  	std::vector<int> _dim_size;
		IdNode *_id_node_ptr;
		bool _is_ptr;

	  public:
		DeclTypeDeducer(ConstExprReplacer &expr_replacer)
		  : const_expr_replacer(expr_replacer), _dim_size(),
			_id_node_ptr(nullptr), _is_ptr(false) {}

		// Do deduce only during visiting.
		void operator() (IdNodePtr &node);
		void operator() (UnaryOpNodePtr &node);
		void operator() (BinaryOpNodePtr &node);
		template<class ErrorType> void operator() (ErrorType &node);
		
		// Do simplification if called from here.
		TypePtr deduce_type(AstPtr &decl_expr,
									const TypePtr &_base_type);

		// Sometimes this function is called through an actual ptr & instead of AstPtr &.
		// template<class T>
		// TypePtr deduce_type(const T &single_var_decl,
		// 	const TypePtr &base_type);
	};

	// Check and set the type of the initializer list. Throw 
	// SemanticError for invalid initializers.
	struct InitializerTypeChecker
	{
		ConstExprReplacer &const_expr_replacer;
	  protected:
		InitializerNode *_parent_ptr; // points to the parent initializer.
		int _idx; // the index of the child node
		bool _const_mode;

	  public:
		InitializerTypeChecker(ConstExprReplacer &expr_replacer)
		  : const_expr_replacer(expr_replacer), _parent_ptr(nullptr),
		  	_idx(0), _const_mode(false) {}

		// Return whether the initializer list is exhausted.
		bool operator() (const ArrayTypePtr &arr_type);
		bool operator() (const IntTypePtr &ele_type);
		template<class ErrorType> bool operator() (const ErrorType &err_type);

		void check_type(AstPtr &initializer, const TypePtr &arr_type);
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

		TypePtr operator() (ConstIntNodePtr &node);
		TypePtr operator() (IdNodePtr &node);
		TypePtr operator() (UnaryOpNodePtr &node);
		TypePtr operator() (BinaryOpNodePtr &node);
		TypePtr operator() (FuncCallNodePtr &node);
		// Missing "TypePtrVec operator() (FuncArgsNodePtr &node)", since
		// we cannot return more than one type for a visitor class. Simply
		// check this in FuncCallNode.
		template<class ErrorType> TypePtr operator() (ErrorType &node);

		TypePtr check_type(AstPtr &expr_node, bool optimize_constants=false);
	};

  public:
	SemanticSymbolTable table;
	ConstExprReplacer const_expr_replacer;
	DeclTypeDeducer type_deducer;
	TypeChecker type_checker;
	InitializerTypeChecker initilaizer_type_checker;

	// For declaration check.
	TypePtr base_type = make_null();

	// For break & continue binding. We use raw pointer here to work as a variable
	// reference.
	const WhileNode *inner_loop = nullptr;

	// For function return value check.
	const FuncDefNode *curr_func = nullptr;
	inline bool is_global() const { return curr_func == nullptr; }

	// Does the semantic checking process.
	SemanticChecker();
	void operator() (ProgramNodePtr &node);
	void operator() (VarDeclNodePtr &node);
	void operator() (SingleVarDeclNodePtr &node);
	void operator() (FuncDefNodePtr &node);
	void operator() (FuncParamsNodePtr &node);
	void operator() (SingleFuncParamNodePtr &node);
	void operator() (BlockNodePtr &node);
	void operator() (IfNodePtr &node);
	void operator() (WhileNodePtr &node);
	void operator() (BreakNodePtr &node);
	void operator() (ContNodePtr &node);
	void operator() (RetNodePtr &node);

	void operator() (ConstIntNodePtr &node);
	void operator() (IdNodePtr &node);
	void operator() (UnaryOpNodePtr &node);
	void operator() (BinaryOpNodePtr &node);
	void operator() (FuncCallNodePtr &node);
	template<class ErrorNodePtr> void operator() (ErrorNodePtr &node);
};

} // namespace compiler::frontend

#endif