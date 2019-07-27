#ifndef LIBGLESV2_RENDERER_HLSL_D3DCOMPILER_H_
#define LIBGLESV2_RENDERER_HLSL_D3DCOMPILER_H_

#include "libGLESv2/Error.h"

#include "common/angleutils.h"
#include "common/platform.h"

#include <vector>
#include <string>

namespace gl
{
class InfoLog;
}

namespace rx
{

struct CompileConfig
{
    UINT flags;
    std::string name;

    CompileConfig();
    CompileConfig(UINT flags, const std::string &name);
};

class HLSLCompiler
{
  public:
    HLSLCompiler();
    ~HLSLCompiler();

    bool initialize();
    void release();

    
    
    gl::Error compileToBinary(gl::InfoLog &infoLog, const std::string &hlsl, const std::string &profile,
                              const std::vector<CompileConfig> &configs, ID3DBlob **outCompiledBlob) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(HLSLCompiler);

    HMODULE mD3DCompilerModule;
    pD3DCompile mD3DCompileFunc;
};

}

#endif 
