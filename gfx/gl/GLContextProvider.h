




#ifndef GLCONTEXTPROVIDER_H_
#define GLCONTEXTPROVIDER_H_

#include "GLContextTypes.h"
#include "nsAutoPtr.h"
#include "SurfaceTypes.h"

#include "nsSize.h" 

class nsIWidget;

namespace mozilla {
namespace gl {

#define IN_GL_CONTEXT_PROVIDER_H


#define GL_CONTEXT_PROVIDER_NAME GLContextProviderNull
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME

#ifdef XP_WIN
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderWGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderWGL
#define DEFAULT_IMPL WGL
#endif

#ifdef XP_MACOSX
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderCGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderCGL
#endif

#if defined(ANDROID) || defined(XP_WIN)
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderEGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#ifndef GL_CONTEXT_PROVIDER_DEFAULT
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderEGL
#endif
#endif

#ifdef MOZ_GL_PROVIDER
#define GL_CONTEXT_PROVIDER_NAME MOZ_GL_PROVIDER
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT MOZ_GL_PROVIDER
#endif

#if defined(MOZ_X11) && !defined(GL_CONTEXT_PROVIDER_DEFAULT)
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderGLX
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderGLX
#endif

#ifdef GL_CONTEXT_PROVIDER_DEFAULT
typedef GL_CONTEXT_PROVIDER_DEFAULT GLContextProvider;
#else
typedef GLContextProviderNull GLContextProvider;
#endif

#undef IN_GL_CONTEXT_PROVIDER_H

} 
} 

#endif
