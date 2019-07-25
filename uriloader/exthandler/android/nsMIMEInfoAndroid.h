




































#ifndef nsMIMEInfoAndroid_h
#define nsMIMEInfoAndroid_h

#include "nsMIMEInfoImpl.h"
#include "nsIMutableArray.h"
#include "nsAndroidHandlerApp.h"
class nsMIMEInfoAndroid : public nsIMIMEInfo
{
public:
  static already_AddRefed<nsIMIMEInfo> GetMimeInfoForMimeType(const nsACString& aMimeType);
  static already_AddRefed<nsIMIMEInfo> GetMimeInfoForFileExt(const nsACString& aFileExt);
  static nsresult GetMimeInfoForProtocol(const nsACString &aScheme,
                                         PRBool *found,
                                         nsIHandlerInfo **info);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMIMEINFO
  NS_DECL_NSIHANDLERINFO
private:
  nsMIMEInfoAndroid(const nsACString& aMIMEType);
  virtual ~nsMIMEInfoAndroid();

protected:
  virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile);
  virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI);
  nsCOMPtr<nsIMutableArray> mHandlerApps;
  nsCString mMimeType;
  nsTArray<nsCString> mExtensions;
  PRBool mAlwaysAsk;
  nsHandlerInfoAction mPrefAction;
  nsString mDescription;
  nsCOMPtr<nsIHandlerApp> mPrefApp;
  
  class SystemChooser : public nsIHandlerApp {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHANDLERAPP
    SystemChooser(nsMIMEInfoAndroid* aOuter): mOuter(aOuter) {};
    
  private:
    nsMIMEInfoAndroid* mOuter;
    
  };
  SystemChooser mSystemChooser;
};

#endif 
