




#include "mozilla/MathAlgorithms.h"

using mozilla::CeilingLog2;
using mozilla::FloorLog2;

static void
TestCeiling()
{
  for (uint32_t i = 0; i <= 1; i++)
    MOZ_ASSERT(CeilingLog2(i) == 0);

  for (uint32_t i = 2; i <= 2; i++)
    MOZ_ASSERT(CeilingLog2(i) == 1);

  for (uint32_t i = 3; i <= 4; i++)
    MOZ_ASSERT(CeilingLog2(i) == 2);

  for (uint32_t i = 5; i <= 8; i++)
    MOZ_ASSERT(CeilingLog2(i) == 3);

  for (uint32_t i = 9; i <= 16; i++)
    MOZ_ASSERT(CeilingLog2(i) == 4);
}

static void
TestFloor()
{
  for (uint32_t i = 0; i <= 1; i++)
    MOZ_ASSERT(FloorLog2(i) == 0);

  for (uint32_t i = 2; i <= 3; i++)
    MOZ_ASSERT(FloorLog2(i) == 1);

  for (uint32_t i = 4; i <= 7; i++)
    MOZ_ASSERT(FloorLog2(i) == 2);

  for (uint32_t i = 8; i <= 15; i++)
    MOZ_ASSERT(FloorLog2(i) == 3);

  for (uint32_t i = 16; i <= 31; i++)
    MOZ_ASSERT(FloorLog2(i) == 4);
}

int main()
{
  TestCeiling();
  TestFloor();
  return 0;
}
