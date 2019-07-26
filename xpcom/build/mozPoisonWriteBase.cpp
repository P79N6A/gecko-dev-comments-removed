





#include "mozPoisonWrite.h"
#include "mozPoisonWriteBase.h"
#include "mozilla/ProcessedStack.h"
#include "mozilla/Scoped.h"
#include "mozilla/SHA1.h"
#include "mozilla/Telemetry.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsStackWalk.h"
#include "nsPrintfCString.h"
#include "plstr.h"
#include <algorithm>
#ifdef XP_WIN
#define NS_T(str) L ## str
#define NS_SLASH "\\"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#else
#define NS_SLASH "/"
#endif
using namespace mozilla;

namespace {
struct DebugFilesAutoLockTraits {
  typedef PRLock *type;
  const static type empty() {
    return nullptr;
  }
  const static void release(type aL) {
    PR_Unlock(aL);
  }
};

class DebugFilesAutoLock : public Scoped<DebugFilesAutoLockTraits> {
  static PRLock *Lock;
public:
  static void Clear();
  static PRLock *getDebugFileIDsLock() {
    
    
    
    
    
    
    static bool Initialized = false;
    if (!Initialized) {
      Lock = PR_NewLock();
      Initialized = true;
    }

    
    
    return Lock;
  }

  DebugFilesAutoLock() :
    Scoped<DebugFilesAutoLockTraits>(getDebugFileIDsLock()) {
    PR_Lock(get());
  }
};

PRLock *DebugFilesAutoLock::Lock;
void DebugFilesAutoLock::Clear() {
  MOZ_ASSERT(Lock != nullptr);
  Lock = nullptr;
}

static char *sProfileDirectory = NULL;



std::vector<intptr_t>* getDebugFileIDs() {
  PRLock *lock = DebugFilesAutoLock::getDebugFileIDsLock();
  PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(lock);
  
  
  static std::vector<intptr_t> *DebugFileIDs = new std::vector<intptr_t>();
  return DebugFileIDs;
}



class SHA1Stream
{
public:
    explicit SHA1Stream(FILE *stream)
      : mFile(stream)
    {
      MozillaRegisterDebugFILE(mFile);
    }

    void Printf(const char *aFormat, ...)
    {
        MOZ_ASSERT(mFile);
        va_list list;
        va_start(list, aFormat);
        nsAutoCString str;
        str.AppendPrintf(aFormat, list);
        va_end(list);
        mSHA1.update(str.get(), str.Length());
        fwrite(str.get(), 1, str.Length(), mFile);
    }
    void Finish(SHA1Sum::Hash &aHash)
    {
        int fd = fileno(mFile);
        fflush(mFile);
        MozillaUnRegisterDebugFD(fd);
        fclose(mFile);
        mSHA1.finish(aHash);
        mFile = NULL;
    }
private:
    FILE *mFile;
    SHA1Sum mSHA1;
};

static void RecordStackWalker(void *aPC, void *aSP, void *aClosure)
{
    std::vector<uintptr_t> *stack =
        static_cast<std::vector<uintptr_t>*>(aClosure);
    stack->push_back(reinterpret_cast<uintptr_t>(aPC));
}


enum PoisonState {
  POISON_UNINITIALIZED = 0,
  POISON_ON,
  POISON_OFF
};







PoisonState sPoisoningState = POISON_UNINITIALIZED;
}

namespace mozilla {

void InitWritePoisoning()
{
  
  MozillaRegisterDebugFD(1);
  MozillaRegisterDebugFD(2);

  nsCOMPtr<nsIFile> mozFile;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mozFile));
  if (mozFile) {
    nsAutoCString nativePath;
    nsresult rv = mozFile->GetNativePath(nativePath);
    if (NS_SUCCEEDED(rv)) {
      sProfileDirectory = PL_strdup(nativePath.get());
    }
  }
}

