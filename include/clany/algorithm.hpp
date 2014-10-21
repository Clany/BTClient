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

#ifndef CLS_ALGORITHM_HPP
#define CLS_ALGORITHM_HPP

#include <algorithm>
#include <numeric>
#include <functional>
#include "traits.hpp"

namespace cls {
//////////////////////////////////////////////////////////////////////////////////////////
// Bring all overloaded std funtions to current namespace
using std::all_of;
using std::any_of;
using std::none_of;
using std::for_each;
using std::count;
using std::count_if;
using std::mismatch;
using std::equal;
using std::find;
using std::find_if;
using std::find_if_not;
using std::find_end;
using std::find_first_of;
using std::adjacent_find;
using std::search;
using std::search_n;
using std::copy;
using std::copy_if;
using std::copy_backward;
using std::move;
using std::move_backward;
using std::fill;
using std::transform;
using std::generate;
using std::remove;
using std::remove_if;
using std::remove_copy;
using std::remove_copy_if;
using std::replace;
using std::replace_if;
using std::replace_copy;
using std::replace_copy_if;
using std::reverse;
using std::reverse_copy;
using std::rotate;
using std::rotate_copy;
using std::shuffle;
using std::unique;
using std::unique_copy;
using std::is_sorted;
using std::is_sorted_until;
using std::sort;
using std::partial_sort;
using std::max_element;
using std::min_element;
using std::minmax_element;
using std::iota;
using std::accumulate;
using std::inner_product;

//////////////////////////////////////////////////////////////////////////////////////////
// Helper function return the size of a container
template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto container_size(Container&& container) -> decltype(container.size())
{
    return container.size();
}

template<typename T, size_t N>
inline size_t container_size(T(&)[N])
{
    return N;
}

// Non-modifying sequence operations
template<typename Container, typename Func,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool all_of(Container&& container, Func func)
{
    return all_of(begin(container), end(container), func);
}

template<typename Container, typename Func,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool any_of(Container&& container, Func func)
{
    return any_of(begin(container), end(container), func);
}

template<typename Container, typename Func,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool none_of(Container&& container, Func func)
{
    return none_of(begin(container), end(container), func);
}

template<typename Container, typename Func,
         typename U = enable_if_t<is_container<Container>::value>>
inline void for_each(Container&& container, Func func)
{
    for_each(begin(container), end(container), func);
}

// Initializer list overload
template<typename T, typename Func>
inline void for_each(initializer_list<T>&& container, Func func)
{
    for_each(begin(container), end(container), func);
}

template<typename Container, typename T,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto count(Container&& container, const T& value) ->
iterator_difference_t<decltype(begin(container))>
{
    return count(begin(container), end(container), value);
}

template<typename Container, typename UPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto count_if(Container&& container, UPred p) ->
iterator_difference_t<decltype(begin(container))>
{
    return count_if(begin(container), end(container), p);
}

template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline auto mismatch(Container1& container1, Container2& container2) ->
pair<decltype(begin(container1)), decltype(begin(container2))>
{
    return mismatch(begin(container1), end(container1), begin(container2));
}

template<typename Container1, typename Container2, typename BPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline auto mismatch(Container1& container1, Container2& container2, BPred p) ->
pair<decltype(begin(container1)), decltype(begin(container2))>
{
    return mismatch(begin(container1), end(container1), begin(container2), p);
}

template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline bool equal(Container1&& container1, Container2&& container2)
{
    return equal(begin(container1), end(container1),begin(container2));
}

template<typename Container1, typename Container2, typename BPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline bool equal(Container1&& container1, Container2&& container2, BPred p)
{
    return equal(begin(container1), end(container1), begin(container2), p);
}

// Initializer list overload
template<typename T, typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool equal(Container&& container1, initializer_list<T>&& container2)
{
    return equal(begin(container1), end(container1),begin(container2));
}

template<typename T, typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool equal(initializer_list<T>&& container1, Container&& container2)
{
    return equal(begin(container1), end(container1),begin(container2));
}

template<typename T, typename Container, typename BPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool equal(Container&& container1, initializer_list<T>&& container2, BPred p)
{
    return equal(begin(container1), end(container1), begin(container2), p);
}

template<typename T, typename Container, typename BPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool equal(initializer_list<T>&& container1, Container&& container2, BPred p)
{
    return equal(begin(container1), end(container1), begin(container2), p);
}

template<typename Container, typename T,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto find(Container& container, const T& value) -> decltype(begin(container))
{
    return find(begin(container), end(container), value);
}

template<typename Container, typename UPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto find_if(Container& container, UPred p) -> decltype(begin(container))
{
    return find_if(begin(container), end(container), p);
}

template<typename Container, typename UPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto find_if_not(Container& container, UPred p) ->
decltype(begin(container))
{
    return find_if_not(begin(container), end(container), p);
}

template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline auto find_end(Container1& container1, Container2&& container2) ->
decltype(begin(container1))
{
    return find_end(begin(container1), end(container1),
                    begin(container2), end(container2));
}

template<typename Container1, typename Container2, typename BPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline auto find_end(Container1& container1, Container2&& container2, BPred p) ->
decltype(begin(container1))
{
    return find_end(begin(container1), end(container1),
                    begin(container2), end(container2), p);
}

// Initializer list overload
template<typename T, typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto find_end(Container& container1, initializer_list<T>&& container2) ->
decltype(begin(container1))
{
    return find_end(begin(container1), end(container1),
                    begin(container2), end(container2));
}

template<typename T, typename Container, typename BPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto find_end(Container& container1, initializer_list<T>&& container2, BPred p) ->
decltype(begin(container1))
{
    return find_end(begin(container1), end(container1),
                    begin(container2), end(container2), p);
}

template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline auto find_first_of(Container1& container1, Container2&& container2) ->
decltype(begin(container1))
{
    return find_first_of(begin(container1), end(container1),
                         begin(container2), end(container2));
}

template<typename Container1, typename Container2, typename BPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline auto find_first_of(Container1& container1, Container2&& container2, BPred p) ->
decltype(begin(container1))
{
    return find_first_of(begin(container1), end(container1),
                         begin(container2), end(container2), p);
}

// Initializer list overload
template<typename T, typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto find_first_of(Container& container1, initializer_list<T>&& container2) ->
decltype(begin(container1))
{
    return find_first_of(begin(container1), end(container1),
                         begin(container2), end(container2));
}

template<typename T, typename Container, typename BPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto find_first_of(Container& container1, initializer_list<T>&& container2, BPred p) ->
decltype(begin(container1))
{
    return find_first_of(begin(container1), end(container1),
                         begin(container2), end(container2), p);
}

template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto adjacent_find(Container& container) -> decltype(begin(container))
{
    return adjacent_find(begin(container), end(container));
}

template<typename Container, typename BPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto adjacent_find(Container& container, BPred p) -> decltype(begin(container))
{
    return adjacent_find(begin(container), end(container), p);
}

template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline auto search(Container1& container1, Container2&& container2) ->
decltype(begin(container1))
{
    return search(begin(container1), end(container1),
                  begin(container2), end(container2));
}

template<typename Container1, typename Container2, typename BPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline auto search(Container1& container1, Container2&& container2, BPred p) ->
decltype(begin(container1))
{
    return search(begin(container1), end(container1),
                  begin(container2), end(container2), p);
}

// Initializer list overload
template<typename T, typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto search(Container& container1, initializer_list<T>&& container2) ->
decltype(begin(container1))
{
    return search(begin(container1), end(container1),
                  begin(container2), end(container2));
}

template<typename T, typename Container, typename BPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto search(Container& container1, initializer_list<T>&& container2, BPred p) ->
decltype(begin(container1))
{
    return search(begin(container1), end(container1),
                  begin(container2), end(container2), p);
}

template<typename Container, typename Size, typename T,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto search_n(Container& container, Size count, const T& value) ->
decltype(begin(container))
{
    return search_n(begin(container), end(container), count, value);
}

template<typename Container, typename Size, typename T, typename BPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto search_n(Container& container, Size count, const T& value, BPred p) ->
decltype(begin(container))
{
    return search_n(begin(container), end(container), count, value, p);
}
//////////////////////////////////////////////////////////////////////////////////////////
// Modifying sequence operations
// Container to container, automatically resize
template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void copy(Container1&& container1, Container2& container2)
{
    container2.resize(container_size(container1));
    copy(begin(container1), end(container1), begin(container2));
}

// Container to output iterator
template<typename Container, typename OutputIt,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto copy(Container&& container, OutputIt d_first) -> OutputIt
{
    return copy(begin(container), end(container), d_first);
}

// Initializer list to output iterator
template<typename T, typename OutputIt,
         typename U = enable_if_t<is_output_iterator<OutputIt>::value>>
inline auto copy(initializer_list<T>&& container, OutputIt d_first) -> OutputIt
{
    return copy(begin(container), end(container), d_first);
}

// Container to container, automatically resize
template<typename Container1, typename Container2, typename UPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void copy_if(Container1&& container1, Container2& container2, UPred p)
{
    container2.resize(container_size(container1));
    auto iter = copy_if(begin(container1), end(container1),
                        begin(container2), p);
    container2.resize(distance(begin(container2), iter));
}

// Container to output iterator
template<typename Container, typename OutputIt, typename UPred,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto copy_if(Container&& container, OutputIt d_first, UPred p) -> OutputIt
{
    return copy_if(begin(container), end(container), d_first, p);
}

// Container to output iterator
template<typename Container, typename OutputIt,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto copy_backward(Container&& container, OutputIt d_first) -> OutputIt
{
    return copy_backward(begin(container), end(container), d_first);
}

// Initializer list to output iterator
template<typename T, typename OutputIt,
         typename U = enable_if_t<is_output_iterator<OutputIt>::value>>
inline auto copy_backward(initializer_list<T>&& container, OutputIt d_first) -> OutputIt
{
    return copy_backward(begin(container), end(container), d_first);
}

// Container to container, automatically resize
template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void move(Container1&& container1, Container2& container2)
{
    container2.resize(container_size(container1));
    move(begin(container1), end(container1), begin(container2));
}

// Container to output iterator
template<typename Container, typename OutputIt,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto move(Container&& container, OutputIt d_first) -> OutputIt
{
    return move(begin(container), end(container), d_first);
}

// Container to output iterator
template<typename Container, typename OutputIt,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto move_backward(Container&& container, OutputIt d_first) -> OutputIt
{
    return move_backward(begin(container), end(container), d_first);
}

template<typename Container, typename T,
         typename U = enable_if_t<is_container<Container>::value>>
inline void fill(Container& container, const T& value)
{
    fill(begin(container), end(container), value);
}

// Container to container, automatically resize
template<typename Container1, typename Container2, typename UPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void transform(Container1&& container1, Container2& container2, UPred p)
{
    container2.resize(container_size(container1));
    transform(begin(container1), end(container1), begin(container2), p);
}

// Container to output iterator
template<typename Container, typename OutputIt, typename UPred,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto transform(Container&& container, OutputIt d_first, UPred p) -> OutputIt
{
    return transform(begin(container), end(container), d_first, p);
}

// Two containers to output iterator
template<typename Container1, typename Container2, typename OutputIt, typename UPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto transform(Container1&& container1, Container2&& container2,
                      OutputIt d_first, UPred p) -> OutputIt
{
    return transform(begin(container1), end(container1), begin(container2),
                     d_first, p);
}

// Initializer list to container, automatically resize
template<typename T, typename Container, typename UPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline void transform(initializer_list<T>&& container1, Container& container2, UPred p)
{
    container2.resize(container_size(container1));
    transform(begin(container1), end(container1), begin(container2), p);
}

// Initializer list to output iterator
template<typename T, typename OutputIt, typename UPred,
         typename U = enable_if_t<is_output_iterator<OutputIt>::value>>
inline auto transform(initializer_list<T>&& container, OutputIt d_first,
                      UPred p) -> OutputIt
{
    return transform(begin(container), end(container), d_first, p);
}

template<typename Container, typename Generator,
         typename U = enable_if_t<is_container<Container>::value>>
inline void generate(Container& container, Generator&& g)
{
    generate(begin(container), end(container), forward<Generator>(g));
}

// Automatically resize after removal
template<typename Container, typename T,
         typename U = enable_if_t<is_container<Container>::value>>
inline void remove(Container& container, const T& value)
{
    auto iter = remove(begin(container), end(container), value);
    container.resize(distance(begin(container), iter));
}

// Automatically resize after removal
template<typename Container, typename UPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline void remove_if(Container& container, UPred p)
{
    auto iter = remove_if(begin(container), end(container), p);
    container.resize(distance(begin(container), iter));
}

// Container to container, automatically resize
template<typename Container1, typename Container2, typename T,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void remove_copy(Container1&& container1, Container2& container2, const T& value)
{
    container2.resize(container_size(container1));
    auto iter = remove_copy(begin(container1), end(container1),
                            begin(container2), value);
    container2.resize(distance(begin(container2), iter));
}

template<typename Container1, typename Container2, typename UPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void remove_copy_if(Container1&& container1, Container2& container2, UPred p)
{
    container2.resize(container_size(container1));
    auto iter = remove_copy_if(begin(container1), end(container1),
                               begin(container2), p);
    container2.resize(distance(begin(container2), iter));
}

template<typename Container, typename T,
         typename U = enable_if_t<is_container<Container>::value>>
inline void replace(Container& container, const T& old_value, const T& new_value)
{
    replace(begin(container), end(container), old_value, new_value);
}

template<typename Container, typename UPred, typename T,
         typename U = enable_if_t<is_container<Container>::value>>
inline void replace_if(Container& container, UPred p, const T& new_value)
{
    replace_if(begin(container), end(container), p, new_value);
}

// Container to container, automatically resize
template<typename Container1, typename Container2, typename T,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void replace_copy(Container1&& container1, Container2& container2,
                         const T& old_value, const T& new_value)
{
    container2.resize(container_size(container1));
    auto iter = replace_copy(begin(container1), end(container1),
                             begin(container2), old_value, new_value);
    container2.resize(distance(begin(container2), iter));
}

// Container to output iterator
template<typename Container, typename OutputIt, typename T,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto replace_copy(Container&& container, OutputIt d_first,
                         const T& old_value, const T& new_value) -> OutputIt
{
    return replace_copy(begin(container), end(container),
                        d_first, old_value, new_value);
}

// Container to container, automatically resize
template<typename Container1, typename Container2, typename UPred, typename T,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void replace_copy_if(Container1&& container1, Container2& container2,
                            UPred p, const T& new_value)
{
    container2.resize(container_size(container1));
    auto iter = replace_copy_if(begin(container1), end(container1),
                                begin(container2), p, new_value);
    container2.resize(distance(begin(container2), iter));
}

// Container to output iterator
template<typename Container, typename OutputIt, typename UPred, typename T,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto replace_copy_if(Container&& container, OutputIt d_first,
                            UPred p, const T& new_value) -> OutputIt
{
    return replace_copy_if(begin(container), end(container),
                           d_first, p, new_value);
}

template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline void reverse(Container& container)
{
    reverse(begin(container), end(container));
}

// Container to container, automatically resize
template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void reverse_copy(Container1&& container1, Container2& container2)
{
    container2.resize(container_size(container1));
    reverse_copy(begin(container1), end(container1), begin(container2));
}

// Container to output iterator
template<typename Container, typename OutputIt,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto reverse_copy(Container&& container, OutputIt d_first) -> OutputIt
{
    return reverse_copy(begin(container), end(container), d_first);
}

template<typename Container, typename Pos,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto rotate(Container& container, Pos pos) -> decltype(begin(container))
{
    auto mid = begin(container);
    advance(mid, pos);
    return rotate(begin(container), mid, end(container));
}

// Container to container, automatically resize
template<typename Container1, typename Container2, typename Pos,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void rotate_copy(Container1&& container1, Pos pos, Container2& container2)
{
    container2.resize(container_size(container1));
    auto mid = begin(container1);
    advance(mid, pos);
    rotate_copy(begin(container1), mid, end(container1), begin(container2));
}

// Container to output iterator
template<typename Container, typename OutputIt, typename Pos,
         typename U = enable_if_t<is_container<Container>::value &&
                                  is_output_iterator<OutputIt>::value>>
inline auto rotate_copy(Container&& container, Pos pos, OutputIt d_first) -> OutputIt
{
    auto mid = begin(container);
    advance(mid, pos);
    return rotate_copy(begin(container), mid, end(container), d_first);
}

template<typename Container, typename URNG,
         typename U = enable_if_t<is_container<Container>::value>>
inline void shuffle(Container& container, URNG&& g)
{
    shuffle(begin(container), end(container), forward<URNG>(g));
}

// Automatically resize
template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline void unique(Container& container)
{
    auto iter = unique(begin(container), end(container));
    container.resize(distance(begin(container), iter));
}

// Automatically resize
template<typename Container, typename BPred,
         typename U = enable_if_t<is_container<Container>::value>>
inline void unique(Container& container, BPred p)
{
    auto iter = unique(begin(container), end(container), p);
    container.resize(distance(begin(container), iter));
}

// Container to container, automatically resize
template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void unique_copy(Container1&& container1, Container2& container2)
{
    container2.resize(container_size(container1));
    auto iter = unique_copy(begin(container1), end(container1),
                            begin(container2));
    container2.resize(distance(begin(container2), iter));
}

// Container to container, automatically resize
template<typename Container1, typename Container2, typename BPred,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>>
inline void unique_copy(Container1&& container1, Container2& container2, BPred p)
{
    container2.resize(container_size(container1));
    auto iter = unique_copy(begin(container1), end(container1),
                            begin(container2), p);
    container2.resize(distance(begin(container2), iter));
}
//////////////////////////////////////////////////////////////////////////////////////////
// Partitioning operations

//////////////////////////////////////////////////////////////////////////////////////////
// Sorting operations
template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool is_sorted(Container& container)
{
    return is_sorted(begin(container), end(container));
}

template<typename Container, typename Comp,
         typename U = enable_if_t<is_container<Container>::value>>
inline bool is_sorted(Container& container, Comp comp)
{
    return is_sorted(begin(container), end(container), comp);
}

template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto is_sorted_until(Container& container) -> decltype(begin(container))
{
    return is_sorted_until(begin(container), end(container));
}

template<typename Container, typename Comp,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto is_sorted_until(Container& container, Comp comp) -> decltype(begin(container))
{
    return is_sorted_until(begin(container), end(container), comp);
}

template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline void sort(Container& container)
{
    sort(begin(container), end(container));
}

template<typename Container, typename Comp,
         typename U = enable_if_t<is_container<Container>::value>>
inline void sort(Container& container, Comp comp)
{
    sort(begin(container), end(container), comp);
}

template<typename Container, typename Size,
         typename U = enable_if_t<is_container<Container>::value>>
inline void partial_sort(Container& container, Size size)
{
    auto mid = begin(container);
    advance(mid, size);
    partial_sort(begin(container), mid, end(container));
}

template<typename Container, typename Size, typename Comp,
         typename U = enable_if_t<is_container<Container>::value>>
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
template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto max_element(Container& container) -> decltype(begin(container))
{
    return max_element(begin(container), end(container));
}

template<typename Container, typename Comp,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto max_element(Container& container, Comp comp) -> decltype(begin(container))
{
    return max_element(begin(container), end(container), comp);
}

template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto min_element(Container& container) -> decltype(begin(container))
{
    return min_element(begin(container), end(container));
}

template<typename Container, typename Comp,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto min_element(Container& container, Comp comp) -> decltype(begin(container))
{
    return min_element(begin(container), end(container), comp);
}

template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto minmax_element(Container& container) -> pair<decltype(begin(container)),
                                                         decltype(begin(container))>
{
    return minmax_element(begin(container), end(container));
}

template<typename Container, typename Comp,
         typename U = enable_if_t<is_container<Container>::value>>
inline auto minmax_element(Container& container, Comp comp) ->
pair<decltype(begin(container)), decltype(begin(container))>
{
    return minmax_element(begin(container), end(container), comp);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Numeric operations
template<typename Container, typename T,
         typename U = enable_if_t<is_container<Container>::value>>
inline void iota(Container& container, T value)
{
    iota(begin(container), end(container), value);
}

template<typename Container,
         typename U = enable_if_t<is_container<Container>::value>,
         typename T = container_value_t<Container>>
inline T accumulate(Container&& container)
{
    T init = T();
    return accumulate(begin(container), end(container), init);
}

template<typename Container, typename BOperator,
         typename U = enable_if_t<is_container<Container>::value>,
         typename T = container_value_t<Container>>
inline T accumulate(Container&& container, BOperator op)
{
    T init = T();
    return accumulate(begin(container), end(container), init, op);
}

template<typename Container, typename T, typename BOperator,
         typename U = enable_if_t<is_container<Container>::value>>
inline T accumulate(Container&& container, T init, BOperator op)
{
    return accumulate(begin(container), end(container), init, op);
}

template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>,
         typename T = decltype(
         declval<container_value_t<Container1>>() *
         declval<container_value_t<Container2>>())>
inline T inner_product(Container1&& container1, Container2&& container2)
{
    T init = T();
    return inner_product(begin(container1), end(container1),
                         begin(container2), init);
}

template<typename Container1, typename Container2,
         typename BOperator1, typename BOperator2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>,
         typename T = decltype(declval<container_value_t<Container1>>() *
                               declval<container_value_t<Container2>>())>
inline T inner_product(Container1&& container1, Container2&& container2,
                       BOperator1 sum_op, BOperator2 mul_op)
{
    T init = T();
    return inner_product(begin(container1), end(container1),
                         begin(container2), init, sum_op, mul_op);
}

template<typename Container1, typename Container2,
         typename U = enable_if_t<is_container<Container1>::value &&
                                  is_container<Container2>::value>,
         typename T, typename BOperator1, typename BOperator2>
inline T inner_product(Container1&& container1, Container2&& container2,
                       T init, BOperator1 sum_op, BOperator2 mul_op)
{
    return inner_product(begin(container1), end(container1),
                         begin(container2), init, sum_op, mul_op);
}
} // End of namespace std

#endif // CLS_ALGORITHM_HPP