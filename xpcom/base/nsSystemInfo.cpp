




#include "mozilla/Util.h"

#include "nsSystemInfo.h"
#include "prsystem.h"
#include "prio.h"
#include "prprf.h"
#include "mozilla/SSE.h"
#include "mozilla/arm.h"

#ifdef XP_WIN
#include <windows.h>
#include <winioctl.h>
#include "base/scoped_handle_win.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#endif

#ifdef MOZ_WIDGET_GTK
#include <gtk/gtk.h>
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
using namespace mozilla::widget::android;
#endif

#ifdef MOZ_WIDGET_GONK
#include <sys/system_properties.h>
#endif

#ifdef ANDROID
extern "C" {
NS_EXPORT int android_sdk_version;
}
#endif

#if defined(XP_WIN)
namespace {
nsresult GetProfileHDDInfo(nsAutoCString& aModel, nsAutoCString& aRevision)
{
    aModel.Truncate();
    aRevision.Truncate();

    nsCOMPtr<nsIFile> profDir;
    nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                         getter_AddRefs(profDir));
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoString profDirPath;
    rv = profDir->GetPath(profDirPath);
    NS_ENSURE_SUCCESS(rv, rv);
    wchar_t volumeMountPoint[MAX_PATH] = {L'\\', L'\\', L'.', L'\\'};
    const size_t PREFIX_LEN = 4;
    if (!::GetVolumePathNameW(profDirPath.get(), volumeMountPoint + PREFIX_LEN,
                              mozilla::ArrayLength(volumeMountPoint) -
                              PREFIX_LEN)) {
        return NS_ERROR_UNEXPECTED;
    }
    size_t volumeMountPointLen = wcslen(volumeMountPoint);
    
    
    
    if (volumeMountPoint[volumeMountPointLen - 1] == L'\\') {
      volumeMountPoint[volumeMountPointLen - 1] = L'\0';
    }
    ScopedHandle handle(::CreateFileW(volumeMountPoint, 0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      nullptr, OPEN_EXISTING, 0, nullptr));
    if (!handle.IsValid()) {
        return NS_ERROR_UNEXPECTED;
    }
    STORAGE_PROPERTY_QUERY queryParameters = {StorageDeviceProperty,
                                              PropertyStandardQuery};
    STORAGE_DEVICE_DESCRIPTOR outputHeader = {sizeof(STORAGE_DEVICE_DESCRIPTOR)};
    DWORD bytesRead = 0;
    if (!::DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
                           &queryParameters, sizeof(queryParameters),
                           &outputHeader, sizeof(outputHeader), &bytesRead,
                           nullptr)) {
        return NS_ERROR_FAILURE;
    }
    PSTORAGE_DEVICE_DESCRIPTOR deviceOutput =
                          (PSTORAGE_DEVICE_DESCRIPTOR)malloc(outputHeader.Size);
    if (!::DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
                           &queryParameters, sizeof(queryParameters),
                           deviceOutput, outputHeader.Size, &bytesRead,
                           nullptr)) {
        free(deviceOutput);
        return NS_ERROR_FAILURE;
    }
    
    
    
    if (deviceOutput->VendorIdOffset) {
        aModel = reinterpret_cast<char*>(deviceOutput) +
                     deviceOutput->VendorIdOffset;
    }
    if (deviceOutput->ProductIdOffset) {
        aModel += reinterpret_cast<char*>(deviceOutput) +
                      deviceOutput->ProductIdOffset;
    }
    aModel.CompressWhitespace();
    if (deviceOutput->ProductRevisionOffset) {
        aRevision = reinterpret_cast<char*>(deviceOutput) +
                        deviceOutput->ProductRevisionOffset;
        aRevision.CompressWhitespace();
    }
    free(deviceOutput);
    return NS_OK;
}
} 
#endif 

using namespace mozilla;

nsSystemInfo::nsSystemInfo()
{
}

nsSystemInfo::~nsSystemInfo()
{
}


static const struct PropItems {
    const char *name;
    bool (*propfun)(void);
} cpuPropItems[] = {
    
    { "hasMMX", mozilla::supports_mmx },
    { "hasSSE", mozilla::supports_sse },
    { "hasSSE2", mozilla::supports_sse2 },
    { "hasSSE3", mozilla::supports_sse3 },
    { "hasSSSE3", mozilla::supports_ssse3 },
    { "hasSSE4A", mozilla::supports_sse4a },
    { "hasSSE4_1", mozilla::supports_sse4_1 },
    { "hasSSE4_2", mozilla::supports_sse4_2 },
    
    { "hasEDSP", mozilla::supports_edsp },
    { "hasARMv6", mozilla::supports_armv6 },
    { "hasARMv7", mozilla::supports_armv7 },
    { "hasNEON", mozilla::supports_neon }
};

