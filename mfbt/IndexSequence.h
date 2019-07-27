




























































#ifndef mozilla_IndexSequence_h
#define mozilla_IndexSequence_h

#include "mozilla/Attributes.h"

#include <stddef.h>

namespace mozilla {




template<size_t... Indices>
struct IndexSequence
{
  static MOZ_CONSTEXPR size_t Size() { return sizeof...(Indices); }
};

namespace detail {



template<size_t... Indices>
struct IndexTuple
{
  typedef IndexTuple<Indices..., sizeof...(Indices)> Next;
};


template<size_t N>
struct BuildIndexTuple
{
  typedef typename BuildIndexTuple<N - 1>::Type::Next Type;
};

template<>
struct BuildIndexTuple<0>
{
  typedef IndexTuple<> Type;
};

template<size_t N, typename IndexTuple>
struct MakeIndexSequenceImpl;

template<size_t N, size_t... Indices>
struct MakeIndexSequenceImpl<N, IndexTuple<Indices...>>
{
  typedef IndexSequence<Indices...> Type;
};

} 







template<size_t N>
struct MakeIndexSequence
{
  typedef typename detail::MakeIndexSequenceImpl<N,
    typename detail::BuildIndexTuple<N>::Type>::Type Type;
};









template<typename... Types>
struct IndexSequenceFor
{
  typedef typename MakeIndexSequence<sizeof...(Types)>::Type Type;
};

} 

#endif 
