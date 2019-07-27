





#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/TypedEnum.h"
#include "mozilla/TypedEnumBits.h"

#include <stdint.h>




#if __cplusplus >= 201103L && !defined(ANDROID)
#  if defined(__clang__)
     




#    ifndef __has_extension
#      define __has_extension __has_feature /* compatibility, for older versions of clang */
#    endif
#    if __has_extension(is_literal) && __has_include(<type_traits>)
#      define MOZ_HAVE_IS_LITERAL
#    endif
#  elif defined(__GNUC__)
#    if defined(__GXX_EXPERIMENTAL_CXX0X__)
#      if MOZ_GCC_VERSION_AT_LEAST(4, 6, 0)
#        define MOZ_HAVE_IS_LITERAL
#      endif
#    endif
#  elif defined(_MSC_VER)
#    define MOZ_HAVE_IS_LITERAL
#  endif
#endif

#if defined(MOZ_HAVE_IS_LITERAL) && defined(MOZ_HAVE_CXX11_CONSTEXPR)
#include <type_traits>
template<typename T>
void
RequireLiteralType()
{
  static_assert(std::is_literal_type<T>::value, "Expected a literal type");
}
#else 
template<typename T>
void
RequireLiteralType()
{
}
#endif

template<typename T>
void
RequireLiteralType(const T&)
{
  RequireLiteralType<T>();
}

MOZ_BEGIN_ENUM_CLASS(AutoEnum)
  A,
  B = -3,
  C
MOZ_END_ENUM_CLASS(AutoEnum)

MOZ_BEGIN_ENUM_CLASS(CharEnum, char)
  A,
  B = 3,
  C
MOZ_END_ENUM_CLASS(CharEnum)

MOZ_BEGIN_ENUM_CLASS(AutoEnumBitField)
  A = 0x10,
  B = 0x20,
  C
MOZ_END_ENUM_CLASS(AutoEnumBitField)

MOZ_BEGIN_ENUM_CLASS(CharEnumBitField, char)
  A = 0x10,
  B,
  C = 0x40
MOZ_END_ENUM_CLASS(CharEnumBitField)

struct Nested
{
  MOZ_BEGIN_NESTED_ENUM_CLASS(AutoEnum)
    A,
    B,
    C = -1
  MOZ_END_NESTED_ENUM_CLASS(AutoEnum)

  MOZ_BEGIN_NESTED_ENUM_CLASS(CharEnum, char)
    A = 4,
    B,
    C = 1
  MOZ_END_NESTED_ENUM_CLASS(CharEnum)

  MOZ_BEGIN_NESTED_ENUM_CLASS(AutoEnumBitField)
    A,
    B = 0x20,
    C
  MOZ_END_NESTED_ENUM_CLASS(AutoEnumBitField)

  MOZ_BEGIN_NESTED_ENUM_CLASS(CharEnumBitField, char)
    A = 1,
    B = 1,
    C = 1
  MOZ_END_NESTED_ENUM_CLASS(CharEnumBitField)
};

MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(AutoEnumBitField)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(CharEnumBitField)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Nested::AutoEnumBitField)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Nested::CharEnumBitField)

