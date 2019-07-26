













#ifndef _NS_LOCAL_FILE_H_
#define _NS_LOCAL_FILE_H_

#include "nscore.h"

#define NS_LOCAL_FILE_CID {0x2e23e220, 0x60be, 0x11d3, {0x8c, 0x4a, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74}}

#define NS_DECL_NSLOCALFILE_UNICODE_METHODS                                                      \
  nsresult AppendUnicode(const char16_t *aNode);                                              \
  nsresult GetUnicodeLeafName(char16_t **aLeafName);                                          \
  nsresult SetUnicodeLeafName(const char16_t *aLeafName);                                     \
  nsresult CopyToUnicode(nsIFile *aNewParentDir, const char16_t *aNewLeafName);               \
  nsresult CopyToFollowingLinksUnicode(nsIFile *aNewParentDir, const char16_t *aNewLeafName); \
  nsresult MoveToUnicode(nsIFile *aNewParentDir, const char16_t *aNewLeafName);               \
  nsresult GetUnicodeTarget(char16_t **aTarget);                                              \
  nsresult GetUnicodePath(char16_t **aPath);                                                  \
  nsresult InitWithUnicodePath(const char16_t *aPath);                                        \
  nsresult AppendRelativeUnicodePath(const char16_t *aRelativePath);





#include <errno.h>
#include "nsILocalFile.h"

#ifdef XP_WIN
#include "nsLocalFileWin.h"
#elif defined(XP_UNIX)
#include "nsLocalFileUnix.h"
#else
#error NOT_IMPLEMENTED
#endif

#define NSRESULT_FOR_RETURN(ret) (((ret) < 0) ? NSRESULT_FOR_ERRNO() : NS_OK)

inline nsresult
nsresultForErrno(int aErr)
{
  switch (aErr) {
    case 0:
      return NS_OK;
#ifdef EDQUOT
    case EDQUOT: 
      
#endif
    case ENOSPC:
      return NS_ERROR_FILE_DISK_FULL;
#ifdef EISDIR
    case EISDIR:    
      return NS_ERROR_FILE_IS_DIRECTORY;
#endif
    case ENAMETOOLONG:
      return NS_ERROR_FILE_NAME_TOO_LONG;
    case ENOEXEC:  
      return NS_ERROR_FILE_EXECUTION_FAILED;
    case ENOENT:
      return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
    case ENOTDIR:
      return NS_ERROR_FILE_DESTINATION_NOT_DIR;
#ifdef ELOOP
    case ELOOP:
      return NS_ERROR_FILE_UNRESOLVABLE_SYMLINK;
#endif 
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
#ifdef EROFS
    case EROFS: 
      return NS_ERROR_FILE_READ_ONLY;
#endif
    




#if ENOTEMPTY != EEXIST
    case ENOTEMPTY:
      return NS_ERROR_FILE_DIR_NOT_EMPTY;
#endif 
    







    case EFBIG: 
      return NS_ERROR_FILE_TOO_BIG;

    default:
      return NS_ERROR_FAILURE;
  }
}

#define NSRESULT_FOR_ERRNO() nsresultForErrno(errno)

void NS_StartupLocalFile();
void NS_ShutdownLocalFile();

#endif
