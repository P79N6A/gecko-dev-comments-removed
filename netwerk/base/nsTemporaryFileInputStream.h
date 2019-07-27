




#ifndef nsTemporaryFileInputStream_h__
#define nsTemporaryFileInputStream_h__

#include "mozilla/Mutex.h"
#include "nsIInputStream.h"
#include "nsAutoPtr.h"
#include "prio.h"

class nsTemporaryFileInputStream : public nsIInputStream
{
public:
  
  class FileDescOwner
  {
    friend class nsTemporaryFileInputStream;
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FileDescOwner)
    explicit FileDescOwner(PRFileDesc* aFD)
      : mFD(aFD),
        mMutex("FileDescOwner::mMutex")
    {
      MOZ_ASSERT(aFD);
    }
  private:
    ~FileDescOwner()
    {
      PR_Close(mFD);
    }
  public:
    mozilla::Mutex& FileMutex() { return mMutex; }

  private:
    PRFileDesc* mFD;
    mozilla::Mutex mMutex;
  };

  nsTemporaryFileInputStream(FileDescOwner* aFileDescOwner, uint64_t aStartPos, uint64_t aEndPos);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM

private:
  virtual ~nsTemporaryFileInputStream() { }

  nsRefPtr<FileDescOwner> mFileDescOwner;
  uint64_t mStartPos;
  uint64_t mEndPos;
  bool mClosed;
};

#endif 
