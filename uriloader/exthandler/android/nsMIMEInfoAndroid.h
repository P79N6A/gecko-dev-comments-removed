




#ifndef nsMIMEInfoAndroid_h
#define nsMIMEInfoAndroid_h

#include "nsMIMEInfoImpl.h"
#include "nsIMutableArray.h"
#include "nsAndroidHandlerApp.h"

class nsMIMEInfoAndroid MOZ_FINAL : public nsIMIMEInfo
{
public:
  static bool
  GetMimeInfoForMimeType(const nsACString& aMimeType, 
                         nsMIMEInfoAndroid** aMimeInfo);
  static bool
  GetMimeInfoForFileExt(const nsACString& aFileExt, 
                        nsMIMEInfoAndroid** aMimeInfo);

  static nsresult 
  GetMimeInfoForURL(const nsACString &aURL, bool *found,
                    nsIHandlerInfo **info);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMIMEINFO
  NS_DECL_NSIHANDLERINFO

  nsMIMEInfoAndroid(const nsACString& aMIMEType);

protected:
  virtual nsresult LaunchDefaultWithFile(nsIFile* aFile);
  virtual nsresult LoadUriInternal(nsIURI *aURI);
  nsCOMPtr<nsIMutableArray> mHandlerApps;
  nsCString mType;
  nsTArray<nsCString> mExtensions;
  bool mAlwaysAsk;
  nsHandlerInfoAction mPrefAction;
  nsString mDescription;
  nsCOMPtr<nsIHandlerApp> mPrefApp;
  
  class SystemChooser MOZ_FINAL : public nsIHandlerApp {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHANDLERAPP
    SystemChooser(nsMIMEInfoAndroid* aOuter): mOuter(aOuter) {};
    
  private:
    nsMIMEInfoAndroid* mOuter;
    
  };
};

#endif 
