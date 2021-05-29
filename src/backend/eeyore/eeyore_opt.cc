#include "dbg.h"
#include <unordered_set>
#include "visitor_helper.h"
#include "eeyore_gen.h"

using std::holds_alternative;

namespace compiler::backend::eeyore
{

void EeyoreGenerator::EeyoreOptimizer::optimize(std::list<EeyoreStatement> &eeyore_code)
{
	_remove_useless_labels_and_jumps(eeyore_code);
}

void EeyoreGenerator::EeyoreOptimizer::_remove_useless_labels_and_jumps(
	std::list<EeyoreStatement> &eeyore_code)
{
	// 1. Find all the labels defined and all the jump statements.
	// 2. Remove all the double jumps.
	// 3. Mark a jump statement as useless if it jumps to the next statement,
	//    or it directly follows another jump statement.
	// 4. Remove all the useless jump statements.
	// 5. Mark a label as useless if it directly follows another label,
	//    or it is not used by any valid jump statements.
	// 6. Remove all the useless labels and re-assign the ids of the
	//    labels so that they are consequtive.

	using StmtPtr = std::list<EeyoreStatement>::iterator;

	DBG(
		std::cout << "initial code: " << std::endl;
		for(const auto &stmt : eeyore_code)
			std::cout << stmt;
		std::cout << "end initial code"  << std::endl << std::endl;
	);

	std::vector<StmtPtr> jump_stmts;
	std::vector<StmtPtr> label_stmts;
	std::unordered_map<int, StmtPtr> id_to_label_decl;

	std::vector<StmtPtr> valid_jump_stmts;
	std::vector<StmtPtr> valid_label_stmts;
	std::unordered_set<int> valid_label_ids;
	std::unordered_map<int, int> label_id_map; // old id -> new id

	static utils::LambdaVisitor label_getter = {
		[](GotoStmt &stmt) -> Label & { return stmt.goto_label; },
		[](CondGotoStmt &stmt) -> Label & { return stmt.goto_label; },
		[](const auto &stmt) -> Label & { INTERNAL_ERROR("invalid call to label_getter"); }
	};
	static utils::LambdaVisitor label_id_getter = {
		[](const GotoStmt &stmt) { return stmt.goto_label.id; },
		[](const CondGotoStmt &stmt) { return stmt.goto_label.id; },
		[](const auto &stmt) -> int { INTERNAL_ERROR("invalid call to label_id_getter"); }
	};

	// Step 1.
	for(auto iter = eeyore_code.begin(); iter != eeyore_code.end(); ++iter)
	{
		static utils::LambdaVisitor visitor = {
			[&](const GotoStmt &stmt) { jump_stmts.push_back(iter); },
			[&](const CondGotoStmt &stmt) { jump_stmts.push_back(iter); },
			[&](const LabelStmt &stmt)
			{
				label_stmts.push_back(iter);
				id_to_label_decl[stmt.label.id] = iter;
			},
			[&](const auto &stmt) {}
		};
		std::visit(visitor, *iter);
	}
	DBG(std::cout << "step 1 completed" << std::endl);

	// Step 2.
	for(StmtPtr jump_stmt_ptr : jump_stmts)
	{
		const static int MAX_JUMP_LIMIT = 10000;
		
		int last_label_id = std::visit(label_id_getter, *jump_stmt_ptr);
		StmtPtr stmt_goto = id_to_label_decl[last_label_id];
		for(int jump_times = 1; ; jump_times++)
		{
			while(stmt_goto != eeyore_code.end()
				&& holds_alternative<LabelStmt>(*stmt_goto))
			{
				++stmt_goto;
			}
			if(!holds_alternative<GotoStmt>(*stmt_goto))
			{
				// We reach the end. Check if we need to modify the jump statement.
				if(jump_times > 1)
				{
					DBG(std::cout << "double jumps detected!" << std::endl);
					Label &label = std::visit(label_getter, *jump_stmt_ptr);
					label = Label(last_label_id);
				}
				break;
			}
			last_label_id = std::get<GotoStmt>(*stmt_goto).goto_label.id;
			stmt_goto = id_to_label_decl[last_label_id];

			// In case we are in an infinite loop, stops when iteration
			// time reaches a limit.
			INTERNAL_ASSERT(jump_times < MAX_JUMP_LIMIT, "loop goto detected");
		}
	}
	DBG(std::cout << "step 2 completed" << std::endl);

	// step 3 & 4.
	for(StmtPtr jump_stmt_ptr : jump_stmts)
	{
		int label_id = std::visit(label_id_getter, *jump_stmt_ptr);
		bool useless = false;

		// Check whether it directly follows a GotoStmt.
		StmtPtr prev_stmt_ptr = jump_stmt_ptr;
		for(--prev_stmt_ptr; prev_stmt_ptr != eeyore_code.begin(); --prev_stmt_ptr)
			// The first eeyore statement could not be a GotoStmt, so just
			// check until the second statement.
		{
			if(holds_alternative<GotoStmt>(*prev_stmt_ptr))
			{
				useless = true;
				break;
			}
			else if(!holds_alternative<LabelStmt>(*prev_stmt_ptr))
				break;
		}
		if(useless)
		{
			eeyore_code.erase(jump_stmt_ptr);
			continue;
		}

		// Check if it jumps to the next statement.
		StmtPtr next_stmt_ptr = jump_stmt_ptr;
		for(++next_stmt_ptr; next_stmt_ptr != eeyore_code.end(); ++next_stmt_ptr)
		{
			if(holds_alternative<LabelStmt>(*next_stmt_ptr)
				&& std::get<LabelStmt>(*next_stmt_ptr).label.id == label_id)
			{
				useless = true;
				break;
			}
			else break;
		}

		if(useless)
			eeyore_code.erase(jump_stmt_ptr);
		else
		{
			valid_jump_stmts.push_back(jump_stmt_ptr);
			valid_label_ids.insert(label_id);
		}
	}
	DBG(std::cout << "step 3 and 4 completed" << std::endl);

	// Step 5 & 6.
	int new_label_id = 0;
	for(StmtPtr label_stmt_ptr : label_stmts)
	{
		int label_id = std::get<LabelStmt>(*label_stmt_ptr).label.id;
		if(valid_label_ids.find(label_id) != valid_label_ids.end())
		{
			StmtPtr prev_stmt_ptr = label_stmt_ptr;
			--prev_stmt_ptr;
			if(!holds_alternative<LabelStmt>(*prev_stmt_ptr))
			{
				valid_label_stmts.push_back(label_stmt_ptr);
				label_id_map[label_id] = new_label_id++;
			}
			else
			{
				int prev_label_id = std::get<LabelStmt>(*prev_stmt_ptr).label.id;
				label_id_map[label_id] = label_id_map[prev_label_id];
				eeyore_code.erase(label_stmt_ptr);
			}
		}
		else
			eeyore_code.erase(label_stmt_ptr);
	}
	for(StmtPtr jump_stmt_ptr : valid_jump_stmts)
	{
		Label &label = std::visit(label_getter, *jump_stmt_ptr);
		label = Label(label_id_map[label.id]);
	}
	for(StmtPtr label_stmt_ptr : valid_label_stmts)
	{
		LabelStmt &label_stmt = std::get<LabelStmt>(*label_stmt_ptr);
		label_stmt.label = Label(label_id_map[label_stmt.label.id]);
	}
	DBG(std::cout << "step 5 and 6 completed" << std::endl);
}

} // namespace compiler::backend::eeyore