




#include "WebGL2Context.h"
#include "GLContext.h"
#include "mozilla/dom/WebGL2RenderingContextBinding.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;
using namespace mozilla::gl;




WebGL2Context::WebGL2Context()
    : WebGLContext()
{
    MOZ_ASSERT(IsSupported(), "not supposed to create a WebGL2Context"
                              "context when not supported");
}

WebGL2Context::~WebGL2Context()
{

}





bool
WebGL2Context::IsSupported()
{
    return Preferences::GetBool("webgl.enable-prototype-webgl2", false);
}

WebGL2Context*
WebGL2Context::Create()
{
    return new WebGL2Context();
}





JSObject*
WebGL2Context::WrapObject(JSContext *cx)
{
    return dom::WebGL2RenderingContextBinding::Wrap(cx, this);
}





bool
WebGLContext::InitWebGL2()
{
    MOZ_ASSERT(IsWebGL2(), "WebGLContext is not a WebGL 2 context!");

    const WebGLExtensionID sExtensionNativelySupportedArr[] = {
        WebGLExtensionID::ANGLE_instanced_arrays,
        WebGLExtensionID::EXT_blend_minmax,
        WebGLExtensionID::OES_element_index_uint,
        WebGLExtensionID::OES_standard_derivatives,
        WebGLExtensionID::OES_texture_float,
        WebGLExtensionID::OES_texture_float_linear,
        WebGLExtensionID::OES_vertex_array_object,
        WebGLExtensionID::WEBGL_depth_texture,
        WebGLExtensionID::WEBGL_draw_buffers
    };
    const GLFeature sFeatureRequiredArr[] = {
        GLFeature::instanced_non_arrays,
        GLFeature::transform_feedback2
    };

    
    for (size_t i = 0; i < size_t(MOZ_ARRAY_LENGTH(sExtensionNativelySupportedArr)); i++)
    {
        WebGLExtensionID extension = sExtensionNativelySupportedArr[i];

        if (!IsExtensionSupported(extension)) {
            GenerateWarning("WebGL 2 requires %s!", GetExtensionString(extension));
            return false;
        }
    }

    
    if (!gl->IsExtensionSupported(GLContext::EXT_gpu_shader4)) {
        GenerateWarning("WebGL 2 requires GL_EXT_gpu_shader4!");
        return false;
    }

    
    if (!gl->IsSupported(GLFeature::occlusion_query) &&
        !gl->IsSupported(GLFeature::occlusion_query_boolean))
    {
        



        GenerateWarning("WebGL 2 requires occlusion queries!");
        return false;
    }

    for (size_t i = 0; i < size_t(MOZ_ARRAY_LENGTH(sFeatureRequiredArr)); i++)
    {
        if (!gl->IsSupported(sFeatureRequiredArr[i])) {
            GenerateWarning("WebGL 2 requires GLFeature::%s!", GLContext::GetFeatureName(sFeatureRequiredArr[i]));
            return false;
        }
    }

    
    for (size_t i = 0; i < size_t(MOZ_ARRAY_LENGTH(sExtensionNativelySupportedArr)); i++) {
        EnableExtension(sExtensionNativelySupportedArr[i]);

        MOZ_ASSERT(IsExtensionEnabled(sExtensionNativelySupportedArr[i]));
    }

    
    gl->GetUIntegerv(LOCAL_GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &mGLMaxTransformFeedbackSeparateAttribs);

    return true;
}
