








#ifndef LIBGLESV2_RENDERER_SHADEREXECUTABLE9_H_
#define LIBGLESV2_RENDERER_SHADEREXECUTABLE9_H_

#include "libGLESv2/renderer/ShaderExecutable.h"

namespace rx
{

class ShaderExecutable9 : public ShaderExecutable
{
  public:
    ShaderExecutable9(const void *function, size_t length, IDirect3DPixelShader9 *executable);
    ShaderExecutable9(const void *function, size_t length, IDirect3DVertexShader9 *executable);
    virtual ~ShaderExecutable9();

    static ShaderExecutable9 *makeShaderExecutable9(ShaderExecutable *executable);

    IDirect3DPixelShader9 *getPixelShader() const;
    IDirect3DVertexShader9 *getVertexShader() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderExecutable9);

    IDirect3DPixelShader9 *mPixelExecutable;
    IDirect3DVertexShader9 *mVertexExecutable;
};

}

#endif 