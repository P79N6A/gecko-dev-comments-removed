




#include "WebGLProgram.h"

#include "GLContext.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/dom/WebGL2RenderingContextBinding.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "nsRefPtr.h"
#include "WebGLActiveInfo.h"
#include "WebGLContext.h"
#include "WebGLShader.h"
#include "WebGLUniformLocation.h"
#include "WebGLValidateStrings.h"

namespace mozilla {













static bool
ParseName(const nsCString& name, nsCString* const out_baseName,
          bool* const out_isArray, size_t* const out_arrayIndex)
{
    int32_t indexEnd = name.RFind("]");
    if (indexEnd == -1 ||
        (uint32_t)indexEnd != name.Length() - 1)
    {
        *out_baseName = name;
        *out_isArray = false;
        return true;
    }

    int32_t indexOpenBracket = name.RFind("[");
    if (indexOpenBracket == -1)
        return false;

    uint32_t indexStart = indexOpenBracket + 1;
    uint32_t indexLen = indexEnd - indexStart;
    if (indexLen == 0)
        return false;

    const nsAutoCString indexStr(Substring(name, indexStart, indexLen));

    nsresult errorcode;
    int32_t indexNum = indexStr.ToInteger(&errorcode);
    if (NS_FAILED(errorcode))
        return false;

    if (indexNum < 0)
        return false;

    *out_baseName = StringHead(name, indexOpenBracket);
    *out_isArray = true;
    *out_arrayIndex = indexNum;
    return true;
}

static void
AddActiveInfo(WebGLContext* webgl, GLint elemCount, GLenum elemType, bool isArray,
              const nsACString& baseUserName, const nsACString& baseMappedName,
              std::vector<RefPtr<WebGLActiveInfo>>* activeInfoList,
              std::map<nsCString, const WebGLActiveInfo*>* infoLocMap)
{
    RefPtr<WebGLActiveInfo> info = new WebGLActiveInfo(webgl, elemCount, elemType,
                                                       isArray, baseUserName,
                                                       baseMappedName);
    activeInfoList->push_back(info);

    infoLocMap->insert(std::make_pair(info->mBaseUserName, info.get()));
}

static void
AddActiveBlockInfo(const nsACString& baseUserName,
                   const nsACString& baseMappedName,
                   std::vector<RefPtr<webgl::UniformBlockInfo>>* activeInfoList)
{
    RefPtr<webgl::UniformBlockInfo> info = new webgl::UniformBlockInfo(baseUserName, baseMappedName);

    activeInfoList->push_back(info);
}



static already_AddRefed<const webgl::LinkedProgramInfo>
QueryProgramInfo(WebGLProgram* prog, gl::GLContext* gl)
{
    RefPtr<webgl::LinkedProgramInfo> info(new webgl::LinkedProgramInfo(prog));

    GLuint maxAttribLenWithNull = 0;
    gl->fGetProgramiv(prog->mGLName, LOCAL_GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                      (GLint*)&maxAttribLenWithNull);
    if (maxAttribLenWithNull < 1)
        maxAttribLenWithNull = 1;

    GLuint maxUniformLenWithNull = 0;
    gl->fGetProgramiv(prog->mGLName, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH,
                      (GLint*)&maxUniformLenWithNull);
    if (maxUniformLenWithNull < 1)
        maxUniformLenWithNull = 1;

    GLuint maxUniformBlockLenWithNull = 0;
    if (gl->IsSupported(gl::GLFeature::uniform_buffer_object)) {
        gl->fGetProgramiv(prog->mGLName, LOCAL_GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH,
                          (GLint*)&maxUniformBlockLenWithNull);
        if (maxUniformBlockLenWithNull < 1)
            maxUniformBlockLenWithNull = 1;
    }

#ifdef DUMP_SHADERVAR_MAPPINGS
    printf_stderr("maxAttribLenWithNull: %d\n", maxAttribLenWithNull);
    printf_stderr("maxUniformLenWithNull: %d\n", maxUniformLenWithNull);
    printf_stderr("maxUniformBlockLenWithNull: %d\n", maxUniformBlockLenWithNull);
#endif

    

    GLuint numActiveAttribs = 0;
    gl->fGetProgramiv(prog->mGLName, LOCAL_GL_ACTIVE_ATTRIBUTES,
                      (GLint*)&numActiveAttribs);

    for (GLuint i = 0; i < numActiveAttribs; i++) {
        nsAutoCString mappedName;
        mappedName.SetLength(maxAttribLenWithNull - 1);

        GLsizei lengthWithoutNull = 0;
        GLint elemCount = 0; 
        GLenum elemType = 0; 
        gl->fGetActiveAttrib(prog->mGLName, i, mappedName.Length()+1, &lengthWithoutNull,
                             &elemCount, &elemType, mappedName.BeginWriting());

        mappedName.SetLength(lengthWithoutNull);

        

        
        
        nsDependentCString userName;
        if (!prog->FindAttribUserNameByMappedName(mappedName, &userName))
            userName.Rebind(mappedName, 0);

#ifdef DUMP_SHADERVAR_MAPPINGS
        printf_stderr("[attrib %i] %s/%s\n", i, mappedName.BeginReading(),
                      userName.BeginReading());
        printf_stderr("    lengthWithoutNull: %d\n", lengthWithoutNull);
#endif

        const bool isArray = false;
        AddActiveInfo(prog->Context(), elemCount, elemType, isArray, userName, mappedName,
                      &info->activeAttribs, &info->attribMap);

        
        GLint loc = gl->fGetAttribLocation(prog->mGLName, mappedName.BeginReading());
        if (loc == -1)
            MOZ_CRASH("Active attrib has no location.");

        info->activeAttribLocs.insert(loc);
    }

    

    const bool needsCheckForArrays = true;

    GLuint numActiveUniforms = 0;
    gl->fGetProgramiv(prog->mGLName, LOCAL_GL_ACTIVE_UNIFORMS,
                      (GLint*)&numActiveUniforms);

    for (GLuint i = 0; i < numActiveUniforms; i++) {
        nsAutoCString mappedName;
        mappedName.SetLength(maxUniformLenWithNull - 1);

        GLsizei lengthWithoutNull = 0;
        GLint elemCount = 0; 
        GLenum elemType = 0; 
        gl->fGetActiveUniform(prog->mGLName, i, mappedName.Length()+1, &lengthWithoutNull,
                              &elemCount, &elemType, mappedName.BeginWriting());

        mappedName.SetLength(lengthWithoutNull);

        nsAutoCString baseMappedName;
        bool isArray;
        size_t arrayIndex;
        if (!ParseName(mappedName, &baseMappedName, &isArray, &arrayIndex))
            MOZ_CRASH("Failed to parse `mappedName` received from driver.");

        
        

        nsAutoCString baseUserName;
        if (!prog->FindUniformByMappedName(baseMappedName, &baseUserName, &isArray)) {
            baseUserName = baseMappedName;

            if (needsCheckForArrays && !isArray) {
                
                
                
                std::string mappedNameStr = baseMappedName.BeginReading();
                mappedNameStr += "[0]";

                GLint loc = gl->fGetUniformLocation(prog->mGLName, mappedNameStr.c_str());
                if (loc != -1)
                    isArray = true;
            }
        }

#ifdef DUMP_SHADERVAR_MAPPINGS
        printf_stderr("[uniform %i] %s/%i/%s/%s\n", i, mappedName.BeginReading(),
                      (int)isArray, baseMappedName.BeginReading(),
                      baseUserName.BeginReading());
        printf_stderr("    lengthWithoutNull: %d\n", lengthWithoutNull);
        printf_stderr("    isArray: %d\n", (int)isArray);
#endif

        AddActiveInfo(prog->Context(), elemCount, elemType, isArray, baseUserName,
                      baseMappedName, &info->activeUniforms, &info->uniformMap);
    }

    

    if (gl->IsSupported(gl::GLFeature::uniform_buffer_object)) {
        GLuint numActiveUniformBlocks = 0;
        gl->fGetProgramiv(prog->mGLName, LOCAL_GL_ACTIVE_UNIFORM_BLOCKS,
                          (GLint*)&numActiveUniformBlocks);

        for (GLuint i = 0; i < numActiveUniformBlocks; i++) {
            nsAutoCString mappedName;
            mappedName.SetLength(maxUniformBlockLenWithNull - 1);

            GLint lengthWithoutNull;
            gl->fGetActiveUniformBlockiv(prog->mGLName, i, LOCAL_GL_UNIFORM_BLOCK_NAME_LENGTH, &lengthWithoutNull);
            gl->fGetActiveUniformBlockName(prog->mGLName, i, maxUniformBlockLenWithNull, &lengthWithoutNull, mappedName.BeginWriting());
            mappedName.SetLength(lengthWithoutNull);

            nsAutoCString baseMappedName;
            bool isArray;
            size_t arrayIndex;
            if (!ParseName(mappedName, &baseMappedName, &isArray, &arrayIndex))
                MOZ_CRASH("Failed to parse `mappedName` received from driver.");

            nsAutoCString baseUserName;
            if (!prog->FindUniformBlockByMappedName(baseMappedName, &baseUserName,
                                                    &isArray))
            {
                baseUserName = baseMappedName;

                if (needsCheckForArrays && !isArray) {
                    std::string mappedNameStr = baseMappedName.BeginReading();
                    mappedNameStr += "[0]";

                    GLuint loc = gl->fGetUniformBlockIndex(prog->mGLName,
                                                           mappedNameStr.c_str());
                    if (loc != LOCAL_GL_INVALID_INDEX)
                        isArray = true;
                }
            }

#ifdef DUMP_SHADERVAR_MAPPINGS
            printf_stderr("[uniform block %i] %s/%i/%s/%s\n", i, mappedName.BeginReading(),
                          (int)isArray, baseMappedName.BeginReading(),
                          baseUserName.BeginReading());
            printf_stderr("    lengthWithoutNull: %d\n", lengthWithoutNull);
            printf_stderr("    isArray: %d\n", (int)isArray);
#endif

            AddActiveBlockInfo(baseUserName, baseMappedName, &info->uniformBlocks);
        }
    }

    return info.forget();
}




webgl::LinkedProgramInfo::LinkedProgramInfo(WebGLProgram* prog)
    : prog(prog)
    , fragDataMap(nullptr)
{ }




static GLuint
CreateProgram(gl::GLContext* gl)
{
    gl->MakeCurrent();
    return gl->fCreateProgram();
}

WebGLProgram::WebGLProgram(WebGLContext* webgl)
    : WebGLContextBoundObject(webgl)
    , mGLName(CreateProgram(webgl->GL()))
    , mTransformFeedbackBufferMode(LOCAL_GL_NONE)
{
    mContext->mPrograms.insertBack(this);
}

WebGLProgram::~WebGLProgram()
{
    DeleteOnce();
}

void
WebGLProgram::Delete()
{
    gl::GLContext* gl = mContext->GL();

    gl->MakeCurrent();
    gl->fDeleteProgram(mGLName);

    mVertShader = nullptr;
    mFragShader = nullptr;

    mMostRecentLinkInfo = nullptr;

    LinkedListElement<WebGLProgram>::removeFrom(mContext->mPrograms);
}




void
WebGLProgram::AttachShader(WebGLShader* shader)
{
    WebGLRefPtr<WebGLShader>* shaderSlot;
    switch (shader->mType) {
    case LOCAL_GL_VERTEX_SHADER:
        shaderSlot = &mVertShader;
        break;
    case LOCAL_GL_FRAGMENT_SHADER:
        shaderSlot = &mFragShader;
        break;
    default:
        mContext->ErrorInvalidOperation("attachShader: Bad type for shader.");
        return;
    }

    if (*shaderSlot) {
        if (shader == *shaderSlot) {
            mContext->ErrorInvalidOperation("attachShader: `shader` is already attached.");
        } else {
            mContext->ErrorInvalidOperation("attachShader: Only one of each type of"
                                            " shader may be attached to a program.");
        }
        return;
    }

    *shaderSlot = shader;

    mContext->MakeContextCurrent();
    mContext->gl->fAttachShader(mGLName, shader->mGLName);
}

void
WebGLProgram::BindAttribLocation(GLuint loc, const nsAString& name)
{
    if (!ValidateGLSLVariableName(name, mContext, "bindAttribLocation"))
        return;

    if (loc >= mContext->MaxVertexAttribs()) {
        mContext->ErrorInvalidValue("bindAttribLocation: `location` must be less than"
                                    " MAX_VERTEX_ATTRIBS.");
        return;
    }

    if (StringBeginsWith(name, NS_LITERAL_STRING("gl_"))) {
        mContext->ErrorInvalidOperation("bindAttribLocation: Can't set the  location of a"
                                        " name that starts with 'gl_'.");
        return;
    }

    NS_LossyConvertUTF16toASCII asciiName(name);

    auto res = mBoundAttribLocs.insert(std::pair<nsCString, GLuint>(asciiName, loc));

    const bool wasInserted = res.second;
    if (!wasInserted) {
        auto itr = res.first;
        itr->second = loc;
    }
}

void
WebGLProgram::DetachShader(WebGLShader* shader)
{
    MOZ_ASSERT(shader);

    WebGLRefPtr<WebGLShader>* shaderSlot;
    switch (shader->mType) {
    case LOCAL_GL_VERTEX_SHADER:
        shaderSlot = &mVertShader;
        break;
    case LOCAL_GL_FRAGMENT_SHADER:
        shaderSlot = &mFragShader;
        break;
    default:
        mContext->ErrorInvalidOperation("attachShader: Bad type for shader.");
        return;
    }

    if (*shaderSlot != shader) {
        mContext->ErrorInvalidOperation("detachShader: `shader` is not attached.");
        return;
    }

    *shaderSlot = nullptr;

    mContext->MakeContextCurrent();
    mContext->gl->fDetachShader(mGLName, shader->mGLName);
}

already_AddRefed<WebGLActiveInfo>
WebGLProgram::GetActiveAttrib(GLuint index) const
{
    if (!mMostRecentLinkInfo) {
        nsRefPtr<WebGLActiveInfo> ret = WebGLActiveInfo::CreateInvalid(mContext);
        return ret.forget();
    }

    const auto& activeList = mMostRecentLinkInfo->activeAttribs;

    if (index >= activeList.size()) {
        mContext->ErrorInvalidValue("`index` (%i) must be less than %s (%i).",
                                    index, "ACTIVE_ATTRIBS", activeList.size());
        return nullptr;
    }

    RefPtr<WebGLActiveInfo> ret = activeList[index];
    return ret.forget();
}

already_AddRefed<WebGLActiveInfo>
WebGLProgram::GetActiveUniform(GLuint index) const
{
    if (!mMostRecentLinkInfo) {
        
        nsRefPtr<WebGLActiveInfo> ret = WebGLActiveInfo::CreateInvalid(mContext);
        return ret.forget();
    }

    const auto& activeList = mMostRecentLinkInfo->activeUniforms;

    if (index >= activeList.size()) {
        mContext->ErrorInvalidValue("`index` (%i) must be less than %s (%i).",
                                    index, "ACTIVE_UNIFORMS", activeList.size());
        return nullptr;
    }

    RefPtr<WebGLActiveInfo> ret = activeList[index];
    return ret.forget();
}

void
WebGLProgram::GetAttachedShaders(nsTArray<nsRefPtr<WebGLShader>>* const out) const
{
    out->TruncateLength(0);

    if (mVertShader)
        out->AppendElement(mVertShader);

    if (mFragShader)
        out->AppendElement(mFragShader);
}

GLint
WebGLProgram::GetAttribLocation(const nsAString& userName_wide) const
{
    if (!ValidateGLSLVariableName(userName_wide, mContext, "getAttribLocation"))
        return -1;

    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getAttribLocation: `program` must be linked.");
        return -1;
    }

