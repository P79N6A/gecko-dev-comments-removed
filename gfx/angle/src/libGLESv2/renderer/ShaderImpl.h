







#ifndef LIBGLESV2_RENDERER_SHADERIMPL_H_
#define LIBGLESV2_RENDERER_SHADERIMPL_H_

#include "common/angleutils.h"

namespace rx
{

class ShaderImpl
{
  public:
    virtual ~ShaderImpl() { }

    virtual bool compile(const std::string &source) = 0;
    virtual const std::string &getInfoLog() const = 0;
    virtual const std::string &getTranslatedSource() const = 0;
};

}

#endif 
