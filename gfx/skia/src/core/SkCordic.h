








#ifndef SkCordic_DEFINED
#define SkCordic_DEFINED

#include "SkTypes.h"
#include "SkFixed.h"

SkFixed SkCordicACos(SkFixed a);
SkFixed SkCordicASin(SkFixed a);
SkFixed SkCordicATan2(SkFixed y, SkFixed x);
SkFixed SkCordicExp(SkFixed a);
SkFixed SkCordicLog(SkFixed a);
SkFixed SkCordicSinCos(SkFixed radians, SkFixed* cosp);
SkFixed SkCordicTan(SkFixed a);

#ifdef SK_DEBUG
    void SkCordic_UnitTest();
#endif

#endif 

