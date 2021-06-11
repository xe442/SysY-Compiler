#ifndef CFG_H
#define CFG_H

#include <vector>
#include <list>
#include <algorithm>
#include "bitmap.h"
#include "eeyore.h"

namespace compiler::backend::tigger
{

class ControlFlowGraph
{
  public:
	using EeyoreCode = std::list<eeyore::EeyoreStatement>;
	using EeyoreStmtPtr = std::list<eeyore::EeyoreStatement>::const_iterator;

	// A basic block of a program.
	struct BasicBlock
	{
		int id;
		int begin_stmt_id;
		int end_stmt_id;
		EeyoreStmtPtr begin;
		EeyoreStmtPtr end;
		utils::Bitmap live_gen;
		utils::Bitmap live_kill;
		utils::Bitmap live_in;
		utils::Bitmap live_out;

		inline EeyoreStmtPtr back() const
		{
			EeyoreStmtPtr l = end;
			return --l;
		}
		inline int back_stmt_id() const { return end_stmt_id - 1; }

		inline void resize_all_bitmap(size_t size)
		{
			live_gen.resize(size);
			live_kill.resize(size);
			live_in.resize(size);
			live_out.resize(size);
		}
	};

  protected:
	std::vector<eeyore::Operand> _global_vars;
	std::vector<eeyore::Operand> _defined_vars;
	std::unordered_map<eeyore::Operand, int> _defined_var_idx_map;
	
	std::vector<BasicBlock> _all_vertices; // All the basic blocks (as vertices of cfg).
							// Also use a basic block (id = 0) to store all global
							// variable decls.
	std::vector<std::vector<int>> _graph; // the actual graph, stored by adjacency list.
	std::vector<int> _func_start; // Where all the function starts.
						// They are the starting block id of each component in the graph.

	void _build_from_code(const EeyoreCode &eeyore_code);

  public:
	ControlFlowGraph(const EeyoreCode &eeyore_code,
			std::vector<eeyore::Operand> defined_vars)
	  : _defined_vars(std::move(defined_vars)) { _build_from_code(eeyore_code); }
	
	void clear();

	inline int vertex_cnt() const { return _all_vertices.size(); }
	inline const BasicBlock &vertex(int u) const { return _all_vertices.at(u); }
	inline const std::vector<BasicBlock> &all_vertices() const
		{ return _all_vertices; }
	inline const std::vector<int> &successor_ids(int u) const
		{ return _graph.at(u); }
	inline const std::vector<eeyore::Operand> &global_vars() const
		{ return _global_vars; }
	inline BasicBlock &vertex_(int u) { return _all_vertices.at(u); }
	inline std::vector<BasicBlock> &all_vertices_() { return _all_vertices; }

	inline const std::vector<int> &func_start_id() const { return _func_start; }
	
	inline const std::vector<eeyore::Operand> &defined_var() const
		{ return _defined_vars; }
	inline eeyore::Operand defined_var(int i) const
		{ return _defined_vars[i]; }
	inline const std::unordered_map<eeyore::Operand, int> &defined_var_idx_map() const
		{ return _defined_var_idx_map; }
	
	inline bool is_global_var(eeyore::Operand opr) const
	{
		// TODO: This is currently an O(n) algorithm. Optimize it later.
		return std::find(_global_vars.begin(), _global_vars.end(), opr) != _global_vars.end();
	}
};

}

#endif