




#ifndef WEBGLPROGRAM_H_
#define WEBGLPROGRAM_H_

#include "WebGLObjectModel.h"
#include "WebGLShader.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"
#include "mozilla/CheckedInt.h"

namespace mozilla {

typedef nsDataHashtable<nsCStringHashKey, nsCString> CStringMap;
typedef nsDataHashtable<nsCStringHashKey, WebGLUniformInfo> CStringToUniformInfoMap;

class WebGLProgram MOZ_FINAL
    : public nsISupports
    , public WebGLRefCountedObject<WebGLProgram>
    , public LinkedListElement<WebGLProgram>
    , public WebGLContextBoundObject
    , public nsWrapperCache
{
public:
    WebGLProgram(WebGLContext *context);

    ~WebGLProgram() {
        DeleteOnce();
    }

    void Delete();

    void DetachShaders() {
        mAttachedShaders.Clear();
    }

    WebGLuint GLName() { return mGLName; }
    const nsTArray<WebGLRefPtr<WebGLShader> >& AttachedShaders() const { return mAttachedShaders; }
    bool LinkStatus() { return mLinkStatus; }
    uint32_t Generation() const { return mGeneration.value(); }
    void SetLinkStatus(bool val) { mLinkStatus = val; }

    bool ContainsShader(WebGLShader *shader) {
        return mAttachedShaders.Contains(shader);
    }

    
    bool AttachShader(WebGLShader *shader);

    
    bool DetachShader(WebGLShader *shader);

    bool HasAttachedShaderOfType(GLenum shaderType);

    bool HasBothShaderTypesAttached() {
        return
            HasAttachedShaderOfType(LOCAL_GL_VERTEX_SHADER) &&
            HasAttachedShaderOfType(LOCAL_GL_FRAGMENT_SHADER);
    }

    bool HasBadShaderAttached();

    size_t UpperBoundNumSamplerUniforms();

    bool NextGeneration()
    {
        if (!(mGeneration + 1).isValid())
            return false; 
        ++mGeneration;
        return true;
    }

    
    bool UpdateInfo();

    
    bool IsAttribInUse(unsigned i) const { return mAttribsInUse[i]; }

    


    void MapIdentifier(const nsACString& name, nsCString *mappedName);

    


    void ReverseMapIdentifier(const nsACString& name, nsCString *reverseMappedName);

    




    WebGLUniformInfo GetUniformInfoForMappedIdentifier(const nsACString& name);

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(WebGLProgram)

protected:

    WebGLuint mGLName;
    bool mLinkStatus;
    
    nsTArray<WebGLRefPtr<WebGLShader> > mAttachedShaders;
    CheckedUint32 mGeneration;

    
    nsTArray<bool> mAttribsInUse;
    nsAutoPtr<CStringMap> mIdentifierMap, mIdentifierReverseMap;
    nsAutoPtr<CStringToUniformInfoMap> mUniformInfoMap;
    int mAttribMaxNameLength;
};

} 

#endif
