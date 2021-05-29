#ifndef EEYORE_GEN_H
#define EEYORE_GEN_H

#include <list>
#include <stack>
#include <variant>
#include "eeyore.h"
#include "eeyore_printer.h"
#include "eeyore_symbol_table.h"

namespace compiler::backend::eeyore
{

class EeyoreGenerator
{
  protected:
	// Register allocation part.
	class ResourceManager
	{
		int _orig_id;
		int _temp_id;
		int _label_id;

	  public:
		ResourceManager(): _orig_id(0), _temp_id(0), _label_id(0) {}
		
		inline OrigVar get_original_var(int _size=sizeof(int))
			{ return OrigVar(_orig_id++, _size); }
		inline TempVar get_temp_var()
			{ return TempVar(_temp_id++); }
		inline Label get_label()
			{ return Label(_label_id++); }
	};
	ResourceManager resources;

	// Argument part. Since std::visit does not allow non-variant type as
	// argument, we use GeneratorState to hold all the arguments needed by
	// visitor methods.
	class GeneratorState
	{
	  protected:
		// General argument for a statement.
		// Where to go for continue and break statement.
		// Controlled by visitor method of WhileStmt.
		std::optional<std::pair<Label, Label>> cont_break_label;

	  public:
		inline bool in_loop() const { return cont_break_label.has_value(); }
		inline Label cont_label() const { return cont_break_label.value().first; }
		inline Label break_label() const { return cont_break_label.value().second; }
		inline std::optional<std::pair<Label, Label>> store_cont_break_label() const
			{ return cont_break_label; }
		inline void restore_cont_break_label(std::optional<std::pair<Label, Label>> labels)
			{ cont_break_label = labels; }
		inline void set_cont_label(Label l) { cont_break_label.value().first = l; }
		inline void set_break_label(Label l) { cont_break_label.value().second = l; }
		inline void set_cont_break_label(Label cont_label, Label break_label)
			{ cont_break_label = std::make_pair(cont_label, break_label); }
		inline void reset_cont_break_label() { cont_break_label.reset(); }

	  protected:
		// Where to go when the condition is true or false.
		// Controlled by visitor methods of IfStmt and WhileStmt.
		std::optional<std::pair<Label, Label>> true_false_label;

	  public:
		inline bool is_cond_expr() const { return true_false_label.has_value(); }
		inline Label true_label() const { return true_false_label.value().first; }
		inline Label false_label() const { return true_false_label.value().second; }
		inline std::optional<std::pair<Label, Label>> store_true_false_label() const
			{ return true_false_label; }
		inline void restore_true_false_label(std::optional<std::pair<Label, Label>> labels)
			{ true_false_label = labels; }
		inline void set_true_label(Label l) { true_false_label.value().first = l; }
		inline void set_false_label(Label l) { true_false_label.value().second = l; }
		inline void set_true_false_label(Label true_label, Label false_label)
			{ true_false_label = std::make_pair(true_label, false_label); }
		inline void reset_true_false_label() { true_false_label.reset(); }
	
	  protected:
	  	// Here we define the lval mode. The behavior is vastly different
		// for lvals and rvals.
		// For lvals, we need to update write_operand (and access_index
		// if it is array access) and return nothing.
		// However for rvals, there are two modes: read mode and write mode.
		// For read mode, simply return the Var calculated. As for write
		// mode, set the write_operand (or the element of it).
	  	bool lval_mode;
	
		// Lvals should set them when reached leaf nodes and let rvals to modify
		// them.
		std::optional<Operand> write_operand;
		std::optional<Operand> array_offset; // If writing to array, we need an offset.

	  public:
	  	inline bool is_lval_mode() const { return lval_mode; }
		inline bool is_rval_mode() const { return !lval_mode; }
		inline void set_lval_mode() { lval_mode = true; }
		inline void set_rval_mode() { lval_mode = false; }
		
