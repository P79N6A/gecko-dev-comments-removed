























#include <dlfcn.h>
#include <android/log.h>
#include "AndroidBridge.h"
#include "ANPBase.h"
#include "GLContextProvider.h"
#include "nsNPAPIPluginInstance.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_opengl_##name

using namespace mozilla;
using namespace mozilla::gl;

static ANPEGLContext anp_opengl_acquireContext(NPP inst) {
    
    NOT_IMPLEMENTED();
    return NULL;
}

static ANPTextureInfo anp_opengl_lockTexture(NPP instance) {
    ANPTextureInfo info = { 0, 0, 0, 0 };
    NOT_IMPLEMENTED();
    return info;
}

static void anp_opengl_releaseTexture(NPP instance, const ANPTextureInfo* info) {
    NOT_IMPLEMENTED();
}

static void anp_opengl_invertPluginContent(NPP instance, bool isContentInverted) {
    NOT_IMPLEMENTED();
}



void InitOpenGLInterface(ANPOpenGLInterfaceV0* i) {
    ASSIGN(i, acquireContext);
    ASSIGN(i, lockTexture);
    ASSIGN(i, releaseTexture);
    ASSIGN(i, invertPluginContent);
}
