








#ifndef LIBGLESV2_RENDERER_SHADER_CACHE_H_
#define LIBGLESV2_RENDERER_SHADER_CACHE_H_

#include "common/debug.h"

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

    ShaderObject *create(const DWORD *function, size_t length)
    {
        std::string key(reinterpret_cast<const char*>(function), length);
        typename Map::iterator it = mMap.find(key);
        if (it != mMap.end())
        {
            it->second->AddRef();
            return it->second;
        }

        ShaderObject *shader;
        HRESULT result = createShader(function, &shader);
        if (FAILED(result))
        {
            return NULL;
        }

        
        if (mMap.size() >= kMaxMapSize)
        {
            mMap.begin()->second->Release();
            mMap.erase(mMap.begin());
        }

        shader->AddRef();
        mMap[key] = shader;

        return shader;
    }

    void clear()
    {
        for (typename Map::iterator it = mMap.begin(); it != mMap.end(); ++it)
        {
            it->second->Release();
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

#ifndef HASH_MAP
# ifdef _MSC_VER
#  define HASH_MAP stdext::hash_map
# else
#  define HASH_MAP std::unordered_map
# endif
#endif

    typedef HASH_MAP<std::string, ShaderObject*> Map;
    Map mMap;

    IDirect3DDevice9 *mDevice;
};

typedef ShaderCache<IDirect3DVertexShader9> VertexShaderCache;
typedef ShaderCache<IDirect3DPixelShader9> PixelShaderCache;

}

#endif   
