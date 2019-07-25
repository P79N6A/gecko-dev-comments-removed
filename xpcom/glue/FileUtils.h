






































#ifndef mozilla_FileUtils_h
#define mozilla_FileUtils_h

#if defined(XP_UNIX)
# include <unistd.h>
#elif defined(XP_WIN)
# include <io.h>
#endif
#include "prio.h"

namespace mozilla {




class AutoFDClose
{
public:
  AutoFDClose(PRFileDesc* fd = nsnull) : mFD(fd) { }
  ~AutoFDClose() { if (mFD) PR_Close(mFD); }

  PRFileDesc* operator= (PRFileDesc *fd) {
    if (mFD) PR_Close(mFD);
    mFD = fd;
    return fd;
  }

  operator PRFileDesc* () { return mFD; }
  PRFileDesc** operator &() { *this = nsnull; return &mFD; }

private:
  PRFileDesc *mFD;
};




struct ScopedClose
{
  ScopedClose(int aFd=-1) : mFd(aFd) {}
  ~ScopedClose() {
    if (0 <= mFd) {
      close(mFd);
    }
  }
  int mFd;
};










NS_COM_GLUE bool fallocate(PRFileDesc *aFD, PRInt64 aLength);

} 
#endif
