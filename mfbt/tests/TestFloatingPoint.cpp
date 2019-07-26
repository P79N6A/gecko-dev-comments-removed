




#include "mozilla/FloatingPoint.h"

#include <math.h>

using mozilla::ExponentComponent;
using mozilla::FloatingPoint;
using mozilla::FuzzyEqualsAdditive;
using mozilla::FuzzyEqualsMultiplicative;
using mozilla::IsFinite;
using mozilla::IsInfinite;
using mozilla::IsNaN;
using mozilla::IsNegative;
using mozilla::IsNegativeZero;
using mozilla::NegativeInfinity;
using mozilla::NumberEqualsInt32;
using mozilla::NumberIsInt32;
using mozilla::NumbersAreIdentical;
using mozilla::PositiveInfinity;
using mozilla::SpecificNaN;
using mozilla::UnspecifiedNaN;

static void
ShouldBeIdentical(double d1, double d2)
{
  MOZ_ASSERT(NumbersAreIdentical(d1, d2));
  MOZ_ASSERT(NumbersAreIdentical(d2, d1));
}

static void
ShouldNotBeIdentical(double d1, double d2)
{
  MOZ_ASSERT(!NumbersAreIdentical(d1, d2));
  MOZ_ASSERT(!NumbersAreIdentical(d2, d1));
}

static void
TestDoublesAreIdentical()
{
  ShouldBeIdentical(+0.0, +0.0);
  ShouldBeIdentical(-0.0, -0.0);
  ShouldNotBeIdentical(+0.0, -0.0);

  ShouldBeIdentical(1.0, 1.0);
  ShouldNotBeIdentical(-1.0, 1.0);
  ShouldBeIdentical(4294967295.0, 4294967295.0);
  ShouldNotBeIdentical(-4294967295.0, 4294967295.0);
  ShouldBeIdentical(4294967296.0, 4294967296.0);
  ShouldBeIdentical(4294967297.0, 4294967297.0);
  ShouldBeIdentical(1e300, 1e300);

  ShouldBeIdentical(PositiveInfinity<double>(), PositiveInfinity<double>());
  ShouldBeIdentical(NegativeInfinity<double>(), NegativeInfinity<double>());
  ShouldNotBeIdentical(PositiveInfinity<double>(), NegativeInfinity<double>());

  ShouldNotBeIdentical(-0.0, NegativeInfinity<double>());
  ShouldNotBeIdentical(+0.0, NegativeInfinity<double>());
  ShouldNotBeIdentical(1e300, NegativeInfinity<double>());
  ShouldNotBeIdentical(3.141592654, NegativeInfinity<double>());

  ShouldBeIdentical(UnspecifiedNaN<double>(), UnspecifiedNaN<double>());
  ShouldBeIdentical(-UnspecifiedNaN<double>(), UnspecifiedNaN<double>());
  ShouldBeIdentical(UnspecifiedNaN<double>(), -UnspecifiedNaN<double>());

  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 42));
  ShouldBeIdentical(SpecificNaN<double>(1, 17), SpecificNaN<double>(1, 42));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(1, 42));
  ShouldBeIdentical(SpecificNaN<double>(1, 17), SpecificNaN<double>(0, 42));

  const uint64_t Mask = 0xfffffffffffffULL;
  for (unsigned i = 0; i < 52; i++) {
    for (unsigned j = 0; j < 52; j++) {
      for (unsigned sign = 0; i < 2; i++) {
        ShouldBeIdentical(SpecificNaN<double>(0, 1ULL << i), SpecificNaN<double>(sign, 1ULL << j));
        ShouldBeIdentical(SpecificNaN<double>(1, 1ULL << i), SpecificNaN<double>(sign, 1ULL << j));

        ShouldBeIdentical(SpecificNaN<double>(0, Mask & ~(1ULL << i)),
                          SpecificNaN<double>(sign, Mask & ~(1ULL << j)));
        ShouldBeIdentical(SpecificNaN<double>(1, Mask & ~(1ULL << i)),
                          SpecificNaN<double>(sign, Mask & ~(1ULL << j)));
      }
    }
  }
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x8000000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x4000000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x2000000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x1000000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x0800000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x0400000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x0200000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x0100000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x0080000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x0040000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x0020000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(0, 17), SpecificNaN<double>(0, 0x0010000000000ULL));
  ShouldBeIdentical(SpecificNaN<double>(1, 17), SpecificNaN<double>(0, 0xff0ffffffffffULL));
  ShouldBeIdentical(SpecificNaN<double>(1, 17), SpecificNaN<double>(0, 0xfffffffffff0fULL));

  ShouldNotBeIdentical(UnspecifiedNaN<double>(), +0.0);
  ShouldNotBeIdentical(UnspecifiedNaN<double>(), -0.0);
  ShouldNotBeIdentical(UnspecifiedNaN<double>(), 1.0);
  ShouldNotBeIdentical(UnspecifiedNaN<double>(), -1.0);
  ShouldNotBeIdentical(UnspecifiedNaN<double>(), PositiveInfinity<double>());
  ShouldNotBeIdentical(UnspecifiedNaN<double>(), NegativeInfinity<double>());
}

