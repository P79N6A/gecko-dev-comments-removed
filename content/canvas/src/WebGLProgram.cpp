




#include "WebGLContext.h"
#include "WebGLProgram.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "nsContentUtils.h"

using namespace mozilla;








static bool SplitLastSquareBracket(nsACString& string, nsCString& bracketPart)
{
    MOZ_ASSERT(bracketPart.IsEmpty(), "SplitLastSquareBracket must be called with empty bracketPart string");

    if (string.IsEmpty())
        return false;

    char *string_start = string.BeginWriting();
    char *s = string_start + string.Length() - 1;

    if (*s != ']')
        return false;

    while (*s != '[' && s != string_start)
        s--;

    if (*s != '[')
        return false;

    bracketPart.Assign(s);
    *s = 0;
    string.EndWriting();
    string.SetLength(s - string_start);
    return true;
}

JSObject*
WebGLProgram::WrapObject(JSContext *cx, JSObject *scope) {
    return dom::WebGLProgramBinding::Wrap(cx, scope, this);
}

WebGLProgram::WebGLProgram(WebGLContext *context)
    : WebGLContextBoundObject(context)
    , mLinkStatus(false)
    , mGeneration(0)
    , mAttribMaxNameLength(0)
{
    SetIsDOMBinding();
    mContext->MakeContextCurrent();
    mGLName = mContext->gl->fCreateProgram();
    mContext->mPrograms.insertBack(this);
}

void
WebGLProgram::Delete() {
    DetachShaders();
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteProgram(mGLName);
    LinkedListElement<WebGLProgram>::removeFrom(mContext->mPrograms);
}

bool
WebGLProgram::AttachShader(WebGLShader *shader) {
    if (ContainsShader(shader))
        return false;
    mAttachedShaders.AppendElement(shader);

    mContext->MakeContextCurrent();
    mContext->gl->fAttachShader(GLName(), shader->GLName());

    return true;
}

bool
WebGLProgram::DetachShader(WebGLShader *shader) {
    if (!mAttachedShaders.RemoveElement(shader))
        return false;

    mContext->MakeContextCurrent();
    mContext->gl->fDetachShader(GLName(), shader->GLName());

    return true;
}

bool
WebGLProgram::HasAttachedShaderOfType(GLenum shaderType) {
    for (uint32_t i = 0; i < mAttachedShaders.Length(); ++i) {
        if (mAttachedShaders[i] && mAttachedShaders[i]->ShaderType() == shaderType) {
            return true;
        }
    }
    return false;
}

bool
WebGLProgram::HasBadShaderAttached() {
    for (uint32_t i = 0; i < mAttachedShaders.Length(); ++i) {
        if (mAttachedShaders[i] && !mAttachedShaders[i]->CompileStatus()) {
            return true;
        }
    }
    return false;
}

size_t
WebGLProgram::UpperBoundNumSamplerUniforms() {
    size_t numSamplerUniforms = 0;
    for (size_t i = 0; i < mAttachedShaders.Length(); ++i) {
        const WebGLShader *shader = mAttachedShaders[i];
        if (!shader)
            continue;
        for (size_t j = 0; j < shader->mUniformInfos.Length(); ++j) {
            WebGLUniformInfo u = shader->mUniformInfos[j];
            if (u.type == SH_SAMPLER_2D ||
                u.type == SH_SAMPLER_CUBE)
            {
                numSamplerUniforms += u.arraySize;
            }
        }
    }
    return numSamplerUniforms;
}

