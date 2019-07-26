





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
#include <errno.h>













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
IsNumeric(const char* aStr)
{
  MOZ_ASSERT(*aStr);  
  while (*aStr) {
    if (!isdigit(*aStr)) {
      return false;
    }
    ++aStr;
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

#define REPORT_WITH_CLEANUP(_path, _units, _amount, _desc, _cleanup)          \
  do {                                                                        \
    size_t amount = _amount;  /* evaluate _amount only once */                \
    if (amount > 0) {                                                         \
      nsresult rv;                                                            \
      rv = aHandleReport->Callback(NS_LITERAL_CSTRING("System"), _path,       \
                                   KIND_NONHEAP, _units, amount, _desc,       \
                                   aData);                                    \
      if (NS_WARN_IF(NS_FAILED(rv))) {                                        \
        _cleanup;                                                             \
        return rv;                                                            \
      }                                                                       \
    }                                                                         \
  } while (0)

#define REPORT(_path, _amount, _desc) \
  REPORT_WITH_CLEANUP(_path, UNITS_BYTES, _amount, _desc, (void)0)

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

    
    rv = CollectPmemReports(aHandleReport, aData);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = CollectZramReports(aHandleReport, aData);
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
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
    ProcessSizes()
    {
      memset(this, 0, sizeof(*this));
    }

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
          if (NS_FAILED(rv)) {
            break;
          }
        }
        fclose(f);

        
        nsPrintfCString procFdPath("/proc/%s/fd", pidStr);
        nsresult rv = CollectOpenFileReports(
                  aHandleReport, aData, procFdPath, processName);
        if (NS_FAILED(rv)) {
          break;
        }
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
      size_t pss = 0;
      nsresult rv = ParseMapBody(aFile, aProcessName, name, description,
                                 aHandleReport, aData, &pss);
      if (NS_FAILED(rv)) {
        break;
      }

      
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

  nsresult CollectPmemReports(nsIHandleReportCallback* aHandleReport,
                              nsISupports* aData)
  {
    
    
    
    
    
    
    
    
    
    
    
    DIR* d = opendir("/sys/kernel/pmem_regions");
    if (!d) {
      if (NS_WARN_IF(errno != ENOENT)) {
        return NS_ERROR_FAILURE;
      }
      
      return NS_OK;
    }

    struct dirent* ent;
    while ((ent = readdir(d))) {
      const char* name = ent->d_name;
      uint64_t size;
      int scanned;

      
      if (name[0] == '.') {
        continue;
      }

      
      
      nsPrintfCString sizePath("/sys/kernel/pmem_regions/%s/size", name);
      FILE* sizeFile = fopen(sizePath.get(), "r");
      if (NS_WARN_IF(!sizeFile)) {
        continue;
      }
      scanned = fscanf(sizeFile, "%" SCNu64, &size);
      if (NS_WARN_IF(scanned != 1)) {
        continue;
      }
      fclose(sizeFile);

      
      uint64_t freeSize = size;
      nsPrintfCString regionsPath("/sys/kernel/pmem_regions/%s/mapped_regions",
                                  name);
      FILE* regionsFile = fopen(regionsPath.get(), "r");
      if (regionsFile) {
        static const size_t bufLen = 4096;
        char buf[bufLen];
        while (fgets(buf, bufLen, regionsFile)) {
          int pid;

          
          if (strncmp(buf, "pid #", 5) == 0) {
            continue;
          }
          
          
          scanned = sscanf(buf, "pid %d", &pid);
          if (NS_WARN_IF(scanned != 1)) {
            continue;
          }
          for (const char* nextParen = strchr(buf, '(');
               nextParen != nullptr;
               nextParen = strchr(nextParen + 1, '(')) {
            uint64_t mapStart, mapLen;

            scanned = sscanf(nextParen + 1, "%" SCNx64 ",%" SCNx64,
                             &mapStart, &mapLen);
            if (NS_WARN_IF(scanned != 2)) {
              break;
            }

            nsPrintfCString path("mem/pmem/used/%s/segment(pid=%d, "
                                 "offset=0x%" PRIx64 ")", name, pid, mapStart);
            nsPrintfCString desc("Physical memory reserved for the \"%s\" pool "
                                 "and allocated to a buffer.", name);
            REPORT_WITH_CLEANUP(path, UNITS_BYTES, mapLen, desc,
                                (fclose(regionsFile), closedir(d)));
            freeSize -= mapLen;
          }
        }
        fclose(regionsFile);
      }

      nsPrintfCString path("mem/pmem/free/%s", name);
      nsPrintfCString desc("Physical memory reserved for the \"%s\" pool and "
                           "unavailable to the rest of the system, but not "
                           "currently allocated.", name);
      REPORT_WITH_CLEANUP(path, UNITS_BYTES, freeSize, desc, closedir(d));
    }
    closedir(d);
    return NS_OK;
  }

  uint64_t
  ReadSizeFromFile(const char* aFilename)
  {
    FILE* sizeFile = fopen(aFilename, "r");
    if (NS_WARN_IF(!sizeFile)) {
      return 0;
    }

    uint64_t size = 0;
    fscanf(sizeFile, "%" SCNu64, &size);
    fclose(sizeFile);

    return size;
  }

  nsresult
  CollectZramReports(nsIHandleReportCallback* aHandleReport,
                     nsISupports* aData)
  {
    
    
    
    
    
    
    
    
    
    

    DIR* d = opendir("/sys/block");
    if (!d) {
      if (NS_WARN_IF(errno != ENOENT)) {
        return NS_ERROR_FAILURE;
      }

      return NS_OK;
    }

    struct dirent* ent;
    while ((ent = readdir(d))) {
      const char* name = ent->d_name;

      
      if (strncmp("zram", name, 4) != 0) {
        continue;
      }

      
      nsPrintfCString diskSizeFile("/sys/block/%s/disksize", name);
      nsPrintfCString origSizeFile("/sys/block/%s/orig_data_size", name);

      uint64_t diskSize = ReadSizeFromFile(diskSizeFile.get());
      uint64_t origSize = ReadSizeFromFile(origSizeFile.get());
      uint64_t unusedSize = diskSize - origSize;

      nsPrintfCString diskUsedPath("zram-disksize/%s/used", name);
      nsPrintfCString diskUsedDesc(
        "The uncompressed size of data stored in \"%s.\" "
        "This excludes zero-filled pages since "
        "no memory is allocated for them.", name);
      REPORT_WITH_CLEANUP(diskUsedPath, UNITS_BYTES, origSize,
                          diskUsedDesc, closedir(d));

      nsPrintfCString diskUnusedPath("zram-disksize/%s/unused", name);
      nsPrintfCString diskUnusedDesc(
        "The amount of uncompressed data that can still be "
        "be stored in \"%s\"", name);
      REPORT_WITH_CLEANUP(diskUnusedPath, UNITS_BYTES, unusedSize,
                          diskUnusedDesc, closedir(d));

      
      nsPrintfCString readsFile("/sys/block/%s/num_reads", name);
      nsPrintfCString writesFile("/sys/block/%s/num_writes", name);

      uint64_t reads = ReadSizeFromFile(readsFile.get());
      uint64_t writes = ReadSizeFromFile(writesFile.get());

      nsPrintfCString readsDesc(
        "The number of reads (failed or successful) done on "
        "\"%s\"", name);
      nsPrintfCString readsPath("zram-accesses/%s/reads", name);
      REPORT_WITH_CLEANUP(readsPath, UNITS_COUNT_CUMULATIVE, reads,
                          readsDesc, closedir(d));

      nsPrintfCString writesDesc(
        "The number of writes (failed or successful) done "
        "on \"%s\"", name);
      nsPrintfCString writesPath("zram-accesses/%s/writes", name);
      REPORT_WITH_CLEANUP(writesPath, UNITS_COUNT_CUMULATIVE, writes,
                          writesDesc, closedir(d));

      
      nsPrintfCString comprSizeFile("/sys/block/%s/compr_data_size", name);
      uint64_t comprSize = ReadSizeFromFile(comprSizeFile.get());

      nsPrintfCString comprSizeDesc(
        "The compressed size of data stored in \"%s\"",
        name);
      nsPrintfCString comprSizePath("zram-compr-data-size/%s", name);
      REPORT_WITH_CLEANUP(comprSizePath, UNITS_BYTES, comprSize,
                          comprSizeDesc, closedir(d));
    }

    closedir(d);
    return NS_OK;
  }

  nsresult
  CollectOpenFileReports(nsIHandleReportCallback* aHandleReport,
                         nsISupports* aData,
                         const nsACString& aProcPath,
                         const nsACString& aProcessName)
  {
    
    
    
    
    const char kFilePrefix[] = "/";
    const char kSocketPrefix[] = "socket:";
    const char kPipePrefix[] = "pipe:";
    const char kAnonInodePrefix[] = "anon_inode:";

    const nsCString procPath(aProcPath);
    DIR* d = opendir(procPath.get());
    if (!d) {
      if (NS_WARN_IF(errno != ENOENT && errno != EACCES)) {
        return NS_ERROR_FAILURE;
      }
      return NS_OK;
    }

    char linkPath[PATH_MAX + 1];
    struct dirent* ent;
    while ((ent = readdir(d))) {
      const char* fd = ent->d_name;

      
      if (fd[0] == '.') {
        continue;
      }

      nsPrintfCString fullPath("%s/%s", procPath.get(), fd);
      ssize_t linkPathSize = readlink(fullPath.get(), linkPath, PATH_MAX);
      if (linkPathSize > 0) {
        linkPath[linkPathSize] = '\0';

#define CHECK_PREFIX(prefix) \
  (strncmp(linkPath, prefix, sizeof(prefix) - 1) == 0)

        const char* category = nullptr;
        const char* descriptionPrefix = nullptr;

        if (CHECK_PREFIX(kFilePrefix)) {
          category = "files"; 
          descriptionPrefix = "An open";
        } else if (CHECK_PREFIX(kSocketPrefix)) {
          category = "sockets/";
          descriptionPrefix = "A socket";
        } else if (CHECK_PREFIX(kPipePrefix)) {
          category = "pipes/";
          descriptionPrefix = "A pipe";
        } else if (CHECK_PREFIX(kAnonInodePrefix)) {
          category = "anon_inodes/";
          descriptionPrefix = "An anon_inode";
        } else {
          category = "";
          descriptionPrefix = "An uncategorized";
        }

#undef CHECK_PREFIX

        const nsCString processName(aProcessName);
        nsPrintfCString entryPath(
            "open-fds/%s/%s%s/%s", processName.get(), category, linkPath, fd);
        nsPrintfCString entryDescription(
            "%s file descriptor opened by the process", descriptionPrefix);
        REPORT_WITH_CLEANUP(
            entryPath, UNITS_COUNT, 1, entryDescription, closedir(d));
      }
    }

    closedir(d);
    return NS_OK;
  }

#undef REPORT
};

NS_IMPL_ISUPPORTS(SystemReporter, nsIMemoryReporter)


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

void
Init()
{
  RegisterStrongMemoryReporter(new SystemReporter());
}

} 
} 
