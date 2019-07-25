













































#ifndef _NS_LOCAL_FILE_H_
#define _NS_LOCAL_FILE_H_

#include "nscore.h"

#define NS_LOCAL_FILE_CID {0x2e23e220, 0x60be, 0x11d3, {0x8c, 0x4a, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74}}

#define NS_DECL_NSLOCALFILE_UNICODE_METHODS                                                      \
    nsresult AppendUnicode(const PRUnichar *aNode);                                              \
    nsresult GetUnicodeLeafName(PRUnichar **aLeafName);                                          \
    nsresult SetUnicodeLeafName(const PRUnichar *aLeafName);                                     \
    nsresult CopyToUnicode(nsIFile *aNewParentDir, const PRUnichar *aNewLeafName);               \
    nsresult CopyToFollowingLinksUnicode(nsIFile *aNewParentDir, const PRUnichar *aNewLeafName); \
    nsresult MoveToUnicode(nsIFile *aNewParentDir, const PRUnichar *aNewLeafName);               \
    nsresult GetUnicodeTarget(PRUnichar **aTarget);                                              \
    nsresult GetUnicodePath(PRUnichar **aPath);                                                  \
    nsresult InitWithUnicodePath(const PRUnichar *aPath);                                        \
    nsresult AppendRelativeUnicodePath(const PRUnichar *aRelativePath);





#include <errno.h>
#include "nsILocalFile.h"

#ifdef XP_WIN
#include "nsLocalFileWin.h"
#elif defined(XP_MACOSX)
#include "nsLocalFileOSX.h"
#elif defined(XP_UNIX) || defined(XP_BEOS)
#include "nsLocalFileUnix.h"
#elif defined(XP_OS2)
#include "nsLocalFileOS2.h"
#else
#error NOT_IMPLEMENTED
#endif

#define NSRESULT_FOR_RETURN(ret) (((ret) < 0) ? NSRESULT_FOR_ERRNO() : NS_OK)

inline nsresult
nsresultForErrno(int err)
{
    switch (err) {
      case 0:
        return NS_OK;
      case ENOENT:
        return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
      case ENOTDIR:
        return NS_ERROR_FILE_DESTINATION_NOT_DIR;
#ifdef ENOLINK
      case ENOLINK:
        return NS_ERROR_FILE_UNRESOLVABLE_SYMLINK;
#endif 
      case EEXIST:
        return NS_ERROR_FILE_ALREADY_EXISTS;
#ifdef EPERM
      case EPERM:
#endif 
      case EACCES:
        return NS_ERROR_FILE_ACCESS_DENIED;
      




#if ENOTEMPTY != EEXIST
      case ENOTEMPTY:
        return NS_ERROR_FILE_DIR_NOT_EMPTY;
#endif 
      default:
        return NS_ERROR_FAILURE;
    }
}

#define NSRESULT_FOR_ERRNO() nsresultForErrno(errno)

void NS_StartupLocalFile();
void NS_ShutdownLocalFile();

#endif
