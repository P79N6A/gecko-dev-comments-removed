






































#include "mozilla/Util.h"

#include "mozilla/MapsMemoryReporter.h"
#include "nsIMemoryReporter.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsHashSets.h"
#include <stdio.h>

namespace mozilla {
namespace MapsMemoryReporter {

#if !defined(XP_LINUX)
#error "This doesn't have a prayer of working if we're not on Linux."
#endif






const char* mozillaLibraries[] =
{
  "libfreebl3.so",
  "libmozalloc.so",
  "libmozsqlite3.so",
  "libnspr4.so",
  "libnss3.so",
  "libnssckbi.so",
  "libnssdbm3.so",
  "libnssutil3.so",
  "libplc4.so",
  "libplds4.so",
  "libsmime3.so",
  "libsoftokn3.so",
  "libssl3.so",
  "libxpcom.so",
  "libxul.so"
};

namespace {

bool EndsWithLiteral(const nsCString &aHaystack, const char *aNeedle)
{
  PRInt32 idx = aHaystack.RFind(aNeedle);
  if (idx == -1) {
    return false;
  }

  return idx + strlen(aNeedle) == aHaystack.Length();
}

void GetDirname(const nsCString &aPath, nsACString &aOut)
{
  PRInt32 idx = aPath.RFind("/");
  if (idx == -1) {
    aOut.Truncate();
  }
  else {
    aOut.Assign(Substring(aPath, 0, idx));
  }
}

void GetBasename(const nsCString &aPath, nsACString &aOut)
{
  nsCString out;
  PRInt32 idx = aPath.RFind("/");
  if (idx == -1) {
    out.Assign(aPath);
  }
  else {
    out.Assign(Substring(aPath, idx + 1));
  }

  
  
  
  if (EndsWithLiteral(out, "(deleted)")) {
    out.Assign(Substring(out, 0, out.RFind("(deleted)")));
  }
  out.StripChars(" ");

  aOut.Assign(out);
}



struct CategoriesSeen {
  CategoriesSeen() :
    mSeenResident(false),
    mSeenPss(false),
    mSeenVsize(false),
    mSeenSwap(false)
  {
  }

  bool mSeenResident;
  bool mSeenPss;
  bool mSeenVsize;
  bool mSeenSwap;
};

} 

class MapsReporter : public nsIMemoryMultiReporter
{
public:
  MapsReporter();

  NS_DECL_ISUPPORTS

  NS_IMETHOD
  CollectReports(nsIMemoryMultiReporterCallback *aCallback,
                 nsISupports *aClosure);

private:
  
  
  nsresult FindLibxul();

  nsresult
  ParseMapping(FILE *aFile,
               nsIMemoryMultiReporterCallback *aCallback,
               nsISupports *aClosure,
               CategoriesSeen *aCategoriesSeen);

  void
  GetReporterNameAndDescription(const char *aPath,
                                const char *aPermissions,
                                nsACString &aName,
                                nsACString &aDesc);

  nsresult
  ParseMapBody(FILE *aFile,
               const nsACString &aName,
               const nsACString &aDescription,
               nsIMemoryMultiReporterCallback *aCallback,
               nsISupports *aClosure,
               CategoriesSeen *aCategoriesSeen);

