








#ifndef LIBGLESV2_RENDERER_SHADER_CACHE_H_
#define LIBGLESV2_RENDERER_SHADER_CACHE_H_

#include "libGLESv2/Error.h"

#include "common/debug.h"

#include <cstddef>
#include <unordered_map>
#include <string>

namespace rx
{
template <typename ShaderObject>
class ShaderCache
{
  public:
    ShaderCache() : mDevice(NULL)
    {
    }

    ~ShaderCache()
    {
        
        ASSERT(mMap.empty());
    }

    void initialize(IDirect3DDevice9* device)
    {
        mDevice = device;
    }

    gl::Error create(const DWORD *function, size_t length, ShaderObject **outShaderObject)
    {
        std::string key(reinterpret_cast<const char*>(function), length);
        typename Map::iterator it = mMap.find(key);
        if (it != mMap.end())
        {
            it->second->AddRef();
            *outShaderObject = it->second;
            return gl::Error(GL_NO_ERROR);
        }

        ShaderObject *shader;
        HRESULT result = createShader(function, &shader);
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create shader, result: 0x%X.", result);
        }

        
        if (mMap.size() >= kMaxMapSize)
        {
            SafeRelease(mMap.begin()->second);
            mMap.erase(mMap.begin());
        }

        shader->AddRef();
        mMap[key] = shader;

        *outShaderObject = shader;
        return gl::Error(GL_NO_ERROR);
    }

    void clear()
    {
        for (typename Map::iterator it = mMap.begin(); it != mMap.end(); ++it)
        {
            SafeRelease(it->second);
        }

        mMap.clear();
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderCache);

    const static size_t kMaxMapSize = 100;

    HRESULT createShader(const DWORD *function, IDirect3DVertexShader9 **shader)
    {
        return mDevice->CreateVertexShader(function, shader);
    }

    HRESULT createShader(const DWORD *function, IDirect3DPixelShader9 **shader)
    {
        return mDevice->CreatePixelShader(function, shader);
    }

    typedef std::unordered_map<std::string, ShaderObject*> Map;
    Map mMap;

    IDirect3DDevice9 *mDevice;
};

typedef ShaderCache<IDirect3DVertexShader9> VertexShaderCache;
typedef ShaderCache<IDirect3DPixelShader9> PixelShaderCache;

}

#endif   
