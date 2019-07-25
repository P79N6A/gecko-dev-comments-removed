






































#ifndef COMPATIBILITY_MANAGER_H
#define COMPATIBILITY_MANAGER_H

#include "prtypes.h"

class nsAccessNodeWrap;

namespace mozilla {
namespace a11y {





class Compatibility
{
public:
  


  static bool IsIA2Off() { return sMode & IA2OffMode; }

  


  static bool IsJAWS() { return sMode & JAWSMode; }

  


  static bool IsWE() { return sMode & WEMode; }

  


  static bool IsDolphin() { return sMode & DolphinMode; }

private:
  Compatibility();
  Compatibility(const Compatibility&);
  Compatibility& operator = (const Compatibility&);

  



  static void Init();
  friend class nsAccessNodeWrap;

  


  enum {
    NoCompatibilityMode = 0,
    JAWSMode = 1 << 0,
    WEMode = 1 << 1,
    DolphinMode = 1 << 2,
    IA2OffMode = 1 << 3
  };

private:
  static PRUint32 sMode;
};

} 
} 

#endif
