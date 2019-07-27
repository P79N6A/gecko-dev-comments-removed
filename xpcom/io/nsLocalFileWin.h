





#ifndef _nsLocalFileWIN_H_
#define _nsLocalFileWIN_H_

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsIFile.h"
#include "nsIFactory.h"
#include "nsILocalFileWin.h"
#include "nsIHashable.h"
#include "nsIClassInfoImpl.h"
#include "prio.h"

#include "mozilla/Attributes.h"

#include "windows.h"
#include "shlobj.h"

#include <sys/stat.h>

class nsLocalFile MOZ_FINAL
  : public nsILocalFileWin
  , public nsIHashable
{
public:
  NS_DEFINE_STATIC_CID_ACCESSOR(NS_LOCAL_FILE_CID)

  nsLocalFile();

  static nsresult nsLocalFileConstructor(nsISupports* aOuter,
                                         const nsIID& aIID,
                                         void** aInstancePtr);

  
  NS_DECL_THREADSAFE_ISUPPORTS

  
  NS_DECL_NSIFILE

  
  NS_DECL_NSILOCALFILE

  
  NS_DECL_NSILOCALFILEWIN

  
  NS_DECL_NSIHASHABLE

public:
  static void GlobalInit();
  static void GlobalShutdown();

private:
  
  enum CopyFileOption {
    FollowSymlinks          = 1u << 0,
    Move                    = 1u << 1,
    SkipNtfsAclReset        = 1u << 2,
    Rename                  = 1u << 3
  };

  nsLocalFile(const nsLocalFile& aOther);
  ~nsLocalFile()
  {
  }

  bool mDirty;            
  bool mResolveDirty;
  bool mFollowSymlinks;   

  
  nsString mWorkingPath;

  
  
  nsString mResolvedPath;

  
  
  nsString mShortWorkingPath;

  PRFileInfo64 mFileInfo64;

  void MakeDirty()
  {
    mDirty = true;
    mResolveDirty = true;
    mShortWorkingPath.Truncate();
  }

  nsresult ResolveAndStat();
  nsresult Resolve();
  nsresult ResolveShortcut();

  void EnsureShortPath();

  nsresult CopyMove(nsIFile* aNewParentDir, const nsAString& aNewName,
                    uint32_t aOptions);
  nsresult CopySingleFile(nsIFile* aSource, nsIFile* aDest,
                          const nsAString& aNewName, uint32_t aOptions);

  nsresult SetModDate(int64_t aLastModifiedTime, const wchar_t* aFilePath);
  nsresult HasFileAttribute(DWORD aFileAttrib, bool* aResult);
  nsresult AppendInternal(const nsAFlatString& aNode,
                          bool aMultipleComponents);

  nsresult OpenNSPRFileDescMaybeShareDelete(int32_t aFlags,
                                            int32_t aMode,
                                            bool aShareDelete,
                                            PRFileDesc** aResult);
};

#endif
