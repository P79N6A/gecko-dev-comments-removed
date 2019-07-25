






































#ifndef mozilla_FileUtils_h
#define mozilla_FileUtils_h
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

} 
#endif
