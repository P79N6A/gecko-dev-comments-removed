




#ifndef WEBGL_PROGRAM_H_
#define WEBGL_PROGRAM_H_

#include <map>
#include "mozilla/CheckedInt.h"
#include "mozilla/LinkedList.h"
#include "nsWrapperCache.h"
#include "WebGLObjectModel.h"
#include "WebGLShader.h"
#include "WebGLUniformInfo.h"

namespace mozilla {

class WebGLShader;
struct WebGLUniformInfo;

typedef nsDataHashtable<nsCStringHashKey, nsCString> CStringMap;
typedef nsDataHashtable<nsCStringHashKey,
                        WebGLUniformInfo> CStringToUniformInfoMap;

class WebGLProgram MOZ_FINAL
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLProgram>
    , public LinkedListElement<WebGLProgram>
    , public WebGLContextBoundObject
{
public:
    explicit WebGLProgram(WebGLContext* webgl);

    void Delete();

    void DetachShaders() {
        mAttachedShaders.Clear();
    }

    GLuint GLName() { return mGLName; }
    const nsTArray<WebGLRefPtr<WebGLShader> >& AttachedShaders() const {
        return mAttachedShaders;
    }
    bool LinkStatus() { return mLinkStatus; }
    uint32_t Generation() const { return mGeneration.value(); }
    void SetLinkStatus(bool val) { mLinkStatus = val; }

    bool ContainsShader(WebGLShader* shader) {
        return mAttachedShaders.Contains(shader);
    }

    
    bool AttachShader(WebGLShader* shader);

    
    bool DetachShader(WebGLShader* shader);

    bool HasAttachedShaderOfType(GLenum shaderType);

    bool HasBothShaderTypesAttached() {
        return HasAttachedShaderOfType(LOCAL_GL_VERTEX_SHADER) &&
               HasAttachedShaderOfType(LOCAL_GL_FRAGMENT_SHADER);
    }

    bool HasBadShaderAttached();

    size_t UpperBoundNumSamplerUniforms();

    bool NextGeneration() {
        if (!(mGeneration + 1).isValid())
            return false; 

        ++mGeneration;
        return true;
    }

    
    bool UpdateInfo();

    
    bool IsAttribInUse(uint32_t i) const { return mAttribsInUse[i]; }

    
    
    void MapIdentifier(const nsACString& name, nsCString* out_mappedName);

    
    
    
    void ReverseMapIdentifier(const nsACString& name,
                              nsCString* out_reverseMappedName);

    





    WebGLUniformInfo GetUniformInfoForMappedIdentifier(const nsACString& name);

    WebGLContext* GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext* cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLProgram)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLProgram)

    
    std::map<GLint, nsCString> mActiveAttribMap;

    static uint64_t IdentifierHashFunction(const char* ident, size_t size);
    static void HashMapIdentifier(const nsACString& name,
                                  nsCString* const out_hashedName);

protected:
    ~WebGLProgram() {
        DeleteOnce();
    }

    GLuint mGLName;
    bool mLinkStatus;
    
    nsTArray<WebGLRefPtr<WebGLShader> > mAttachedShaders;
    CheckedUint32 mGeneration;

    
    FallibleTArray<bool> mAttribsInUse;
    nsAutoPtr<CStringMap> mIdentifierMap, mIdentifierReverseMap;
    nsAutoPtr<CStringToUniformInfoMap> mUniformInfoMap;
    int mAttribMaxNameLength;
};

} 

#endif 
