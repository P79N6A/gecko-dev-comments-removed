



#include <dlfcn.h>
#include <android/log.h>
#include "AndroidBridge.h"
#include "ANPBase.h"
#include "GLContextProvider.h"
#include "nsNPAPIPluginInstance.h"
#include "nsPluginInstanceOwner.h"
#include "GLContextProvider.h"
#include "GLContextEGL.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_opengl_##name

using namespace mozilla;
using namespace mozilla::gl;

typedef nsNPAPIPluginInstance::TextureInfo TextureInfo;

static ANPEGLContext anp_opengl_acquireContext(NPP instance) {
    nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

    GLContext* context = pinst->GLContext();
    if (!context)
        return nullptr;

    context->MakeCurrent();
    return GLContextEGL::Cast(context)->GetEGLContext();
}

static ANPTextureInfo anp_opengl_lockTexture(NPP instance) {
    nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

    TextureInfo pluginInfo = pinst->LockContentTexture();

    ANPTextureInfo info;
    info.textureId = pluginInfo.mTexture;
    info.width = pluginInfo.mWidth;
    info.height = pluginInfo.mHeight;

    
    
    
    
    
    info.internalFormat = 0;

    return info;
}

static void anp_opengl_releaseTexture(NPP instance, const ANPTextureInfo* info) {
    nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

    TextureInfo pluginInfo(info->textureId, info->width, info->height, info->internalFormat);
    pinst->ReleaseContentTexture(pluginInfo);
    pinst->RedrawPlugin();
}

static void anp_opengl_invertPluginContent(NPP instance, bool isContentInverted) {
    
    gl::OriginPos newOriginPos = gl::OriginPos::BottomLeft;
    if (isContentInverted)
        newOriginPos = gl::OriginPos::TopLeft;

    nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

    pinst->SetOriginPos(newOriginPos);
    pinst->RedrawPlugin();
}



void InitOpenGLInterface(ANPOpenGLInterfaceV0* i) {
    ASSIGN(i, acquireContext);
    ASSIGN(i, lockTexture);
    ASSIGN(i, releaseTexture);
    ASSIGN(i, invertPluginContent);
}