#define MAKE_STANDARD_BITFIELD_FOR_TYPE(IntType)                   \
  MOZ_BEGIN_ENUM_CLASS(BitFieldFor_##IntType, IntType)             \
    A = 1,                                                         \
    B = 2,                                                         \
    C = 4,                                                         \
  MOZ_END_ENUM_CLASS(BitFieldFor_##IntType)                        \
  MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(BitFieldFor_##IntType)

MAKE_STANDARD_BITFIELD_FOR_TYPE(int8_t)
MAKE_STANDARD_BITFIELD_FOR_TYPE(uint8_t)
MAKE_STANDARD_BITFIELD_FOR_TYPE(int16_t)
MAKE_STANDARD_BITFIELD_FOR_TYPE(uint16_t)
MAKE_STANDARD_BITFIELD_FOR_TYPE(int32_t)
MAKE_STANDARD_BITFIELD_FOR_TYPE(uint32_t)
MAKE_STANDARD_BITFIELD_FOR_TYPE(int64_t)
MAKE_STANDARD_BITFIELD_FOR_TYPE(uint64_t)
MAKE_STANDARD_BITFIELD_FOR_TYPE(char)
typedef signed char signed_char;
MAKE_STANDARD_BITFIELD_FOR_TYPE(signed_char)
typedef unsigned char unsigned_char;
MAKE_STANDARD_BITFIELD_FOR_TYPE(unsigned_char)
MAKE_STANDARD_BITFIELD_FOR_TYPE(short)
typedef unsigned short unsigned_short;
MAKE_STANDARD_BITFIELD_FOR_TYPE(unsigned_short)
MAKE_STANDARD_BITFIELD_FOR_TYPE(int)
typedef unsigned int unsigned_int;
MAKE_STANDARD_BITFIELD_FOR_TYPE(unsigned_int)
MAKE_STANDARD_BITFIELD_FOR_TYPE(long)
typedef unsigned long unsigned_long;
MAKE_STANDARD_BITFIELD_FOR_TYPE(unsigned_long)
typedef long long long_long;
MAKE_STANDARD_BITFIELD_FOR_TYPE(long_long)
typedef unsigned long long unsigned_long_long;
MAKE_STANDARD_BITFIELD_FOR_TYPE(unsigned_long_long)

#undef MAKE_STANDARD_BITFIELD_FOR_TYPE

template<typename T>
void
TestNonConvertibilityForOneType()
{
  using mozilla::IsConvertible;

#if defined(MOZ_HAVE_CXX11_STRONG_ENUMS) && defined(MOZ_HAVE_EXPLICIT_CONVERSION)
  static_assert(!IsConvertible<T, bool>::value, "should not be convertible");
  static_assert(!IsConvertible<T, int>::value, "should not be convertible");
  static_assert(!IsConvertible<T, uint64_t>::value, "should not be convertible");
#endif

  static_assert(!IsConvertible<bool, T>::value, "should not be convertible");
  static_assert(!IsConvertible<int, T>::value, "should not be convertible");
  static_assert(!IsConvertible<uint64_t, T>::value, "should not be convertible");
}

template<typename TypedEnum>
void
TestTypedEnumBasics()
{
  const TypedEnum a = TypedEnum::A;
  int unused = int(a);
  (void) unused;
  RequireLiteralType(TypedEnum::A);
  RequireLiteralType(a);
  TestNonConvertibilityForOneType<TypedEnum>();
}










template<char o, typename T1, typename T2>
auto Op(const T1& aT1, const T2& aT2)
  -> decltype(aT1 | aT2) 
                         
                         
{
  using mozilla::IsSame;
  static_assert(IsSame<decltype(aT1 | aT2), decltype(aT1 & aT2)>::value,
                "binary ops should have the same result type");
  static_assert(IsSame<decltype(aT1 | aT2), decltype(aT1 ^ aT2)>::value,
                "binary ops should have the same result type");

  static_assert(o == '|' ||
                o == '&' ||
                o == '^', "unexpected operator character");

  return o == '|' ? aT1 | aT2
       : o == '&' ? aT1 & aT2
                  : aT1 ^ aT2;
}











template<char o, typename T1, typename T2>
T1& OpAssign(T1& aT1, const T2& aT2)
{
  static_assert(o == '|' ||
                o == '&' ||
                o == '^', "unexpected operator character");

  switch (o) {
    case '|': return aT1 |= aT2;
    case '&': return aT1 &= aT2;
    case '^': return aT1 ^= aT2;
    default: MOZ_CRASH();
  }
}



















template<typename TypedEnum, char o, typename T1, typename T2, typename T3>
void TestBinOp(const T1& aT1, const T2& aT2, const T3& aT3)
{
  typedef typename mozilla::detail::UnsignedIntegerTypeForEnum<TypedEnum>::Type
          UnsignedIntegerType;

  
  
  
  auto result = Op<o>(aT1, aT2);

  typedef decltype(result) ResultType;

  RequireLiteralType<ResultType>();
  TestNonConvertibilityForOneType<ResultType>();

  UnsignedIntegerType unsignedIntegerResult =
    Op<o>(UnsignedIntegerType(aT1), UnsignedIntegerType(aT2));

  MOZ_RELEASE_ASSERT(unsignedIntegerResult == UnsignedIntegerType(result));
  MOZ_RELEASE_ASSERT(TypedEnum(unsignedIntegerResult) == TypedEnum(result));
  MOZ_RELEASE_ASSERT((!unsignedIntegerResult) == (!result));
  MOZ_RELEASE_ASSERT((!!unsignedIntegerResult) == (!!result));
  MOZ_RELEASE_ASSERT(bool(unsignedIntegerResult) == bool(result));

  
  
  
  TypedEnum newResult = result;
  OpAssign<o>(newResult, aT3);
  UnsignedIntegerType unsignedIntegerNewResult = unsignedIntegerResult;
  OpAssign<o>(unsignedIntegerNewResult, UnsignedIntegerType(aT3));
  MOZ_RELEASE_ASSERT(TypedEnum(unsignedIntegerNewResult) == newResult);

  
  
  
  
  MOZ_RELEASE_ASSERT(result == TypedEnum(result));
  MOZ_RELEASE_ASSERT(!(result != TypedEnum(result)));
  MOZ_RELEASE_ASSERT((result && true) == bool(result));
  MOZ_RELEASE_ASSERT((result && false) == false);
  MOZ_RELEASE_ASSERT((true && result) == bool(result));
  MOZ_RELEASE_ASSERT((false && result && false) == false);
  MOZ_RELEASE_ASSERT((result || false) == bool(result));
  MOZ_RELEASE_ASSERT((result || true) == true);
  MOZ_RELEASE_ASSERT((false || result) == bool(result));
  MOZ_RELEASE_ASSERT((true || result) == true);
}


template<typename TypedEnum, typename T>
void TestTilde(const T& aT)
{
  typedef typename mozilla::detail::UnsignedIntegerTypeForEnum<TypedEnum>::Type
          UnsignedIntegerType;

  auto result = ~aT;

  typedef decltype(result) ResultType;

  RequireLiteralType<ResultType>();
  TestNonConvertibilityForOneType<ResultType>();

  UnsignedIntegerType unsignedIntegerResult = ~(UnsignedIntegerType(aT));

  MOZ_RELEASE_ASSERT(unsignedIntegerResult == UnsignedIntegerType(result));
  MOZ_RELEASE_ASSERT(TypedEnum(unsignedIntegerResult) == TypedEnum(result));
  MOZ_RELEASE_ASSERT((!unsignedIntegerResult) == (!result));
  MOZ_RELEASE_ASSERT((!!unsignedIntegerResult) == (!!result));
  MOZ_RELEASE_ASSERT(bool(unsignedIntegerResult) == bool(result));
}



template<typename TypedEnum, typename T1, typename T2, typename T3>
void TestAllOpsForGivenOperands(const T1& aT1, const T2& aT2, const T3& aT3)
{
  TestBinOp<TypedEnum, '|'>(aT1, aT2, aT3);
  TestBinOp<TypedEnum, '&'>(aT1, aT2, aT3);
  TestBinOp<TypedEnum, '^'>(aT1, aT2, aT3);
  TestTilde<TypedEnum>(aT1);
}



template<typename TypedEnum, char o>
void TestAllOpsForOperandsBuiltUsingGivenOp()
{
  
  
  
  
  
  

  const TypedEnum a_plain = TypedEnum::A;
  const TypedEnum b_plain = TypedEnum::B;
  const TypedEnum c_plain = TypedEnum::C;

  auto a_auto = TypedEnum::A;
  auto b_auto = TypedEnum::B;
  auto c_auto = TypedEnum::C;

  auto ab_plain = Op<o>(a_plain, b_plain);
  auto bc_plain = Op<o>(b_plain, c_plain);
  auto ab_auto = Op<o>(a_auto, b_auto);
  auto bc_auto = Op<o>(b_auto, c_auto);

  
  
  
  
  
  
  
  
  
  
  
  TestAllOpsForGivenOperands<TypedEnum>(a_plain,  b_plain,  c_plain);
  TestAllOpsForGivenOperands<TypedEnum>(a_plain,  bc_plain, b_auto);
  TestAllOpsForGivenOperands<TypedEnum>(ab_plain, c_plain,  a_plain);
  TestAllOpsForGivenOperands<TypedEnum>(ab_plain, bc_plain, a_auto);

  TestAllOpsForGivenOperands<TypedEnum>(a_plain,  b_auto,   c_plain);
  TestAllOpsForGivenOperands<TypedEnum>(a_plain,  bc_auto,  b_auto);
  TestAllOpsForGivenOperands<TypedEnum>(ab_plain, c_auto,   a_plain);
  TestAllOpsForGivenOperands<TypedEnum>(ab_plain, bc_auto,  a_auto);

  TestAllOpsForGivenOperands<TypedEnum>(a_auto,   b_plain,  c_plain);
  TestAllOpsForGivenOperands<TypedEnum>(a_auto,   bc_plain, b_auto);
  TestAllOpsForGivenOperands<TypedEnum>(ab_auto,  c_plain,  a_plain);
  TestAllOpsForGivenOperands<TypedEnum>(ab_auto,  bc_plain, a_auto);

  TestAllOpsForGivenOperands<TypedEnum>(a_auto,   b_auto,   c_plain);
  TestAllOpsForGivenOperands<TypedEnum>(a_auto,   bc_auto,  b_auto);
  TestAllOpsForGivenOperands<TypedEnum>(ab_auto,  c_auto,   a_plain);
  TestAllOpsForGivenOperands<TypedEnum>(ab_auto,  bc_auto,  a_auto);
}


template<typename TypedEnum>
void
TestTypedEnumBitField()
{
  TestTypedEnumBasics<TypedEnum>();

  TestAllOpsForOperandsBuiltUsingGivenOp<TypedEnum, '|'>();
  TestAllOpsForOperandsBuiltUsingGivenOp<TypedEnum, '&'>();
  TestAllOpsForOperandsBuiltUsingGivenOp<TypedEnum, '^'>();
}




void TestNoConversionsBetweenUnrelatedTypes()
{
  using mozilla::IsConvertible;

  
  
  typedef CharEnumBitField T1;
  typedef Nested::CharEnumBitField T2;

  static_assert(!IsConvertible<T1, T2>::value,
                "should not be convertible");
  static_assert(!IsConvertible<T1, decltype(T2::A)>::value,
                "should not be convertible");
  static_assert(!IsConvertible<T1, decltype(T2::A | T2::B)>::value,
                "should not be convertible");

  static_assert(!IsConvertible<decltype(T1::A), T2>::value,
                "should not be convertible");
  static_assert(!IsConvertible<decltype(T1::A), decltype(T2::A)>::value,
                "should not be convertible");
  static_assert(!IsConvertible<decltype(T1::A), decltype(T2::A | T2::B)>::value,
                "should not be convertible");

  
  
  
  
#ifdef MOZ_HAVE_EXPLICIT_CONVERSION
  static_assert(!IsConvertible<decltype(T1::A | T1::B), T2>::value,
                "should not be convertible");
  static_assert(!IsConvertible<decltype(T1::A | T1::B), decltype(T2::A)>::value,
                "should not be convertible");
  static_assert(!IsConvertible<decltype(T1::A | T1::B), decltype(T2::A | T2::B)>::value,
                "should not be convertible");
#endif
}

MOZ_BEGIN_ENUM_CLASS(Int8EnumWithHighBits, int8_t)
  A = 0x20,
  B = 0x40
MOZ_END_ENUM_CLASS(Int8EnumWithHighBits)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Int8EnumWithHighBits)

MOZ_BEGIN_ENUM_CLASS(Uint8EnumWithHighBits, uint8_t)
  A = 0x40,
  B = 0x80
MOZ_END_ENUM_CLASS(Uint8EnumWithHighBits)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Uint8EnumWithHighBits)

MOZ_BEGIN_ENUM_CLASS(Int16EnumWithHighBits, int16_t)
  A = 0x2000,
  B = 0x4000
MOZ_END_ENUM_CLASS(Int16EnumWithHighBits)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Int16EnumWithHighBits)