    const NS_LossyConvertUTF16toASCII userName(userName_wide);

    const WebGLActiveInfo* info;
    if (!LinkInfo()->FindAttrib(userName, &info))
        return -1;

    const nsCString& mappedName = info->mBaseMappedName;

    gl::GLContext* gl = mContext->GL();
    gl->MakeCurrent();

    return gl->fGetAttribLocation(mGLName, mappedName.BeginReading());
}

GLint
WebGLProgram::GetFragDataLocation(const nsAString& userName_wide) const
{
    if (!ValidateGLSLVariableName(userName_wide, mContext, "getFragDataLocation"))
        return -1;

    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getFragDataLocation: `program` must be linked.");
        return -1;
    }

    const NS_LossyConvertUTF16toASCII userName(userName_wide);

    nsCString mappedName;
    if (!LinkInfo()->FindFragData(userName, &mappedName))
        return -1;

    gl::GLContext* gl = mContext->GL();
    gl->MakeCurrent();

    return gl->fGetFragDataLocation(mGLName, mappedName.BeginReading());
}

void
WebGLProgram::GetProgramInfoLog(nsAString* const out) const
{
    CopyASCIItoUTF16(mLinkLog, *out);
}

static GLint
GetProgramiv(gl::GLContext* gl, GLuint program, GLenum pname)
{
    GLint ret = 0;
    gl->fGetProgramiv(program, pname, &ret);
    return ret;
}