static void
TestExponentComponent()
{
  MOZ_ASSERT(ExponentComponent(0.0) == -int_fast16_t(FloatingPoint<double>::ExponentBias));
  MOZ_ASSERT(ExponentComponent(-0.0) == -int_fast16_t(FloatingPoint<double>::ExponentBias));
  MOZ_ASSERT(ExponentComponent(0.125) == -3);
  MOZ_ASSERT(ExponentComponent(0.5) == -1);
  MOZ_ASSERT(ExponentComponent(1.0) == 0);
  MOZ_ASSERT(ExponentComponent(1.5) == 0);
  MOZ_ASSERT(ExponentComponent(2.0) == 1);
  MOZ_ASSERT(ExponentComponent(7.0) == 2);
  MOZ_ASSERT(ExponentComponent(PositiveInfinity<double>()) == FloatingPoint<double>::ExponentBias + 1);
  MOZ_ASSERT(ExponentComponent(NegativeInfinity<double>()) == FloatingPoint<double>::ExponentBias + 1);
  MOZ_ASSERT(ExponentComponent(UnspecifiedNaN<double>()) == FloatingPoint<double>::ExponentBias + 1);
}

static void
TestPredicates()
{
  MOZ_ASSERT(IsNaN(UnspecifiedNaN<double>()));
  MOZ_ASSERT(IsNaN(SpecificNaN<double>(1, 17)));;
  MOZ_ASSERT(IsNaN(SpecificNaN<double>(0, 0xfffffffffff0fULL)));
  MOZ_ASSERT(!IsNaN(0.0));
  MOZ_ASSERT(!IsNaN(-0.0));
  MOZ_ASSERT(!IsNaN(1.0));
  MOZ_ASSERT(!IsNaN(PositiveInfinity<double>()));
  MOZ_ASSERT(!IsNaN(NegativeInfinity<double>()));

  MOZ_ASSERT(IsInfinite(PositiveInfinity<double>()));
  MOZ_ASSERT(IsInfinite(NegativeInfinity<double>()));
  MOZ_ASSERT(!IsInfinite(UnspecifiedNaN<double>()));
  MOZ_ASSERT(!IsInfinite(0.0));
  MOZ_ASSERT(!IsInfinite(-0.0));
  MOZ_ASSERT(!IsInfinite(1.0));

  MOZ_ASSERT(!IsFinite(PositiveInfinity<double>()));
  MOZ_ASSERT(!IsFinite(NegativeInfinity<double>()));
  MOZ_ASSERT(!IsFinite(UnspecifiedNaN<double>()));
  MOZ_ASSERT(IsFinite(0.0));
  MOZ_ASSERT(IsFinite(-0.0));
  MOZ_ASSERT(IsFinite(1.0));

  MOZ_ASSERT(!IsNegative(PositiveInfinity<double>()));
  MOZ_ASSERT(IsNegative(NegativeInfinity<double>()));
  MOZ_ASSERT(IsNegative(-0.0));
  MOZ_ASSERT(!IsNegative(0.0));
  MOZ_ASSERT(IsNegative(-1.0));
  MOZ_ASSERT(!IsNegative(1.0));

  MOZ_ASSERT(!IsNegativeZero(PositiveInfinity<double>()));
  MOZ_ASSERT(!IsNegativeZero(NegativeInfinity<double>()));
  MOZ_ASSERT(!IsNegativeZero(SpecificNaN<double>(1, 17)));;
  MOZ_ASSERT(!IsNegativeZero(SpecificNaN<double>(1, 0xfffffffffff0fULL)));
  MOZ_ASSERT(!IsNegativeZero(SpecificNaN<double>(0, 17)));;
  MOZ_ASSERT(!IsNegativeZero(SpecificNaN<double>(0, 0xfffffffffff0fULL)));
  MOZ_ASSERT(!IsNegativeZero(UnspecifiedNaN<double>()));
  MOZ_ASSERT(IsNegativeZero(-0.0));
  MOZ_ASSERT(!IsNegativeZero(0.0));
  MOZ_ASSERT(!IsNegativeZero(-1.0));
  MOZ_ASSERT(!IsNegativeZero(1.0));

  int32_t i;
  MOZ_ASSERT(NumberIsInt32(0.0, &i)); MOZ_ASSERT(i == 0);
  MOZ_ASSERT(!NumberIsInt32(-0.0, &i));
  MOZ_ASSERT(NumberEqualsInt32(0.0, &i)); MOZ_ASSERT(i == 0);
  MOZ_ASSERT(NumberEqualsInt32(-0.0, &i)); MOZ_ASSERT(i == 0);
  MOZ_ASSERT(NumberIsInt32(double(INT32_MIN), &i)); MOZ_ASSERT(i == INT32_MIN);
  MOZ_ASSERT(NumberIsInt32(double(INT32_MAX), &i)); MOZ_ASSERT(i == INT32_MAX);
  MOZ_ASSERT(NumberEqualsInt32(double(INT32_MIN), &i)); MOZ_ASSERT(i == INT32_MIN);
  MOZ_ASSERT(NumberEqualsInt32(double(INT32_MAX), &i)); MOZ_ASSERT(i == INT32_MAX);
  MOZ_ASSERT(!NumberIsInt32(0.5, &i));
  MOZ_ASSERT(!NumberIsInt32(double(INT32_MAX) + 0.1, &i));
  MOZ_ASSERT(!NumberIsInt32(double(INT32_MIN) - 0.1, &i));
  MOZ_ASSERT(!NumberIsInt32(NegativeInfinity<double>(), &i));
  MOZ_ASSERT(!NumberIsInt32(PositiveInfinity<double>(), &i));
  MOZ_ASSERT(!NumberIsInt32(UnspecifiedNaN<double>(), &i));
  MOZ_ASSERT(!NumberEqualsInt32(0.5, &i));
  MOZ_ASSERT(!NumberEqualsInt32(double(INT32_MAX) + 0.1, &i));
  MOZ_ASSERT(!NumberEqualsInt32(double(INT32_MIN) - 0.1, &i));
  MOZ_ASSERT(!NumberEqualsInt32(NegativeInfinity<double>(), &i));
  MOZ_ASSERT(!NumberEqualsInt32(PositiveInfinity<double>(), &i));
  MOZ_ASSERT(!NumberEqualsInt32(UnspecifiedNaN<double>(), &i));
}

