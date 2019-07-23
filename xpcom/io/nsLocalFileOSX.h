





































#ifndef nsLocalFileMac_h_
#define nsLocalFileMac_h_

#include "nsILocalFileMac.h"
#include "nsString.h"
#include "nsIHashable.h"

class nsDirEnumerator;


#if defined(HAVE_STAT64) && defined(HAVE_LSTAT64) && (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_4)
#define STAT stat64
#define LSTAT lstat64
#else
#define STAT stat
#define LSTAT lstat
#endif


#if defined(HAVE_STATVFS64) && (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_4)
#define STATVFS statvfs64
#else
#define STATVFS statvfs
#endif








class NS_COM nsLocalFile : public nsILocalFileMac,
                           public nsIHashable
{
  friend class nsDirEnumerator;
    
public:
  NS_DEFINE_STATIC_CID_ACCESSOR(NS_LOCAL_FILE_CID)

  nsLocalFile();

  static NS_METHOD nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFILE
  NS_DECL_NSILOCALFILE
  NS_DECL_NSILOCALFILEMAC
  NS_DECL_NSIHASHABLE

public:
  static void GlobalInit();
  static void GlobalShutdown();

private:
  ~nsLocalFile();

protected:
  nsLocalFile(const nsLocalFile& src);
  nsresult SetBaseURL(CFURLRef aCFURLRef); 
  nsresult GetFSRefInternal(FSRef& aFSRef);
  nsresult GetPathInternal(nsACString& path); 
  nsresult CopyInternal(nsIFile* newParentDir,
                        const nsAString& newName,
                        PRBool followLinks);
  nsresult FillStatBufferInternal(struct STAT *statBuffer);

protected:
  CFURLRef     mBaseURL; 
  char         mPath[PATH_MAX]; 
  PRPackedBool mFollowLinks;
};

#endif 
