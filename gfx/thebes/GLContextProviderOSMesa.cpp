




































#include "GLContextProvider.h"
#include "GLContext.h"
#include "nsDebug.h"
#include "nsString.h"
#include "nsIWidget.h"
#include "nsDirectoryServiceUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIConsoleService.h"
#include "mozilla/Preferences.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include "gfxCrashReporterUtils.h"


#define OSMESA_RGBA     GL_RGBA
#define OSMESA_BGRA     0x1
#define OSMESA_ARGB     0x2
#define OSMESA_RGB      GL_RGB
#define OSMESA_BGR      0x4
#define OSMESA_RGB_565  0x5
#define OSMESA_Y_UP     0x11

namespace mozilla {
namespace gl {

static void LogMessage(const char *msg)
{
  nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
  if (console) {
    console->LogStringMessage(NS_ConvertUTF8toUTF16(nsDependentCString(msg)).get());
    fprintf(stderr, "%s\n", msg);
  }
}

typedef void* PrivateOSMesaContext;

class OSMesaLibrary
{
public:
    OSMesaLibrary() : mInitialized(PR_FALSE), mOSMesaLibrary(nsnull) {}

    typedef PrivateOSMesaContext (GLAPIENTRY * PFNOSMESACREATECONTEXTEXT) (GLenum, GLint, GLint, GLint, PrivateOSMesaContext);
    typedef void (GLAPIENTRY * PFNOSMESADESTROYCONTEXT) (PrivateOSMesaContext);
    typedef bool (GLAPIENTRY * PFNOSMESAMAKECURRENT) (PrivateOSMesaContext, void *, GLenum, GLsizei, GLsizei);
    typedef PrivateOSMesaContext (GLAPIENTRY * PFNOSMESAGETCURRENTCONTEXT) (void);
    typedef void (GLAPIENTRY * PFNOSMESAPIXELSTORE) (GLint, GLint);
    typedef PRFuncPtr (GLAPIENTRY * PFNOSMESAGETPROCADDRESS) (const char*);

    PFNOSMESACREATECONTEXTEXT fCreateContextExt;
    PFNOSMESADESTROYCONTEXT fDestroyContext;
    PFNOSMESAMAKECURRENT fMakeCurrent;
    PFNOSMESAGETCURRENTCONTEXT fGetCurrentContext;
    PFNOSMESAPIXELSTORE fPixelStore;
    PFNOSMESAGETPROCADDRESS fGetProcAddress;

    PRBool EnsureInitialized();

private:
    PRBool mInitialized;
    PRLibrary *mOSMesaLibrary;
};

OSMesaLibrary sOSMesaLibrary;

PRBool
OSMesaLibrary::EnsureInitialized()
{
    if (mInitialized)
        return PR_TRUE;

    nsAdoptingCString osmesalib = Preferences::GetCString("webgl.osmesalib");
    if (osmesalib.IsEmpty()) {
        return PR_FALSE;
    }

    mOSMesaLibrary = PR_LoadLibrary(osmesalib.get());

    if (!mOSMesaLibrary) {
        LogMessage("Couldn't open OSMesa lib for software rendering -- webgl.osmesalib path is incorrect, or not a valid shared library");
        return PR_FALSE;
    }

    LibrarySymbolLoader::SymLoadStruct symbols[] = {
        { (PRFuncPtr*) &fCreateContextExt, { "OSMesaCreateContextExt", NULL } },
        { (PRFuncPtr*) &fMakeCurrent, { "OSMesaMakeCurrent", NULL } },
        { (PRFuncPtr*) &fPixelStore, { "OSMesaPixelStore", NULL } },
        { (PRFuncPtr*) &fDestroyContext, { "OSMesaDestroyContext", NULL } },
        { (PRFuncPtr*) &fGetCurrentContext, { "OSMesaGetCurrentContext", NULL } },
        { (PRFuncPtr*) &fMakeCurrent, { "OSMesaMakeCurrent", NULL } },
        { (PRFuncPtr*) &fGetProcAddress, { "OSMesaGetProcAddress", NULL } },
        { NULL, { NULL } }
    };

    if (!LibrarySymbolLoader::LoadSymbols(mOSMesaLibrary, &symbols[0])) {
        LogMessage("Couldn't find required entry points in OSMesa libary");
        return PR_FALSE;
    }

    mInitialized = PR_TRUE;
    return PR_TRUE;
}

class GLContextOSMesa : public GLContext
{
public:
    GLContextOSMesa(const ContextFormat& aFormat)
        : GLContext(aFormat, PR_TRUE, nsnull),
          mThebesSurface(nsnull),
          mContext(nsnull)
    {
    }

