





































#include <vector>
#include <string.h>
#include <stdlib.h>
#include <mozilla/StdInt.h>

class SharedLibrary {
public:
  SharedLibrary(unsigned long aStart, unsigned long aEnd, unsigned long aOffset, char *aName)
    : mStart(aStart)
    , mEnd(aEnd)
    , mOffset(aOffset)
    , mName(strdup(aName))
  {}

  SharedLibrary(const SharedLibrary& aEntry)
    : mStart(aEntry.mStart)
    , mEnd(aEntry.mEnd)
    , mOffset(aEntry.mOffset)
    , mName(strdup(aEntry.mName))
  {}

  SharedLibrary& operator=(const SharedLibrary& aEntry)
  {
    
    if (this == &aEntry) return *this;

    mStart = aEntry.mStart;
    mEnd = aEntry.mEnd;
    mOffset = aEntry.mOffset;
    if (mName)
      free(mName);
    mName = strdup(aEntry.mName);
    return *this;
  }

  ~SharedLibrary()
  {
    free(mName);
  }

  uintptr_t GetStart() { return mStart; }
  uintptr_t GetEnd() { return mEnd; }
  char* GetName() { return mName; }

private:
  explicit SharedLibrary() {}

  uintptr_t mStart;
  uintptr_t mEnd;
  uintptr_t mOffset;
  char *mName;
};

class SharedLibraryInfo {
public:
  static SharedLibraryInfo GetInfoForSelf();
  SharedLibraryInfo() {}

  void AddSharedLibrary(SharedLibrary entry)
  {
    mEntries.push_back(entry);
  }

  SharedLibrary& GetEntry(size_t i)
  {
    return mEntries[i];
  }

  size_t GetSize()
  {
    return mEntries.size();
  }
private:
  std::vector<SharedLibrary> mEntries;
};
