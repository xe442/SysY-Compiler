#ifndef DBG_PRINTF_H
#define DBG_PRINTF_H

// If DEBUG flag is on, DBG(X) -> X. Else DBG(X) -> empty.
// Example usage: DBG(cout << "Hello world!" << endl);
#ifdef DEBUG
#define DBG(...) __VA_ARGS__
#else
#define DBG(...)
#endif

#endif