MOZ_BEGIN_ENUM_CLASS(Uint16EnumWithHighBits, uint16_t)
  A = 0x4000,
  B = 0x8000
MOZ_END_ENUM_CLASS(Uint16EnumWithHighBits)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Uint16EnumWithHighBits)

MOZ_BEGIN_ENUM_CLASS(Int32EnumWithHighBits, int32_t)
  A = 0x20000000,
  B = 0x40000000
MOZ_END_ENUM_CLASS(Int32EnumWithHighBits)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Int32EnumWithHighBits)

MOZ_BEGIN_ENUM_CLASS(Uint32EnumWithHighBits, uint32_t)
  A = 0x40000000u,
  B = 0x80000000u
MOZ_END_ENUM_CLASS(Uint32EnumWithHighBits)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Uint32EnumWithHighBits)

MOZ_BEGIN_ENUM_CLASS(Int64EnumWithHighBits, int64_t)
  A = 0x2000000000000000ll,
  B = 0x4000000000000000ll
MOZ_END_ENUM_CLASS(Int64EnumWithHighBits)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Int64EnumWithHighBits)

MOZ_BEGIN_ENUM_CLASS(Uint64EnumWithHighBits, uint64_t)
  A = 0x4000000000000000ull,
  B = 0x8000000000000000ull
