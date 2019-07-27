





#include "mozilla/MathAlgorithms.h"

using mozilla::CountPopulation32;

static void
TestCountPopulation32()
{
  MOZ_RELEASE_ASSERT(CountPopulation32(0xFFFFFFFF) == 32);
  MOZ_RELEASE_ASSERT(CountPopulation32(0xF0FF1000) == 13);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x7F8F0001) == 13);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x3FFF0100) == 15);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x1FF50010) == 12);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x00800000) == 1);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x00400000) == 1);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x00008000) == 1);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x00004000) == 1);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x00000080) == 1);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x00000040) == 1);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x00000001) == 1);
  MOZ_RELEASE_ASSERT(CountPopulation32(0x00000000) == 0);
}

int
main()
{
  TestCountPopulation32();
  return 0;
}
