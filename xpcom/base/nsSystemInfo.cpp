





































#include "nsSystemInfo.h"
#include "prsystem.h"
#include "nsString.h"
#include "prprf.h"
#include "mozilla/SSE.h"
#include "mozilla/arm.h"

#ifdef MOZ_WIDGET_GTK2
#include <gtk/gtk.h>
#endif

#ifdef ANDROID
#include "AndroidBridge.h"

extern "C" {
extern int android_sdk_version;
}
#endif

nsSystemInfo::nsSystemInfo()
{
}

nsSystemInfo::~nsSystemInfo()
{
}

nsresult
nsSystemInfo::Init()
{
    nsresult rv = nsHashPropertyBag::Init();
    NS_ENSURE_SUCCESS(rv, rv);

    static const struct {
      PRSysInfo cmd;
      const char *name;
    } items[] = {
      { PR_SI_SYSNAME, "name" },
      { PR_SI_HOSTNAME, "host" },
      { PR_SI_ARCHITECTURE, "arch" },
      { PR_SI_RELEASE, "version" }
    };

    for (PRUint32 i = 0; i < (sizeof(items) / sizeof(items[0])); i++) {
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

    
    static const struct {
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
        { "hasNEON", mozilla::supports_neon }
    };

    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(cpuPropItems); i++) {
        rv = SetPropertyAsBool(NS_ConvertASCIItoUTF16(cpuPropItems[i].name),
                               cpuPropItems[i].propfun());
        NS_ENSURE_SUCCESS(rv, rv);
    }

#ifdef MOZ_WIDGET_GTK2
    
    char* gtkver = PR_smprintf("GTK %u.%u.%u", gtk_major_version, gtk_minor_version, gtk_micro_version);
    if (gtkver) {
      rv = SetPropertyAsACString(NS_LITERAL_STRING("secondaryLibrary"),
                                 nsDependentCString(gtkver));
      PR_smprintf_free(gtkver);
      NS_ENSURE_SUCCESS(rv, rv);
    }
#endif


#ifdef MOZ_PLATFORM_MAEMO
    char *  line = nsnull;
    size_t  len = 0;
    ssize_t read;
#if MOZ_PLATFORM_MAEMO > 5
    FILE *fp = popen("/usr/bin/sysinfoclient --get /component/product", "r");
#else
    FILE *fp = fopen("/proc/component_version", "r");
#endif
    if (fp) {
      while ((read = getline(&line, &len, fp)) != -1) {
        if (line) {
          if (strstr(line, "RX-51")) {
            SetPropertyAsACString(NS_LITERAL_STRING("device"), NS_LITERAL_CSTRING("Nokia N900"));
            SetPropertyAsACString(NS_LITERAL_STRING("manufacturer"), NS_LITERAL_CSTRING("Nokia"));
            SetPropertyAsACString(NS_LITERAL_STRING("hardware"), NS_LITERAL_CSTRING("RX-51"));
            break;
          } else if (strstr(line, "RX-44") ||
                     strstr(line, "RX-48") ||
                     strstr(line, "RX-32") ) {
            
            SetPropertyAsACString(NS_LITERAL_STRING("device"), NS_LITERAL_CSTRING("Nokia N8xx"));
            SetPropertyAsACString(NS_LITERAL_STRING("manufacturer"), NS_LITERAL_CSTRING("Nokia"));
            SetPropertyAsACString(NS_LITERAL_STRING("hardware"), NS_LITERAL_CSTRING("N8xx"));
            break;
          }
        }
      }
      if (line)
        free(line);
#if MOZ_PLATFORM_MAEMO > 5
      pclose(fp);
#else
      fclose(fp);
#endif
    }
#endif

#ifdef ANDROID
    if (mozilla::AndroidBridge::Bridge()) {
        nsAutoString str;
        if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MODEL", str))
            SetPropertyAsAString(NS_LITERAL_STRING("device"), str);
        if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MANUFACTURER", str))
            SetPropertyAsAString(NS_LITERAL_STRING("manufacturer"), str);
        PRInt32 version;
        if (!mozilla::AndroidBridge::Bridge()->GetStaticIntField("android/os/Build$VERSION", "SDK_INT", &version))
            version = 0;
        android_sdk_version = version;
        if (version >= 8 && mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "HARDWARE", str))
            SetPropertyAsAString(NS_LITERAL_STRING("hardware"), str);
        SetPropertyAsAString(NS_LITERAL_STRING("shellName"), NS_LITERAL_STRING("Android"));
        if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build$VERSION", "CODENAME", str)) {
            if (version) {
                str.Append(NS_LITERAL_STRING(" ("));
                str.AppendInt(version);
                str.Append(NS_LITERAL_STRING(")"));
            }   
            SetPropertyAsAString(NS_LITERAL_STRING("shellVersion"), str);
        }
                
        
    }
#endif
    return NS_OK;
}

void
nsSystemInfo::SetInt32Property(const nsAString &aPropertyName,
                               const PRInt32 aValue)
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
                                const PRUint64 aValue)
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