		inline bool is_write_mode() const { return write_operand.has_value(); }
		inline bool is_read_mode() const { return !write_operand.has_value(); }
		inline bool is_write_array() const { return is_write_mode() && array_offset.has_value(); }
		inline Operand write_opr() const { return write_operand.value(); }
		inline Operand arr_offset() const { return array_offset.value(); }
		inline void set_write_opr(Operand opr) { write_operand = opr; }
		inline void reset_write_opr() { write_operand.reset(); }
		inline void set_arr_offset(Operand offset) { array_offset = offset; }
		inline void reset_arr_offset() { array_offset.reset(); }
		inline std::optional<Operand> store_write_opr() { return write_operand; }
		inline std::optional<Operand> store_arr_offset() { return array_offset; }
		inline void restore_write_opr(std::optional<Operand> opr) { write_operand = opr; }
		inline void restore_arr_offset(std::optional<Operand> opr) { array_offset = opr; }

	  protected:
		// For array access, we need a series of indices to generate the access code.
		std::vector<Operand> access_index;

	  public:
	  	inline bool is_arr_access() const { return !access_index.empty(); }
	  	inline const std::vector<Operand> &access_idx() const { return access_index; }
		inline std::vector<Operand> &&store_access_idx() { return std::move(access_index); }
		inline void clear_access_idx() { access_index.clear(); }
		inline void add_access_idx(Operand idx) { access_index.push_back(idx); }
		inline void pop_access_idx() { access_index.pop_back(); }
		inline void restore_access_idx(std::vector<Operand> &&idx) { access_index = idx; }
	
	  protected:
	  	// For initializer matching. This holds the type of the initializer that is matching.
	  	std::optional<define::TypePtr> curr_type;
	
	  public:
	  	inline define::TypePtr arr_type() const { return curr_type.value(); }
		inline void set_arr_type(define::TypePtr arr_type) { curr_type = arr_type; }
		inline void reset_arr_type() { curr_type.reset(); }
	  	inline void set_initializer_write(define::TypePtr arr_type)
			{ set_arr_offset(0); curr_type = arr_type; }
		inline void reset_initializer_write()
			{ reset_arr_offset(); curr_type.reset(); }
		
		void reset_all();
	};
	GeneratorState state;

	EeyoreSymbolTable table; // Generator symbol table.

	// the generated statements are stored here.
	std::list<EeyoreStatement> eeyore_code;

	// Rearranges the code generated.
	// Moves variable definition forward to the beginning of a function, and
	// moves global assignment to f_main.
	class EeyoreRearranger
	{
	  public:
		void rearrange(std::list<EeyoreStatement> &eeyore_code);
	};
	EeyoreRearranger rearranger;

	// Optimize the generated eeyore_code locally.
	class EeyoreOptimizer
	{
	  protected:
		void _remove_useless_labels_and_jumps(
			std::list<EeyoreStatement> &eeyore_code);
		
	  public:
	  	void optimize(std::list<EeyoreStatement> &eeyore_code);
	};
	EeyoreOptimizer optimizer;

  public:
	EeyoreGenerator() = default;

	// visitor methods, returning the value read.
	std::optional<Operand> operator() (const define::ProgramNodePtr &node);
	std::optional<Operand> operator() (const define::VarDeclNodePtr &node);
	std::optional<Operand> operator() (const define::SingleVarDeclNodePtr &node);
	std::optional<Operand> operator() (const define::ConstIntNodePtr &node);
	std::optional<Operand> operator() (const define::IdNodePtr &node);
	std::optional<Operand> operator() (const define::InitializerNodePtr &node);
	std::optional<Operand> operator() (const define::FuncDefNodePtr &node);
	std::optional<Operand> operator() (const define::BlockNodePtr &node);
	std::optional<Operand> operator() (const define::UnaryOpNodePtr &node);
	std::optional<Operand> operator() (const define::BinaryOpNodePtr &node);
	std::optional<Operand> operator() (const define::IfNodePtr &node);
	std::optional<Operand> operator() (const define::WhileNodePtr &node);
	std::optional<Operand> operator() (const define::BreakNodePtr &node);
	std::optional<Operand> operator() (const define::ContNodePtr &node);
	std::optional<Operand> operator() (const define::RetNodePtr &node);
	std::optional<Operand> operator() (const define::FuncCallNodePtr &node);

	template<class NodePtr> // other nodes do not generate any code.
	std::optional<Operand> operator() (const NodePtr &node);

	// Main entrance of this class.
	const std::list<EeyoreStatement> &generate_eeyore(const define::AstPtr &ast);
};

}

#endif