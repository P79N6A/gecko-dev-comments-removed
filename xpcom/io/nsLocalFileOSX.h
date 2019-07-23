





































#ifndef nsLocalFileMac_h_
#define nsLocalFileMac_h_

#include "nsILocalFileMac.h"
#include "nsString.h"
#include "nsIHashable.h"
#include "nsIClassInfoImpl.h"

class nsDirEnumerator;








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

  nsresult SetBaseRef(CFURLRef aCFURLRef); 
  nsresult UpdateTargetRef();

  nsresult GetFSRefInternal(FSRef& aFSRef);
  nsresult GetPathInternal(nsACString& path); 
  nsresult EqualsInternal(nsISupports* inFile, PRBool *_retval);

  nsresult CopyInternal(nsIFile* newParentDir,
                        const nsAString& newName,
                        PRBool followLinks);

  static PRInt64  HFSPlustoNSPRTime(const UTCDateTime& utcTime);
  static void     NSPRtoHFSPlusTime(PRInt64 nsprTime, UTCDateTime& utcTime);
  static nsresult CFStringReftoUTF8(CFStringRef aInStrRef, nsACString& aOutStr);

protected:
  CFURLRef mBaseURL;   
  CFURLRef mTargetURL; 

  PRPackedBool mFollowLinks;
  PRPackedBool mFollowLinksDirty;
};

#endif 
