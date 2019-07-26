



#ifndef CacheEntriesEnumerator__h__
#define CacheEntriesEnumerator__h__

#include "nsCOMPtr.h"

class nsIFile;
class nsIDirectoryEnumerator;
class nsIThread;

namespace mozilla {
namespace net {

class CacheFileIOManager;
class CacheFileListener;
class CacheFile;

class CacheEntriesEnumeratorCallback : public nsISupports
{
public:
  virtual void OnFile(CacheFile* aFile) = 0;
};

class CacheEntriesEnumerator
{
public:
  ~CacheEntriesEnumerator();

  bool HasMore();
  nsresult GetNextFile(nsIFile** aFile);

protected:
  friend class CacheFileIOManager;
  CacheEntriesEnumerator(nsIFile* aEntriesDirectory);
  nsresult Init();

private:
  nsCOMPtr<nsIDirectoryEnumerator> mEnumerator;
  nsCOMPtr<nsIFile> mEntriesDirectory;
  nsCOMPtr<nsIFile> mCurrentFile;

#ifdef DEBUG
  nsCOMPtr<nsIThread> mThreadCheck;
#endif
};

} 
} 

#endif
