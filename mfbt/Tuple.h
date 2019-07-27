







#ifndef mozilla_Tuple_h
#define mozilla_Tuple_h

#include "mozilla/Move.h"
#include "mozilla/TemplateLib.h"
#include "mozilla/TypeTraits.h"

#include <stddef.h>

namespace mozilla {

namespace detail {





template<typename... Ts>
struct Group;

















template<typename Source, typename Target, bool SameSize>
struct CheckConvertibilityImpl;

template<typename Source, typename Target>
struct CheckConvertibilityImpl<Source, Target, false>
  : FalseType {};

template<typename... SourceTypes, typename... TargetTypes>
struct CheckConvertibilityImpl<Group<SourceTypes...>, Group<TargetTypes...>, true>
  : IntegralConstant<bool, tl::And<IsConvertible<SourceTypes, TargetTypes>::value...>::value> { };

template<typename Source, typename Target>
struct CheckConvertibility;

template<typename... SourceTypes, typename... TargetTypes>
struct CheckConvertibility<Group<SourceTypes...>, Group<TargetTypes...>>
  : CheckConvertibilityImpl<Group<SourceTypes...>, Group<TargetTypes...>,
        sizeof...(SourceTypes) == sizeof...(TargetTypes)> { };

























template<std::size_t Index, typename... Elements>
struct TupleImpl;





template<std::size_t Index>
struct TupleImpl<Index> {};






template<std::size_t Index, typename HeadT, typename... TailT>
struct TupleImpl<Index, HeadT, TailT...>
  : public TupleImpl<Index + 1, TailT...>
{
  typedef TupleImpl<Index + 1, TailT...> Base;

  
  
  
  
  
  static HeadT& Head(TupleImpl& aTuple) { return aTuple.mHead; }
  static const HeadT& Head(const TupleImpl& aTuple) { return aTuple.mHead; }
  static Base& Tail(TupleImpl& aTuple) { return aTuple; }
  static const Base& Tail(const TupleImpl& aTuple) { return aTuple; }

  TupleImpl() : Base(), mHead() { }

  
  explicit TupleImpl(const HeadT& aHead, const TailT&... aTail)
    : Base(aTail...), mHead(aHead) { }

  
  
  
  
  template <typename OtherHeadT, typename... OtherTailT,
            typename = typename EnableIf<
                CheckConvertibility<
                    Group<OtherHeadT, OtherTailT...>,
                    Group<HeadT, TailT...>>::value>::Type>
  explicit TupleImpl(OtherHeadT&& aHead, OtherTailT&&... aTail)
    : Base(Forward<OtherTailT>(aTail)...), mHead(Forward<OtherHeadT>(aHead)) { }

  
  
  
  TupleImpl(const TupleImpl& aOther)
    : Base(Tail(aOther))
    , mHead(Head(aOther)) {}
  TupleImpl(TupleImpl&& aOther)
    : Base(Move(Tail(aOther)))
    , mHead(Move(Head(aOther))) {}

  
  TupleImpl& operator=(const TupleImpl& aOther)
  {
    Head(*this) = Head(aOther);
    Tail(*this) = Tail(aOther);
    return *this;
  }
  TupleImpl& operator=(TupleImpl&& aOther)
  {
    Head(*this) = Move(Head(aOther));
    Tail(*this) = Move(Tail(aOther));
    return *this;
  }
private:
  HeadT mHead;  
};

} 









template<typename... Elements>
class Tuple : public detail::TupleImpl<0, Elements...>
{
  typedef detail::TupleImpl<0, Elements...> Impl;
public:
  
  

  Tuple() : Impl() { }
  explicit Tuple(const Elements&... aElements) : Impl(aElements...) { }
  
  
  
  
  template <typename OtherHead, typename... OtherTail,
            typename = typename EnableIf<
                detail::CheckConvertibility<
                    detail::Group<OtherHead, OtherTail...>,
                    detail::Group<Elements...>>::value>::Type>
  explicit Tuple(OtherHead&& aHead, OtherTail&&... aTail)
    : Impl(Forward<OtherHead>(aHead), Forward<OtherTail>(aTail)...) { }
  Tuple(const Tuple& aOther) : Impl(aOther) { }
  Tuple(Tuple&& aOther) : Impl(Move(aOther)) { }

  Tuple& operator=(const Tuple& aOther)
  {
    static_cast<Impl&>(*this) = aOther;
    return *this;
  }
  Tuple& operator=(Tuple&& aOther)
  {
    static_cast<Impl&>(*this) = Move(aOther);
    return *this;
  }
};







template <>
class Tuple<> {};

namespace detail {










template<std::size_t Index, typename... Elements>
auto TupleGetHelper(TupleImpl<Index, Elements...>& aTuple)
    -> decltype(TupleImpl<Index, Elements...>::Head(aTuple))
{
  return TupleImpl<Index, Elements...>::Head(aTuple);
}


template<std::size_t Index, typename... Elements>
auto TupleGetHelper(const TupleImpl<Index, Elements...>& aTuple)
    -> decltype(TupleImpl<Index, Elements...>::Head(aTuple))
{
  return TupleImpl<Index, Elements...>::Head(aTuple);
}

} 













template<std::size_t Index, typename... Elements>
auto Get(Tuple<Elements...>& aTuple)
    -> decltype(detail::TupleGetHelper<Index>(aTuple))
{
  return detail::TupleGetHelper<Index>(aTuple);
}


template<std::size_t Index, typename... Elements>
auto Get(const Tuple<Elements...>& aTuple)
    -> decltype(detail::TupleGetHelper<Index>(aTuple))
{
  return detail::TupleGetHelper<Index>(aTuple);
}


template<std::size_t Index, typename... Elements>
auto Get(Tuple<Elements...>&& aTuple)
    -> decltype(Move(mozilla::Get<Index>(aTuple)))
{
  
  
  return Move(mozilla::Get<Index>(aTuple));
}










template<typename... Elements>
Tuple<Elements...> MakeTuple(Elements&&... aElements)
{
  return Tuple<Elements...>(Forward<Elements>(aElements)...);
}

} 

#endif 
