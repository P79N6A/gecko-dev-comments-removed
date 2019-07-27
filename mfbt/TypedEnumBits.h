









#ifndef mozilla_TypedEnumBits_h
#define mozilla_TypedEnumBits_h

#include "mozilla/IntegerTypeTraits.h"
#include "mozilla/TypedEnumInternal.h"

namespace mozilla {

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

#ifndef MOZ_HAVE_CXX11_STRONG_ENUMS

#define MOZ_CASTABLETYPEDENUMRESULT_BINOP_EXTRA_NON_CXX11(Op, ReturnType) \
template<typename E> \
MOZ_CONSTEXPR ReturnType \
operator Op(typename E::Enum aE, const CastableTypedEnumResult<E>& aR) \
{ \
  return ReturnType(aE Op E(aR)); \
} \
template<typename E> \
MOZ_CONSTEXPR ReturnType \
operator Op(const CastableTypedEnumResult<E>& aR, typename E::Enum aE) \
{ \
  return ReturnType(E(aR) Op aE); \
}

MOZ_CASTABLETYPEDENUMRESULT_BINOP_EXTRA_NON_CXX11(|, CastableTypedEnumResult<E>)
MOZ_CASTABLETYPEDENUMRESULT_BINOP_EXTRA_NON_CXX11(&, CastableTypedEnumResult<E>)
MOZ_CASTABLETYPEDENUMRESULT_BINOP_EXTRA_NON_CXX11(^, CastableTypedEnumResult<E>)
MOZ_CASTABLETYPEDENUMRESULT_BINOP_EXTRA_NON_CXX11(==, bool)
MOZ_CASTABLETYPEDENUMRESULT_BINOP_EXTRA_NON_CXX11(!=, bool)

#undef MOZ_CASTABLETYPEDENUMRESULT_BINOP_EXTRA_NON_CXX11

#endif 

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

#define MOZ_MAKE_ENUM_CLASS_OPS_IMPL(Name) \
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

#ifndef MOZ_HAVE_CXX11_STRONG_ENUMS
#  define MOZ_MAKE_ENUM_CLASS_BITWISE_BINOP_EXTRA_NON_CXX11(Name, Op) \
     inline MOZ_CONSTEXPR mozilla::CastableTypedEnumResult<Name> \
     operator Op(Name a, Name::Enum b) \
     { \
       return a Op Name(b); \
     } \
 \
     inline MOZ_CONSTEXPR mozilla::CastableTypedEnumResult<Name> \
     operator Op(Name::Enum a, Name b) \
     { \
       return Name(a) Op b; \
     } \
 \
     inline MOZ_CONSTEXPR mozilla::CastableTypedEnumResult<Name> \
     operator Op(Name::Enum a, Name::Enum b) \
     { \
       return Name(a) Op Name(b); \
     } \
 \
     inline Name& \
     operator Op##=(Name& a, Name::Enum b) \
     { \
       return a = a Op Name(b); \
    }

#  define MOZ_MAKE_ENUM_CLASS_OPS_EXTRA_NON_CXX11(Name) \
     MOZ_MAKE_ENUM_CLASS_BITWISE_BINOP_EXTRA_NON_CXX11(Name, |) \
     MOZ_MAKE_ENUM_CLASS_BITWISE_BINOP_EXTRA_NON_CXX11(Name, &) \
     MOZ_MAKE_ENUM_CLASS_BITWISE_BINOP_EXTRA_NON_CXX11(Name, ^) \
     inline MOZ_CONSTEXPR mozilla::CastableTypedEnumResult<Name> \
     operator~(Name::Enum a) \
     { \
       return ~(Name(a)); \
     }
#endif





#ifdef MOZ_HAVE_CXX11_STRONG_ENUMS
#  define MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Name) \
     MOZ_MAKE_ENUM_CLASS_OPS_IMPL(Name)
#else
#  define MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Name) \
     MOZ_MAKE_ENUM_CLASS_OPS_IMPL(Name) \
     MOZ_MAKE_ENUM_CLASS_OPS_EXTRA_NON_CXX11(Name)
#endif

#endif 
