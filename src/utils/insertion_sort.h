#ifndef INSERTION_SORT_H
#define INSERTION_SORT_H

#include <functional>

namespace compiler::utils
{

// Insert ele to the container, so that container is kept sorted by cmp.
template<class Container, class T, class Cmp>
void insertion_sort(Container &container, T ele, Cmp cmp)
{
	auto iter = container.begin();
	for( ; iter != container.end(); ++iter)
		if(!cmp(*iter, ele))
			break;
	container.insert(iter, ele);
}

template<class Container, class T>
void insertion_sort(Container &container, T ele)
{
	insertion_sort(container, ele, std::less<T>());
}

} // namespace compiler::utils

#endif