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

#ifndef CLS_TRAITS_HPP
#define CLS_TRAITS_HPP

#include <iterator>
#include <type_traits>
#include "clany_defs.h"

_CLANY_BEGIN
//////////////////////////////////////////////////////////////////////////////////////////
// Common type traits
template<bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

template<typename T, typename = void>
struct is_const_ref : false_type
{};

template<typename T>
struct is_const_ref<T, enable_if_t<is_reference<T>::value &&
    is_const<typename remove_reference<T>::type>::value>> : true_type
{};

template<typename T>
struct remove_const_ref {
    using type = typename remove_const<typename remove_reference<T>::type>::type;
};
//////////////////////////////////////////////////////////////////////////////////////////
// Container traits
template<typename T, typename = void>
struct is_container : false_type
{};

template<typename T>
struct is_container<T, enable_if_t<!is_same<
    typename remove_reference<T>::type::iterator,
    void>::value>> : true_type
{};

template<typename T, size_t N>
struct is_container<T(&)[N], void> : true_type
{};

template<typename T, size_t N>
struct is_container<T[N], void> : true_type
{};

template <typename Container, bool = is_container<Container>::value>
struct ContainerTraits
{};

template <typename Container>
struct ContainerTraits<Container, true> {
    using iterator   = typename remove_reference<Container>::type::iterator;
    using value_type = typename iterator_traits<iterator>::value_type;
    using reference  = typename iterator_traits<iterator>::reference;
    using pointer    = typename iterator_traits<iterator>::pointer;
};

template <typename T, size_t N>
struct ContainerTraits<T[N], true> {
    using iterator   = T*;
    using value_type = T;
    using reference  = T&;
    using pointer    = T*;
};

template <typename T, size_t N>
struct ContainerTraits<T(&)[N], true> {
    using iterator   = T*;
    using value_type = T;
    using reference  = T&;
    using pointer    = T*;
};

template <typename Container>
using container_value_t     = typename ContainerTraits<Container>::value_type;

template <typename Container>
using container_reference_t = typename ContainerTraits<Container>::reference;

template <typename Container>
using container_pointer_t   = typename ContainerTraits<Container>::pointer;

template <typename Container>
using container_iterator_t  = typename ContainerTraits<Container>::pointer;
//////////////////////////////////////////////////////////////////////////////////////////
// Iterator traits
template<typename T, typename = void>
struct is_iterator : false_type
{};

template<typename T>
struct is_iterator<T, enable_if_t<!is_same<
    typename remove_reference<T>::type::iterator_category,
    void>::value>> : true_type
{};

template<typename T>
struct is_iterator<T*, void> : true_type
{};

template <typename Iterator, bool = is_iterator<Iterator>::value>
struct IteratorTraits
{};

template <typename Iterator>
struct IteratorTraits<Iterator, true> {
private:
    using traits = iterator_traits<typename remove_reference<Iterator>::type>;

public:
    using value_type        = typename traits::value_type;
    using pointer           = typename traits::pointer;
    using reference         = typename traits::reference;
    using iterator_category = typename traits::iterator_category;
    using difference_type   = typename traits::difference_type;
};

template <typename Iterator>
using iterator_value_t      = typename IteratorTraits<Iterator>::value_type;

template <typename Iterator>
using iterator_reference_t  = typename IteratorTraits<Iterator>::reference;

template <typename Iterator>
using iterator_pointer_t    = typename IteratorTraits<Iterator>::pointer;

template <typename Iterator>
using iterator_difference_t = typename IteratorTraits<Iterator>::difference_type;

template <typename Iterator>
using iterator_category_t   = typename IteratorTraits<Iterator>::iterator_category;

template<typename T, bool = is_iterator<T>::value>
struct is_const_iterator : false_type
{};

template<typename T>
struct is_const_iterator<T, true>
    : integral_constant<bool, is_const_ref<iterator_reference_t<T>>::value>
{};

template<typename T, bool = is_iterator<T>::value>
struct is_input_iterator : false_type
{};

template<typename T>
struct is_input_iterator<T, true>
    : integral_constant<bool, is_base_of<input_iterator_tag,
      iterator_category_t<T>>::value>
{};

template<typename T, bool = is_iterator<T>::value && !is_const_iterator<T>::value>
struct is_output_iterator : false_type
{};

template<typename T>
struct is_output_iterator<T, true>
    : integral_constant<bool,
      is_same<output_iterator_tag,        iterator_category_t<T>>::value ||
      is_same<forward_iterator_tag,       iterator_category_t<T>>::value ||
      is_same<bidirectional_iterator_tag, iterator_category_t<T>>::value ||
      is_same<random_access_iterator_tag, iterator_category_t<T>>::value>
{};

template<typename T, bool = is_iterator<T>::value>
struct is_forward_iterator : false_type
{};

template<typename T>
struct is_forward_iterator<T, true>
    : integral_constant<bool, is_base_of<forward_iterator_tag,
      iterator_category_t<T>>::value>
{};

template<typename T, bool = is_iterator<T>::value>
struct is_bidirectional_iterator : false_type
{};

template<typename T>
struct is_bidirectional_iterator<T, true>
    : integral_constant<bool, is_base_of<bidirectional_iterator_tag,
      iterator_category_t<T>>::value>
{};

template<typename T, bool = is_iterator<T>::value>
struct is_random_access_iterator : false_type
{};

template<typename T>
struct is_random_access_iterator<T, true>
    : integral_constant<bool, is_base_of<random_access_iterator_tag,
      iterator_category_t<T>>::value>
{};
_CLANY_END

#endif // CLS_TRAITS_HPP