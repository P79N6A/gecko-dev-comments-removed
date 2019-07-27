





#ifndef nsVersionComparator_h__
#define nsVersionComparator_h__

#include "nscore.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(XP_WIN) && !defined(UPDATER_NO_STRING_GLUE_STL)
#include <wchar.h>
#include "nsStringGlue.h"
#endif


























namespace mozilla {

int32_t CompareVersions(const char* aStrA, const char* aStrB);

#ifdef XP_WIN
int32_t CompareVersions(const char16_t* aStrA, const char16_t* aStrB);
#endif

struct Version
{
  explicit Version(const char* aVersionString)
  {
    versionContent = strdup(aVersionString);
  }

  const char* ReadContent() const
  {
    return versionContent;
  }

  ~Version()
  {
    free(versionContent);
  }

  bool operator<(const Version& aRhs) const
  {
    return CompareVersions(versionContent, aRhs.ReadContent()) == -1;
  }
  bool operator<=(const Version& aRhs) const
  {
    return CompareVersions(versionContent, aRhs.ReadContent()) < 1;
  }
  bool operator>(const Version& aRhs) const
  {
    return CompareVersions(versionContent, aRhs.ReadContent()) == 1;
  }
  bool operator>=(const Version& aRhs) const
  {
    return CompareVersions(versionContent, aRhs.ReadContent()) > -1;
  }
  bool operator==(const Version& aRhs) const
  {
    return CompareVersions(versionContent, aRhs.ReadContent()) == 0;
  }
  bool operator!=(const Version& aRhs) const
  {
    return CompareVersions(versionContent, aRhs.ReadContent()) != 0;
  }
  bool operator<(const char* aRhs) const
  {
    return CompareVersions(versionContent, aRhs) == -1;
  }
  bool operator<=(const char* aRhs) const
  {
    return CompareVersions(versionContent, aRhs) < 1;
  }
  bool operator>(const char* aRhs) const
  {
    return CompareVersions(versionContent, aRhs) == 1;
  }
  bool operator>=(const char* aRhs) const
  {
    return CompareVersions(versionContent, aRhs) > -1;
  }
  bool operator==(const char* aRhs) const
  {
    return CompareVersions(versionContent, aRhs) == 0;
  }
  bool operator!=(const char* aRhs) const
  {
    return CompareVersions(versionContent, aRhs) != 0;
  }

private:
  char* versionContent;
};

#ifdef XP_WIN
struct VersionW
{
  VersionW(const char16_t* aVersionStringW)
  {
    versionContentW =
      reinterpret_cast<char16_t*>(wcsdup(char16ptr_t(aVersionStringW)));
  }

  const char16_t* ReadContentW() const
  {
    return versionContentW;
  }

  ~VersionW()
  {
    free(versionContentW);
  }

  bool operator<(const VersionW& aRhs) const
  {
    return CompareVersions(versionContentW, aRhs.ReadContentW()) == -1;
  }
  bool operator<=(const VersionW& aRhs) const
  {
    return CompareVersions(versionContentW, aRhs.ReadContentW()) < 1;
  }
  bool operator>(const VersionW& aRhs) const
  {
    return CompareVersions(versionContentW, aRhs.ReadContentW()) == 1;
  }
  bool operator>=(const VersionW& aRhs) const
  {
    return CompareVersions(versionContentW, aRhs.ReadContentW()) > -1;
  }
  bool operator==(const VersionW& aRhs) const
  {
    return CompareVersions(versionContentW, aRhs.ReadContentW()) == 0;
  }
  bool operator!=(const VersionW& aRhs) const
  {
    return CompareVersions(versionContentW, aRhs.ReadContentW()) != 0;
  }

private:
  char16_t* versionContentW;
};
#endif

} 

#endif 

