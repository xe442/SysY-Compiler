#define DEBUG
#include "dbg.h"
#include "visitor_helper.h"
#include "tigger_gen.h"

using std::holds_alternative;

namespace compiler::backend::tigger
{

void ControlFlowGraph::clear()
{
	_global_vars.clear();
	_defined_var_idx_map.clear();
	_all_vertices.clear();
	_graph.clear();
	_func_start.clear();
}

void ControlFlowGraph::_build_from_code(
	const std::list<eeyore::EeyoreStatement> &eeyore_code)
{
	static utils::LambdaVisitor label_id_getter = {
		[](const eeyore::GotoStmt &stmt) { return stmt.goto_label.id; },
		[](const eeyore::CondGotoStmt &stmt) { return stmt.goto_label.id; },
		[](const eeyore::LabelStmt &stmt) { return stmt.label.id; },
		[](const auto &stmt) -> int { INTERNAL_ERROR("invalid call to label_id_getter"); }
	};

	clear();

	// Build defined variable -> index mapping.
	for(int i = 0; i < _defined_vars.size(); i++)
		_defined_var_idx_map.try_emplace(_defined_vars[i], i);

	// Generate all basic blocks.
	int basic_block_id = 0;
	std::unordered_map<int, int> label_id_to_block_id;
			// Maps a label to the block that contains it.

	auto iter = eeyore_code.cbegin();
	int stmt_id = 0;

	// global_vars block initialization
	BasicBlock global_vars;
	global_vars.id = basic_block_id++;
	global_vars.begin = iter;
	global_vars.begin_stmt_id = stmt_id;
	while(iter != eeyore_code.end()
		&& holds_alternative<eeyore::DeclStmt>(*iter))
	{
		eeyore::Operand global_var = std::get<eeyore::DeclStmt>(*iter).var;
		_global_vars.push_back(global_var);
		++iter; ++stmt_id;
	}
	global_vars.end = iter;
	global_vars.end_stmt_id = stmt_id;
	_all_vertices.push_back(global_vars);

	// other basic block initialization
	while(iter != eeyore_code.end())
	{
		BasicBlock block;
		block.id = basic_block_id++;
		block.begin = iter;
		block.begin_stmt_id = stmt_id;

		if(holds_alternative<eeyore::LabelStmt>(*iter))
		{
			int label_id = std::visit(label_id_getter, *iter);
			label_id_to_block_id[label_id] = block.id;
			++iter, ++stmt_id;
		}
		else if(holds_alternative<eeyore::FuncDefStmt>(*iter))
			_func_start.push_back(block.id);
		
		// Find all the way to the last statement. The last statement of a basic
		// block can only be: GotoStmt, CondGotoStmt, FuncEndStmt, ReturnStmt,
		// or a statement that is followed by a label statement.
		for( ; iter != eeyore_code.end(); ++iter, ++stmt_id)
		{
			if(holds_alternative<eeyore::LabelStmt>(*iter))
			{
				block.end = iter;
				block.end_stmt_id = stmt_id;
				break;
			}
			if(holds_alternative<eeyore::GotoStmt>(*iter)
				|| holds_alternative<eeyore::CondGotoStmt>(*iter))
			{
				block.end = ++iter;
				block.end_stmt_id = ++stmt_id;
				break;
			}
			if(holds_alternative<eeyore::RetStmt>(*iter))
				// EndFuncDefStmt is special: it can only occur after a RetStmt,
				// and is contained in this basic block after RetStmt.
			{
				++iter; ++stmt_id;
				if(iter != eeyore_code.end()
					&& holds_alternative<eeyore::EndFuncDefStmt>(*iter))
				{
					block.end = ++iter;
					block.end_stmt_id = ++stmt_id;
				}
				else
				{
					block.end = iter;
					block.end_stmt_id = stmt_id;
				}
				break;
			}
		}
		_all_vertices.push_back(block);
	}

	// Set the bitmap size.
	for(auto &v : _all_vertices)
		v.resize_all_bitmap(_defined_vars.size());

	DBG(std::cout << "all vertices created!" << std::endl);

	// Create edges.
	_graph.resize(basic_block_id);
	for(int i = 1; i < vertex_cnt(); i++) // The successors of the global variable
										  // block are ignored.
	{
		const BasicBlock &block = vertex(i);
		const auto &last_stmt = *block.back();

		if(holds_alternative<eeyore::GotoStmt>(last_stmt))
		{
			int label_id = std::visit(label_id_getter, last_stmt);
			_graph[i].push_back(label_id_to_block_id[label_id]);
		}
		else if(holds_alternative<eeyore::CondGotoStmt>(last_stmt))
		{
			int label_id = std::visit(label_id_getter, last_stmt);
			int goto_when_true = label_id_to_block_id[label_id];
			int goto_when_false = i + 1;

			_graph[i].push_back(goto_when_true);
			if(goto_when_true != goto_when_false) // Remove multiple edges.
				_graph[i].push_back(goto_when_false);
		}
		else if(!holds_alternative<eeyore::EndFuncDefStmt>(last_stmt)
			&& !holds_alternative<eeyore::RetStmt>(last_stmt))
		{
			_graph[i].push_back(i + 1);
		}
	}
	DBG(
		std::cout << "all edges created!" << std::endl;
		for(const BasicBlock &v : all_vertices())
		{
			std::cout << "basic block #" << v.id << ": ["
				<< v.begin_stmt_id + 1 << ',' << v.end_stmt_id << ']' << std::endl;
		}
		for(int i = 0; i < vertex_cnt(); i++)
		{
			std::cout << i << ": ";
			for(int id : successor_ids(i))
				std::cout << id << ' ';
			std::cout << std::endl;
		}
		std::cout << "end cfg construction" << std::endl;
	);
}

} // namespace compiler::backend::tigger