JS::Value
WebGLProgram::GetProgramParameter(GLenum pname) const
{
    gl::GLContext* gl = mContext->gl;
    gl->MakeCurrent();

    if (mContext->IsWebGL2()) {
        switch (pname) {
        case LOCAL_GL_ACTIVE_UNIFORM_BLOCKS:
            return JS::Int32Value(GetProgramiv(gl, mGLName, pname));

        case LOCAL_GL_TRANSFORM_FEEDBACK_VARYINGS:
            return JS::Int32Value(mTransformFeedbackVaryings.size());
       }
    }

    switch (pname) {
    case LOCAL_GL_ATTACHED_SHADERS:
    case LOCAL_GL_ACTIVE_UNIFORMS:
    case LOCAL_GL_ACTIVE_ATTRIBUTES:
        return JS::Int32Value(GetProgramiv(gl, mGLName, pname));

    case LOCAL_GL_DELETE_STATUS:
        return JS::BooleanValue(IsDeleteRequested());

    case LOCAL_GL_LINK_STATUS:
        return JS::BooleanValue(IsLinked());

    case LOCAL_GL_VALIDATE_STATUS:
#ifdef XP_MACOSX
        
        if (gl->WorkAroundDriverBugs())
            return JS::BooleanValue(true);
#endif
        return JS::BooleanValue(bool(GetProgramiv(gl, mGLName, pname)));

    default:
        mContext->ErrorInvalidEnumInfo("getProgramParameter: `pname`",
                                       pname);
        return JS::NullValue();
    }
}