nsresult
nsSystemInfo::Init()
{
    nsresult rv;

    static const struct {
      PRSysInfo cmd;
      const char *name;
    } items[] = {
      { PR_SI_SYSNAME, "name" },
      { PR_SI_HOSTNAME, "host" },
      { PR_SI_ARCHITECTURE, "arch" },
      { PR_SI_RELEASE, "version" }
    };

    for (uint32_t i = 0; i < (sizeof(items) / sizeof(items[0])); i++) {
      char buf[SYS_INFO_BUFFER_LENGTH];
      if (PR_GetSystemInfo(items[i].cmd, buf, sizeof(buf)) == PR_SUCCESS) {
        rv = SetPropertyAsACString(NS_ConvertASCIItoUTF16(items[i].name),
                                   nsDependentCString(buf));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        NS_WARNING("PR_GetSystemInfo failed");
      }
    }

    
    SetInt32Property(NS_LITERAL_STRING("pagesize"), PR_GetPageSize());
    SetInt32Property(NS_LITERAL_STRING("pageshift"), PR_GetPageShift());
    SetInt32Property(NS_LITERAL_STRING("memmapalign"), PR_GetMemMapAlignment());
    SetInt32Property(NS_LITERAL_STRING("cpucount"), PR_GetNumberOfProcessors());
    SetUint64Property(NS_LITERAL_STRING("memsize"), PR_GetPhysicalMemorySize());

    for (uint32_t i = 0; i < ArrayLength(cpuPropItems); i++) {
        rv = SetPropertyAsBool(NS_ConvertASCIItoUTF16(cpuPropItems[i].name),
                               cpuPropItems[i].propfun());
        NS_ENSURE_SUCCESS(rv, rv);
    }

#ifdef XP_WIN
    BOOL isWow64;
    BOOL gotWow64Value = IsWow64Process(GetCurrentProcess(), &isWow64);
    NS_WARN_IF_FALSE(gotWow64Value, "IsWow64Process failed");
    if (gotWow64Value) {
      rv = SetPropertyAsBool(NS_LITERAL_STRING("isWow64"), !!isWow64);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    nsAutoCString hddModel, hddRevision;
    if (NS_SUCCEEDED(GetProfileHDDInfo(hddModel, hddRevision))) {
      rv = SetPropertyAsACString(NS_LITERAL_STRING("profileHDDModel"), hddModel);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = SetPropertyAsACString(NS_LITERAL_STRING("profileHDDRevision"),
                                 hddRevision);
      NS_ENSURE_SUCCESS(rv, rv);
    }
#endif

#if defined(MOZ_WIDGET_GTK)
    
    char* gtkver = PR_smprintf("GTK %u.%u.%u", gtk_major_version, gtk_minor_version, gtk_micro_version);
    if (gtkver) {
      rv = SetPropertyAsACString(NS_LITERAL_STRING("secondaryLibrary"),
                                 nsDependentCString(gtkver));
      PR_smprintf_free(gtkver);
      NS_ENSURE_SUCCESS(rv, rv);
    }
#endif

#ifdef MOZ_WIDGET_ANDROID
    if (mozilla::AndroidBridge::Bridge()) {
        nsAutoString str;
        if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MODEL", str))
            SetPropertyAsAString(NS_LITERAL_STRING("device"), str);
        if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MANUFACTURER", str))
            SetPropertyAsAString(NS_LITERAL_STRING("manufacturer"), str);
        if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build$VERSION", "RELEASE", str))
            SetPropertyAsAString(NS_LITERAL_STRING("release_version"), str);
        int32_t version;
        if (!mozilla::AndroidBridge::Bridge()->GetStaticIntField("android/os/Build$VERSION", "SDK_INT", &version))
            version = 0;
        android_sdk_version = version;
        if (version >= 8 && mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "HARDWARE", str))
            SetPropertyAsAString(NS_LITERAL_STRING("hardware"), str);
        bool isTablet = GeckoAppShell::IsTablet();
        SetPropertyAsBool(NS_LITERAL_STRING("tablet"), isTablet);
        
        
        rv = GetPropertyAsAString(NS_LITERAL_STRING("version"), str);
        if (NS_SUCCEEDED(rv)) {
            SetPropertyAsAString(NS_LITERAL_STRING("kernel_version"), str);
        }
        SetPropertyAsInt32(NS_LITERAL_STRING("version"), android_sdk_version);
    }
#endif

#ifdef MOZ_WIDGET_GONK
    char sdk[PROP_VALUE_MAX], characteristics[PROP_VALUE_MAX];
    if (__system_property_get("ro.build.version.sdk", sdk))
      android_sdk_version = atoi(sdk);
    if (__system_property_get("ro.build.characteristics", characteristics)) {
      if (!strcmp(characteristics, "tablet"))
        SetPropertyAsBool(NS_LITERAL_STRING("tablet"), true);
    }
#endif

    return NS_OK;
}

void
nsSystemInfo::SetInt32Property(const nsAString &aPropertyName,
                               const int32_t aValue)
{
  NS_WARN_IF_FALSE(aValue > 0, "Unable to read system value");
  if (aValue > 0) {
#ifdef DEBUG
    nsresult rv =
#endif
      SetPropertyAsInt32(aPropertyName, aValue);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to set property");
  }
}

void
nsSystemInfo::SetUint64Property(const nsAString &aPropertyName,
                                const uint64_t aValue)
{
  NS_WARN_IF_FALSE(aValue > 0, "Unable to read system value");
  if (aValue > 0) {
#ifdef DEBUG
    nsresult rv =
#endif
      SetPropertyAsUint64(aPropertyName, aValue);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to set property");
  }
}
