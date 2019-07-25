




































#if defined(XP_UNIX)
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#elif defined(XP_WIN)
#include <windows.h>
#endif

#include "nscore.h"
#include "private/pprio.h"
#include "mozilla/FileUtils.h"

bool 
mozilla::fallocate(PRFileDesc *aFD, PRInt64 aLength) 
{
#if defined(HAVE_POSIX_FALLOCATE)
  return posix_fallocate(PR_FileDesc2NativeHandle(aFD), 0, aLength) == 0;
#elif defined(XP_WIN)
  return PR_Seek64(aFD, aLength, PR_SEEK_SET) == aLength
    && 0 != SetEndOfFile((HANDLE)PR_FileDesc2NativeHandle(aFD));
#elif defined(XP_MACOSX)
  int fd = PR_FileDesc2NativeHandle(aFD);
  fstore_t store = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, aLength};
  
  int ret = fcntl(fd, F_PREALLOCATE, &store);
	if(-1 == ret){
    
    store.fst_flags = F_ALLOCATEALL;
    ret = fcntl(fd, F_PREALLOCATE, &store);
    if (-1 == ret)
      return false;
  }
  return 0 == ftruncate(fd, aLength);
#elif defined(XP_UNIX)
  
  





  struct stat buf;
  int fd = PR_FileDesc2NativeHandle(aFD);
  if (fstat(fd, &buf))
    return false;

  if (buf.st_size >= aLength)
    return false;

  const int nBlk = buf.st_blksize;

  if (!nBlk)
    return false;

  if (ftruncate(fd, aLength))
    return false;
  
  int nWrite; 
  PRInt64 iWrite = ((buf.st_size + 2 * nBlk - 1) / nBlk) * nBlk - 1; 
  do {
    nWrite = 0;
    if (PR_Seek64(aFD, iWrite, PR_SEEK_SET) == iWrite)
      nWrite = PR_Write(aFD, "", 1);
    iWrite += nBlk;
  } while (nWrite == 1 && iWrite < aLength);
  return nWrite == 1;
#endif
  return false;
}