GLuint
WebGLProgram::GetUniformBlockIndex(const nsAString& userName_wide) const
{
    if (!ValidateGLSLVariableName(userName_wide, mContext, "getUniformBlockIndex"))
        return LOCAL_GL_INVALID_INDEX;

    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getUniformBlockIndex: `program` must be linked.");
        return LOCAL_GL_INVALID_INDEX;
    }

    const NS_LossyConvertUTF16toASCII userName(userName_wide);

    nsDependentCString baseUserName;
    bool isArray;
    size_t arrayIndex;
    if (!ParseName(userName, &baseUserName, &isArray, &arrayIndex))
        return LOCAL_GL_INVALID_INDEX;

    RefPtr<const webgl::UniformBlockInfo> info;
    if (!LinkInfo()->FindUniformBlock(baseUserName, &info)) {
        return LOCAL_GL_INVALID_INDEX;
    }

    const nsCString& baseMappedName = info->mBaseMappedName;
    nsAutoCString mappedName(baseMappedName);
    if (isArray) {
        mappedName.AppendLiteral("[");
        mappedName.AppendInt(uint32_t(arrayIndex));
        mappedName.AppendLiteral("]");
    }

    gl::GLContext* gl = mContext->GL();
    gl->MakeCurrent();

    return gl->fGetUniformBlockIndex(mGLName, mappedName.BeginReading());
}