static void
TestFloatsAreApproximatelyEqual()
{
  float epsilon = mozilla::detail::FuzzyEqualsEpsilon<float>::value();
  float lessThanEpsilon = epsilon / 2.0f;
  float moreThanEpsilon = epsilon * 2.0f;

  
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0f, 1.0f + lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0f, 1.0f - lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0f, 1.0f + epsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0f, 1.0f - epsilon));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0f, 1.0f + moreThanEpsilon));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0f, 1.0f - moreThanEpsilon));
  
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e2f, 1.0e2f + lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e2f, 1.0e2f + epsilon));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e2f, 1.0e2f + moreThanEpsilon));
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e-10f, 1.0e-10f + lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e-10f, 1.0e-10f + epsilon));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e-10f, 1.0e-10f + moreThanEpsilon));
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e-6f, -1.0e-6f));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e-5f, -1.0e-5f));
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e-5f, 1.0e-5f + 1.0e-10f, 1.0e-9f));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e-5f, 1.0e-5f + 1.0e-10f, 1.0e-11f));
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e20f, 1.0e20f + 1.0e15f, 1.0e16f));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e20f, 1.0e20f + 1.0e15f, 1.0e14f));

  
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0f, 1.0f + lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0f, 1.0f - lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0f, 1.0f + epsilon));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0f, 1.0f - epsilon));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0f, 1.0f + moreThanEpsilon));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0f, 1.0f - moreThanEpsilon));
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e10f, 1.0e10f + (lessThanEpsilon * 1.0e10f)));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e10f, 1.0e10f + (moreThanEpsilon * 1.0e10f)));
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e-10f, 1.0e-10f + (lessThanEpsilon * 1.0e-10f)));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e-10f, 1.0e-10f + (moreThanEpsilon * 1.0e-10f)));
  
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e-6f, -1.0e-6f));
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e-6f, -1.0e-6f, 1.0e2f));
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e-5f, 1.0e-5f + 1.0e-10f, 1.0e-4f));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e-5f, 1.0e-5f + 1.0e-10f, 1.0e-5f));
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0f, 2.0f, 1.0f));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0f, 2.0f, 0.1f));

  
  float oneThird = 10.0f / 3.0f;
  MOZ_ASSERT(FuzzyEqualsAdditive(10.0f, 3.0f * oneThird));
  MOZ_ASSERT(FuzzyEqualsMultiplicative(10.0f, 3.0f * oneThird));
  
  MOZ_ASSERT(!FuzzyEqualsAdditive(SpecificNaN<float>(1, 1), SpecificNaN<float>(1, 1)));
  MOZ_ASSERT(!FuzzyEqualsAdditive(SpecificNaN<float>(1, 2), SpecificNaN<float>(0, 8)));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(SpecificNaN<float>(1, 1), SpecificNaN<float>(1, 1)));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(SpecificNaN<float>(1, 2), SpecificNaN<float>(0, 200)));
}

