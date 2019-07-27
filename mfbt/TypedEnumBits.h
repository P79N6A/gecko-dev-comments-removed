









#ifndef mozilla_TypedEnumBits_h
#define mozilla_TypedEnumBits_h

#include "mozilla/Attributes.h"
#include "mozilla/IntegerTypeTraits.h"

namespace mozilla {

























template<typename E>
class CastableTypedEnumResult
{
private:
  const E mValue;

public:
  explicit MOZ_CONSTEXPR CastableTypedEnumResult(E aValue)
    : mValue(aValue)
  {}

  MOZ_CONSTEXPR operator E() const { return mValue; }

  template<typename DestinationType>
  MOZ_EXPLICIT_CONVERSION MOZ_CONSTEXPR
  operator DestinationType() const { return DestinationType(mValue); }

  MOZ_CONSTEXPR bool operator !() const { return !bool(mValue); }
};

#define MOZ_CASTABLETYPEDENUMRESULT_BINOP(Op, OtherType, ReturnType) \
template<typename E> \
MOZ_CONSTEXPR ReturnType \
operator Op(const OtherType& aE, const CastableTypedEnumResult<E>& aR) \
{ \
  return ReturnType(aE Op OtherType(aR)); \
} \
template<typename E> \
MOZ_CONSTEXPR ReturnType \
operator Op(const CastableTypedEnumResult<E>& aR, const OtherType& aE) \
{ \
  return ReturnType(OtherType(aR) Op aE); \
} \
template<typename E> \
MOZ_CONSTEXPR ReturnType \
operator Op(const CastableTypedEnumResult<E>& aR1, \
            const CastableTypedEnumResult<E>& aR2) \
{ \
  return ReturnType(OtherType(aR1) Op OtherType(aR2)); \
}

MOZ_CASTABLETYPEDENUMRESULT_BINOP(|, E, CastableTypedEnumResult<E>)
MOZ_CASTABLETYPEDENUMRESULT_BINOP(&, E, CastableTypedEnumResult<E>)
MOZ_CASTABLETYPEDENUMRESULT_BINOP(^, E, CastableTypedEnumResult<E>)
MOZ_CASTABLETYPEDENUMRESULT_BINOP(==, E, bool)
MOZ_CASTABLETYPEDENUMRESULT_BINOP(!=, E, bool)
MOZ_CASTABLETYPEDENUMRESULT_BINOP(||, bool, bool)
MOZ_CASTABLETYPEDENUMRESULT_BINOP(&&, bool, bool)

template <typename E>
MOZ_CONSTEXPR CastableTypedEnumResult<E>
operator ~(const CastableTypedEnumResult<E>& aR)
{
  return CastableTypedEnumResult<E>(~(E(aR)));
}

#define MOZ_CASTABLETYPEDENUMRESULT_COMPOUND_ASSIGN_OP(Op) \
template<typename E> \
E& \
operator Op(E& aR1, \
            const CastableTypedEnumResult<E>& aR2) \
{ \
  return aR1 Op E(aR2); \
}

MOZ_CASTABLETYPEDENUMRESULT_COMPOUND_ASSIGN_OP(&=)
MOZ_CASTABLETYPEDENUMRESULT_COMPOUND_ASSIGN_OP(|=)
MOZ_CASTABLETYPEDENUMRESULT_COMPOUND_ASSIGN_OP(^=)

#undef MOZ_CASTABLETYPEDENUMRESULT_COMPOUND_ASSIGN_OP

#undef MOZ_CASTABLETYPEDENUMRESULT_BINOP

namespace detail {
template<typename E>
struct UnsignedIntegerTypeForEnum
  : UnsignedStdintTypeForSize<sizeof(E)>
{};
} 

} 

#define MOZ_MAKE_ENUM_CLASS_BINOP_IMPL(Name, Op) \
   inline MOZ_CONSTEXPR mozilla::CastableTypedEnumResult<Name> \
   operator Op(Name a, Name b) \
   { \
     typedef mozilla::CastableTypedEnumResult<Name> Result; \
     typedef mozilla::detail::UnsignedIntegerTypeForEnum<Name>::Type U; \
     return Result(Name(U(a) Op U(b))); \
   } \
 \
   inline Name& \
   operator Op##=(Name& a, Name b) \
   { \
     return a = a Op b; \
   }





#define MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Name) \
   MOZ_MAKE_ENUM_CLASS_BINOP_IMPL(Name, |) \
   MOZ_MAKE_ENUM_CLASS_BINOP_IMPL(Name, &) \
   MOZ_MAKE_ENUM_CLASS_BINOP_IMPL(Name, ^) \
   inline MOZ_CONSTEXPR mozilla::CastableTypedEnumResult<Name> \
   operator~(Name a) \
   { \
     typedef mozilla::CastableTypedEnumResult<Name> Result; \
     typedef mozilla::detail::UnsignedIntegerTypeForEnum<Name>::Type U; \
     return Result(Name(~(U(a)))); \
   }

#endif 