void
WebGLProgram::GetActiveUniformBlockName(GLuint uniformBlockIndex, nsAString& retval) const
{
    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getActiveUniformBlockName: `program` must be linked.");
        return;
    }

    const webgl::LinkedProgramInfo* linkInfo = LinkInfo();
    GLuint uniformBlockCount = (GLuint) linkInfo->uniformBlocks.size();
    if (uniformBlockIndex >= uniformBlockCount) {
        mContext->ErrorInvalidValue("getActiveUniformBlockName: index %u invalid.", uniformBlockIndex);
        return;
    }

    const webgl::UniformBlockInfo* blockInfo = linkInfo->uniformBlocks[uniformBlockIndex];

    retval.Assign(NS_ConvertASCIItoUTF16(blockInfo->mBaseUserName));
}

void
WebGLProgram::GetActiveUniformBlockParam(GLuint uniformBlockIndex, GLenum pname,
                                         dom::Nullable<dom::OwningUnsignedLongOrUint32ArrayOrBoolean>& retval) const
{
    retval.SetNull();
    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getActiveUniformBlockParameter: `program` must be linked.");
        return;
    }

    const webgl::LinkedProgramInfo* linkInfo = LinkInfo();
    GLuint uniformBlockCount = (GLuint)linkInfo->uniformBlocks.size();
    if (uniformBlockIndex >= uniformBlockCount) {
        mContext->ErrorInvalidValue("getActiveUniformBlockParameter: index %u invalid.", uniformBlockIndex);
        return;
    }

    gl::GLContext* gl = mContext->GL();
    GLint param = 0;

    switch (pname) {
    case LOCAL_GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
    case LOCAL_GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
        gl->fGetActiveUniformBlockiv(mGLName, uniformBlockIndex, pname, &param);
        retval.SetValue().SetAsBoolean() = (param != 0);
        return;

    case LOCAL_GL_UNIFORM_BLOCK_BINDING:
    case LOCAL_GL_UNIFORM_BLOCK_DATA_SIZE:
    case LOCAL_GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
        gl->fGetActiveUniformBlockiv(mGLName, uniformBlockIndex, pname, &param);
        retval.SetValue().SetAsUnsignedLong() = param;
        return;
    }
}

