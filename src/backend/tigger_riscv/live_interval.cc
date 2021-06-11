#include <algorithm>
#include "live_interval.h"

namespace compiler::backend::tigger
{

void LiveInterval::add_range(int begin, int end)
{
	if(empty())
	{
		begin_stmt_id = begin;
		back_stmt_id = end;
	}
	else
	{
		begin_stmt_id = std::min(begin_stmt_id, begin);
		back_stmt_id = std::max(back_stmt_id, end);
	}
}

void LiveInterval::set_begin(int begin)
{
	if(!empty())
	{
		if(begin > back_stmt_id)
			reset();
		else
			begin_stmt_id = begin;
	}
}

}