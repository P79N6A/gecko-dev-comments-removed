




































#include "nsISuiteRemoteService.h"

#include "nsCOMPtr.h"
#include "nsString.h"

class nsIDOMWindowInternal;



#define NS_XREMOTESERVICE_CID \
  { 0x3dfe7324, 0x1dd2, 0x11b2, \
  { 0x9f, 0xf2, 0x88, 0x53, 0xf9, 0x1e, 0x8a, 0x20 } }

class XRemoteService : public nsISuiteRemoteService
{
 public:
  XRemoteService();
  virtual ~XRemoteService();

  NS_DEFINE_STATIC_CID_ACCESSOR(NS_XREMOTESERVICE_CID)

  NS_DECL_ISUPPORTS

  NS_DECL_NSISUITEREMOTESERVICE

 private:
  
  void FindLastInList(nsCString &aString, nsCString &retString,
                      PRUint32 *aIndexRet);

  
  void FindRestInList(nsCString &aString, nsCString &retString,
		      PRUint32 *aIndexRet);

  
  nsresult OpenChromeWindow(nsIDOMWindow *aParent,
			    const char *aUrl,
			    const char *aFeatures,
			    nsISupports *aArguments,
			    nsIDOMWindow **_retval);

  
  nsresult GetBrowserLocation(char **_retval);
  nsresult GetMailLocation(char **_retval);
  nsresult GetComposeLocation(const char **_retval);
  nsresult GetCalendarLocation(char **_retval);

  
  PRBool   MayOpenURL(const nsCString &aURL);

  
  nsresult OpenURL(nsCString &aArgument,
                   nsIDOMWindow* aParent,
                   PRBool aOpenBrowser);

  
  nsresult XfeDoCommand(nsCString &aArgument,
                        nsIDOMWindow* aParent);

  
  nsresult FindWindow(const PRUnichar *aType,
                      nsIDOMWindowInternal **_retval);
};
