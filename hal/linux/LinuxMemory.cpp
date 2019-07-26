






#include <stdio.h>
#include "Hal.h"

namespace mozilla {
namespace hal_impl {

uint32_t
GetTotalSystemMemory()
{
  static uint32_t sTotalMemory;
  static bool sTotalMemoryObtained = false;

  if (!sTotalMemoryObtained) {
    sTotalMemoryObtained = true;

    FILE* fd = fopen("/proc/meminfo", "r");
    if (!fd) {
      return 0;
    }

    int rv = fscanf(fd, "MemTotal: %i kB", &sTotalMemory);

    if (fclose(fd) || rv != 1) {
      return 0;
    }
  }

  return sTotalMemory * 1024;
}

uint32_t
GetTotalSystemMemoryLevel()
{
  static uint32_t sTotalMemoryLevel = 1;
  uint32_t sTotalMemory;
  static bool sTotalMemoryObtained = false;

  if (!sTotalMemoryObtained) {
    sTotalMemoryObtained = true;

    FILE* fd = fopen("/proc/meminfo", "r");
    if (!fd) {
      return 0;
    }

    int rv = fscanf(fd, "MemTotal: %i kB", &sTotalMemory);

    if (fclose(fd) || rv != 1) {
      return 0;
    }

    
    sTotalMemory /= 1024;

    while (sTotalMemoryLevel <= sTotalMemory) {
      sTotalMemoryLevel *= 2;
    }
  }

  return sTotalMemoryLevel;
}

} 
} 
