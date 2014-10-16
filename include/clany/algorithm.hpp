/////////////////////////////////////////////////////////////////////////////////
// The MIT License(MIT)
// 
// Copyright (c) 2014 Tiangang Song
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////

#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <algorithm>
#include <numeric>
#include <functional>
#include "traits.hpp"

namespace std {
//////////////////////////////////////////////////////////////////////////////////////////
// Non-modifying sequence operations
template<typename Container, typename Func>
inline bool all_of(Container&& container, Func func)
{
    return all_of(begin(container), end(container), func);
}

template<typename Container, typename Func>
inline bool any_of(Container&& container, Func func)
{
    return any_of(begin(container), end(container), func);
}

template<typename Container, typename Func>
inline bool none_of(Container&& container, Func func)
{
    return none_of(begin(container), end(container), func);
}

template<typename Container, typename Func>
inline void for_each(Container&& container, Func func)
{
    for_each(begin(container), end(container), func);
}

template<typename Container, typename T>
inline auto count(Container&& container, const T& value) ->
typename iterator_traits<decltype(begin(container))>::difference_type
{
    return count(begin(container), end(container), value);
}

template<typename Container, typename UPred>
inline auto count_if(Container&& container, UPred p)->
typename iterator_traits<decltype(begin(container))>::difference_type
{
    return count_if(begin(container), end(container), p);
}

template<typename Container1, typename Container2>
inline auto mismatch(Container1&& container1, Container2&& container2) ->
pair<decltype(begin(container1)), decltype(begin(container2))>
{
    return mismatch(begin(container1), end(container1), begin(container2));
}

template<typename Container1, typename Container2, typename BPred>
inline auto mismatch(Container1&& container1, Container2&& container2, BPred p) ->
pair<decltype(begin(container1)), decltype(begin(container2))>
{
    return mismatch(begin(container1), end(container1), begin(container2), p);
}

template<typename Container1, typename Container2>
inline bool equal(Container1&& container1, Container2&& container2)
{
    return equal(begin(container1), end(container1),begin(container2));
}

template<typename Container1, typename Container2, typename BPred>
inline bool equal(Container1&& container1, Container2&& container2, BPred p)
{
    return equal(begin(container1), end(container1), begin(container2), p);
}

template<typename Container, typename T>
inline auto find(Container&& container, const T& value) -> decltype(begin(container))
{
    return find(begin(container), end(container), value);
}

template<typename Container, typename UPred>
inline auto find_if(Container&& container, UPred p) -> decltype(begin(container))
{
    return find_if(begin(container), end(container), p);
}

template<typename Container, typename UPred>
inline auto find_if_not(Container&& container, UPred p)->
decltype(begin(container))
{
    return find_if_not(begin(container), end(container), p);
}

template<typename Container1, typename Container2>
inline auto find_first_of(Container1&& container1, Container2&& container2) ->
decltype(begin(container1))
{
    return find_first_of(begin(container1), end(container1),
                         begin(container2), end(container2));
}

template<typename Container1, typename Container2, typename BPred>
inline auto find_first_of(Container1&& container1, Container2&& container2, BPred p) ->
decltype(begin(container1))
{
    return find_first_of(begin(container1), end(container1),
                         begin(container2), end(container2), p);
}

// Initializer list overload
template<typename T, typename Container>
inline auto find_first_of(Container&& container1, initializer_list<T>&& container2) ->
decltype(begin(container1))
{
    return find_first_of(begin(container1), end(container1),
                         begin(container2), end(container2));
}

template<typename T, typename Container, typename BPred>
inline auto find_first_of(Container&& container1, initializer_list<T>&& container2, BPred p) ->
decltype(begin(container1))
{
    return find_first_of(begin(container1), end(container1),
                         begin(container2), end(container2), p);
}

template<typename Container>
inline auto adjacent_find(Container&& container) -> decltype(begin(container))
{
    return adjacent_find(begin(container), end(container));
}

template<typename Container, typename BPred>
inline auto adjacent_find(Container&& container, BPred p) -> decltype(begin(container))
{
    return adjacent_find(begin(container), end(container), p);
}

template<typename Container1, typename Container2>
inline auto search(Container1&& container1, Container2&& container2) ->
decltype(begin(container1))
{
    return search(begin(container1), end(container1),
                  begin(container2), end(container2));
}

template<typename Container1, typename Container2, typename BPred>
inline auto search(Container1&& container1, Container2&& container2, BPred p) ->
decltype(begin(container1))
{
    return search(begin(container1), end(container1),
                  begin(container2), end(container2), p);
}

// Initializer list overload
template<typename T, typename Container>
inline auto search(Container&& container1, initializer_list<T>&& container2) ->
decltype(begin(container1))
{
    return search(begin(container1), end(container1),
                  begin(container2), end(container2));
}

template<typename T, typename Container, typename BPred>
inline auto search(Container&& container1, initializer_list<T>&& container2, BPred p) ->
decltype(begin(container1))
{
    return search(begin(container1), end(container1),
                  begin(container2), end(container2), p);
}

template<typename Container, typename Size, typename T>
inline auto search_n(Container&& container, Size count, const T& value) ->
decltype(begin(container))
{
    return search_n(begin(container), end(container), count, value);
}

template<typename Container, typename Size, typename T, typename BPred>
inline auto search_n(Container&& container, Size count, const T& value, BPred p) ->
decltype(begin(container))
{
    return search_n(begin(container), end(container), count, value, p);
}
//////////////////////////////////////////////////////////////////////////////////////////
// Modifying sequence operations
template<typename Container1, typename Container2>
inline auto copy(Container1&& container1, Container2& container2) ->
decltype(begin(container2))
{
    return copy(begin(container1), end(container1), begin(container2));
}

template<typename Container1, typename Container2, typename UPred>
inline auto copy_if(Container1&& container1, Container2& container2, UPred p) ->
decltype(begin(container2))
{
    return copy_if(begin(container1), end(container1), begin(container2), p);
}

// copy_n not availbe for container

template<typename Container1, typename Container2>
inline auto copy_backward(Container1&& container1, Container2& container2) ->
decltype(begin(container2))
{
    return copy_backward(begin(container1), end(container1), end(container2));
}

// Initializer list overload
template<typename T, typename Container>
inline auto copy_backward(initializer_list<T>&& container1, Container& container2) ->
decltype(begin(container2))
{
    return copy_backward(begin(container1), end(container1), end(container2));
}

template<typename Container1, typename Container2>
inline auto move(Container1&& container1, Container2& container2) ->
decltype(begin(container2))
{
    return move(begin(container1), end(container1), begin(container2));
}

template<typename Container1, typename Container2>
inline auto move_backward(Container1&& container1, Container2& container2) ->
decltype(begin(container2))
{
    return move_backward(begin(container1), end(container1), end(container2));
}

// Initializer list overload
template<typename T, typename Container>
inline auto move_backward(initializer_list<T>&& container1, Container& container2) ->
decltype(begin(container2))
{
    return move_backward(begin(container1), end(container1), end(container2));
}

template<typename Container, typename T>
inline void fill(Container& container, const T& value)
{
    fill(begin(container), end(container), value);
}

template<typename Container1, typename Container2, typename UPred>
inline auto transform(Container1&& container1, Container2& container2, UPred p) ->
decltype(begin(container2))
{
    return transform(begin(container1), end(container1), begin(container2), p);
}

template<typename Container1, typename Container2, typename Container3, typename UPred>
inline auto transform(Container1&& container1, Container2&& container2,
                      Container3& container3, UPred p) ->
decltype(begin(container2))
{
    return transform(begin(container1), end(container1), begin(container2),
                     begin(container3), p);
}

template<typename Container, typename Generator>
inline void generate(Container& container, Generator&& g)
{
    generate(begin(container), end(container), forward<Generator>(g));
}

template<typename Container>
inline void reverse(Container& container)
{
    reverse(begin(container), end(container));
}

template<typename Container1, typename Container2>
inline auto reverse_copy(Container1&& container1, Container2& container2) ->
decltype(begin(container2))
{
    return reverse_copy(begin(container1), end(container1), begin(container2));
}

template<typename Container, typename Pos>
inline auto rotate(Container& container, Pos pos) -> decltype(begin(container))
{
    auto mid = begin(container);
    advance(mid, pos);
    return rotate(begin(container), mid, end(container));
}

template<typename Container1, typename Container2, typename Pos>
inline auto rotate_copy(Container1&& container1, Pos pos, Container2& container2) ->
decltype(begin(container2))
{
    auto mid = begin(container1);
    advance(mid, pos);
    return rotate_copy(begin(container1), mid, end(container1), begin(container2));
}

template<typename Container>
inline auto unique(Container& container) -> decltype(begin(container))
{
    return unique(begin(container), end(container));
}

template<typename Container, typename BPred>
inline auto unique(Container& container, BPred p) -> decltype(begin(container))
{
    return unique(begin(container), end(container), p);
}

template<typename Container1, typename Container2>
inline auto unique_copy(Container1&& container1, Container2& container2) ->
decltype(begin(container2))
{
    return unique_copy(begin(container1), end(container1), begin(container2));
}

template<typename Container1, typename Container2, typename BPred>
inline auto unique_copy(Container1&& container1, Container2& container2, BPred p) ->
decltype(begin(container2))
{
    return unique_copy(begin(container1), end(container1), begin(container2), p);
}

template<typename Container, typename URNG >
inline void shuffle(Container& container, URNG&& g)
{
    shuffle(begin(container), end(container), forward<URNG>(g));
}

//////////////////////////////////////////////////////////////////////////////////////////
// Partitioning operations

//////////////////////////////////////////////////////////////////////////////////////////
// Sorting operations
template<typename Container>
inline bool is_sorted(Container& container)
{
    return is_sorted(begin(container), end(container));
}

template<typename Container, typename Comp>
inline bool is_sorted(Container& container, Comp comp)
{
    return is_sorted(begin(container), end(container), comp);
}

template<typename Container>
inline auto is_sorted_until(Container& container) -> decltype(begin(container))
{
    return is_sorted_until(begin(container), end(container));
}

template<typename Container, typename Comp>
inline auto is_sorted_until(Container& container, Comp comp) -> decltype(begin(container))
{
    return is_sorted_until(begin(container), end(container), comp);
}

template<typename Container>
inline void sort(Container& container)
{
    sort(begin(container), end(container));
}

template<typename Container, typename Comp>
inline void sort(Container& container, Comp comp)
{
    sort(begin(container), end(container), comp);
}

template<typename Container, typename Size>
inline void partial_sort(Container& container, Size size)
{
    auto mid = begin(container);
    advance(mid, size);
    partial_sort(begin(container), mid, end(container));
}

template<typename Container, typename Size, typename Comp>
inline void partial_sort(Container& container, Size size, Comp comp)
{
    auto mid = begin(container);
    advance(mid, size);
    partial_sort(begin(container), mid, end(container), comp);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Binary search operations (on sorted ranges)

//////////////////////////////////////////////////////////////////////////////////////////
// Set operations (on sorted ranges)

//////////////////////////////////////////////////////////////////////////////////////////
// Heap operations

//////////////////////////////////////////////////////////////////////////////////////////
// Minimum/maximum operations
template<typename Container>
inline auto max_element(Container& container) -> decltype(begin(container))
{
    return max_element(begin(container), end(container));
}

template<typename Container, typename Comp>
inline auto max_element(Container& container, Comp comp) -> decltype(begin(container))
{
    return max_element(begin(container), end(container), comp);
}

template<typename Container>
inline auto min_element(Container& container) -> decltype(begin(container))
{
    return min_element(begin(container), end(container));
}

template<typename Container, typename Comp>
inline auto min_element(Container& container, Comp comp) -> decltype(begin(container))
{
    return min_element(begin(container), end(container), comp);
}

template<typename Container>
inline auto minmax_element(Container& container) -> pair<decltype(begin(container)), 
                                                         decltype(begin(container))>
{
    return minmax_element(begin(container), end(container));
}

template<typename Container, typename Comp>
inline auto minmax_element(Container& container, Comp comp) ->
pair<decltype(begin(container)), decltype(begin(container))>
{
    return minmax_element(begin(container), end(container), comp);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Numeric operations
template<typename Container, typename T>
inline void iota(Container& container, T value)
{
    iota(begin(container), end(container), value);
}

template<typename Container,
         typename T = typename cls::container_value<Container>::type>
inline T accumulate(Container&& container)
{
    T init = T();
    return accumulate(begin(container), end(container), init);
}

template<typename Container, typename BOperator,
         typename T = typename cls::container_value<Container>::type>
inline T accumulate(Container&& container, BOperator op)
{
    T init = T();
    return accumulate(begin(container), end(container), init, op);
}

template<typename Container, typename T, typename BOperator>
inline T accumulate(Container&& container, T init, BOperator op)
{
    return accumulate(begin(container), end(container), init, op);
}

template<typename Container1, typename Container2,
         typename T = decltype(
         declval<typename cls::container_value<Container1>::type>() *
         declval<typename cls::container_value<Container2>::type>())>
inline T inner_product(Container1&& container1, Container2&& container2)
{
    T init = T();
    return inner_product(begin(container1), end(container1), begin(container2), init);
}

#ifdef _MSC_VER
template<typename Container1, typename Container2,
         typename BOperator1, typename BOperator2,
         typename T = decltype(
         declval<typename cls::container_value<Container1>::type>() *
         declval<typename cls::container_value<Container2>::type>())>
inline T inner_product(Container1&& container1, Container2&& container2, 
                       BOperator1 sum_op, BOperator2 mul_op)
{
    T init = T();
    return inner_product(begin(container1), end(container1), begin(container2), init,
                         sum_op, mul_op);
}
#endif // _MSC_VER

template<typename Container1, typename Container2,
         typename T, typename BOperator1, typename BOperator2>
inline T inner_product(Container1&& container1, Container2&& container2, 
                       T init, BOperator1 sum_op, BOperator2 mul_op)
{
    return inner_product(begin(container1), end(container1), begin(container2), init,
                         sum_op, mul_op);
}

} // End of namespace std

#endif // ALGORITHM_HPP