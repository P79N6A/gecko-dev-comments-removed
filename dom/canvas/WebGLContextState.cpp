




#include "WebGLContext.h"
#include "WebGLContextUtils.h"
#include "WebGLBuffer.h"
#include "WebGLShader.h"
#include "WebGLProgram.h"
#include "WebGLFramebuffer.h"
#include "WebGLRenderbuffer.h"
#include "WebGLTexture.h"
#include "WebGLVertexArray.h"
#include "GLContext.h"
#include "mozilla/dom/ToJSValue.h"

namespace mozilla {

void
WebGLContext::Disable(GLenum cap)
{
    if (IsContextLost())
        return;

    if (!ValidateCapabilityEnum(cap, "disable"))
        return;

    realGLboolean* trackingSlot = GetStateTrackingSlot(cap);

    if (trackingSlot)
    {
        *trackingSlot = 0;
    }

    MakeContextCurrent();
    gl->fDisable(cap);
}

void
WebGLContext::Enable(GLenum cap)
{
    if (IsContextLost())
        return;

    if (!ValidateCapabilityEnum(cap, "enable"))
        return;

    realGLboolean* trackingSlot = GetStateTrackingSlot(cap);

    if (trackingSlot)
    {
        *trackingSlot = 1;
    }

    MakeContextCurrent();
    gl->fEnable(cap);
}

bool
WebGLContext::GetStencilBits(GLint* out_stencilBits)
{
    *out_stencilBits = 0;
    if (mBoundDrawFramebuffer) {
        if (mBoundDrawFramebuffer->HasDepthStencilConflict()) {
            
            ErrorInvalidFramebufferOperation("getParameter: framebuffer has two stencil buffers bound");
            return false;
        }

        if (mBoundDrawFramebuffer->StencilAttachment().IsDefined() ||
            mBoundDrawFramebuffer->DepthStencilAttachment().IsDefined())
        {
            *out_stencilBits = 8;
        }
    } else if (mOptions.stencil) {
        *out_stencilBits = 8;
    }

    return true;
}

JS::Value
WebGLContext::GetParameter(JSContext* cx, GLenum pname, ErrorResult& rv)
{
    if (IsContextLost())
        return JS::NullValue();

    MakeContextCurrent();

    if (MinCapabilityMode()) {
        switch(pname) {
            
            

            
            case LOCAL_GL_MAX_VERTEX_ATTRIBS:
                return JS::Int32Value(MINVALUE_GL_MAX_VERTEX_ATTRIBS);

            case LOCAL_GL_MAX_FRAGMENT_UNIFORM_VECTORS:
                return JS::Int32Value(MINVALUE_GL_MAX_FRAGMENT_UNIFORM_VECTORS);

            case LOCAL_GL_MAX_VERTEX_UNIFORM_VECTORS:
                return JS::Int32Value(MINVALUE_GL_MAX_VERTEX_UNIFORM_VECTORS);

            case LOCAL_GL_MAX_VARYING_VECTORS:
                return JS::Int32Value(MINVALUE_GL_MAX_VARYING_VECTORS);

            case LOCAL_GL_MAX_TEXTURE_SIZE:
                return JS::Int32Value(MINVALUE_GL_MAX_TEXTURE_SIZE);

            case LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE:
                return JS::Int32Value(MINVALUE_GL_MAX_CUBE_MAP_TEXTURE_SIZE);

            case LOCAL_GL_MAX_TEXTURE_IMAGE_UNITS:
                return JS::Int32Value(MINVALUE_GL_MAX_TEXTURE_IMAGE_UNITS);

            case LOCAL_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
                return JS::Int32Value(MINVALUE_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);

            case LOCAL_GL_MAX_RENDERBUFFER_SIZE:
                return JS::Int32Value(MINVALUE_GL_MAX_RENDERBUFFER_SIZE);

            default:
                
                break;
        }
    }

    if (IsExtensionEnabled(WebGLExtensionID::WEBGL_draw_buffers)) {
        if (pname == LOCAL_GL_MAX_COLOR_ATTACHMENTS) {
            return JS::Int32Value(mGLMaxColorAttachments);

        } else if (pname == LOCAL_GL_MAX_DRAW_BUFFERS) {
            return JS::Int32Value(mGLMaxDrawBuffers);

        } else if (pname >= LOCAL_GL_DRAW_BUFFER0 &&
                   pname < GLenum(LOCAL_GL_DRAW_BUFFER0 + mGLMaxDrawBuffers))
        {
            GLint iv = 0;
            gl->fGetIntegerv(pname, &iv);

            if (mBoundDrawFramebuffer)
                return JS::Int32Value(iv);

            const GLint index = (pname - LOCAL_GL_DRAW_BUFFER0);
            if (iv == LOCAL_GL_COLOR_ATTACHMENT0 + index)
                return JS::Int32Value(LOCAL_GL_BACK);

            return JS::Int32Value(LOCAL_GL_NONE);
        }
    }

    if (IsExtensionEnabled(WebGLExtensionID::OES_vertex_array_object)) {
        if (pname == LOCAL_GL_VERTEX_ARRAY_BINDING) {
            WebGLVertexArray* vao =
                (mBoundVertexArray != mDefaultVertexArray) ? mBoundVertexArray.get() : nullptr;
            return WebGLObjectAsJSValue(cx, vao, rv);
        }
    }

    if (IsExtensionEnabled(WebGLExtensionID::EXT_disjoint_timer_query)) {
        if (pname == LOCAL_GL_TIMESTAMP_EXT) {
            GLuint64 iv = 0;
            gl->fGetInteger64v(pname, (GLint64*) &iv);
            
            
            return JS::NumberValue(static_cast<double>(iv));
        } else if (pname == LOCAL_GL_GPU_DISJOINT_EXT) {
            
            realGLboolean disjoint = LOCAL_GL_FALSE;
            if (gl->IsExtensionSupported(gl::GLContext::EXT_disjoint_timer_query)) {
                gl->fGetBooleanv(pname, &disjoint);
            }
            return JS::BooleanValue(bool(disjoint));
        }
    }

    
    if (IsExtensionEnabled(WebGLExtensionID::WEBGL_debug_renderer_info)) {
        switch (pname) {
        case UNMASKED_VENDOR_WEBGL:
        case UNMASKED_RENDERER_WEBGL:
            GLenum glstringname = LOCAL_GL_NONE;
            if (pname == UNMASKED_VENDOR_WEBGL) {
                glstringname = LOCAL_GL_VENDOR;
            } else if (pname == UNMASKED_RENDERER_WEBGL) {
                glstringname = LOCAL_GL_RENDERER;
            }
            const GLchar* string = (const GLchar*) gl->fGetString(glstringname);
            return StringValue(cx, string, rv);
        }
    }

    if (IsExtensionEnabled(WebGLExtensionID::OES_standard_derivatives)) {
        if (pname == LOCAL_GL_FRAGMENT_SHADER_DERIVATIVE_HINT) {
            GLint i = 0;
            gl->fGetIntegerv(pname, &i);
            return JS::Int32Value(i);
        }
    }

    if (IsExtensionEnabled(WebGLExtensionID::EXT_texture_filter_anisotropic)) {
        if (pname == LOCAL_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT) {
            GLfloat f = 0.f;
            gl->fGetFloatv(pname, &f);
            return JS::NumberValue(f);
        }
    }

    switch (pname) {
        
        
        
        case LOCAL_GL_VENDOR:
        case LOCAL_GL_RENDERER:
            return StringValue(cx, "Mozilla", rv);
        case LOCAL_GL_VERSION:
            return StringValue(cx, "WebGL 1.0", rv);
        case LOCAL_GL_SHADING_LANGUAGE_VERSION:
            return StringValue(cx, "WebGL GLSL ES 1.0", rv);

        
        

        
        case LOCAL_GL_CULL_FACE_MODE:
        case LOCAL_GL_FRONT_FACE:
        case LOCAL_GL_ACTIVE_TEXTURE:
        case LOCAL_GL_STENCIL_FUNC:
        case LOCAL_GL_STENCIL_FAIL:
        case LOCAL_GL_STENCIL_PASS_DEPTH_FAIL:
        case LOCAL_GL_STENCIL_PASS_DEPTH_PASS:
        case LOCAL_GL_STENCIL_BACK_FUNC:
        case LOCAL_GL_STENCIL_BACK_FAIL:
        case LOCAL_GL_STENCIL_BACK_PASS_DEPTH_FAIL:
        case LOCAL_GL_STENCIL_BACK_PASS_DEPTH_PASS:
        case LOCAL_GL_DEPTH_FUNC:
        case LOCAL_GL_BLEND_SRC_RGB:
        case LOCAL_GL_BLEND_SRC_ALPHA:
        case LOCAL_GL_BLEND_DST_RGB:
        case LOCAL_GL_BLEND_DST_ALPHA:
        case LOCAL_GL_BLEND_EQUATION_RGB:
        case LOCAL_GL_BLEND_EQUATION_ALPHA:
        case LOCAL_GL_GENERATE_MIPMAP_HINT: {
            GLint i = 0;
            gl->fGetIntegerv(pname, &i);
            return JS::NumberValue(uint32_t(i));
        }
        case LOCAL_GL_IMPLEMENTATION_COLOR_READ_TYPE: {
            if (mBoundReadFramebuffer) {
                FBStatus status = mBoundReadFramebuffer->CheckFramebufferStatus();
                if (status != LOCAL_GL_FRAMEBUFFER_COMPLETE) {
                    ErrorInvalidOperation("getParameter: Read framebuffer must be"
                                          " complete before querying"
                                          " IMPLEMENTATION_COLOR_READ_TYPE.");
                    return JS::NullValue();
                }
            }

            GLint i = 0;
            if (gl->IsSupported(gl::GLFeature::ES2_compatibility)) {
                gl->fGetIntegerv(pname, &i);
            } else {
                i = LOCAL_GL_UNSIGNED_BYTE;
            }
            return JS::NumberValue(uint32_t(i));
        }
        case LOCAL_GL_IMPLEMENTATION_COLOR_READ_FORMAT: {
            if (mBoundReadFramebuffer) {
                FBStatus status = mBoundReadFramebuffer->CheckFramebufferStatus();
                if (status != LOCAL_GL_FRAMEBUFFER_COMPLETE) {
                    ErrorInvalidOperation("getParameter: Read framebuffer must be"
                                          " complete before querying"
                                          " IMPLEMENTATION_COLOR_READ_FORMAT.");
                    return JS::NullValue();
                }
            }

            GLint i = 0;
            if (gl->IsSupported(gl::GLFeature::ES2_compatibility)) {
                gl->fGetIntegerv(pname, &i);
            } else {
                i = LOCAL_GL_RGBA;
            }
            return JS::NumberValue(uint32_t(i));
        }
        
        case LOCAL_GL_STENCIL_REF:
        case LOCAL_GL_STENCIL_BACK_REF: {
            GLint stencilBits = 0;
            if (!GetStencilBits(&stencilBits))
                return JS::NullValue();

            
            const GLint stencilMask = (1 << stencilBits) - 1;

            GLint refValue = 0;
            gl->fGetIntegerv(pname, &refValue);

            return JS::Int32Value(refValue & stencilMask);
        }
        case LOCAL_GL_STENCIL_BITS: {
            GLint stencilBits = 0;
            GetStencilBits(&stencilBits);
            return JS::Int32Value(stencilBits);
        }
        case LOCAL_GL_STENCIL_CLEAR_VALUE:
        case LOCAL_GL_UNPACK_ALIGNMENT:
        case LOCAL_GL_PACK_ALIGNMENT:
        case LOCAL_GL_SUBPIXEL_BITS:
        case LOCAL_GL_SAMPLE_BUFFERS:
        case LOCAL_GL_SAMPLES:
        case LOCAL_GL_MAX_VERTEX_ATTRIBS:
        case LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_MAX_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_RED_BITS:
        case LOCAL_GL_GREEN_BITS:
        case LOCAL_GL_BLUE_BITS:
        case LOCAL_GL_DEPTH_BITS: {
            GLint i = 0;
            gl->fGetIntegerv(pname, &i);
            return JS::Int32Value(i);
        }
        case LOCAL_GL_ALPHA_BITS: {
            GLint i = 0;
            if (!mNeedsFakeNoAlpha) {
                gl->fGetIntegerv(pname, &i);
            }
            return JS::Int32Value(i);
        }
        case LOCAL_GL_MAX_TEXTURE_SIZE:
            return JS::Int32Value(mGLMaxTextureSize);

        case LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE:
            return JS::Int32Value(mGLMaxCubeMapTextureSize);

        case LOCAL_GL_MAX_RENDERBUFFER_SIZE:
            return JS::Int32Value(mGLMaxRenderbufferSize);

        case LOCAL_GL_MAX_VERTEX_UNIFORM_VECTORS:
            return JS::Int32Value(mGLMaxVertexUniformVectors);

        case LOCAL_GL_MAX_FRAGMENT_UNIFORM_VECTORS:
            return JS::Int32Value(mGLMaxFragmentUniformVectors);

        case LOCAL_GL_MAX_VARYING_VECTORS:
            return JS::Int32Value(mGLMaxVaryingVectors);

        case LOCAL_GL_NUM_COMPRESSED_TEXTURE_FORMATS:
            return JS::Int32Value(0);
        case LOCAL_GL_COMPRESSED_TEXTURE_FORMATS: {
            uint32_t length = mCompressedTextureFormats.Length();
            JSObject* obj = dom::Uint32Array::Create(cx, this, length,
                                                     mCompressedTextureFormats.Elements());
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        
        
        case LOCAL_GL_STENCIL_BACK_VALUE_MASK:
            return JS::DoubleValue(mStencilValueMaskBack); 

        case LOCAL_GL_STENCIL_BACK_WRITEMASK:
            return JS::DoubleValue(mStencilWriteMaskBack);

        case LOCAL_GL_STENCIL_VALUE_MASK:
            return JS::DoubleValue(mStencilValueMaskFront);

        case LOCAL_GL_STENCIL_WRITEMASK:
            return JS::DoubleValue(mStencilWriteMaskFront);

        
        case LOCAL_GL_DEPTH_CLEAR_VALUE:
        case LOCAL_GL_LINE_WIDTH:
        case LOCAL_GL_POLYGON_OFFSET_FACTOR:
        case LOCAL_GL_POLYGON_OFFSET_UNITS:
        case LOCAL_GL_SAMPLE_COVERAGE_VALUE: {
            GLfloat f = 0.f;
            gl->fGetFloatv(pname, &f);
            return JS::DoubleValue(f);
        }

        
        case LOCAL_GL_BLEND:
        case LOCAL_GL_DEPTH_TEST:
        case LOCAL_GL_STENCIL_TEST:
        case LOCAL_GL_CULL_FACE:
        case LOCAL_GL_DITHER:
        case LOCAL_GL_POLYGON_OFFSET_FILL:
        case LOCAL_GL_SCISSOR_TEST:
        case LOCAL_GL_SAMPLE_COVERAGE_INVERT:
        case LOCAL_GL_DEPTH_WRITEMASK: {
            realGLboolean b = 0;
            gl->fGetBooleanv(pname, &b);
            return JS::BooleanValue(bool(b));
        }

        
        case UNPACK_FLIP_Y_WEBGL:
            return JS::BooleanValue(mPixelStoreFlipY);
        case UNPACK_PREMULTIPLY_ALPHA_WEBGL:
            return JS::BooleanValue(mPixelStorePremultiplyAlpha);

        
        case UNPACK_COLORSPACE_CONVERSION_WEBGL:
            return JS::NumberValue(uint32_t(mPixelStoreColorspaceConversion));

        
        

        
        case LOCAL_GL_DEPTH_RANGE:
        case LOCAL_GL_ALIASED_POINT_SIZE_RANGE:
        case LOCAL_GL_ALIASED_LINE_WIDTH_RANGE: {
            GLfloat fv[2] = { 0 };
            gl->fGetFloatv(pname, fv);
            JSObject* obj = dom::Float32Array::Create(cx, this, 2, fv);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        
        case LOCAL_GL_COLOR_CLEAR_VALUE:
        case LOCAL_GL_BLEND_COLOR: {
            GLfloat fv[4] = { 0 };
            gl->fGetFloatv(pname, fv);
            JSObject* obj = dom::Float32Array::Create(cx, this, 4, fv);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        
        case LOCAL_GL_MAX_VIEWPORT_DIMS: {
            GLint iv[2] = { 0 };
            gl->fGetIntegerv(pname, iv);
            JSObject* obj = dom::Int32Array::Create(cx, this, 2, iv);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        
        case LOCAL_GL_SCISSOR_BOX:
        case LOCAL_GL_VIEWPORT: {
            GLint iv[4] = { 0 };
            gl->fGetIntegerv(pname, iv);
            JSObject* obj = dom::Int32Array::Create(cx, this, 4, iv);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        
        case LOCAL_GL_COLOR_WRITEMASK: {
            realGLboolean gl_bv[4] = { 0 };
            gl->fGetBooleanv(pname, gl_bv);
            bool vals[4] = { bool(gl_bv[0]), bool(gl_bv[1]),
                             bool(gl_bv[2]), bool(gl_bv[3]) };
            JS::Rooted<JS::Value> arr(cx);
            if (!dom::ToJSValue(cx, vals, &arr)) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return arr;
        }

        case LOCAL_GL_ARRAY_BUFFER_BINDING: {
            return WebGLObjectAsJSValue(cx, mBoundArrayBuffer.get(), rv);
        }

        case LOCAL_GL_ELEMENT_ARRAY_BUFFER_BINDING: {
            return WebGLObjectAsJSValue(cx, mBoundVertexArray->mElementArrayBuffer.get(), rv);
        }

        case LOCAL_GL_RENDERBUFFER_BINDING: {
            return WebGLObjectAsJSValue(cx, mBoundRenderbuffer.get(), rv);
        }

        
        case LOCAL_GL_FRAMEBUFFER_BINDING: {
            return WebGLObjectAsJSValue(cx, mBoundDrawFramebuffer.get(), rv);
        }

        case LOCAL_GL_CURRENT_PROGRAM: {
            return WebGLObjectAsJSValue(cx, mCurrentProgram.get(), rv);
        }

        case LOCAL_GL_TEXTURE_BINDING_2D: {
            return WebGLObjectAsJSValue(cx, mBound2DTextures[mActiveTexture].get(), rv);
        }

        case LOCAL_GL_TEXTURE_BINDING_CUBE_MAP: {
            return WebGLObjectAsJSValue(cx, mBoundCubeMapTextures[mActiveTexture].get(), rv);
        }

        default:
            break;
    }

    ErrorInvalidEnumInfo("getParameter: parameter", pname);
    return JS::NullValue();
}

void
WebGLContext::GetParameterIndexed(JSContext* cx, GLenum pname, GLuint index,
                                  JS::MutableHandle<JS::Value> retval)
{
    if (IsContextLost()) {
        retval.setNull();
        return;
    }

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
        {
            if (index >= mGLMaxTransformFeedbackSeparateAttribs) {
                ErrorInvalidValue("getParameterIndexed: index should be less than MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS", index);
                retval.setNull();
                return;
            }
            retval.setNull(); 
            return;
        }

        default:
            break;
    }

    ErrorInvalidEnumInfo("getParameterIndexed: parameter", pname);
    retval.setNull();
}

bool
WebGLContext::IsEnabled(GLenum cap)
{
    if (IsContextLost())
        return false;

    if (!ValidateCapabilityEnum(cap, "isEnabled"))
        return false;

    MakeContextCurrent();
    return gl->fIsEnabled(cap);
}

bool
WebGLContext::ValidateCapabilityEnum(GLenum cap, const char* info)
{
    switch (cap) {
        case LOCAL_GL_BLEND:
        case LOCAL_GL_CULL_FACE:
        case LOCAL_GL_DEPTH_TEST:
        case LOCAL_GL_DITHER:
        case LOCAL_GL_POLYGON_OFFSET_FILL:
        case LOCAL_GL_SAMPLE_ALPHA_TO_COVERAGE:
        case LOCAL_GL_SAMPLE_COVERAGE:
        case LOCAL_GL_SCISSOR_TEST:
        case LOCAL_GL_STENCIL_TEST:
            return true;
        case LOCAL_GL_RASTERIZER_DISCARD:
            return IsWebGL2();
        default:
            ErrorInvalidEnumInfo(info, cap);
            return false;
    }
}

realGLboolean*
WebGLContext::GetStateTrackingSlot(GLenum cap)
{
    switch (cap) {
        case LOCAL_GL_DITHER:
            return &mDitherEnabled;
        case LOCAL_GL_RASTERIZER_DISCARD:
            return &mRasterizerDiscardEnabled;
        case LOCAL_GL_SCISSOR_TEST:
            return &mScissorTestEnabled;
        case LOCAL_GL_STENCIL_TEST:
            return &mStencilTestEnabled;
    }

    return nullptr;
}

} 
