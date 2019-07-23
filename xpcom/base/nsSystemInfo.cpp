





































#include "nsSystemInfo.h"
#include "prsystem.h"
#include "nsString.h"
#include "prprf.h"

#ifdef MOZ_WIDGET_GTK2
#include <gtk/gtk.h>
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

#ifdef MOZ_WIDGET_GTK2
    
    char* gtkver = PR_smprintf("GTK %u.%u.%u", gtk_major_version, gtk_minor_version, gtk_micro_version);
    if (gtkver) {
      rv = SetPropertyAsACString(NS_ConvertASCIItoUTF16("secondaryLibrary"),
                                 nsDependentCString(gtkver));
      PR_smprintf_free(gtkver);
      NS_ENSURE_SUCCESS(rv, rv);
    }
#endif


#ifdef MOZ_PLATFORM_HILDON
    char *  line = nsnull;
    size_t  len = 0;
    ssize_t read;
    FILE *fp = fopen ("/proc/component_version", "r");
    if (fp) {
      while ((read = getline(&line, &len, fp)) != -1) {
        if (line) {
          if (strstr(line, "RX-51")) {
            SetPropertyAsACString(NS_ConvertASCIItoUTF16("device"), NS_LITERAL_CSTRING("Nokia N900"));
            break;
          } else if (strstr(line, "RX-44") ||
                     strstr(line, "RX-48") ||
                     strstr(line, "RX-32") ) {
            SetPropertyAsACString(NS_ConvertASCIItoUTF16("device"), NS_LITERAL_CSTRING("Nokia N8xx"));
            break;
          }
        }
      }
      if (line)
        free(line);
      fclose(fp);
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
    nsresult rv = SetPropertyAsInt32(aPropertyName, aValue);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to set property");
  }
}

void
nsSystemInfo::SetUint64Property(const nsAString &aPropertyName,
                                const PRUint64 aValue)
{
  NS_WARN_IF_FALSE(aValue > 0, "Unable to read system value");
  if (aValue > 0) {
    nsresult rv = SetPropertyAsUint64(aPropertyName, aValue);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to set property");
  }
}
