






































#include "nsDirectoryServiceUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIPrefService.h"

#include "glwrap.h"

#include "nsGLPbuffer.h"

#include "gfxContext.h"
#include "gfxImageSurface.h"

using namespace mozilla;

#if 0
#include <GL/osmesa.h>
#else
#define OSMESA_RGBA		GL_RGBA
#define OSMESA_BGRA		0x1
#define OSMESA_ARGB		0x2
#define OSMESA_Y_UP		0x11
#endif

static OSMesaWrap gMesaWrap;

static PRUint32 gActiveBuffers = 0;

nsGLPbufferOSMESA::nsGLPbufferOSMESA()
    : mMesaContext(nsnull)
{
    gActiveBuffers++;
    fprintf (stderr, "nsGLPbufferOSMESA: gActiveBuffers: %d\n", gActiveBuffers);
}

PRBool
nsGLPbufferOSMESA::Init(WebGLContext *priv)
{
    mPriv = priv;
    nsresult rv;

    nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    nsCOMPtr<nsIPrefBranch> prefBranch;
    rv = prefService->GetBranch("webgl.", getter_AddRefs(prefBranch));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    nsCString osmesalib;

    rv = prefBranch->GetCharPref("osmesalib", getter_Copies(osmesalib));

#if 0
    if (NS_FAILED(rv)) {
        osmesalib.Truncate();

        
        nsCOMPtr<nsIFile> libfile;
        rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(libfile));
        if (NS_FAILED(rv)) {
            fprintf (stderr, "NS_GetSpecialDirectory failed?\n");
            return rv;
        }

        
        rv |= libfile->Append(NS_LITERAL_STRING("extensions"));
        rv |= libfile->Append(NS_LITERAL_STRING("canvas3d@mozilla.com"));
        rv |= libfile->Append(NS_LITERAL_STRING("platform"));

#if defined(XP_WIN)
        rv |= libfile->Append(NS_LITERAL_STRING("WINNT"));
        rv |= libfile->Append(NS_LITERAL_STRING("osmesa32.dll"));
#elif defined(XP_MACOSX)
        rv |= libfile->Append(NS_LITERAL_STRING("Darwin"));
        rv |= libfile->Append(NS_LITERAL_STRING("libOSMesa.7.dylib"));
#elif defined(XP_UNIX)
        rv |= libfile->Append(NS_LITERAL_STRING("Linux"));
        rv |= libfile->Append(NS_LITERAL_STRING("libOSMesa.so.7"));
#else
#warning No default osmesa library path available
        LogMessage("Canvas 3D: No default OSMesa lib path available -- please set the extensions.canvas3d.osmesalib pref to the full path to the OSMesa shared library");
        rv = NS_ERROR_FAILURE;
#endif

        if (NS_FAILED(rv))
            return PR_FALSE;

        PRBool exists = PR_FALSE;
        rv = libfile->Exists(&exists);
        if (NS_FAILED(rv) || !exists) {
            LogMessage("Canvas 3D: Couldn't find OSMesa lib -- either default or extension.canvas3d.osmesalib path is incorrect");
            return PR_FALSE;
        }

        
        rv = libfile->GetNativeTarget(osmesalib);
        if (NS_FAILED(rv)) {
            LogMessage("Canvas 3D: Couldn't find OSMesa lib");
            return PR_FALSE;
        }
    }
#endif
    if (NS_FAILED(rv) ||
        osmesalib.Length() == 0 ||
        !gMesaWrap.OpenLibrary(osmesalib.get()))
    {
        LogMessage("Canvas 3D: Couldn't open OSMesa lib -- webgl.osmesalib path is incorrect, or not a valid shared library");
        return PR_FALSE;
    }

    if (!gMesaWrap.Init())
        return PR_FALSE;

    PRInt32 prefAntialiasing;
    rv = prefBranch->GetIntPref("antialiasing", &prefAntialiasing);
    if (NS_FAILED(rv))
        prefAntialiasing = 0;

    Resize (2, 2);

    if (!mGLWrap.OpenLibrary(osmesalib.get())) {
        LogMessage("Canvas 3D: Couldn't open OSMesa lib [1]");
        return PR_FALSE;
    }

    mGLWrap.SetLookupFunc((LibrarySymbolLoader::PlatformLookupFunction) gMesaWrap.fGetProcAddress);

    if (!mGLWrap.Init(GLES20Wrap::TRY_SOFTWARE_GL)) {
        LogMessage("Canvas 3D: GLWrap init failed");
        return PR_FALSE;
    }

    return PR_TRUE;
}

PRBool
nsGLPbufferOSMESA::Resize(PRInt32 width, PRInt32 height)
{
    if (mWidth == width &&
        mHeight == height)
    {
        return PR_TRUE;
    }

    Destroy();

    mThebesSurface = new gfxImageSurface(gfxIntSize(width, height),
                                         gfxASurface::ImageFormatARGB32);
    if (mThebesSurface->CairoStatus() != 0) {
        fprintf (stderr, "image surface failed\n");
        return PR_FALSE;
    }

    mMesaContext = gMesaWrap.fCreateContextExt (OSMESA_BGRA, 16, 0, 0, NULL);
    if (!mMesaContext) {
        fprintf (stderr, "OSMesaCreateContextExt failed!\n");
        return PR_FALSE;
    }

    fprintf (stderr, "Surface: %p\n", mThebesSurface->Data());

    if (!gMesaWrap.fMakeCurrent (mMesaContext, mThebesSurface->Data(), LOCAL_GL_UNSIGNED_BYTE, width, height))
    {
        fprintf (stderr, "OSMesaMakeCurrent failed!\n");
        return PR_FALSE;
    }

    gMesaWrap.fPixelStore (OSMESA_Y_UP, 1);

    mWidth = width;
    mHeight = height;

    fprintf (stderr, "Resize: %d %d\n", width, height);
    return PR_TRUE;
}

void
nsGLPbufferOSMESA::Destroy()
{
    if (mMesaContext) {
        gMesaWrap.fDestroyContext (mMesaContext);
        mMesaContext = 0;
    }

    sCurrentContextToken = nsnull;
    mThebesSurface = nsnull;
}

nsGLPbufferOSMESA::~nsGLPbufferOSMESA()
{
    Destroy();

    gActiveBuffers--;
    fprintf (stderr, "nsGLPbufferOSMESA: gActiveBuffers: %d\n", gActiveBuffers);
    fflush (stderr);
}

void
nsGLPbufferOSMESA::MakeContextCurrent()
{
    if (gMesaWrap.fGetCurrentContext() == mMesaContext)
        return;

    gMesaWrap.fMakeCurrent (mMesaContext, mThebesSurface->Data(), LOCAL_GL_UNSIGNED_BYTE, mWidth, mHeight);
}

void
nsGLPbufferOSMESA::SwapBuffers()
{
    
}

gfxASurface*
nsGLPbufferOSMESA::ThebesSurface()
{
    return mThebesSurface;
}
