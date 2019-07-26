




#ifndef mozilla_FileUtils_h
#define mozilla_FileUtils_h

#include "nscore.h" 

#if defined(XP_UNIX) || defined(XP_OS2)
# include <unistd.h>
#elif defined(XP_WIN)
# include <io.h>
#endif
#include "prio.h"

#include "mozilla/Scoped.h"
#include "nsIFile.h"
#include <errno.h>
#include <limits.h>

namespace mozilla {

#if defined(XP_WIN)
typedef void* filedesc_t;
typedef const wchar_t* pathstr_t;
#else
typedef int filedesc_t;
typedef const char* pathstr_t;
#endif






struct ScopedCloseFDTraits
{
  typedef int type;
  static type empty() { return -1; }
  static void release(type fd) {
    if (fd != -1) {
      while ((close(fd) == -1) && (errno == EINTR)) {
        ;
      }
    }
  }
};
typedef Scoped<ScopedCloseFDTraits> ScopedClose;

#if !defined(XPCOM_GLUE)






struct ScopedClosePRFDTraits
{
  typedef PRFileDesc* type;
  static type empty() { return nullptr; }
  static void release(type fd) {
    if (fd != nullptr) {
      PR_Close(fd);
    }
  }
};
typedef Scoped<ScopedClosePRFDTraits> AutoFDClose;


struct ScopedCloseFileTraits
{
  typedef FILE *type;
  static type empty() { return nullptr; }
  static void release(type f) {
    if (f) {
      fclose(f);
    }
  }
};
typedef Scoped<ScopedCloseFileTraits> ScopedCloseFile;










NS_COM_GLUE bool fallocate(PRFileDesc *aFD, int64_t aLength);








NS_COM_GLUE void ReadAheadLib(nsIFile* aFile);












NS_COM_GLUE void ReadAheadFile(nsIFile* aFile, const size_t aOffset = 0,
                               const size_t aCount = SIZE_MAX,
                               filedesc_t* aOutFd = nullptr);

#endif 








NS_COM_GLUE void ReadAheadLib(pathstr_t aFilePath);












NS_COM_GLUE void ReadAheadFile(pathstr_t aFilePath, const size_t aOffset = 0,
                               const size_t aCount = SIZE_MAX,
                               filedesc_t* aOutFd = nullptr);













NS_COM_GLUE void ReadAhead(filedesc_t aFd, const size_t aOffset = 0,
                           const size_t aCount = SIZE_MAX);





#if (defined(MOZ_WIDGET_GONK) || defined(DEBUG)) && defined(XP_UNIX)

#ifndef ReadSysFile_PRESENT
#define ReadSysFile_PRESENT
#endif 

#define MOZ_TEMP_FAILURE_RETRY(exp) (__extension__({ \
  typeof (exp) _rc; \
  do { \
    _rc = (exp); \
  } while (_rc == -1 && errno == EINTR); \
  _rc; \
}))













bool
ReadSysFile(
  const char* aFilename,
  char* aBuf,
  size_t aBufSize);





bool
ReadSysFile(
  const char* aFilename,
  int* aVal);






bool
ReadSysFile(
  const char* aFilename,
  bool* aVal);

#endif 

} 
#endif