    ~GLContextOSMesa()
    {
        MarkDestroyed();

        if (mContext)
            sOSMesaLibrary.fDestroyContext(mContext);
    }

    GLContextType GetContextType() {
        return ContextTypeOSMesa;
    }

    PRBool Init(const gfxIntSize &aSize)
    {
        int osmesa_format = -1;
        int gfxasurface_imageformat = -1;
        PRBool format_accepted = PR_FALSE;

        if (mCreationFormat.red > 0 &&
            mCreationFormat.green > 0 &&
            mCreationFormat.blue > 0 &&
            mCreationFormat.red <= 8 &&
            mCreationFormat.green <= 8 &&
            mCreationFormat.blue <= 8)
        {
            if (mCreationFormat.alpha == 0) {
                
                
                
                osmesa_format = OSMESA_BGRA;
                gfxasurface_imageformat = gfxASurface::ImageFormatRGB24;
                format_accepted = PR_TRUE;
            } else if (mCreationFormat.alpha <= 8) {
                osmesa_format = OSMESA_BGRA;
                gfxasurface_imageformat = gfxASurface::ImageFormatARGB32;
                format_accepted = PR_TRUE;
            }
        }
        if (!format_accepted) {
            NS_WARNING("Pixel format not supported with OSMesa.");
            return PR_FALSE;
        }

        mThebesSurface = new gfxImageSurface(aSize, gfxASurface::gfxImageFormat(gfxasurface_imageformat));
        if (mThebesSurface->CairoStatus() != 0) {
            NS_WARNING("image surface failed");
            return PR_FALSE;
        }

        mContext = sOSMesaLibrary.fCreateContextExt(osmesa_format, mCreationFormat.depth, mCreationFormat.stencil, 0, NULL);
        if (!mContext) {
            NS_WARNING("OSMesaCreateContextExt failed!");
            return PR_FALSE;
        }

        if (!MakeCurrent()) return PR_FALSE;
        if (!SetupLookupFunction()) return PR_FALSE;

        
        sOSMesaLibrary.fPixelStore(OSMESA_Y_UP, 0);

        return InitWithPrefix("gl", PR_TRUE);
    }

    PRBool MakeCurrentImpl(PRBool aForce = PR_FALSE)
    {
        PRBool succeeded
          = sOSMesaLibrary.fMakeCurrent(mContext, mThebesSurface->Data(),
                                        LOCAL_GL_UNSIGNED_BYTE,
                                        mThebesSurface->Width(),
                                        mThebesSurface->Height());
        NS_ASSERTION(succeeded, "Failed to make OSMesa context current!");

        return succeeded;
    }

    PRBool SetupLookupFunction()
    {
        mLookupFunc = (PlatformLookupFunction)sOSMesaLibrary.fGetProcAddress;
        return PR_TRUE;
    }

    void *GetNativeData(NativeDataType aType)
    {
        switch (aType) {
        case NativeImageSurface:
            return mThebesSurface.get();
        default:
            return nsnull;
        }
    }

private:
    nsRefPtr<gfxImageSurface> mThebesSurface;
    PrivateOSMesaContext mContext;
};

already_AddRefed<GLContext>
GLContextProviderOSMesa::CreateForWindow(nsIWidget *aWidget)
{
    return nsnull;
}

already_AddRefed<GLContext>
GLContextProviderOSMesa::CreateOffscreen(const gfxIntSize& aSize,
                                         const ContextFormat& aFormat)
{
    if (!sOSMesaLibrary.EnsureInitialized()) {
        return nsnull;
    }

    nsRefPtr<GLContextOSMesa> glContext = new GLContextOSMesa(aFormat);

    if (!glContext->Init(aSize))
    {
        return nsnull;
    }

    return glContext.forget();
}

GLContext *
GLContextProviderOSMesa::GetGlobalContext()
{
    return nsnull;
}

void
GLContextProviderOSMesa::Shutdown()
{
}

} 
} 
