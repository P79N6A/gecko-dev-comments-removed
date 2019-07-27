





#ifndef COMPATIBILITY_MANAGER_H
#define COMPATIBILITY_MANAGER_H

#include <stdint.h>

namespace mozilla {
namespace a11y {





class Compatibility
{
public:
  


  static bool IsIA2Off() { return !!(sConsumers & OLDJAWS); }

  


  static bool IsJAWS() { return !!(sConsumers & (JAWS | OLDJAWS)); }

  


  static bool IsWE() { return !!(sConsumers & WE); }

  


  static bool IsDolphin() { return !!(sConsumers & DOLPHIN); }

private:
  Compatibility();
  Compatibility(const Compatibility&);
  Compatibility& operator = (const Compatibility&);

  



  static void Init();
  friend void PlatformInit();

  


  enum {
    NVDA = 1 << 0,
    JAWS = 1 << 1,
    OLDJAWS = 1 << 2,
    WE = 1 << 3,
    DOLPHIN = 1 << 4,
    SEROTEK = 1 << 5,
    COBRA = 1 << 6,
    ZOOMTEXT = 1 << 7,
    KAZAGURU = 1 << 8,
    YOUDAO = 1 << 9,
    UNKNOWN = 1 << 10,
    UIAUTOMATION = 1 << 11
  };

private:
  static uint32_t sConsumers;
};

} 
} 

#endif
