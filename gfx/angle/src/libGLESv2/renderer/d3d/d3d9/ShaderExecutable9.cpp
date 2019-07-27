








#include "libGLESv2/renderer/d3d/d3d9/ShaderExecutable9.h"

#include "common/debug.h"

namespace rx
{

ShaderExecutable9::ShaderExecutable9(const void *function, size_t length, IDirect3DPixelShader9 *executable)
    : ShaderExecutable(function, length)
{
    mPixelExecutable = executable;
    mVertexExecutable = NULL;
}

ShaderExecutable9::ShaderExecutable9(const void *function, size_t length, IDirect3DVertexShader9 *executable)
    : ShaderExecutable(function, length)
{
    mVertexExecutable = executable;
    mPixelExecutable = NULL;
}

ShaderExecutable9::~ShaderExecutable9()
{
    SafeRelease(mVertexExecutable);
    SafeRelease(mPixelExecutable);
}

ShaderExecutable9 *ShaderExecutable9::makeShaderExecutable9(ShaderExecutable *executable)
{
    ASSERT(HAS_DYNAMIC_TYPE(ShaderExecutable9*, executable));
    return static_cast<ShaderExecutable9*>(executable);
}

IDirect3DVertexShader9 *ShaderExecutable9::getVertexShader() const
{
    return mVertexExecutable;
}

IDirect3DPixelShader9 *ShaderExecutable9::getPixelShader() const
{
    return mPixelExecutable;
}

}