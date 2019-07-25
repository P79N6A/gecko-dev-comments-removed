






#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/utsname.h>
#include "nsCRTGlue.h"
#include "prenv.h"

#include "GfxInfoX11.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#endif

namespace mozilla {
namespace widget {

#ifdef DEBUG
NS_IMPL_ISUPPORTS_INHERITED1(GfxInfo, GfxInfoBase, nsIGfxInfoDebug)
#endif


int glxtest_pipe = 0;
pid_t glxtest_pid = 0;

nsresult
GfxInfo::Init()
{
    mGLMajorVersion = 0;
    mMajorVersion = 0;
    mMinorVersion = 0;
    mRevisionVersion = 0;
    mIsMesa = false;
    mIsNVIDIA = false;
    mIsFGLRX = false;
    mIsNouveau = false;
    mHasTextureFromPixmap = false;
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
    int waitpid_errno = 0;
    while(wait_for_glxtest_process) {
        wait_for_glxtest_process = false;
        if (waitpid(glxtest_pid, &glxtest_status, 0) == -1) {
            waitpid_errno = errno;
            if (waitpid_errno == EINTR) {
                wait_for_glxtest_process = true;
            } else {
                
                
                
                
                waiting_for_glxtest_process_failed = (waitpid_errno != ECHILD);
            }
        }
    }

    bool exited_with_error_code = !waiting_for_glxtest_process_failed &&
                                  WIFEXITED(glxtest_status) && 
                                  WEXITSTATUS(glxtest_status) != EXIT_SUCCESS;
    bool received_signal = !waiting_for_glxtest_process_failed &&
                           WIFSIGNALED(glxtest_status);

    bool error = waiting_for_glxtest_process_failed || exited_with_error_code || received_signal;

    nsCString textureFromPixmap; 
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
            else if(!strcmp(line, "TFP"))
                stringToFill = &textureFromPixmap;
        }
    }

    if (!strcmp(textureFromPixmap.get(), "TRUE"))
        mHasTextureFromPixmap = true;

    
    
    utsname unameobj;
    if (!uname(&unameobj))
    {
      mOS.Assign(unameobj.sysname);
      mOSRelease.Assign(unameobj.release);
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
    const char *spoofedOS = PR_GetEnv("MOZ_GFX_SPOOF_OS");
    if (spoofedOS)
        mOS.Assign(spoofedOS);
    const char *spoofedOSRelease = PR_GetEnv("MOZ_GFX_SPOOF_OS_RELEASE");
    if (spoofedOSRelease)
        mOSRelease.Assign(spoofedOSRelease);

    if (error ||
        mVendor.IsEmpty() ||
        mRenderer.IsEmpty() ||
        mVersion.IsEmpty() ||
        mOS.IsEmpty() ||
        mOSRelease.IsEmpty())
    {
        mAdapterDescription.AppendLiteral("GLXtest process failed");
        if (waiting_for_glxtest_process_failed)
            mAdapterDescription.AppendPrintf(" (waitpid failed with errno=%d for pid %d)", waitpid_errno, glxtest_pid);
        if (exited_with_error_code)
            mAdapterDescription.AppendPrintf(" (exited with status %d)", WEXITSTATUS(glxtest_status));
        if (received_signal)
            mAdapterDescription.AppendPrintf(" (received signal %d)", WTERMSIG(glxtest_status));
        if (bytesread) {
            mAdapterDescription.AppendLiteral(": ");
            mAdapterDescription.Append(nsDependentCString(buf));
            mAdapterDescription.AppendLiteral("\n");
        }
#ifdef MOZ_CRASHREPORTER
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
    if (mHasTextureFromPixmap)
        note.Append(" -- texture_from_pixmap");
    note.Append("\n");
#ifdef MOZ_CRASHREPORTER
    CrashReporter::AppendAppNotesToCrashReport(note);
#endif

    
    mGLMajorVersion = strtol(mVersion.get(), 0, 10);

    
    
    const char *whereToReadVersionNumbers = nsnull;
    const char *Mesa_in_version_string = strstr(mVersion.get(), "Mesa");
    if (Mesa_in_version_string) {
        mIsMesa = true;
        
        
        whereToReadVersionNumbers = Mesa_in_version_string + strlen("Mesa");
        if (strcasestr(mVendor.get(), "nouveau"))
            mIsNouveau = true;
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
            if (token) {
                mMinorVersion = strtol(token, 0, 10);
                token = NS_strtok(".", &bufptr);
                if (token)
                    mRevisionVersion = strtol(token, 0, 10);
            }
        }
    }
}

static inline PRUint64 version(PRUint32 major, PRUint32 minor, PRUint32 revision = 0)
{
    return (PRUint64(major) << 32) + (PRUint64(minor) << 16) + PRUint64(revision);
}

const nsTArray<GfxDriverInfo>&
GfxInfo::GetGfxDriverInfo()
{
  
  
  
  
  return *mDriverInfo;
}

nsresult
GfxInfo::GetFeatureStatusImpl(PRInt32 aFeature, 
                              PRInt32 *aStatus, 
                              nsAString & aSuggestedDriverVersion, 
                              const nsTArray<GfxDriverInfo>& aDriverInfo, 
                              OperatingSystem* aOS )

{
  GetData();

  NS_ENSURE_ARG_POINTER(aStatus);
  *aStatus = nsIGfxInfo::FEATURE_STATUS_UNKNOWN;
  aSuggestedDriverVersion.SetIsVoid(true);
  OperatingSystem os = DRIVER_OS_LINUX;
  if (aOS)
    *aOS = os;

  if (mGLMajorVersion == 1) {
    
    
    
    *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
    return NS_OK;
  }

#ifdef MOZ_PLATFORM_MAEMO
  *aStatus = nsIGfxInfo::FEATURE_NO_INFO;
  
  return NS_OK;
#endif

  
  if (!aDriverInfo.Length()) {
    
    if (aFeature == nsIGfxInfo::FEATURE_OPENGL_LAYERS ||
        aFeature == nsIGfxInfo::FEATURE_WEBGL_OPENGL ||
        aFeature == nsIGfxInfo::FEATURE_WEBGL_MSAA) {

      
      if (aFeature == nsIGfxInfo::FEATURE_OPENGL_LAYERS && !mHasTextureFromPixmap) {
        *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
        aSuggestedDriverVersion.AssignLiteral("<Anything with EXT_texture_from_pixmap support>");
        return NS_OK;
      }

      
      
      
      
      
      if (mIsNVIDIA &&
          !strcmp(mRenderer.get(), "GeForce 9400/PCI/SSE2") &&
          !strcmp(mVersion.get(), "3.2.0 NVIDIA 190.42"))
      {
        *aStatus = nsIGfxInfo::FEATURE_NO_INFO;
        return NS_OK;
      }

      if (mIsMesa) {
        if (mIsNouveau && version(mMajorVersion, mMinorVersion) < version(8,0)) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
          aSuggestedDriverVersion.AssignLiteral("Mesa 8.0");
        } else if (version(mMajorVersion, mMinorVersion, mRevisionVersion) < version(7,10,3)) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
          aSuggestedDriverVersion.AssignLiteral("Mesa 7.10.3");
        }
      } else if (mIsNVIDIA) {
        if (version(mMajorVersion, mMinorVersion, mRevisionVersion) < version(257,21)) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
          aSuggestedDriverVersion.AssignLiteral("NVIDIA 257.21");
        }
      } else if (mIsFGLRX) {
        
        
        if (version(mMajorVersion, mMinorVersion, mRevisionVersion) < version(3, 0)) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
          aSuggestedDriverVersion.AssignLiteral("<Something recent>");
        }
        
