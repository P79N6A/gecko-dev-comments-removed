



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

int32_t NS_COM_GLUE
CompareVersions(const char *A, const char *B);

#ifdef XP_WIN
int32_t NS_COM_GLUE
CompareVersions(const char16_t *A, const char16_t *B);
#endif

struct NS_COM_GLUE Version
{
  Version(const char* versionString)
  {
    versionContent = strdup(versionString);
  }

  const char* ReadContent() const
  {
    return versionContent;
  }

  ~Version()
  {
    free(versionContent);
  }

  bool operator< (const Version& rhs) const
  {
    return CompareVersions(versionContent, rhs.ReadContent()) == -1;
  }
  bool operator<= (const Version& rhs) const
  {
    return CompareVersions(versionContent, rhs.ReadContent()) < 1;
  }
  bool operator> (const Version& rhs) const
  {
    return CompareVersions(versionContent, rhs.ReadContent()) == 1;
  }
  bool operator>= (const Version& rhs) const
  {
    return CompareVersions(versionContent, rhs.ReadContent()) > -1;
  }
  bool operator== (const Version& rhs) const
  {
    return CompareVersions(versionContent, rhs.ReadContent()) == 0;
  }
  bool operator!= (const Version& rhs) const
  {
    return CompareVersions(versionContent, rhs.ReadContent()) != 0;
  }

 private:
  char* versionContent;
};

#ifdef XP_WIN
struct NS_COM_GLUE VersionW
{
  VersionW(const char16_t *versionStringW)
  {
    versionContentW = reinterpret_cast<char16_t*>(wcsdup(char16ptr_t(versionStringW)));
  }

  const char16_t* ReadContentW() const
  {
    return versionContentW;
  }

  ~VersionW()
  {
    free(versionContentW);
  }

  bool operator< (const VersionW& rhs) const
  {
    return CompareVersions(versionContentW, rhs.ReadContentW()) == -1;
  }
  bool operator<= (const VersionW& rhs) const
  {
    return CompareVersions(versionContentW, rhs.ReadContentW()) < 1;
  }
  bool operator> (const VersionW& rhs) const
  {
    return CompareVersions(versionContentW, rhs.ReadContentW()) == 1;
  }
  bool operator>= (const VersionW& rhs) const
  {
    return CompareVersions(versionContentW, rhs.ReadContentW()) > -1;
  }
  bool operator== (const VersionW& rhs) const
  {
    return CompareVersions(versionContentW, rhs.ReadContentW()) == 0;
  }
  bool operator!= (const VersionW& rhs) const
  {
    return CompareVersions(versionContentW, rhs.ReadContentW()) != 0;
  }

 private:
  char16_t* versionContentW;
};
#endif

} 

#endif 

