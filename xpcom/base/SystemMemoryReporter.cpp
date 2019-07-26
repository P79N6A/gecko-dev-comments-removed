





#include "mozilla/SystemMemoryReporter.h"

#include "mozilla/Attributes.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"

#include "nsIMemoryReporter.h"
#include "nsPrintfCString.h"
#include "nsString.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

#include <dirent.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>













namespace mozilla {
namespace SystemMemoryReporter {

#if !defined(XP_LINUX)
#error "This won't work if we're not on Linux."
#endif

static bool
EndsWithLiteral(const nsCString& aHaystack, const char* aNeedle)
{
  int32_t idx = aHaystack.RFind(aNeedle);
  return idx != -1 && idx + strlen(aNeedle) == aHaystack.Length();
}

static void
GetDirname(const nsCString& aPath, nsACString& aOut)
{
  int32_t idx = aPath.RFind("/");
  if (idx == -1) {
    aOut.Truncate();
  } else {
    aOut.Assign(Substring(aPath, 0, idx));
  }
}

static void
GetBasename(const nsCString& aPath, nsACString& aOut)
{
  nsCString out;
  int32_t idx = aPath.RFind("/");
  if (idx == -1) {
    out.Assign(aPath);
  } else {
    out.Assign(Substring(aPath, idx + 1));
  }

  
  
  
  if (EndsWithLiteral(out, "(deleted)")) {
    out.Assign(Substring(out, 0, out.RFind("(deleted)")));
  }
  out.StripChars(" ");

  aOut.Assign(out);
}

static bool
IsNumeric(const char* s)
{
  MOZ_ASSERT(*s);   
  while (*s) {
    if (!isdigit(*s)) {
      return false;
    }
    s++;
  }
  return true;
}

static bool
IsAnonymous(const nsACString& aName)
{
  
  
  
  
  
  
  return aName.IsEmpty() ||
         StringBeginsWith(aName, NS_LITERAL_CSTRING("[stack:"));
}

class SystemReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

#define REPORT(_path, _amount, _desc)                                         \
  do {                                                                        \
    size_t amount = _amount;  /* evaluate _amount only once */                \
    if (amount > 0) {                                                         \
      nsresult rv;                                                            \
      rv = aHandleReport->Callback(NS_LITERAL_CSTRING("System"), _path,       \
                                   KIND_NONHEAP, UNITS_BYTES, amount, _desc,  \
                                   aData);                                    \
      if (NS_WARN_IF(NS_FAILED(rv))) {                                        \
        return rv;                                                            \
      }                                                                       \
    }                                                                         \
  } while (0)

  NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                            nsISupports* aData)
  {
    if (!Preferences::GetBool("memory.system_memory_reporter")) {
      return NS_OK;
    }

    
    int64_t memTotal = 0, memFree = 0;
    nsresult rv = ReadMemInfo(&memTotal, &memFree);

    
    int64_t totalPss = 0;
    rv = CollectProcessReports(aHandleReport, aData, &totalPss);
    NS_ENSURE_SUCCESS(rv, rv);

    
    int64_t other = memTotal - memFree - totalPss;
    REPORT(NS_LITERAL_CSTRING("mem/other"), other, NS_LITERAL_CSTRING(
"Memory which is neither owned by any user-space process nor free. Note that "
"this includes memory holding cached files from the disk which can be "
"reclaimed by the OS at any time."));

    REPORT(NS_LITERAL_CSTRING("mem/free"), memFree, NS_LITERAL_CSTRING(
"Memory which is free and not being used for any purpose."));

    return NS_OK;
  }

private:
  
  enum ProcessSizeKind {
    AnonymousOutsideBrk  = 0,
    AnonymousBrkHeap     = 1,
    SharedLibrariesRX    = 2,
    SharedLibrariesRW    = 3,
    SharedLibrariesR     = 4,
    SharedLibrariesOther = 5,
    OtherFiles           = 6,
    MainThreadStack      = 7,
    Vdso                 = 8,

    ProcessSizeKindLimit = 9  
  };

  static const char* kindPathSuffixes[ProcessSizeKindLimit];

  
  struct ProcessSizes
  {
    ProcessSizes() { memset(this, 0, sizeof(*this)); }

    size_t mSizes[ProcessSizeKindLimit];
  };

  nsresult ReadMemInfo(int64_t* aMemTotal, int64_t* aMemFree)
  {
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) {
      return NS_ERROR_FAILURE;
    }

