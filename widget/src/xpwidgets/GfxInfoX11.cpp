






































#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "nsCRTGlue.h"
#include "prenv.h"

#include "GfxInfoX11.h"

#if defined(MOZ_CRASHREPORTER) && defined(MOZ_ENABLE_LIBXUL)
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#endif

namespace mozilla {
namespace widget {


int glxtest_pipe = 0;
pid_t glxtest_pid = 0;

nsresult
GfxInfo::Init()
{
    mMajorVersion = 0;
    mMinorVersion = 0;
    mIsMesa = false;
    mIsNVIDIA = false;
    mIsFGLRX = false;
    return GfxInfoBase::Init();
}

void
GfxInfo::GetData()
{
    
    

    
    if (!glxtest_pipe)
        return;

    enum { buf_size = 1024 };
    char buf[buf_size];
    ssize_t bytesread = read(glxtest_pipe,
                             &buf,
                             buf_size-1); 
    close(glxtest_pipe);
    glxtest_pipe = 0;

    
    if (bytesread < 0)
        bytesread = 0;

    
    buf[bytesread] = 0;

    
    
    
    int glxtest_status = 0;
    bool wait_for_glxtest_process = true;
    bool waiting_for_glxtest_process_failed = false;
    while(wait_for_glxtest_process) {
        wait_for_glxtest_process = false;
        if (waitpid(glxtest_pid, &glxtest_status, 0) == -1) {
            if (errno == EINTR)
                wait_for_glxtest_process = true;
            else
                waiting_for_glxtest_process_failed = true;
        }
    }

    bool exited_with_error_code = !waiting_for_glxtest_process_failed &&
                                  WIFEXITED(glxtest_status) && 
                                  WEXITSTATUS(glxtest_status) != EXIT_SUCCESS;
    bool received_signal = !waiting_for_glxtest_process_failed &&
                           WIFSIGNALED(glxtest_status);

    bool error = waiting_for_glxtest_process_failed || exited_with_error_code || received_signal;

    nsCString *stringToFill = nsnull;
    char *bufptr = buf;
    if (!error) {
        while(true) {
            char *line = NS_strtok("\n", &bufptr);
            if (!line)
                break;
            if (stringToFill) {
                stringToFill->Assign(line);
                stringToFill = nsnull;
            }
            else if(!strcmp(line, "VENDOR"))
                stringToFill = &mVendor;
            else if(!strcmp(line, "RENDERER"))
                stringToFill = &mRenderer;
            else if(!strcmp(line, "VERSION"))
                stringToFill = &mVersion;
        }
    }

    const char *spoofedVendor = PR_GetEnv("MOZ_GFX_SPOOF_GL_VENDOR");
    if (spoofedVendor)
        mVendor.Assign(spoofedVendor);
    const char *spoofedRenderer = PR_GetEnv("MOZ_GFX_SPOOF_GL_RENDERER");
    if (spoofedRenderer)
        mRenderer.Assign(spoofedRenderer);
    const char *spoofedVersion = PR_GetEnv("MOZ_GFX_SPOOF_GL_VERSION");
    if (spoofedVersion)
        mVersion.Assign(spoofedVersion);

    if (error ||
        mVendor.IsEmpty() ||
        mRenderer.IsEmpty() ||
        mVersion.IsEmpty())
    {
        mAdapterDescription.AppendLiteral("GLXtest process failed");
        if (waiting_for_glxtest_process_failed)
            mAdapterDescription.AppendLiteral(" (waitpid failed)");
        if (exited_with_error_code)
            mAdapterDescription.AppendPrintf(" (exited with status %d)", WEXITSTATUS(glxtest_status));
        if (received_signal)
            mAdapterDescription.AppendPrintf(" (received signal %d)", WTERMSIG(glxtest_status));
        if (bytesread) {
            mAdapterDescription.AppendLiteral(": ");
            mAdapterDescription.Append(nsDependentCString(buf));
            mAdapterDescription.AppendLiteral("\n");
        }
#if defined(MOZ_CRASHREPORTER) && defined(MOZ_ENABLE_LIBXUL)
        CrashReporter::AppendAppNotesToCrashReport(mAdapterDescription);
#endif
        return;
    }

    mAdapterDescription.Append(mVendor);
    mAdapterDescription.AppendLiteral(" -- ");
    mAdapterDescription.Append(mRenderer);

    nsCAutoString note;
    note.Append("OpenGL: ");
    note.Append(mAdapterDescription);
    note.Append(" -- ");
    note.Append(mVersion);
    note.Append("\n");
#if defined(MOZ_CRASHREPORTER) && defined(MOZ_ENABLE_LIBXUL)
    CrashReporter::AppendAppNotesToCrashReport(note);
#endif

    
    
    const char *whereToReadVersionNumbers = nsnull;
    const char *Mesa_in_version_string = strstr(mVersion.get(), "Mesa");
    if (Mesa_in_version_string) {
        mIsMesa = true;
        
        
        whereToReadVersionNumbers = Mesa_in_version_string + strlen("Mesa");
    } else if (strstr(mVendor.get(), "NVIDIA Corporation")) {
        mIsNVIDIA = true;
        
        
        
        const char *NVIDIA_in_version_string = strstr(mVersion.get(), "NVIDIA");
        if (NVIDIA_in_version_string)
            whereToReadVersionNumbers = NVIDIA_in_version_string + strlen("NVIDIA");
    } else if (strstr(mVendor.get(), "ATI Technologies Inc")) {
        mIsFGLRX = true;
        
        
        whereToReadVersionNumbers = mVersion.get();
    }

    
    if (whereToReadVersionNumbers) {
        
        strncpy(buf, whereToReadVersionNumbers, buf_size);
        bufptr = buf;

        
        
        char *token = NS_strtok(".", &bufptr);
        if (token) {
            mMajorVersion = strtol(token, 0, 10);
            token = NS_strtok(".", &bufptr);
            if (token)
                mMinorVersion = strtol(token, 0, 10);
        }
    }
}

static inline PRUint64 version(PRUint32 major, PRUint32 minor)
{
    return (PRUint64(major) << 32) + PRUint64(minor);
}

nsresult
GfxInfo::GetFeatureStatusImpl(PRInt32 aFeature, PRInt32 *aStatus, nsAString & aSuggestedDriverVersion, GfxDriverInfo* aDriverInfo )
{
    GetData();
    *aStatus = nsIGfxInfo::FEATURE_NO_INFO;
    aSuggestedDriverVersion.SetIsVoid(PR_TRUE);

#ifdef MOZ_PLATFORM_MAEMO
    
    return NS_OK;
#endif

    
    
    
    
    
    if (mIsNVIDIA &&
        !strcmp(mRenderer.get(), "GeForce 9400/PCI/SSE2") &&
        !strcmp(mVersion.get(), "3.2.0 NVIDIA 190.42"))
    {
        return NS_OK;
    }

    if (mIsMesa) {
        if (version(mMajorVersion, mMinorVersion) < version(7,10)) {
            *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
            aSuggestedDriverVersion.AssignLiteral("Mesa 7.10");
        }
        else if (strstr(mRenderer.get(), "Gallium")) {
            
            
            *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
            aSuggestedDriverVersion.AssignLiteral("<NOT Gallium>");
        }
    } else if (mIsNVIDIA) {
        if (version(mMajorVersion, mMinorVersion) < version(257,21)) {
            *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
            aSuggestedDriverVersion.AssignLiteral("NVIDIA 257.21");
        }
    } else if (mIsFGLRX) {
        
        
        if (version(mMajorVersion, mMinorVersion) < version(3, 0)) {
            *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
        }
    } else {
        
        
        *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
    }
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetD2DEnabled(PRBool *aEnabled)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
GfxInfo::GetDWriteEnabled(PRBool *aEnabled)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetDWriteVersion(nsAString & aDwriteVersion)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription(nsAString & aAdapterDescription)
{
  GetData();
  AppendASCIItoUTF16(mAdapterDescription, aAdapterDescription);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM(nsAString & aAdapterRAM)
{
  aAdapterRAM.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver(nsAString & aAdapterDriver)
{
  aAdapterDriver.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion(nsAString & aAdapterDriverVersion)
{
  GetData();
  CopyASCIItoUTF16(mVersion, aAdapterDriverVersion);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate(nsAString & aAdapterDriverDate)
{
  aAdapterDriverDate.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID(PRUint32 *aAdapterVendorID)
{
  *aAdapterVendorID = 0;
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(PRUint32 *aAdapterDeviceID)
{
  *aAdapterDeviceID = 0;
  return NS_OK;
}


} 
} 
