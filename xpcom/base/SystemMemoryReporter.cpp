





#include "mozilla/SystemMemoryReporter.h"

#include "mozilla/Attributes.h"
#include "mozilla/LinuxUtils.h"
#include "mozilla/PodOperations.h"
#include "mozilla/Preferences.h"
#include "mozilla/TaggedAnonymousMemory.h"
#include "mozilla/unused.h"

#include "nsDataHashtable.h"
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




struct MOZ_STACK_CLASS AutoDir
{
  explicit AutoDir(DIR* aDir) : mDir(aDir) {}
  ~AutoDir() { if (mDir) closedir(mDir); };
  DIR* mDir;
};




struct MOZ_STACK_CLASS AutoFile
{
  explicit AutoFile(FILE* aFile) : mFile(aFile) {}
  ~AutoFile() { if (mFile) fclose(mFile); }
  FILE* mFile;
};

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
  ~SystemReporter() {}

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
                            nsISupports* aData, bool aAnonymize)
  {
    
    
    if (aAnonymize) {
      return NS_OK;
    }

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

    
    rv = CollectKgslReports(aHandleReport, aData);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = CollectIonReports(aHandleReport, aData);
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
  }

private:
  
  class ProcessSizes
  {
  public:
    void Add(const nsACString& aKey, size_t aSize)
    {
      mTagged.Put(aKey, mTagged.Get(aKey) + aSize);
    }

    void Report(nsIHandleReportCallback* aHandleReport, nsISupports* aData)
    {
      EnumArgs env = { aHandleReport, aData };
      mTagged.EnumerateRead(ReportSizes, &env);
    }

  private:
    nsDataHashtable<nsCStringHashKey, size_t> mTagged;

    struct EnumArgs
    {
      nsIHandleReportCallback* mHandleReport;
      nsISupports* mData;
    };

    static PLDHashOperator ReportSizes(nsCStringHashKey::KeyType aKey,
                                       size_t aAmount,
                                       void* aUserArg)
    {
      const EnumArgs* envp = reinterpret_cast<const EnumArgs*>(aUserArg);

      nsAutoCString path("processes/");
      path.Append(aKey);

      nsAutoCString desc("This is the sum of all processes' '");
      desc.Append(aKey);
      desc.AppendLiteral("' numbers.");

      envp->mHandleReport->Callback(NS_LITERAL_CSTRING("System"), path,
                                    KIND_NONHEAP, UNITS_BYTES, aAmount,
                                    desc, envp->mData);

      return PL_DHASH_NEXT;
    }
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
            processName.AppendLiteral(", ");
          }
          fclose(f);
        }
        processName.AppendLiteral("pid=");
        processName.Append(pidStr);
        processName.Append(')');

        
        nsPrintfCString smapsPath("/proc/%s/smaps", pidStr);
        f = fopen(smapsPath.get(), "r");
        if (!f) {
          
          
          continue;
        }
        nsresult rv = ParseMappings(f, processName, aHandleReport, aData,
                                    &processSizes, aTotalPss);
        fclose(f);
        if (NS_FAILED(rv)) {
          continue;
        }

        
        nsPrintfCString procFdPath("/proc/%s/fd", pidStr);
        rv = CollectOpenFileReports(aHandleReport, aData, procFdPath,
                                    processName);
        if (NS_FAILED(rv)) {
          break;
        }
      }
    }
    closedir(d);

    
    processSizes.Report(aHandleReport, aData);

    return NS_OK;
  }

  nsresult ParseMappings(FILE* aFile,
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
    char line[1025];
    
    
    nsAutoCString path;
    int pathOffset;

    path.SetIsVoid(true);
    while (fgets(line, sizeof(line), aFile)) {
      if (path.IsVoid()) {
        int n = sscanf(line,
                       "%llx-%llx %4s %llx "
                       "%16[0-9a-fA-F]:%16[0-9a-fA-F] %u %n",
                       &addrStart, &addrEnd, perms, &offset, devMajor,
                       devMinor, &inode, &pathOffset);

        if (n >= argCount - 1) {
          path.Assign(line + pathOffset);
          path.StripChars("\n");
        }
        continue;
      }

      
      size_t pss_kb;
      int n = sscanf(line, "Pss: %zu", &pss_kb);
      if (n < 1) {
        continue;
      }

      size_t pss = pss_kb * 1024;
      if (pss > 0) {
        nsAutoCString name, description, tag;
        GetReporterNameAndDescription(path.get(), perms, name, description, tag);

        nsAutoCString path("mem/processes/");
        path.Append(aProcessName);
        path.Append('/');
        path.Append(name);

        REPORT(path, pss, description);

        
        aProcessSizes->Add(tag, pss);
        *aTotalPss += pss;
      }

      
      path.SetIsVoid(true);
    }
    return NS_OK;
  }

  void GetReporterNameAndDescription(const char* aPath,
                                     const char* aPerms,
                                     nsACString& aName,
                                     nsACString& aDesc,
                                     nsACString& aTag)
  {
    aName.Truncate();
    aDesc.Truncate();
    aTag.Truncate();

    
    
    
    nsAutoCString absPath;
    absPath.Append(aPath);
    absPath.StripChars(" ");

    if (absPath.EqualsLiteral("[heap]")) {
      aName.AppendLiteral("anonymous/brk-heap");
      aDesc.AppendLiteral(
        "Memory in anonymous mappings within the boundaries defined by "
        "brk() / sbrk().  This is likely to be just a portion of the "
        "application's heap; the remainder lives in other anonymous mappings. "
        "This corresponds to '[heap]' in /proc/<pid>/smaps.");
      aTag = aName;
    } else if (absPath.EqualsLiteral("[stack]")) {
      aName.AppendLiteral("stack/main-thread");
      aDesc.AppendPrintf(
        "The stack size of the process's main thread.  This corresponds to "
        "'[stack]' in /proc/<pid>/smaps.");
      aTag = aName;
    } else if (MozTaggedMemoryIsSupported() &&
               StringBeginsWith(absPath, NS_LITERAL_CSTRING("[stack:"))) {
      
      
      
      
      pid_t tid = atoi(absPath.get() + 7);
      nsAutoCString threadName, escapedThreadName;
      LinuxUtils::GetThreadName(tid, threadName);
      if (threadName.IsEmpty()) {
        threadName.AssignLiteral("<unknown>");
      }
      escapedThreadName.Assign(threadName);
      escapedThreadName.StripChars("()");
      escapedThreadName.ReplaceChar('/', '\\');

      aName.AppendLiteral("stack/non-main-thread");
      aName.AppendLiteral("/name(");
      aName.Append(escapedThreadName);
      aName.Append(')');
      aTag = aName;
      aName.AppendPrintf("/thread(%d)", tid);

      aDesc.AppendPrintf("The stack size of a non-main thread named '%s' with "
                         "thread ID %d.  This corresponds to '[stack:%d]' "
                         "in /proc/%d/smaps.", threadName.get(), tid, tid);
    } else if (absPath.EqualsLiteral("[vdso]")) {
      aName.AppendLiteral("vdso");
      aDesc.AppendLiteral(
        "The virtual dynamically-linked shared object, also known as the "
        "'vsyscall page'. This is a memory region mapped by the operating "
        "system for the purpose of allowing processes to perform some "
        "privileged actions without the overhead of a syscall.");
      aTag = aName;
    } else if (StringBeginsWith(absPath, NS_LITERAL_CSTRING("[anon:")) &&
               EndsWithLiteral(absPath, "]")) {
      
      nsAutoCString tag(Substring(absPath, 6, absPath.Length() - 7));

      aName.AppendLiteral("anonymous/");
      aName.Append(tag);
      aTag = aName;
      aDesc.AppendLiteral("Memory in anonymous mappings tagged with '");
      aDesc.Append(tag);
      aDesc.Append('\'');
    } else if (!IsAnonymous(absPath)) {
      
      
      nsAutoCString basename, dirname;
      GetBasename(absPath, basename);
      GetDirname(absPath, dirname);

      
      
      if (EndsWithLiteral(basename, ".so") ||
          (basename.Find(".so") != -1 && dirname.Find("/lib") != -1)) {
        aName.AppendLiteral("shared-libraries/");
        aTag = aName;

        if (strncmp(aPerms, "r-x", 3) == 0) {
          aTag.AppendLiteral("read-executable");
        } else if (strncmp(aPerms, "rw-", 3) == 0) {
          aTag.AppendLiteral("read-write");
        } else if (strncmp(aPerms, "r--", 3) == 0) {
          aTag.AppendLiteral("read-only");
        } else {
          aTag.AppendLiteral("other");
        }

      } else {
        aName.AppendLiteral("other-files");
        if (EndsWithLiteral(basename, ".xpi")) {
          aName.AppendLiteral("/extensions");
        } else if (dirname.Find("/fontconfig") != -1) {
          aName.AppendLiteral("/fontconfig");
        }
        aTag = aName;
        aName.Append('/');
      }

      aName.Append(basename);
      aDesc.Append(absPath);
    } else {
      if (MozTaggedMemoryIsSupported()) {
        aName.AppendLiteral("anonymous/untagged");
        aDesc.AppendLiteral("Memory in untagged anonymous mappings.");
        aTag = aName;
      } else {
        aName.AppendLiteral("anonymous/outside-brk");
        aDesc.AppendLiteral("Memory in anonymous mappings outside the "
                            "boundaries defined by brk() / sbrk().");
        aTag = aName;
      }
    }

    aName.AppendLiteral("/[");
    aName.Append(aPerms);
    aName.Append(']');

    
    
    
    aDesc.AppendLiteral(" [");
    aDesc.Append(aPerms);
    aDesc.Append(']');
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
      fclose(sizeFile);
      if (NS_WARN_IF(scanned != 1)) {
        continue;
      }

      
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

  nsresult
  CollectIonReports(nsIHandleReportCallback* aHandleReport,
                    nsISupports* aData)
  {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    const char* const kFormatString = "%63s %" SCNu32 " %" SCNu64;
    const size_t kNumFields = 3;
    const size_t kStringSize = 64;
    const char* const kIonIommuPath = "/sys/kernel/debug/ion/iommu";

    FILE* iommu = fopen(kIonIommuPath, "r");
    if (!iommu) {
      if (NS_WARN_IF(errno != ENOENT)) {
        return NS_ERROR_FAILURE;
      }
      
      return NS_OK;
    }

    AutoFile iommuGuard(iommu);

    const size_t kBufferLen = 256;
    char buffer[kBufferLen];
    char client[kStringSize];
    uint32_t pid;
    uint64_t size;

    
    fgets(buffer, kBufferLen, iommu);

    
    fgets(buffer, kBufferLen, iommu);

    const char* const kSep = "----";
    const size_t kSepLen = 4;

    
    while (fgets(buffer, kBufferLen, iommu) &&
           strncmp(kSep, buffer, kSepLen) != 0) {
      if (sscanf(buffer, kFormatString, client, &pid, &size) == kNumFields) {
        nsPrintfCString entryPath("ion-memory/%s (pid=%d)", client, pid);
        REPORT(entryPath,
               size,
               NS_LITERAL_CSTRING("An ION kernel memory allocation."));
      }
    }

    
    fgets(buffer, kBufferLen, iommu);

    
    while (fgets(buffer, kBufferLen, iommu) &&
           strncmp(kSep, buffer, kSepLen) != 0) {
      if (sscanf(buffer, kFormatString, client, &pid, &size) == kNumFields) {
        nsPrintfCString entryPath("ion-memory/%s (pid=%d)", client, pid);
        REPORT(entryPath,
               size,
               NS_LITERAL_CSTRING("An ION kernel memory allocation."));
      }
    }

    

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
        nsPrintfCString entryPath("open-fds/%s/%s%s/%s",
                                  processName.get(), category, linkPath, fd);
        nsPrintfCString entryDescription("%s file descriptor opened by the process",
                                         descriptionPrefix);
        REPORT_WITH_CLEANUP(entryPath, UNITS_COUNT, 1, entryDescription,
                            closedir(d));
      }
    }

    closedir(d);
    return NS_OK;
  }

  nsresult
  CollectKgslReports(nsIHandleReportCallback* aHandleReport,
                     nsISupports* aData)
  {
    
    
    
    
    
    

    
    const char* const kScanFormat =
      "%" SCNx64 " %" SCNx64 " %" SCNu64 " %" SCNu64
      " %63s %63s %63s %" SCNu64;
    const int kNumFields = 8;
    const size_t kStringSize = 64;

    DIR* d = opendir("/sys/kernel/debug/kgsl/proc/");
    if (!d) {
      if (NS_WARN_IF(errno != ENOENT && errno != EACCES)) {
        return NS_ERROR_FAILURE;
      }
      return NS_OK;
    }

    AutoDir dirGuard(d);

    struct dirent* ent;
    while ((ent = readdir(d))) {
      const char* pid = ent->d_name;

      
      if (pid[0] == '.') {
        continue;
      }

      nsPrintfCString memPath("/sys/kernel/debug/kgsl/proc/%s/mem", pid);
      FILE* memFile = fopen(memPath.get(), "r");
      if (NS_WARN_IF(!memFile)) {
        continue;
      }

      AutoFile fileGuard(memFile);

      
      nsAutoCString procName;
      LinuxUtils::GetThreadName(atoi(pid), procName);

      if (procName.IsEmpty()) {
        procName.Append("pid=");
        procName.Append(pid);
      } else {
        procName.Append(" (pid=");
        procName.Append(pid);
        procName.Append(")");
      }

      uint64_t gpuaddr, useraddr, size, id, sglen;
      char flags[kStringSize];
      char type[kStringSize];
      char usage[kStringSize];

      
      char buff[1024];
      fgets(buff, 1024, memFile);

      while (fscanf(memFile, kScanFormat, &gpuaddr, &useraddr, &size, &id,
                    flags, type, usage, &sglen) == kNumFields) {
        nsPrintfCString entryPath("kgsl-memory/%s/%s", procName.get(), usage);
        REPORT(entryPath,
               size,
               NS_LITERAL_CSTRING("A kgsl graphics memory allocation."));
      }
    }

    return NS_OK;
  }

#undef REPORT
};

NS_IMPL_ISUPPORTS(SystemReporter, nsIMemoryReporter)

void
Init()
{
  RegisterStrongMemoryReporter(new SystemReporter());
}

} 
} 