    int n1 = fscanf(f, "MemTotal: %" SCNd64 " kB\n", aMemTotal);
    int n2 = fscanf(f, "MemFree: %"  SCNd64 " kB\n", aMemFree);

    fclose(f);

    if (n1 != 1 || n2 != 1) {
      return NS_ERROR_FAILURE;
    }

    
    *aMemTotal *= 1024;
    *aMemFree  *= 1024;

    return NS_OK;
  }

  nsresult CollectProcessReports(nsIHandleReportCallback* aHandleReport,
                                 nsISupports* aData,
                                 int64_t* aTotalPss)
  {
    *aTotalPss = 0;
    ProcessSizes processSizes;

    DIR* d = opendir("/proc");
    if (NS_WARN_IF(!d)) {
      return NS_ERROR_FAILURE;
    }
    struct dirent* ent;
    while ((ent = readdir(d))) {
      struct stat statbuf;
      const char* pidStr = ent->d_name;
      
      
      stat(pidStr, &statbuf);
      if (S_ISDIR(statbuf.st_mode) && IsNumeric(pidStr)) {
        nsCString processName("process(");

        
        
        nsPrintfCString cmdlinePath("/proc/%s/cmdline", pidStr);
        FILE* f = fopen(cmdlinePath.get(), "r");
        if (f) {
          static const size_t len = 256;
          char buf[len];
          if (fgets(buf, len, f)) {
            processName.Append(buf);
            
            
            
            processName.ReplaceChar('/', '\\');
            processName.Append(", ");
          }
          fclose(f);
        }
        processName.Append("pid=");
        processName.Append(pidStr);
        processName.Append(")");

        
        nsPrintfCString smapsPath("/proc/%s/smaps", pidStr);
        f = fopen(smapsPath.get(), "r");
        if (!f) {
          
          
          continue;
        }
        while (true) {
          nsresult rv = ParseMapping(f, processName, aHandleReport, aData,
                                     &processSizes, aTotalPss);
          if (NS_FAILED(rv))
            break;
        }
        fclose(f);
      }
    }
    closedir(d);

    

    for (size_t i = 0; i < ProcessSizeKindLimit; i++) {
      nsAutoCString path("processes/");
      path.Append(kindPathSuffixes[i]);

      nsAutoCString desc("This is the sum of all processes' '");
      desc.Append(kindPathSuffixes[i]);
      desc.Append("' numbers.");

      REPORT(path, processSizes.mSizes[i], desc);
    }

    return NS_OK;
  }

  nsresult ParseMapping(FILE* aFile,
                        const nsACString& aProcessName,
                        nsIHandleReportCallback* aHandleReport,
                        nsISupports* aData,
                        ProcessSizes* aProcessSizes,
                        int64_t* aTotalPss)
  {
    
    
    
    
    

    const int argCount = 8;

    unsigned long long addrStart, addrEnd;
    char perms[5];
    unsigned long long offset;
    
    
    
    char devMajor[17];
    char devMinor[17];
    unsigned int inode;
    char path[1025];

    
    path[0] = '\0';

    
    
    
    
    
    
    int n = fscanf(aFile,
                   "%llx-%llx %4s %llx "
                   "%16[0-9a-fA-F]:%16[0-9a-fA-F] %u%1024[^\n]",
                   &addrStart, &addrEnd, perms, &offset, devMajor,
                   devMinor, &inode, path);

    
    unused << fscanf(aFile, " ");

    
    
    if (n != argCount && n != argCount - 1) {
      return NS_ERROR_FAILURE;
    }

    nsAutoCString name, description;
    ProcessSizeKind kind;
    GetReporterNameAndDescription(path, perms, name, description, &kind);

    while (true) {
      size_t pss;
      nsresult rv = ParseMapBody(aFile, aProcessName, name, description,
                                 aHandleReport, aData, &pss);
      if (NS_FAILED(rv))
        break;

      
      aProcessSizes->mSizes[kind] += pss;
      *aTotalPss += pss;
    }

    return NS_OK;
  }

  void GetReporterNameAndDescription(const char* aPath,
                                     const char* aPerms,
                                     nsACString& aName,
                                     nsACString& aDesc,
                                     ProcessSizeKind* aProcessSizeKind)
  {
    aName.Truncate();
    aDesc.Truncate();

    
    
    
    nsAutoCString absPath;
    absPath.Append(aPath);
    absPath.StripChars(" ");

    nsAutoCString basename;
    GetBasename(absPath, basename);

    if (basename.EqualsLiteral("[heap]")) {
      aName.Append("anonymous/brk-heap");
      aDesc.Append("Memory in anonymous mappings within the boundaries "
                   "defined by brk() / sbrk().  This is likely to be just "
                   "a portion of the application's heap; the remainder "
                   "lives in other anonymous mappings. This corresponds to "
                   "'[heap]' in /proc/<pid>/smaps.");
      *aProcessSizeKind = AnonymousBrkHeap;

    } else if (basename.EqualsLiteral("[stack]")) {
      aName.Append("main-thread-stack");
      aDesc.Append("The stack size of the process's main thread.  This "
                   "corresponds to '[stack]' in /proc/<pid>/smaps.");
      *aProcessSizeKind = MainThreadStack;

    } else if (basename.EqualsLiteral("[vdso]")) {
      aName.Append("vdso");
      aDesc.Append("The virtual dynamically-linked shared object, also known "
                   "as the 'vsyscall page'. This is a memory region mapped by "
                   "the operating system for the purpose of allowing processes "
                   "to perform some privileged actions without the overhead of "
                   "a syscall.");
      *aProcessSizeKind = Vdso;

    } else if (!IsAnonymous(basename)) {
      nsAutoCString dirname;
      GetDirname(absPath, dirname);

      
      
      if (EndsWithLiteral(basename, ".so") ||
          (basename.Find(".so") != -1 && dirname.Find("/lib") != -1)) {
        aName.Append("shared-libraries/");

        if (strncmp(aPerms, "r-x", 3) == 0) {
          *aProcessSizeKind = SharedLibrariesRX;
        } else if (strncmp(aPerms, "rw-", 3) == 0) {
          *aProcessSizeKind = SharedLibrariesRW;
        } else if (strncmp(aPerms, "r--", 3) == 0) {
          *aProcessSizeKind = SharedLibrariesR;
        } else {
          *aProcessSizeKind = SharedLibrariesOther;
        }

      } else {
        aName.Append("other-files/");
        if (EndsWithLiteral(basename, ".xpi")) {
          aName.Append("extensions/");
        } else if (dirname.Find("/fontconfig") != -1) {
          aName.Append("fontconfig/");
        }
        *aProcessSizeKind = OtherFiles;
      }

      aName.Append(basename);
      aDesc.Append(absPath);

    } else {
      aName.Append("anonymous/outside-brk");
      aDesc.Append("Memory in anonymous mappings outside the boundaries "
                   "defined by brk() / sbrk().");
      *aProcessSizeKind = AnonymousOutsideBrk;
    }

    aName.Append("/[");
    aName.Append(aPerms);
    aName.Append("]");

    
    
    
    aDesc.Append(" [");
    aDesc.Append(aPerms);
    aDesc.Append("]");
  }

  nsresult ParseMapBody(
    FILE* aFile,
    const nsACString& aProcessName,
    const nsACString& aName,
    const nsACString& aDescription,
    nsIHandleReportCallback* aHandleReport,
    nsISupports* aData,
    size_t* aPss)
  {
    
    
    
    
    
    
    
    
    
    
    
    

    char desc[1025];
    int64_t sizeKB;
    int n = fscanf(aFile, "%1024[a-zA-Z_]: %" SCNd64 " kB\n", desc, &sizeKB);
    if (n == EOF || n == 0) {
      return NS_ERROR_FAILURE;
    } else if (n == 1 && strcmp(desc, "VmFlags") == 0) {
      
      fscanf(aFile, "%*1024[a-z ]\n");
      return NS_ERROR_FAILURE;
    }

    
    if (strcmp(desc, "Pss") == 0) {
      *aPss = sizeKB * 1024;

      
      if (*aPss == 0) {
        return NS_OK;
      }

      nsAutoCString path("mem/processes/");
      path.Append(aProcessName);
      path.Append("/");
      path.Append(aName);

      REPORT(path, *aPss, aDescription);
    } else {
      *aPss = 0;
    }

    return NS_OK;
  }

#undef REPORT
};

NS_IMPL_ISUPPORTS1(SystemReporter, nsIMemoryReporter)


const char* SystemReporter::kindPathSuffixes[] = {
    "anonymous/outside-brk",
    "anonymous/brk-heap",
    "shared-libraries/read-executable",
    "shared-libraries/read-write",
    "shared-libraries/read-only",
    "shared-libraries/other",
    "other-files",
    "main-thread-stack",
    "vdso"
};

void Init()
{
  RegisterStrongMemoryReporter(new SystemReporter());
}

} 
} 
