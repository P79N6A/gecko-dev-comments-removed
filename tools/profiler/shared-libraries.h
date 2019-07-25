





































#ifndef SHARED_LIBRARIES_H_
#define SHARED_LIBRARIES_H_

#ifndef MOZ_ENABLE_PROFILER_SPS
#error This header does not have a useful implementation on your platform!
#endif

#include <algorithm>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <mozilla/StandardInteger.h>
#include <nsID.h>

class SharedLibrary {
public:

  SharedLibrary(unsigned long aStart,
                unsigned long aEnd,
                unsigned long aOffset,
#ifdef XP_WIN
                nsID aPdbSignature,
                unsigned long aPdbAge,
#endif
                char *aName)
    : mStart(aStart)
    , mEnd(aEnd)
    , mOffset(aOffset)
#ifdef XP_WIN
    , mPdbSignature(aPdbSignature)
    , mPdbAge(aPdbAge)
#endif
    , mName(strdup(aName))
  {}

  SharedLibrary(const SharedLibrary& aEntry)
    : mStart(aEntry.mStart)
    , mEnd(aEntry.mEnd)
    , mOffset(aEntry.mOffset)
#ifdef XP_WIN
    , mPdbSignature(aEntry.mPdbSignature)
    , mPdbAge(aEntry.mPdbAge)
#endif
    , mName(strdup(aEntry.mName))
  {}

  SharedLibrary& operator=(const SharedLibrary& aEntry)
  {
    
    if (this == &aEntry) return *this;

    mStart = aEntry.mStart;
    mEnd = aEntry.mEnd;
    mOffset = aEntry.mOffset;
#ifdef XP_WIN
    mPdbSignature = aEntry.mPdbSignature;
    mPdbAge = aEntry.mPdbAge;
#endif
    if (mName)
      free(mName);
    mName = strdup(aEntry.mName);
    return *this;
  }

  bool operator==(const SharedLibrary& other) const
  {
    bool equal = ((mStart == other.mStart) &&
                  (mEnd == other.mEnd) &&
                  (mOffset == other.mOffset) &&
                  (mName && other.mName && (strcmp(mName, other.mName) == 0)));
#ifdef XP_WIN
    equal = equal &&
            (mPdbSignature.Equals(other.mPdbSignature)) &&
            (mPdbAge == other.mPdbAge);
#endif
    return equal;
  }

  ~SharedLibrary()
  {
    free(mName);
    mName = NULL;
  }

  uintptr_t GetStart() const { return mStart; }
  uintptr_t GetEnd() const { return mEnd; }
#ifdef XP_WIN
  nsID GetPdbSignature() const { return mPdbSignature; }
  uint32_t GetPdbAge() const { return mPdbAge; }
#endif
  char* GetName() const { return mName; }

private:
  explicit SharedLibrary() {}

  uintptr_t mStart;
  uintptr_t mEnd;
  uintptr_t mOffset;
#ifdef XP_WIN
  
  nsID mPdbSignature;
  uint32_t mPdbAge;
#endif
  char *mName;
};

static bool
CompareAddresses(const SharedLibrary& first, const SharedLibrary& second)
{
  return first.GetStart() < second.GetStart();
}

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

  
  
  void RemoveEntries(size_t first, size_t last)
  {
    mEntries.erase(mEntries.begin() + first, mEntries.begin() + last);
  }

  bool Contains(const SharedLibrary& searchItem) const
  {
    return (mEntries.end() !=
              std::find(mEntries.begin(), mEntries.end(), searchItem));
  }

  size_t GetSize() const
  {
    return mEntries.size();
  }

  void SortByAddress()
  {
    std::sort(mEntries.begin(), mEntries.end(), CompareAddresses);
  }

  void Clear()
  {
    mEntries.clear();
  }

private:
  std::vector<SharedLibrary> mEntries;
};

#endif