bool ValidWriteAssert(bool ok)
{
    if (gShutdownChecks == SCM_CRASH && !ok) {
        MOZ_CRASH();
    }

    
    
    
    if (gShutdownChecks == SCM_NOTHING || ok || !sProfileDirectory ||
        !Telemetry::CanRecord()) {
        return ok;
    }

    
    
    std::vector<uintptr_t> rawStack;

    NS_StackWalk(RecordStackWalker,  0,  0,
                 reinterpret_cast<void*>(&rawStack), 0, nullptr);
    Telemetry::ProcessedStack stack = Telemetry::GetStackAndModules(rawStack);

    nsPrintfCString nameAux("%s%s%s", sProfileDirectory,
                            NS_SLASH, "Telemetry.LateWriteTmpXXXXXX");
    char *name;
    nameAux.GetMutableData(&name);

    
    
    FILE *stream;
#ifdef XP_WIN
    HANDLE hFile;
    do {
      
      int result = _mktemp_s(name, strlen(name) + 1);
      hFile = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    } while (GetLastError() == ERROR_FILE_EXISTS);

    if (hFile == INVALID_HANDLE_VALUE) {
      NS_RUNTIMEABORT("Um, how did we get here?");
    }

    
    int fd = _open_osfhandle((intptr_t)hFile, _O_APPEND);
    if (fd == -1) {
      NS_RUNTIMEABORT("Um, how did we get here?");
    }

    stream = _fdopen(fd, "w");
#else
    int fd = mkstemp(name);
    stream = fdopen(fd, "w");
#endif

    SHA1Stream sha1Stream(stream);

    size_t numModules = stack.GetNumModules();
    sha1Stream.Printf("%u\n", (unsigned)numModules);
    for (size_t i = 0; i < numModules; ++i) {
        Telemetry::ProcessedStack::Module module = stack.GetModule(i);
        sha1Stream.Printf("%s %s\n", module.mBreakpadId.c_str(),
                          module.mName.c_str());
    }

    size_t numFrames = stack.GetStackSize();
    sha1Stream.Printf("%u\n", (unsigned)numFrames);
    for (size_t i = 0; i < numFrames; ++i) {
        const Telemetry::ProcessedStack::Frame &frame =
            stack.GetFrame(i);
        
        
        
        
        
        
        
        
        
        sha1Stream.Printf("%d %x\n", frame.mModIndex, (unsigned)frame.mOffset);
    }

    SHA1Sum::Hash sha1;
    sha1Stream.Finish(sha1);

    
    
    
    

    
    
    nsPrintfCString finalName("%s%s", sProfileDirectory, "/Telemetry.LateWriteFinal-");
    for (int i = 0; i < 20; ++i) {
        finalName.AppendPrintf("%02x", sha1[i]);
    }
    PR_Delete(finalName.get());
    PR_Rename(name, finalName.get());
    return false;
}

void DisableWritePoisoning() {
  if (sPoisoningState != POISON_ON)
    return;

  sPoisoningState = POISON_OFF;
  PL_strfree(sProfileDirectory);
  sProfileDirectory = nullptr;

  PRLock *Lock;
  {
    DebugFilesAutoLock lockedScope;
    delete getDebugFileIDs();
    Lock = DebugFilesAutoLock::getDebugFileIDsLock();
    DebugFilesAutoLock::Clear();
  }
  PR_DestroyLock(Lock);
}
void EnableWritePoisoning() {
  sPoisoningState = POISON_ON;
}

bool PoisonWriteEnabled()
{
  return sPoisoningState == POISON_ON;
}

bool IsDebugFile(intptr_t aFileID) {
  DebugFilesAutoLock lockedScope;

  std::vector<intptr_t> &Vec = *getDebugFileIDs();
  return std::find(Vec.begin(), Vec.end(), aFileID) != Vec.end();
}

} 

extern "C" {
  void MozillaRegisterDebugFD(int fd) {
    if (sPoisoningState == POISON_OFF)
      return;
    DebugFilesAutoLock lockedScope;
    intptr_t fileId = FileDescriptorToID(fd);
    std::vector<intptr_t> &Vec = *getDebugFileIDs();
    MOZ_ASSERT(std::find(Vec.begin(), Vec.end(), fileId) == Vec.end());
    Vec.push_back(fileId);
  }
  void MozillaRegisterDebugFILE(FILE *f) {
    if (sPoisoningState == POISON_OFF)
      return;
    int fd = fileno(f);
    if (fd == 1 || fd == 2)
      return;
    MozillaRegisterDebugFD(fd);
  }
  void MozillaUnRegisterDebugFD(int fd) {
    if (sPoisoningState == POISON_OFF)
      return;
    DebugFilesAutoLock lockedScope;
    intptr_t fileId = FileDescriptorToID(fd);
    std::vector<intptr_t> &Vec = *getDebugFileIDs();
    std::vector<intptr_t>::iterator i =
      std::find(Vec.begin(), Vec.end(), fileId);
    MOZ_ASSERT(i != Vec.end());
    Vec.erase(i);
  }
  void MozillaUnRegisterDebugFILE(FILE *f) {
    if (sPoisoningState == POISON_OFF)
      return;
    int fd = fileno(f);
    if (fd == 1 || fd == 2)
      return;
    fflush(f);
    MozillaUnRegisterDebugFD(fd);
  }
}
