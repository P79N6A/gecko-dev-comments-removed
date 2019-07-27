





#ifndef COMPILER_UTIL_H
#define COMPILER_UTIL_H

#include <stack>

#include "angle_gl.h"
#include <GLSLANG/ShaderLang.h>

#include "compiler/translator/Types.h"





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
BlockLayoutType GetBlockLayoutType(TLayoutBlockStorage blockStorage);
TString ArrayString(const TType &type);

class GetVariableTraverser
{
  public:
    GetVariableTraverser() {}

    template <typename VarT>
    void traverse(const TType &type, const TString &name, std::vector<VarT> *output);

  protected:
    
    virtual void visitVariable(ShaderVariable *newVar) {}

  private:
    DISALLOW_COPY_AND_ASSIGN(GetVariableTraverser);
};

void GetInterfaceBlockFields(const TInterfaceBlock &interfaceBlock, std::vector<InterfaceBlockField> *fieldsOut);

}

#endif 