MOZ_END_ENUM_CLASS(Uint64EnumWithHighBits)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(Uint64EnumWithHighBits)



template<typename EnumType, typename IntType>
void TestIsNotTruncated()
{
  EnumType a = EnumType::A;
  EnumType b = EnumType::B;
  MOZ_RELEASE_ASSERT(IntType(a));
  MOZ_RELEASE_ASSERT(IntType(b));
  MOZ_RELEASE_ASSERT(a | EnumType::B);
  MOZ_RELEASE_ASSERT(a | b);
  MOZ_RELEASE_ASSERT(EnumType::A | EnumType::B);
  EnumType c = EnumType::A | EnumType::B;
  MOZ_RELEASE_ASSERT(IntType(c));
  MOZ_RELEASE_ASSERT(c & c);
  MOZ_RELEASE_ASSERT(c | c);
  MOZ_RELEASE_ASSERT(c == (EnumType::A | EnumType::B));
  MOZ_RELEASE_ASSERT(a != (EnumType::A | EnumType::B));
  MOZ_RELEASE_ASSERT(b != (EnumType::A | EnumType::B));
  MOZ_RELEASE_ASSERT(c & EnumType::A);
  MOZ_RELEASE_ASSERT(c & EnumType::B);
  EnumType d = EnumType::A;
  d |= EnumType::B;
  MOZ_RELEASE_ASSERT(d == c);
}