void
WebGLProgram::GetActiveUniformBlockActiveUniforms(JSContext* cx, GLuint uniformBlockIndex,
                                                  dom::Nullable<dom::OwningUnsignedLongOrUint32ArrayOrBoolean>& retval,
                                                  ErrorResult& rv) const
{
    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getActiveUniformBlockParameter: `program` must be linked.");
        return;
    }

    const webgl::LinkedProgramInfo* linkInfo = LinkInfo();
    GLuint uniformBlockCount = (GLuint)linkInfo->uniformBlocks.size();
    if (uniformBlockIndex >= uniformBlockCount) {
        mContext->ErrorInvalidValue("getActiveUniformBlockParameter: index %u invalid.", uniformBlockIndex);
        return;
    }

    gl::GLContext* gl = mContext->GL();
    GLint activeUniformCount = 0;
    gl->fGetActiveUniformBlockiv(mGLName, uniformBlockIndex,
                                 LOCAL_GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
                                 &activeUniformCount);
    JS::RootedObject obj(cx, dom::Uint32Array::Create(cx, mContext, activeUniformCount,
                                                      nullptr));
    if (!obj) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        return;
    }

    dom::Uint32Array result;
    DebugOnly<bool> inited = result.Init(obj);
    MOZ_ASSERT(inited);
    result.ComputeLengthAndData();
    gl->fGetActiveUniformBlockiv(mGLName, uniformBlockIndex,
                                 LOCAL_GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                                 (GLint*)result.Data());

    inited = retval.SetValue().SetAsUint32Array().Init(obj);
    MOZ_ASSERT(inited);
}

already_AddRefed<WebGLUniformLocation>
WebGLProgram::GetUniformLocation(const nsAString& userName_wide) const
{
    if (!ValidateGLSLVariableName(userName_wide, mContext, "getUniformLocation"))
        return nullptr;

    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getUniformLocation: `program` must be linked.");
        return nullptr;
    }

    const NS_LossyConvertUTF16toASCII userName(userName_wide);

    nsDependentCString baseUserName;
    bool isArray;
    size_t arrayIndex;
    if (!ParseName(userName, &baseUserName, &isArray, &arrayIndex))
        return nullptr;

    const WebGLActiveInfo* activeInfo;
    if (!LinkInfo()->FindUniform(baseUserName, &activeInfo))
        return nullptr;

    const nsCString& baseMappedName = activeInfo->mBaseMappedName;

    nsAutoCString mappedName(baseMappedName);
    if (isArray) {
        mappedName.AppendLiteral("[");
        mappedName.AppendInt(uint32_t(arrayIndex));
        mappedName.AppendLiteral("]");
    }

    gl::GLContext* gl = mContext->GL();
    gl->MakeCurrent();

    GLint loc = gl->fGetUniformLocation(mGLName, mappedName.BeginReading());
    if (loc == -1)
        return nullptr;

    nsRefPtr<WebGLUniformLocation> locObj = new WebGLUniformLocation(mContext, LinkInfo(),
                                                                     loc, activeInfo);
    return locObj.forget();
}

void
WebGLProgram::UniformBlockBinding(GLuint uniformBlockIndex, GLuint uniformBlockBinding) const
{
    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getActiveUniformBlockName: `program` must be linked.");
        return;
    }

    const webgl::LinkedProgramInfo* linkInfo = LinkInfo();
    GLuint uniformBlockCount = (GLuint)linkInfo->uniformBlocks.size();
    if (uniformBlockIndex >= uniformBlockCount) {
        mContext->ErrorInvalidValue("getActiveUniformBlockName: index %u invalid.", uniformBlockIndex);
        return;
    }

    if (uniformBlockBinding > mContext->mGLMaxUniformBufferBindings) {
        mContext->ErrorInvalidEnum("getActiveUniformBlockName: binding %u invalid.", uniformBlockBinding);
        return;
    }

    gl::GLContext* gl = mContext->GL();
    gl->MakeCurrent();
    gl->fUniformBlockBinding(mGLName, uniformBlockIndex, uniformBlockBinding);
}