  nsCString mLibxulDir;
  nsCStringHashSet mMozillaLibraries;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(MapsReporter, nsIMemoryMultiReporter)

MapsReporter::MapsReporter()
{
  const PRUint32 len = ArrayLength(mozillaLibraries);
  mMozillaLibraries.Init(len);
  for (PRUint32 i = 0; i < len; i++) {
    nsCAutoString str;
    str.Assign(mozillaLibraries[i]);
    mMozillaLibraries.Put(str);
  }
}

NS_IMETHODIMP
MapsReporter::CollectReports(nsIMemoryMultiReporterCallback *aCallback,
                             nsISupports *aClosure)
{
  CategoriesSeen categoriesSeen;

  FILE *f = fopen("/proc/self/smaps", "r");
  if (!f)
    return NS_ERROR_FAILURE;

  while (true) {
    nsresult rv = ParseMapping(f, aCallback, aClosure, &categoriesSeen);
    if (NS_FAILED(rv))
      break;
  }

  fclose(f);

  
  
  
  

  NS_ASSERTION(categoriesSeen.mSeenVsize, "Didn't create a vsize node?");
  NS_ASSERTION(categoriesSeen.mSeenVsize, "Didn't create a resident node?");
  if (!categoriesSeen.mSeenSwap) {
    aCallback->Callback(NS_LITERAL_CSTRING(""),
                        NS_LITERAL_CSTRING("map/swap"),
                        nsIMemoryReporter::KIND_NONHEAP,
                        nsIMemoryReporter::UNITS_BYTES,
                        0,
                        NS_LITERAL_CSTRING("This process uses no swap space."),
                        aClosure);
  }

  return NS_OK;
}

nsresult
MapsReporter::FindLibxul()
{
  mLibxulDir.Truncate();

  
  FILE *f = fopen("/proc/self/maps", "r");
  if (!f) {
    return NS_ERROR_FAILURE;
  }

  while (true) {
    
    
    char path[1025];
    int numRead = fscanf(f, "%*[^/]%1024[^\n]", path);
    if (numRead != 1) {
      break;
    }

    nsCAutoString pathStr;
    pathStr.Append(path);

    nsCAutoString basename;
    GetBasename(pathStr, basename);

    if (basename.EqualsLiteral("libxul.so")) {
      GetDirname(pathStr, mLibxulDir);
      break;
    }
  }

  fclose(f);
  return mLibxulDir.IsEmpty() ? NS_ERROR_FAILURE : NS_OK;
}

nsresult
MapsReporter::ParseMapping(
  FILE *aFile,
  nsIMemoryMultiReporterCallback *aCallback,
  nsISupports *aClosure,
  CategoriesSeen *aCategoriesSeen)
{
  
  
  PR_STATIC_ASSERT(sizeof(long long) == sizeof(PRInt64));
  PR_STATIC_ASSERT(sizeof(int) == sizeof(PRInt32));

  
  
  FindLibxul();

  
  
  
  
  

  const int argCount = 8;

  unsigned long long addrStart, addrEnd;
  char perms[5];
  unsigned long long offset;
  char devMajor[3];
  char devMinor[3];
  unsigned int inode;
  char path[1025];

  
  path[0] = '\0';

  
  
  
  
  
  int numRead = fscanf(aFile, "%llx-%llx %4s %llx %2s:%2s %u%1024[^\n]",
                       &addrStart, &addrEnd, perms, &offset, devMajor,
                       devMinor, &inode, path);

  
  fscanf(aFile, " ");

  
  
  if (numRead != argCount && numRead != argCount - 1) {
    return NS_ERROR_FAILURE;
  }

  nsCAutoString name, description;
  GetReporterNameAndDescription(path, perms, name, description);

  while (true) {
    nsresult rv = ParseMapBody(aFile, name, description, aCallback,
                               aClosure, aCategoriesSeen);
    if (NS_FAILED(rv))
      break;
  }

  return NS_OK;
}

void
MapsReporter::GetReporterNameAndDescription(
  const char *aPath,
  const char *aPerms,
  nsACString &aName,
  nsACString &aDesc)
{
  aName.Truncate();
  aDesc.Truncate();

  
  
  
  nsCAutoString absPath;
  absPath.Append(aPath);
  absPath.StripChars(" ");

  nsCAutoString basename;
  GetBasename(absPath, basename);

  if (basename.EqualsLiteral("[heap]")) {
    aName.Append("anonymous/anonymous, within brk()");
    aDesc.Append("Memory in anonymous mappings within the boundaries "
                 "defined by brk() / sbrk().  This is likely to be just "
                 "a portion of the application's heap; the remainder "
                 "lives in other anonymous mappings. This node corresponds to "
                 "'[heap]' in /proc/self/smaps.");
  }
  else if (basename.EqualsLiteral("[stack]")) {
    aName.Append("main thread's stack");
    aDesc.Append("The stack size of the process's main thread.  This node "
                 "corresponds to '[stack]' in /proc/self/smaps.");
  }
  else if (basename.EqualsLiteral("[vdso]")) {
    aName.Append("vdso");
    aDesc.Append("The virtual dynamically-linked shared object, also known as "
                 "the 'vsyscall page'. This is a memory region mapped by the "
                 "operating system for the purpose of allowing processes to "
                 "perform some privileged actions without the overhead of a "
                 "syscall.");
  }
  else if (!basename.IsEmpty()) {
    nsCAutoString dirname;
    GetDirname(absPath, dirname);

    
    
    if (EndsWithLiteral(basename, ".so") ||
        (basename.Find(".so") != -1 && dirname.Find("/lib") != -1)) {
      aName.Append("shared-libraries/");
      if ((!mLibxulDir.IsEmpty() && dirname.Equals(mLibxulDir)) ||
          mMozillaLibraries.Contains(basename)) {
        aName.Append("shared-libraries-mozilla/");
      }
      else {
        aName.Append("shared-libraries-other/");
      }
    }
    else {
      aName.Append("other-files/");
      if (EndsWithLiteral(basename, ".xpi")) {
        aName.Append("extensions/");
      }
      else if (dirname.Find("/fontconfig") != -1) {
        aName.Append("fontconfig/");
      }
    }

    aName.Append(basename);
    aDesc.Append(absPath);
  }
  else {
    aName.Append("anonymous/anonymous, outside brk()");
    aDesc.Append("Memory in anonymous mappings outside the boundaries defined "
                 "by brk() / sbrk().");
  }

  aName.Append(" [");
  aName.Append(aPerms);
  aName.Append("]");

  
  aDesc.Append(" (");
  if (strstr(aPerms, "rw")) {
    aDesc.Append("read/write, ");
  }
  else if (strchr(aPerms, 'r')) {
    aDesc.Append("read-only, ");
  }
  else if (strchr(aPerms, 'w')) {
    aDesc.Append("write-only, ");
  }
  else {
    aDesc.Append("not readable, not writable, ");
  }

  if (strchr(aPerms, 'x')) {
    aDesc.Append("executable, ");
  }
  else {
    aDesc.Append("not executable, ");
  }

  if (strchr(aPerms, 's')) {
    aDesc.Append("shared");
  }
  else if (strchr(aPerms, 'p')) {
    aDesc.Append("private");
  }
  else {
    aDesc.Append("not shared or private??");
  }
  aDesc.Append(")");
}

nsresult
MapsReporter::ParseMapBody(
  FILE *aFile,
  const nsACString &aName,
  const nsACString &aDescription,
  nsIMemoryMultiReporterCallback *aCallback,
  nsISupports *aClosure,
  CategoriesSeen *aCategoriesSeen)
{
  PR_STATIC_ASSERT(sizeof(long long) == sizeof(PRInt64));

  const int argCount = 2;

  char desc[1025];
  unsigned long long size;
  if (fscanf(aFile, "%1024[a-zA-Z_]: %llu kB\n",
             desc, &size) != argCount) {
    return NS_ERROR_FAILURE;
  }

  
  if (size == 0)
    return NS_OK;

  const char* category;
  if (strcmp(desc, "Size") == 0) {
    category = "vsize";
    aCategoriesSeen->mSeenVsize = PR_TRUE;
  }
  else if (strcmp(desc, "Rss") == 0) {
    category = "resident";
    aCategoriesSeen->mSeenResident = PR_TRUE;
  }
  else if (strcmp(desc, "Pss") == 0) {
    category = "pss";
    aCategoriesSeen->mSeenPss = PR_TRUE;
  }
  else if (strcmp(desc, "Swap") == 0) {
    category = "swap";
    aCategoriesSeen->mSeenSwap = PR_TRUE;
  }
  else {
    
    return NS_OK;
  }

  nsCAutoString path;
  path.Append("map/");
  path.Append(category);
  path.Append("/");
  path.Append(aName);

  aCallback->Callback(NS_LITERAL_CSTRING(""),
                      path,
                      nsIMemoryReporter::KIND_NONHEAP,
                      nsIMemoryReporter::UNITS_BYTES,
                      PRInt64(size) * 1024, 
                      aDescription, aClosure);

  return NS_OK;
}

void Init()
{
  nsCOMPtr<nsIMemoryMultiReporter> reporter = new MapsReporter();
  NS_RegisterMemoryMultiReporter(reporter);
}

} 
} 
