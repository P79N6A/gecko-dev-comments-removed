





#include "mozilla/Mutex.h"
#include "mozilla/Scoped.h"
#include "mozilla/UniquePtr.h"

#include <algorithm>

#include "PoisonIOInterposer.h"

#ifdef MOZ_REPLACE_MALLOC
#include "replace_malloc_bridge.h"
#endif


#if defined(XP_WIN32)
#include <io.h>
inline intptr_t
FileDescriptorToHandle(int aFd)
{
  return _get_osfhandle(aFd);
}
#else
inline intptr_t
FileDescriptorToHandle(int aFd)
{
  return aFd;
}
#endif 

using namespace mozilla;

namespace {
struct DebugFilesAutoLockTraits
{
  typedef PRLock* type;
  const static type empty() { return nullptr; }
  const static void release(type aL) { PR_Unlock(aL); }
};

class DebugFilesAutoLock : public Scoped<DebugFilesAutoLockTraits>
{
  static PRLock* Lock;
public:
  static void Clear();
  static PRLock* getDebugFileIDsLock()
  {
    
    
    
    
    
    
    if (!Lock) {
      Lock = PR_NewLock();
    }

    
    
    return Lock;
  }

  DebugFilesAutoLock()
    : Scoped<DebugFilesAutoLockTraits>(getDebugFileIDsLock())
  {
    PR_Lock(get());
  }
};

PRLock* DebugFilesAutoLock::Lock;
void
DebugFilesAutoLock::Clear()
{
  MOZ_ASSERT(Lock != nullptr);
  Lock = nullptr;
}











template <typename T, size_t chunk_size=64>
class ChunkedList {
  struct ListChunk {
    static const size_t kLength = \
      (chunk_size - sizeof(ListChunk*)) / sizeof(mozilla::Atomic<T>);

    mozilla::Atomic<T> mElements[kLength];
    mozilla::UniquePtr<ListChunk> mNext;

    ListChunk() : mNext(nullptr) {}
  };

  ListChunk mList;
  mozilla::Atomic<size_t> mLength;

public:
  ChunkedList() : mLength(0) {}

  ~ChunkedList() {
    
    
    
    
    
    MOZ_RELEASE_ASSERT(mLength <= ListChunk::kLength);
  }

  
  
  
  void Add(T aValue)
  {
    ListChunk *list = &mList;
    size_t position = mLength;
    for (; position >= ListChunk::kLength; position -= ListChunk::kLength) {
      if (!list->mNext) {
        list->mNext.reset(new ListChunk());
      }
      list = list->mNext.get();
    }
    
    
    list->mElements[position] = aValue;
    mLength++;
  }

  
  
  
  void Remove(T aValue)
  {
    if (!mLength) {
      return;
    }
    ListChunk *list = &mList;
    size_t last = mLength - 1;
    do {
      size_t position = 0;
      
      for (; position < ListChunk::kLength; position++) {
        if (aValue == list->mElements[position]) {
          ListChunk *last_list = list;
          
          
          for (; last >= ListChunk::kLength; last -= ListChunk::kLength) {
            last_list = last_list->mNext.get();
          }
          
          
          T value = last_list->mElements[last];
          list->mElements[position] = value;
          mLength--;
          return;
        }
      }
      last -= ListChunk::kLength;
      list = list->mNext.get();
    } while (list);
  }

  
  
  
  bool Contains(T aValue)
  {
    ListChunk *list = &mList;
    
    
    size_t length = mLength;
    do {
      size_t list_length = ListChunk::kLength;
      list_length = std::min(list_length, length);
      for (size_t position = 0; position < list_length; position++) {
        if (aValue == list->mElements[position]) {
          return true;
        }
      }
      length -= ListChunk::kLength;
      list = list->mNext.get();
    } while (list);

    return false;
  }
};

typedef ChunkedList<intptr_t> FdList;



FdList&
getDebugFileIDs()
{
  static FdList DebugFileIDs;
  return DebugFileIDs;
}


} 

namespace mozilla {



bool
IsDebugFile(intptr_t aFileID)
{
  return getDebugFileIDs().Contains(aFileID);
}

} 

extern "C" {

void
MozillaRegisterDebugHandle(intptr_t aHandle)
{
  DebugFilesAutoLock lockedScope;
  FdList& DebugFileIDs = getDebugFileIDs();
  MOZ_ASSERT(!DebugFileIDs.Contains(aHandle));
  DebugFileIDs.Add(aHandle);
}

void
MozillaRegisterDebugFD(int aFd)
{
  MozillaRegisterDebugHandle(FileDescriptorToHandle(aFd));
}

void
MozillaRegisterDebugFILE(FILE* aFile)
{
  int fd = fileno(aFile);
  if (fd == 1 || fd == 2) {
    return;
  }
  MozillaRegisterDebugFD(fd);
}

void
MozillaUnRegisterDebugHandle(intptr_t aHandle)
{
  DebugFilesAutoLock lockedScope;
  FdList& DebugFileIDs = getDebugFileIDs();
  MOZ_ASSERT(DebugFileIDs.Contains(aHandle));
  DebugFileIDs.Remove(aHandle);
}

void
MozillaUnRegisterDebugFD(int aFd)
{
  MozillaUnRegisterDebugHandle(FileDescriptorToHandle(aFd));
}

void
MozillaUnRegisterDebugFILE(FILE* aFile)
{
  int fd = fileno(aFile);
  if (fd == 1 || fd == 2) {
    return;
  }
  fflush(aFile);
  MozillaUnRegisterDebugFD(fd);
}

}  

#ifdef MOZ_REPLACE_MALLOC
void
DebugFdRegistry::RegisterHandle(intptr_t aHandle)
{
  MozillaRegisterDebugHandle(aHandle);
}

void
DebugFdRegistry::UnRegisterHandle(intptr_t aHandle)
{
  MozillaUnRegisterDebugHandle(aHandle);
}
#endif
