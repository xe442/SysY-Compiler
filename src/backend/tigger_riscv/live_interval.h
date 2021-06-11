#ifndef LIVE_INTERVAL_H
#define LIVE_INTERVAL_H

#include "eeyore.h"
#include "tigger.h"

namespace compiler::backend::tigger
{

struct LiveInterval
{
	int begin_stmt_id;
	int back_stmt_id;
	bool cross_func_call; // If there is a func-call inside the interval.
			// When assigning registers to a non-func-call-crossing interval,
			// caller saved registers are preferred. Else callee saved registers
			// are preferred.
	eeyore::Operand opr;
	std::optional<Reg> pre_assigned_reg; // Function arguments have pre-assigned
			// registers (i.e. aX) for them. However, if there is a func call
			// inside their intervals, they have to be moved to other callee saved
			// registers instead.
	std::optional<Reg> reg;
	std::optional<int> stack_loc;

	LiveInterval(): begin_stmt_id(-1), back_stmt_id(-1) {}
	LiveInterval(const eeyore::Operand &_opr)
	  : begin_stmt_id(-1), back_stmt_id(-1), opr(_opr) {}
	LiveInterval(const eeyore::Operand &_opr, int _begin_stmt_id, int _back_stmt_id)
	  : begin_stmt_id(_begin_stmt_id), back_stmt_id(_back_stmt_id), opr(_opr) {}

	inline void reset() { begin_stmt_id = back_stmt_id = -1; }
	inline bool empty() const { return begin_stmt_id == -1; }
	void add_range(int begin, int end);
	void set_begin(int begin);

	bool operator == (const LiveInterval &other) const
		{ return opr == other.opr; }
	
	// When sorted, the start points are in increasing order.
	struct StartPointLessCmp
	{
		bool operator() (const LiveInterval &interv1, const LiveInterval &interv2) const
		{
			return interv1.begin_stmt_id < interv2.begin_stmt_id;
		}
	};

	// When sorted, the end points are in increasing order.
	struct EndPointLessCmp
	{
		bool operator() (const LiveInterval &interv1, const LiveInterval &interv2) const
		{
			return interv1.back_stmt_id < interv2.back_stmt_id;
		}
	};
};

} // namespace compiler::backend::tigger

namespace std
{

template<>
struct hash<class compiler::backend::tigger::LiveInterval>
{
	hash<compiler::backend::eeyore::Operand> opr_hash;

	size_t operator() (const compiler::backend::tigger::LiveInterval &interv) const
	{
		return opr_hash(interv.opr);
	}
};

}

#endif