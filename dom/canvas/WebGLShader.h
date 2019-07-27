




#ifndef WEBGLSHADER_H_
#define WEBGLSHADER_H_

#include "WebGLObjectModel.h"
#include "WebGLUniformInfo.h"

#include "nsWrapperCache.h"

#include "angle/ShaderLang.h"

#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {

struct WebGLMappedIdentifier {
    nsCString original, mapped; 
    WebGLMappedIdentifier(const nsACString& o, const nsACString& m) : original(o), mapped(m) {}
};

class WebGLShader MOZ_FINAL
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLShader>
    , public LinkedListElement<WebGLShader>
    , public WebGLContextBoundObject
{
    friend class WebGLContext;
    friend class WebGLProgram;

public:
    WebGLShader(WebGLContext *context, GLenum stype);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

    GLuint GLName() { return mGLName; }
    sh::GLenum ShaderType() { return mType; }

    void SetSource(const nsAString& src) {
        
        mSource.Assign(src);
    }

    const nsString& Source() const { return mSource; }

    void SetNeedsTranslation() { mNeedsTranslation = true; }
    bool NeedsTranslation() const { return mNeedsTranslation; }

    void SetCompileStatus (bool status) {
        mCompileStatus = status;
    }

    void Delete();

    bool CompileStatus() const {
        return mCompileStatus;
    }

    void SetTranslationSuccess();

    void SetTranslationFailure(const nsCString& msg) {
        mTranslationLog.Assign(msg); 
    }

    const nsCString& TranslationLog() const { return mTranslationLog; }

    const nsString& TranslatedSource() const { return mTranslatedSource; }

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLShader)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLShader)

protected:
    ~WebGLShader() {
        DeleteOnce();
    }

    GLuint mGLName;
    sh::GLenum mType;
    nsString mSource;
    nsString mTranslatedSource;
    nsCString mTranslationLog; 
    bool mNeedsTranslation;
    nsTArray<WebGLMappedIdentifier> mAttributes;
    nsTArray<WebGLMappedIdentifier> mUniforms;
    nsTArray<WebGLUniformInfo> mUniformInfos;
    int mAttribMaxNameLength;
    bool mCompileStatus;
};
} 

#endif
