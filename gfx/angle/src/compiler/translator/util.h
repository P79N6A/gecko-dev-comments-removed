





#ifndef COMPILER_UTIL_H
#define COMPILER_UTIL_H

#include "compiler/translator/Types.h"
#include "angle_gl.h"
#include "common/shadervars.h"





extern bool atof_clamp(const char *str, float *value);



extern bool atoi_clamp(const char *str, int *value);

namespace sh
{

GLenum GLVariableType(const TType &type);
GLenum GLVariablePrecision(const TType &type);
bool IsVaryingIn(TQualifier qualifier);
bool IsVaryingOut(TQualifier qualifier);
bool IsVarying(TQualifier qualifier);
InterpolationType GetInterpolationType(TQualifier qualifier);
TString ArrayString(const TType &type);

}

#endif 