bool
WebGLProgram::LinkProgram()
{
    mContext->InvalidateBufferFetching(); 
    

    mLinkLog.Truncate();
    mMostRecentLinkInfo = nullptr;

    if (!mVertShader || !mVertShader->IsCompiled()) {
        mLinkLog.AssignLiteral("Must have a compiled vertex shader attached.");
        mContext->GenerateWarning("linkProgram: %s", mLinkLog.BeginReading());
        return false;
    }

    if (!mFragShader || !mFragShader->IsCompiled()) {
        mLinkLog.AssignLiteral("Must have an compiled fragment shader attached.");
        mContext->GenerateWarning("linkProgram: %s", mLinkLog.BeginReading());
        return false;
    }

    if (!mFragShader->CanLinkTo(mVertShader, &mLinkLog)) {
        mContext->GenerateWarning("linkProgram: %s", mLinkLog.BeginReading());
        return false;
    }

    gl::GLContext* gl = mContext->gl;
    gl->MakeCurrent();

    
    
    size_t numSamplerUniforms_upperBound = mVertShader->CalcNumSamplerUniforms() +
                                           mFragShader->CalcNumSamplerUniforms();
    if (gl->WorkAroundDriverBugs() &&
        mContext->mIsMesa &&
        numSamplerUniforms_upperBound > 16)
    {
        mLinkLog.AssignLiteral("Programs with more than 16 samplers are disallowed on"
                               " Mesa drivers to avoid crashing.");
        mContext->GenerateWarning("linkProgram: %s", mLinkLog.BeginReading());
        return false;
    }

    
    
    for (auto itr = mBoundAttribLocs.begin(); itr != mBoundAttribLocs.end(); ++itr) {
        const nsCString& name = itr->first;
        GLuint index = itr->second;

        mVertShader->BindAttribLocation(mGLName, name, index);
    }

    if (!mTransformFeedbackVaryings.empty()) {
        
        
        mVertShader->ApplyTransformFeedbackVaryings(mGLName,
                                                    mTransformFeedbackVaryings,
                                                    mTransformFeedbackBufferMode,
                                                    &mTempMappedVaryings);
    }

    if (LinkAndUpdate())
        return true;

    
    if (mContext->ShouldGenerateWarnings()) {
        
        
        
        
        
        if (!mLinkLog.IsEmpty()) {
            mContext->GenerateWarning("linkProgram: Failed to link, leaving the following"
                                      " log:\n%s\n",
                                      mLinkLog.BeginReading());
        }
    }

    return false;
}

bool
WebGLProgram::UseProgram() const
{
    if (!mMostRecentLinkInfo) {
        mContext->ErrorInvalidOperation("useProgram: Program has not been successfully"
                                        " linked.");
        return false;
    }

    mContext->MakeContextCurrent();

    mContext->InvalidateBufferFetching();

    mContext->gl->fUseProgram(mGLName);
    return true;
}

void
WebGLProgram::ValidateProgram() const
{
    mContext->MakeContextCurrent();
    gl::GLContext* gl = mContext->gl;

#ifdef XP_MACOSX
    
    
    if (gl->WorkAroundDriverBugs()) {
        mContext->GenerateWarning("validateProgram: Implemented as a no-op on"
                                  " Mac to work around crashes.");
        return;
    }
#endif

    gl->fValidateProgram(mGLName);
}




bool
WebGLProgram::LinkAndUpdate()
{
    mMostRecentLinkInfo = nullptr;

    gl::GLContext* gl = mContext->gl;
    gl->fLinkProgram(mGLName);

    
    GLuint logLenWithNull = 0;
    gl->fGetProgramiv(mGLName, LOCAL_GL_INFO_LOG_LENGTH, (GLint*)&logLenWithNull);
    if (logLenWithNull > 1) {
        mLinkLog.SetLength(logLenWithNull - 1);
        gl->fGetProgramInfoLog(mGLName, logLenWithNull, nullptr, mLinkLog.BeginWriting());
    } else {
        mLinkLog.SetLength(0);
    }

    
    
    std::vector<std::string> empty;
    empty.swap(mTempMappedVaryings);

    GLint ok = 0;
    gl->fGetProgramiv(mGLName, LOCAL_GL_LINK_STATUS, &ok);
    if (!ok)
        return false;

    mMostRecentLinkInfo = QueryProgramInfo(this, gl);

    MOZ_ASSERT(mMostRecentLinkInfo);
    if (!mMostRecentLinkInfo)
        mLinkLog.AssignLiteral("Failed to gather program info.");

    return mMostRecentLinkInfo;
}

