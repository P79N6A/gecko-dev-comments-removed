








#ifndef SkParsePath_DEFINED
#define SkParsePath_DEFINED

#include "SkPath.h"

class SkString;

class SkParsePath {
public:
    static bool FromSVGString(const char str[], SkPath*);
    static void ToSVGString(const SkPath&, SkString*);
};

#endif