static void
TestDoublesAreApproximatelyEqual()
{
  double epsilon = mozilla::detail::FuzzyEqualsEpsilon<double>::value();
  double lessThanEpsilon = epsilon / 2.0;
  double moreThanEpsilon = epsilon * 2.0;

  
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0, 1.0 + lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0, 1.0 - lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0, 1.0 + epsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0, 1.0 - epsilon));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0, 1.0 + moreThanEpsilon));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0, 1.0 - moreThanEpsilon));
  
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e4, 1.0e4 + lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e4, 1.0e4 + epsilon));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e4, 1.0e4 + moreThanEpsilon));
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e-25, 1.0e-25 + lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e-25, 1.0e-25 + epsilon));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e-25, 1.0e-25 + moreThanEpsilon));
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e-13, -1.0e-13));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e-12, -1.0e-12));
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e-15, 1.0e-15 + 1.0e-30, 1.0e-29));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e-15, 1.0e-15 + 1.0e-30, 1.0e-31));
  
  MOZ_ASSERT(FuzzyEqualsAdditive(1.0e40, 1.0e40 + 1.0e25, 1.0e26));
  MOZ_ASSERT(!FuzzyEqualsAdditive(1.0e40, 1.0e40 + 1.0e25, 1.0e24));

  
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0, 1.0 + lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0, 1.0 - lessThanEpsilon));
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0, 1.0 + epsilon));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0, 1.0 - epsilon));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0, 1.0 + moreThanEpsilon));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0, 1.0 - moreThanEpsilon));
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e30, 1.0e30 + (lessThanEpsilon * 1.0e30)));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e30, 1.0e30 + (moreThanEpsilon * 1.0e30)));
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e-30, 1.0e-30 + (lessThanEpsilon * 1.0e-30)));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e-30, 1.0e-30 + (moreThanEpsilon * 1.0e-30)));
  
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e-6, -1.0e-6));
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e-6, -1.0e-6, 1.0e2));
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e-15, 1.0e-15 + 1.0e-30, 1.0e-15));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e-15, 1.0e-15 + 1.0e-30, 1.0e-16));
  
  MOZ_ASSERT(FuzzyEqualsMultiplicative(1.0e40, 2.0e40, 1.0));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(1.0e40, 2.0e40, 0.1));

  
  double oneThird = 10.0 / 3.0;
  MOZ_ASSERT(FuzzyEqualsAdditive(10.0, 3.0 * oneThird));
  MOZ_ASSERT(FuzzyEqualsMultiplicative(10.0, 3.0 * oneThird));
  
  MOZ_ASSERT(!FuzzyEqualsAdditive(SpecificNaN<double>(1, 1), SpecificNaN<double>(1, 1)));
  MOZ_ASSERT(!FuzzyEqualsAdditive(SpecificNaN<double>(1, 2), SpecificNaN<double>(0, 8)));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(SpecificNaN<double>(1, 1), SpecificNaN<double>(1, 1)));
  MOZ_ASSERT(!FuzzyEqualsMultiplicative(SpecificNaN<double>(1, 2), SpecificNaN<double>(0, 200)));
}

static void
TestAreApproximatelyEqual()
{
  TestFloatsAreApproximatelyEqual();
  TestDoublesAreApproximatelyEqual();
}

int
main()
{
  TestDoublesAreIdentical();
  TestExponentComponent();
  TestPredicates();
  TestAreApproximatelyEqual();
}