bool
WebGLProgram::FindAttribUserNameByMappedName(const nsACString& mappedName,
                                             nsDependentCString* const out_userName) const
{
    if (mVertShader->FindAttribUserNameByMappedName(mappedName, out_userName))
        return true;

    return false;
}

bool
WebGLProgram::FindUniformByMappedName(const nsACString& mappedName,
                                      nsCString* const out_userName,
                                      bool* const out_isArray) const
{
    if (mVertShader->FindUniformByMappedName(mappedName, out_userName, out_isArray))
        return true;

    if (mFragShader->FindUniformByMappedName(mappedName, out_userName, out_isArray))
        return true;

    return false;
}

void
WebGLProgram::TransformFeedbackVaryings(const dom::Sequence<nsString>& varyings,
                                        GLenum bufferMode)
{
    if (bufferMode != LOCAL_GL_INTERLEAVED_ATTRIBS &&
        bufferMode != LOCAL_GL_SEPARATE_ATTRIBS)
    {
        mContext->ErrorInvalidEnum("transformFeedbackVaryings: `bufferMode` %s is "
                                   "invalid. Must be one of gl.INTERLEAVED_ATTRIBS or "
                                   "gl.SEPARATE_ATTRIBS.",
                                   mContext->EnumName(bufferMode));
        return;
    }

    size_t varyingsCount = varyings.Length();
    if (bufferMode == LOCAL_GL_SEPARATE_ATTRIBS &&
        varyingsCount >= mContext->mGLMaxTransformFeedbackSeparateAttribs)
    {
        mContext->ErrorInvalidValue("transformFeedbackVaryings: Number of `varyings` exc"
                                    "eeds gl.MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS.");
        return;
    }

    std::vector<nsCString> asciiVaryings;
    for (size_t i = 0; i < varyingsCount; i++) {
        if (!ValidateGLSLVariableName(varyings[i], mContext, "transformFeedbackVaryings"))
            return;

        NS_LossyConvertUTF16toASCII asciiName(varyings[i]);
        asciiVaryings.push_back(asciiName);
    }

    
    
    mTransformFeedbackBufferMode = bufferMode;
    mTransformFeedbackVaryings.swap(asciiVaryings);
}

already_AddRefed<WebGLActiveInfo>
WebGLProgram::GetTransformFeedbackVarying(GLuint index)
{
    
    
    if (!IsLinked()) {
        mContext->ErrorInvalidOperation("getTransformFeedbackVarying: `program` must be "
                                        "linked.");
        return nullptr;
    }

    if (index >= mTransformFeedbackVaryings.size()) {
        mContext->ErrorInvalidValue("getTransformFeedbackVarying: `index` is greater or "
                                    "equal to TRANSFORM_FEEDBACK_VARYINGS.");
        return nullptr;
    }

    const nsCString& varyingUserName = mTransformFeedbackVaryings[index];

    WebGLActiveInfo* info;
    LinkInfo()->FindAttrib(varyingUserName, (const WebGLActiveInfo**) &info);
    MOZ_ASSERT(info);

    RefPtr<WebGLActiveInfo> ret(info);
    return ret.forget();
}

bool
WebGLProgram::FindUniformBlockByMappedName(const nsACString& mappedName,
                                           nsCString* const out_userName,
                                           bool* const out_isArray) const
{
    if (mVertShader->FindUniformBlockByMappedName(mappedName, out_userName, out_isArray))
        return true;

    if (mFragShader->FindUniformBlockByMappedName(mappedName, out_userName, out_isArray))
        return true;

    return false;
}



JSObject*
WebGLProgram::WrapObject(JSContext* js, JS::Handle<JSObject*> givenProto)
{
    return dom::WebGLProgramBinding::Wrap(js, this, givenProto);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(WebGLProgram, mVertShader, mFragShader)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLProgram, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLProgram, Release)

} 