void
WebGLProgram::MapIdentifier(const nsACString& name, nsCString *mappedName) {
    if (!mIdentifierMap) {
        
        mIdentifierMap = new CStringMap;
        mIdentifierMap->Init();
        for (size_t i = 0; i < mAttachedShaders.Length(); i++) {
            for (size_t j = 0; j < mAttachedShaders[i]->mAttributes.Length(); j++) {
                const WebGLMappedIdentifier& attrib = mAttachedShaders[i]->mAttributes[j];
                mIdentifierMap->Put(attrib.original, attrib.mapped);
            }
            for (size_t j = 0; j < mAttachedShaders[i]->mUniforms.Length(); j++) {
                const WebGLMappedIdentifier& uniform = mAttachedShaders[i]->mUniforms[j];
                mIdentifierMap->Put(uniform.original, uniform.mapped);
            }
        }
    }

    nsCString mutableName(name);
    nsCString bracketPart;
    bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
    if (hadBracketPart)
        mutableName.AppendLiteral("[0]");

    if (mIdentifierMap->Get(mutableName, mappedName)) {
        if (hadBracketPart) {
            nsCString mappedBracketPart;
            bool mappedHadBracketPart = SplitLastSquareBracket(*mappedName, mappedBracketPart);
            if (mappedHadBracketPart)
                mappedName->Append(bracketPart);
        }
        return;
    }

    
    
    mutableName.AppendLiteral("[0]");
    if (mIdentifierMap->Get(mutableName, mappedName))
        return;

    
    
    
    mappedName->Assign(name);
}

void
WebGLProgram::ReverseMapIdentifier(const nsACString& name, nsCString *reverseMappedName) {
    if (!mIdentifierReverseMap) {
        
        mIdentifierReverseMap = new CStringMap;
        mIdentifierReverseMap->Init();
        for (size_t i = 0; i < mAttachedShaders.Length(); i++) {
            for (size_t j = 0; j < mAttachedShaders[i]->mAttributes.Length(); j++) {
                const WebGLMappedIdentifier& attrib = mAttachedShaders[i]->mAttributes[j];
                mIdentifierReverseMap->Put(attrib.mapped, attrib.original);
            }
            for (size_t j = 0; j < mAttachedShaders[i]->mUniforms.Length(); j++) {
                const WebGLMappedIdentifier& uniform = mAttachedShaders[i]->mUniforms[j];
                mIdentifierReverseMap->Put(uniform.mapped, uniform.original);
            }
        }
    }

    nsCString mutableName(name);
    nsCString bracketPart;
    bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
    if (hadBracketPart)
        mutableName.AppendLiteral("[0]");

    if (mIdentifierReverseMap->Get(mutableName, reverseMappedName)) {
        if (hadBracketPart) {
            nsCString reverseMappedBracketPart;
            bool reverseMappedHadBracketPart = SplitLastSquareBracket(*reverseMappedName, reverseMappedBracketPart);
            if (reverseMappedHadBracketPart)
                reverseMappedName->Append(bracketPart);
        }
        return;
    }

    
    
    mutableName.AppendLiteral("[0]");
    if (mIdentifierReverseMap->Get(mutableName, reverseMappedName))
        return;

    
    
    
    reverseMappedName->Assign(name);
}

WebGLUniformInfo
WebGLProgram::GetUniformInfoForMappedIdentifier(const nsACString& name) {
    if (!mUniformInfoMap) {
        
        mUniformInfoMap = new CStringToUniformInfoMap;
        mUniformInfoMap->Init();
        for (size_t i = 0; i < mAttachedShaders.Length(); i++) {
            for (size_t j = 0; j < mAttachedShaders[i]->mUniforms.Length(); j++) {
                const WebGLMappedIdentifier& uniform = mAttachedShaders[i]->mUniforms[j];
                const WebGLUniformInfo& info = mAttachedShaders[i]->mUniformInfos[j];
                mUniformInfoMap->Put(uniform.mapped, info);
            }
        }
    }

    nsCString mutableName(name);
    nsCString bracketPart;
    bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
    
    if (hadBracketPart)
        mutableName.AppendLiteral("[0]");

    WebGLUniformInfo info;
    mUniformInfoMap->Get(mutableName, &info);
    

    
    if (hadBracketPart && !bracketPart.EqualsLiteral("[0]")) {
        info.isArray = false;
        info.arraySize = 1;
    }
    return info;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(WebGLProgram, mAttachedShaders)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLProgram)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLProgram)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLProgram)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
