





#include "WebGL2Context.h"
#include "WebGLContextUtils.h"

namespace mozilla {

JS::Value
WebGL2Context::GetParameter(JSContext* cx, GLenum pname, ErrorResult& rv)
{
  
  
  
  

  if (IsContextLost())
    return JS::NullValue();

  MakeContextCurrent();

  switch (pname) {
    
    case LOCAL_GL_RASTERIZER_DISCARD:
    case LOCAL_GL_SAMPLE_ALPHA_TO_COVERAGE:
    case LOCAL_GL_SAMPLE_COVERAGE:
    case LOCAL_GL_TRANSFORM_FEEDBACK_PAUSED:
    case LOCAL_GL_TRANSFORM_FEEDBACK_ACTIVE:
    case LOCAL_GL_UNPACK_SKIP_IMAGES:
    case LOCAL_GL_UNPACK_SKIP_PIXELS:
    case LOCAL_GL_UNPACK_SKIP_ROWS: {
      realGLboolean b = 0;
      gl->fGetBooleanv(pname, &b);
      return JS::BooleanValue(bool(b));
    }

    
    case LOCAL_GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
    case LOCAL_GL_READ_BUFFER:
      

    
    case LOCAL_GL_MAX_3D_TEXTURE_SIZE:
    case LOCAL_GL_MAX_ARRAY_TEXTURE_LAYERS:
    case LOCAL_GL_MAX_COMBINED_UNIFORM_BLOCKS:
    case LOCAL_GL_MAX_ELEMENTS_INDICES:
    case LOCAL_GL_MAX_ELEMENTS_VERTICES:
    case LOCAL_GL_MAX_FRAGMENT_INPUT_COMPONENTS:
    case LOCAL_GL_MAX_FRAGMENT_UNIFORM_BLOCKS:
    case LOCAL_GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
    case LOCAL_GL_MAX_PROGRAM_TEXEL_OFFSET:
    case LOCAL_GL_MAX_SAMPLES:
    case LOCAL_GL_MAX_TEXTURE_LOD_BIAS:
    case LOCAL_GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:
    case LOCAL_GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS:
    case LOCAL_GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS:
    case LOCAL_GL_MAX_UNIFORM_BUFFER_BINDINGS:
    case LOCAL_GL_MAX_VARYING_COMPONENTS:
    case LOCAL_GL_MAX_VERTEX_OUTPUT_COMPONENTS:
    case LOCAL_GL_MAX_VERTEX_UNIFORM_BLOCKS:
    case LOCAL_GL_MAX_VERTEX_UNIFORM_COMPONENTS:
    case LOCAL_GL_MIN_PROGRAM_TEXEL_OFFSET:
    case LOCAL_GL_PACK_ROW_LENGTH:
    case LOCAL_GL_PACK_SKIP_PIXELS:
    case LOCAL_GL_PACK_SKIP_ROWS:
    case LOCAL_GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT:
    case LOCAL_GL_UNPACK_IMAGE_HEIGHT:
    case LOCAL_GL_UNPACK_ROW_LENGTH: {
      GLint val;
      gl->fGetIntegerv(pname, &val);
      return JS::Int32Value(val);
    }

    
    case LOCAL_GL_MAX_CLIENT_WAIT_TIMEOUT_WEBGL:
      return JS::NumberValue(0); 

    case LOCAL_GL_MAX_ELEMENT_INDEX:
    case LOCAL_GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS:
    case LOCAL_GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS:
    case LOCAL_GL_MAX_UNIFORM_BLOCK_SIZE: {
      GLint64 val;
      gl->fGetInteger64v(pname, &val);
      return JS::DoubleValue(static_cast<double>(val));
    }


      
    case LOCAL_GL_MAX_SERVER_WAIT_TIMEOUT: {
      GLuint64 val;
      gl->fGetInteger64v(pname, (GLint64*) &val);
      return JS::DoubleValue(static_cast<double>(val));
    }

    case LOCAL_GL_COPY_READ_BUFFER_BINDING:
      return WebGLObjectAsJSValue(cx, mBoundCopyReadBuffer.get(), rv);

    case LOCAL_GL_COPY_WRITE_BUFFER_BINDING:
      return WebGLObjectAsJSValue(cx, mBoundCopyWriteBuffer.get(), rv);

    case LOCAL_GL_PIXEL_PACK_BUFFER_BINDING:
      return WebGLObjectAsJSValue(cx, mBoundPixelPackBuffer.get(), rv);

    case LOCAL_GL_PIXEL_UNPACK_BUFFER_BINDING:
      return WebGLObjectAsJSValue(cx, mBoundPixelUnpackBuffer.get(), rv);

    case LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
      return WebGLObjectAsJSValue(cx, mBoundTransformFeedbackBuffer.get(), rv);

    case LOCAL_GL_UNIFORM_BUFFER_BINDING:
      return WebGLObjectAsJSValue(cx, mBoundUniformBuffer.get(), rv);

    
    case LOCAL_GL_READ_FRAMEBUFFER_BINDING:
      return WebGLObjectAsJSValue(cx, mBoundReadFramebuffer.get(), rv);

    case LOCAL_GL_SAMPLER_BINDING:
      return WebGLObjectAsJSValue(cx, mBoundSamplers[mActiveTexture].get(), rv);

    case LOCAL_GL_TEXTURE_BINDING_2D_ARRAY:
      
      
      return JS::NullValue();

    case LOCAL_GL_TEXTURE_BINDING_3D:
      return WebGLObjectAsJSValue(cx, mBound3DTextures[mActiveTexture].get(), rv);

    case LOCAL_GL_TRANSFORM_FEEDBACK_BINDING: {
      WebGLTransformFeedback* tf =
        (mBoundTransformFeedback != mDefaultTransformFeedback) ? mBoundTransformFeedback.get() : nullptr;
      return WebGLObjectAsJSValue(cx, tf, rv);
    }

    case LOCAL_GL_VERTEX_ARRAY_BINDING: {
      WebGLVertexArray* vao =
        (mBoundVertexArray != mDefaultVertexArray) ? mBoundVertexArray.get() : nullptr;
      return WebGLObjectAsJSValue(cx, vao, rv);
    }

    case LOCAL_GL_VERSION:
      return StringValue(cx, "WebGL 2.0", rv);

    case LOCAL_GL_SHADING_LANGUAGE_VERSION:
      return StringValue(cx, "WebGL GLSL ES 3.00", rv);

    default:
      return WebGLContext::GetParameter(cx, pname, rv);
  }
}

} 
