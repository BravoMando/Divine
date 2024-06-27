/*
 *
 ******************************************************************************
 *    Copyright [2024] [YongSong]
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ******************************************************************************
 *
 */

#ifndef VULKAN_GENERICS_HEADER
#define VULKAN_GENERICS_HEADER

#pragma once

#include <cstddef>

#include <typeinfo>
#include <type_traits>
#include <utility>

template <bool B>
struct BoolType
{
    static constexpr bool value = B;
};

template <bool B, typename V = void>
struct EnableIf;

template <typename V>
struct EnableIf<true, V>
{
    using type = V;
};

template <typename...>
struct VoidType
{
    using type = void;
};

#define STRUCT(_NAME__) struct _NAME__

#define ClassHasMemFunc(_Name__, _Func__)                                                               \
    template <class T, typename V = void>                                                               \
    STRUCT(_Name__) : BoolType<false>{};                                                                \
    template <class T>                                                                                  \
    STRUCT(_Name__)<T, typename VoidType<decltype(std::declval<T>()._Func__())>::type> : BoolType<true> \
    {                                                                                                   \
    }

#define ClassHasNestedClass(_Name__, _Nested__)                                         \
    template <class T, typename V = void>                                               \
    STRUCT(_Name__) : BoolType<false>{};                                                \
    template <class T>                                                                  \
    STRUCT(_Name__)<T, typename VoidType<typename T::_Nested__>::type> : BoolType<true> \
    {                                                                                   \
    }

#define ClassMemFuncNoException(_Name__, _Func__)                     \
    template <class T>                                                \
    STRUCT(_Name__) : BoolType<noexcept(std::declval<T>()._Func__())> \
    {                                                                 \
    }

ClassHasMemFunc(HasMemFuncBegin, Begin);
ClassHasMemFunc(HasMemFuncEnd, End);
ClassHasNestedClass(HasNestedIterator, Iterator);

ClassMemFuncNoException(BeginNoException, Begin);
ClassMemFuncNoException(EndNoException, End);

template <typename... ArgType>
struct ArgTypeList
{
    static constexpr size_t size = sizeof...(ArgType);
};

template <typename TypeList>
struct FrontTypeOf;

template <typename Head, typename... Tail>
struct FrontTypeOf<ArgTypeList<Head, Tail...>>
{
    using type = Head;
};

template <typename TypeList>
struct PopFrontOf;

template <typename Head, typename... Tail>
struct PopFrontOf<ArgTypeList<Head, Tail...>>
{
    using type = ArgTypeList<Tail...>;
};

template <typename TypeList, unsigned N>
struct Get : Get<typename PopFrontOf<TypeList>::type, N - 1>
{
};

template <typename TypeList>
struct Get<TypeList, 0U>
{
    using type = typename FrontTypeOf<TypeList>::type;
};

template <typename TypeList, unsigned N>
struct PopNFront : PopNFront<typename PopFrontOf<TypeList>::type, N - 1>
{
};

template <typename TypeList>
struct PopNFront<TypeList, 0U>
{
    using type = TypeList;
};

#endif
