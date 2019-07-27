




#include "WebGLContext.h"
#include "WebGLShader.h"
#include "WebGLProgram.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "GLContext.h"

#include "MurmurHash3.h"

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
WebGLProgram::WrapObject(JSContext *cx) {
    return dom::WebGLProgramBinding::Wrap(cx, this);
}

WebGLProgram::WebGLProgram(WebGLContext *context)
    : WebGLContextBoundObject(context)
    , mLinkStatus(false)
    , mGeneration(0)
    , mIdentifierMap(new CStringMap)
    , mIdentifierReverseMap(new CStringMap)
    , mUniformInfoMap(new CStringToUniformInfoMap)
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
            if (u.type == LOCAL_GL_SAMPLER_2D ||
                u.type == LOCAL_GL_SAMPLER_CUBE)
            {
                numSamplerUniforms += u.arraySize;
            }
        }
    }
    return numSamplerUniforms;
}

void
WebGLProgram::MapIdentifier(const nsACString& name, nsCString *mappedName) {
    MOZ_ASSERT(mIdentifierMap);

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
    MOZ_ASSERT(mIdentifierReverseMap);

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
    MOZ_ASSERT(mUniformInfoMap);

    nsCString mutableName(name);
    nsCString bracketPart;
    bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
    
    if (hadBracketPart)
        mutableName.AppendLiteral("[0]");

    WebGLUniformInfo info;
    mUniformInfoMap->Get(mutableName, &info);
    

    return info;
}

 uint64_t
WebGLProgram::IdentifierHashFunction(const char *ident, size_t size)
{
    uint64_t outhash[2];
    
    
    MurmurHash3_x86_128(ident, size, 0, &outhash[0]);
    return outhash[0];
}

 void
WebGLProgram::HashMapIdentifier(const nsACString& name, nsCString *hashedName)
{
    uint64_t hash = IdentifierHashFunction(name.BeginReading(), name.Length());
    hashedName->Truncate();
    
    hashedName->AppendPrintf("webgl_%llx", hash);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(WebGLProgram, mAttachedShaders)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLProgram, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLProgram, Release)
