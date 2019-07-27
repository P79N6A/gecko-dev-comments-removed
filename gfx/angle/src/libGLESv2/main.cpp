







#include "libGLESv2/main.h"
#include "libGLESv2/Context.h"

#include "common/tls.h"

static TLSIndex currentTLS = TLS_INVALID_INDEX;

namespace gl
{










bool CreateThreadLocalIndex()
{
    currentTLS = CreateTLSIndex();
    if (currentTLS == TLS_INVALID_INDEX)
    {
        return false;
    }
    return true;
}


void DestroyThreadLocalIndex()
{
    DestroyTLSIndex(currentTLS);
    currentTLS = TLS_INVALID_INDEX;
}


Current *AllocateCurrent()
{
    ASSERT(currentTLS != TLS_INVALID_INDEX);
    if (currentTLS == TLS_INVALID_INDEX)
    {
        return NULL;
    }

    Current *current = new Current();
    current->context = NULL;
    current->display = NULL;

    if (!SetTLSValue(currentTLS, current))
    {
        ERR("Could not set thread local storage.");
        return NULL;
    }

    return current;
}


void DeallocateCurrent()
{
    Current *current = reinterpret_cast<Current*>(GetTLSValue(currentTLS));
    SafeDelete(current);
    SetTLSValue(currentTLS, NULL);
}

}

#ifdef ANGLE_PLATFORM_WINDOWS
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        {
            if (!gl::CreateThreadLocalIndex())
            {
                return FALSE;
            }

#ifdef ANGLE_ENABLE_DEBUG_ANNOTATIONS
            gl::InitializeDebugAnnotations();
#endif
        }
        
      case DLL_THREAD_ATTACH:
        {
            gl::AllocateCurrent();
        }
        break;
      case DLL_THREAD_DETACH:
        {
            gl::DeallocateCurrent();
        }
        break;
      case DLL_PROCESS_DETACH:
        {
            gl::DeallocateCurrent();
            gl::DestroyThreadLocalIndex();

#ifdef ANGLE_ENABLE_DEBUG_ANNOTATIONS
            gl::UninitializeDebugAnnotations();
#endif
        }
        break;
      default:
        break;
    }

    return TRUE;
}
#endif

namespace gl
{

Current *GetCurrentData()
{
    Current *current = reinterpret_cast<Current*>(GetTLSValue(currentTLS));

    
    
    return (current ? current : AllocateCurrent());
}

void makeCurrent(Context *context, egl::Display *display, egl::Surface *surface)
{
    Current *current = GetCurrentData();

    current->context = context;
    current->display = display;

    if (context && display && surface)
    {
        context->makeCurrent(surface);
    }
}

Context *getContext()
{
    Current *current = GetCurrentData();

    return current->context;
}

Context *getNonLostContext()
{
    Context *context = getContext();

    if (context)
    {
        if (context->isContextLost())
        {
            gl::error(GL_OUT_OF_MEMORY);
            return NULL;
        }
        else
        {
            return context;
        }
    }
    return NULL;
}

egl::Display *getDisplay()
{
    Current *current = GetCurrentData();

    return current->display;
}


void error(GLenum errorCode)
{
    gl::Context *context = glGetCurrentContext();
    context->recordError(Error(errorCode));

    switch (errorCode)
    {
      case GL_INVALID_ENUM:
        TRACE("\t! Error generated: invalid enum\n");
        break;
      case GL_INVALID_VALUE:
        TRACE("\t! Error generated: invalid value\n");
        break;
      case GL_INVALID_OPERATION:
        TRACE("\t! Error generated: invalid operation\n");
        break;
      case GL_OUT_OF_MEMORY:
        TRACE("\t! Error generated: out of memory\n");
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        TRACE("\t! Error generated: invalid framebuffer operation\n");
        break;
      default: UNREACHABLE();
    }
}

}