        bool unknownOS = mOS.IsEmpty() || mOSRelease.IsEmpty();
        bool badOS = mOS.Find("Linux", true) != -1 &&
                     mOSRelease.Find("2.6.32") != -1;
        if (unknownOS || badOS) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_OS_VERSION;
        }
      } else {
        
        
        *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
      }
    }
  }

  return GfxInfoBase::GetFeatureStatusImpl(aFeature, aStatus, aSuggestedDriverVersion, aDriverInfo, &os);
}


NS_IMETHODIMP
GfxInfo::GetD2DEnabled(bool *aEnabled)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
GfxInfo::GetDWriteEnabled(bool *aEnabled)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
GfxInfo::GetAzureEnabled(bool *aEnabled)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetDWriteVersion(nsAString & aDwriteVersion)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetCleartypeParameters(nsAString & aCleartypeParams)
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
GfxInfo::GetAdapterDescription2(nsAString & aAdapterDescription)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM(nsAString & aAdapterRAM)
{
  aAdapterRAM.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM2(nsAString & aAdapterRAM)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver(nsAString & aAdapterDriver)
{
  aAdapterDriver.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver2(nsAString & aAdapterDriver)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion(nsAString & aAdapterDriverVersion)
{
  GetData();
  CopyASCIItoUTF16(mVersion, aAdapterDriverVersion);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate(nsAString & aAdapterDriverDate)
{
  aAdapterDriverDate.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate2(nsAString & aAdapterDriverDate)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID(nsAString & aAdapterVendorID)
{
  GetData();
  CopyUTF8toUTF16(mVendor, aAdapterVendorID);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID2(nsAString & aAdapterVendorID)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(nsAString & aAdapterDeviceID)
{
  GetData();
  CopyUTF8toUTF16(mRenderer, aAdapterDeviceID);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID2(nsAString & aAdapterDeviceID)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetIsGPU2Active(bool* aIsGPU2Active)
{
  return NS_ERROR_FAILURE;
}

#ifdef DEBUG





NS_IMETHODIMP GfxInfo::SpoofVendorID(const nsAString & aVendorID)
{
  CopyUTF16toUTF8(aVendorID, mVendor);
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDeviceID(const nsAString & aDeviceID)
{
  CopyUTF16toUTF8(aDeviceID, mRenderer);
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDriverVersion(const nsAString & aDriverVersion)
{
  CopyUTF16toUTF8(aDriverVersion, mVersion);
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofOSVersion(PRUint32 aVersion)
{
  
  return NS_OK;
}

#endif

} 
} 
