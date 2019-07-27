





#include "mozilla/TemplateLib.h"

using mozilla::tl::And;

static_assert(And<>::value == true,
              "And<>::value should be true");
static_assert(And<true>::value == true,
              "And<true>::value should be true");
static_assert(And<false>::value == false,
              "And<false>::value should be false");
static_assert(And<false, true>::value == false,
              "And<false, true>::value should be false");
static_assert(And<false, false>::value == false,
              "And<false, false>::value should be false");
static_assert(And<true, false>::value == false,
              "And<true, false>::value should be false");
static_assert(And<true, true>::value == true,
              "And<true, true>::value should be true");
static_assert(And<true, true, true>::value == true,
              "And<true, true, true>::value should be true");
static_assert(And<true, false, true>::value == false,
              "And<true, false, true>::value should be false");

int
main()
{
  
  return 0;
}
