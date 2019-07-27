




#include "WebGLProgram.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "MurmurHash3.h"
#include "WebGLContext.h"
#include "WebGLShader.h"

namespace mozilla {











static bool
SplitLastSquareBracket(nsACString& string, nsCString& bracketPart)
{
    MOZ_ASSERT(bracketPart.IsEmpty(),
               "SplitLastSquareBracket must be called with empty bracketPart"
               " string.");

    if (string.IsEmpty())
        return false;

    char* string_start = string.BeginWriting();
    char* s = string_start + string.Length() - 1;

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
WebGLProgram::WrapObject(JSContext* cx) {
    return dom::WebGLProgramBinding::Wrap(cx, this);
}

WebGLProgram::WebGLProgram(WebGLContext* webgl)
    : WebGLContextBoundObject(webgl)
    , mLinkStatus(false)
    , mGeneration(0)
    , mIdentifierMap(new CStringMap)
    , mIdentifierReverseMap(new CStringMap)
    , mUniformInfoMap(new CStringToUniformInfoMap)
    , mAttribMaxNameLength(0)
{
    mContext->MakeContextCurrent();
    mGLName = mContext->gl->fCreateProgram();
    mContext->mPrograms.insertBack(this);
}

void
WebGLProgram::Delete()
{
    DetachShaders();
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteProgram(mGLName);
    LinkedListElement<WebGLProgram>::removeFrom(mContext->mPrograms);
}

bool
WebGLProgram::AttachShader(WebGLShader* shader)
{
    if (ContainsShader(shader))
        return false;

    mAttachedShaders.AppendElement(shader);

    mContext->MakeContextCurrent();
    mContext->gl->fAttachShader(GLName(), shader->GLName());

    return true;
}

bool
WebGLProgram::DetachShader(WebGLShader* shader)
{
    if (!mAttachedShaders.RemoveElement(shader))
        return false;

    mContext->MakeContextCurrent();
    mContext->gl->fDetachShader(GLName(), shader->GLName());

    return true;
}

bool
WebGLProgram::HasAttachedShaderOfType(GLenum shaderType)
{
    for (uint32_t i = 0; i < mAttachedShaders.Length(); ++i) {
        if (mAttachedShaders[i] && mAttachedShaders[i]->ShaderType() == shaderType)
            return true;
    }

    return false;
}

bool
WebGLProgram::HasBadShaderAttached()
{
    for (uint32_t i = 0; i < mAttachedShaders.Length(); ++i) {
        if (mAttachedShaders[i] && !mAttachedShaders[i]->CompileStatus())
            return true;
    }

    return false;
}

size_t
WebGLProgram::UpperBoundNumSamplerUniforms()
{
    size_t numSamplerUniforms = 0;

    for (size_t i = 0; i < mAttachedShaders.Length(); ++i) {
        const WebGLShader* shader = mAttachedShaders[i];
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
WebGLProgram::MapIdentifier(const nsACString& name,
                            nsCString* const out_mappedName)
{
    MOZ_ASSERT(mIdentifierMap);

    nsCString mutableName(name);
    nsCString bracketPart;
    bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
    if (hadBracketPart)
        mutableName.AppendLiteral("[0]");

    if (mIdentifierMap->Get(mutableName, out_mappedName)) {
        if (hadBracketPart) {
            nsCString mappedBracketPart;
            bool mappedHadBracketPart = SplitLastSquareBracket(*out_mappedName,
                                                               mappedBracketPart);
            if (mappedHadBracketPart)
                out_mappedName->Append(bracketPart);
        }
        return;
    }

    
    
    
    mutableName.AppendLiteral("[0]");
    if (mIdentifierMap->Get(mutableName, out_mappedName))
        return;

    




    out_mappedName->Assign(name);
}

void
WebGLProgram::ReverseMapIdentifier(const nsACString& name,
                                   nsCString* const out_reverseMappedName)
{
    MOZ_ASSERT(mIdentifierReverseMap);

    nsCString mutableName(name);
    nsCString bracketPart;
    bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
    if (hadBracketPart)
        mutableName.AppendLiteral("[0]");

    if (mIdentifierReverseMap->Get(mutableName, out_reverseMappedName)) {
        if (hadBracketPart) {
            nsCString reverseMappedBracketPart;
            bool reverseMappedHadBracketPart = SplitLastSquareBracket(*out_reverseMappedName,
                                                                      reverseMappedBracketPart);
            if (reverseMappedHadBracketPart)
                out_reverseMappedName->Append(bracketPart);
        }
        return;
    }

    
    
    
    mutableName.AppendLiteral("[0]");
    if (mIdentifierReverseMap->Get(mutableName, out_reverseMappedName))
        return;

    




    out_reverseMappedName->Assign(name);
}

WebGLUniformInfo
WebGLProgram::GetUniformInfoForMappedIdentifier(const nsACString& name)
{
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

bool
WebGLProgram::UpdateInfo()
{
    mAttribMaxNameLength = 0;
    for (size_t i = 0; i < mAttachedShaders.Length(); i++) {
        mAttribMaxNameLength = std::max(mAttribMaxNameLength,
                                        mAttachedShaders[i]->mAttribMaxNameLength);
    }

    GLint attribCount;
    mContext->gl->fGetProgramiv(mGLName, LOCAL_GL_ACTIVE_ATTRIBUTES, &attribCount);

    if (!mAttribsInUse.SetLength(mContext->mGLMaxVertexAttribs)) {
        mContext->ErrorOutOfMemory("updateInfo: Out of memory to allocate %d"
                                   " attribs.", mContext->mGLMaxVertexAttribs);
        return false;
    }

    for (size_t i = 0; i < mAttribsInUse.Length(); i++)
        mAttribsInUse[i] = false;

    nsAutoArrayPtr<char> nameBuf(new char[mAttribMaxNameLength]);

    for (int i = 0; i < attribCount; ++i) {
        GLint attrnamelen;
        GLint attrsize;
        GLenum attrtype;
        mContext->gl->fGetActiveAttrib(mGLName, i, mAttribMaxNameLength,
                                       &attrnamelen, &attrsize, &attrtype,
                                       nameBuf);
        if (attrnamelen > 0) {
            GLint loc = mContext->gl->fGetAttribLocation(mGLName, nameBuf);
            MOZ_ASSERT(loc >= 0, "Major oops in managing the attributes of a"
                                 " WebGL program.");
            if (loc < mContext->mGLMaxVertexAttribs) {
                mAttribsInUse[loc] = true;
            } else {
                mContext->GenerateWarning("Program exceeds MAX_VERTEX_ATTRIBS.");
                return false;
            }
        }
    }

    
    mIdentifierMap = new CStringMap;
    mIdentifierReverseMap = new CStringMap;
    mUniformInfoMap = new CStringToUniformInfoMap;
    for (size_t i = 0; i < mAttachedShaders.Length(); i++) {
        
        for (size_t j = 0; j < mAttachedShaders[i]->mAttributes.Length(); j++) {
            const WebGLMappedIdentifier& attrib = mAttachedShaders[i]->mAttributes[j];

            
            mIdentifierMap->Put(attrib.original, attrib.mapped);
            
            mIdentifierReverseMap->Put(attrib.mapped, attrib.original);
        }

        
        for (size_t j = 0; j < mAttachedShaders[i]->mUniforms.Length(); j++) {
            
            const WebGLMappedIdentifier& uniform = mAttachedShaders[i]->mUniforms[j];

            
            mIdentifierMap->Put(uniform.original, uniform.mapped);
            
            mIdentifierReverseMap->Put(uniform.mapped, uniform.original);

            
            const WebGLUniformInfo& info = mAttachedShaders[i]->mUniformInfos[j];
            mUniformInfoMap->Put(uniform.mapped, info);
        }
    }

    mActiveAttribMap.clear();

    GLint numActiveAttrs = 0;
    mContext->gl->fGetProgramiv(mGLName, LOCAL_GL_ACTIVE_ATTRIBUTES, &numActiveAttrs);

    
    
    char attrName[257];

    GLint dummySize;
    GLenum dummyType;
    for (GLint i = 0; i < numActiveAttrs; i++) {
        mContext->gl->fGetActiveAttrib(mGLName, i, 257, nullptr, &dummySize,
                                       &dummyType, attrName);
        GLint attrLoc = mContext->gl->fGetAttribLocation(mGLName, attrName);
        MOZ_ASSERT(attrLoc >= 0);
        mActiveAttribMap.insert(std::make_pair(attrLoc, nsCString(attrName)));
    }

    return true;
}

 uint64_t
WebGLProgram::IdentifierHashFunction(const char* ident, size_t size)
{
    uint64_t outhash[2];
    
    
    MurmurHash3_x86_128(ident, size, 0, &outhash[0]);
    return outhash[0];
}

 void
WebGLProgram::HashMapIdentifier(const nsACString& name,
                                nsCString* const out_hashedName)
{
    uint64_t hash = IdentifierHashFunction(name.BeginReading(), name.Length());
    out_hashedName->Truncate();
    
    
    out_hashedName->AppendPrintf("webgl_%llx", hash);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(WebGLProgram, mAttachedShaders)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLProgram, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLProgram, Release)

} 