int
main()
{
  TestTypedEnumBasics<AutoEnum>();
  TestTypedEnumBasics<CharEnum>();
  TestTypedEnumBasics<Nested::AutoEnum>();
  TestTypedEnumBasics<Nested::CharEnum>();

  TestTypedEnumBitField<AutoEnumBitField>();
  TestTypedEnumBitField<CharEnumBitField>();
  TestTypedEnumBitField<Nested::AutoEnumBitField>();
  TestTypedEnumBitField<Nested::CharEnumBitField>();

  TestTypedEnumBitField<BitFieldFor_uint8_t>();
  TestTypedEnumBitField<BitFieldFor_int8_t>();
  TestTypedEnumBitField<BitFieldFor_uint16_t>();
  TestTypedEnumBitField<BitFieldFor_int16_t>();
  TestTypedEnumBitField<BitFieldFor_uint32_t>();
  TestTypedEnumBitField<BitFieldFor_int32_t>();
  TestTypedEnumBitField<BitFieldFor_uint64_t>();
  TestTypedEnumBitField<BitFieldFor_int64_t>();
  TestTypedEnumBitField<BitFieldFor_char>();
  TestTypedEnumBitField<BitFieldFor_signed_char>();
  TestTypedEnumBitField<BitFieldFor_unsigned_char>();
  TestTypedEnumBitField<BitFieldFor_short>();
  TestTypedEnumBitField<BitFieldFor_unsigned_short>();
  TestTypedEnumBitField<BitFieldFor_int>();
  TestTypedEnumBitField<BitFieldFor_unsigned_int>();
  TestTypedEnumBitField<BitFieldFor_long>();
  TestTypedEnumBitField<BitFieldFor_unsigned_long>();
  TestTypedEnumBitField<BitFieldFor_long_long>();
  TestTypedEnumBitField<BitFieldFor_unsigned_long_long>();

  TestNoConversionsBetweenUnrelatedTypes();

  TestIsNotTruncated<Int8EnumWithHighBits, int8_t>();
  TestIsNotTruncated<Int16EnumWithHighBits, int16_t>();
  TestIsNotTruncated<Int32EnumWithHighBits, int32_t>();
  TestIsNotTruncated<Int64EnumWithHighBits, int64_t>();
  TestIsNotTruncated<Uint8EnumWithHighBits, uint8_t>();
  TestIsNotTruncated<Uint16EnumWithHighBits, uint16_t>();
  TestIsNotTruncated<Uint32EnumWithHighBits, uint32_t>();
  TestIsNotTruncated<Uint64EnumWithHighBits, uint64_t>();

  return 0;
}
