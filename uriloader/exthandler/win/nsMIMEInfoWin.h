



































#ifndef nsMIMEInfoWin_h_
#define nsMIMEInfoWin_h_

#include "nsMIMEInfoImpl.h"
#include "nsIPropertyBag.h"
#include "nsIMutableArray.h"
#include "nsTArray.h"

class nsMIMEInfoWin : public nsMIMEInfoBase, public nsIPropertyBag {
  public:
    nsMIMEInfoWin(const char* aType = "") : nsMIMEInfoBase(aType) {}
    nsMIMEInfoWin(const nsACString& aMIMEType) : nsMIMEInfoBase(aMIMEType) {}
    nsMIMEInfoWin(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoBase(aType, aClass) {}
    virtual ~nsMIMEInfoWin();

    NS_IMETHOD LaunchWithFile(nsIFile* aFile);
    NS_IMETHOD GetHasDefaultHandler(bool * _retval);
    NS_IMETHOD GetPossibleLocalHandlers(nsIArray **_retval); 

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIPROPERTYBAG

    void SetDefaultApplicationHandler(nsIFile* aDefaultApplication) 
    { 
      mDefaultApplication = aDefaultApplication; 
    }

  protected:
    virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI);
    virtual nsresult LaunchDefaultWithFile(nsIFile* aFile);

  private:
    nsCOMPtr<nsIFile>      mDefaultApplication;
    
    
    
    bool GetLocalHandlerApp(const nsAString& aCommandHandler,
                              nsCOMPtr<nsILocalHandlerApp>& aApp);

    
    
    bool GetAppsVerbCommandHandler(const nsAString& appExeName,
                                     nsAString& applicationPath,
                                     bool bEdit);

    
    
    bool GetProgIDVerbCommandHandler(const nsAString& appProgIDName,
                                       nsAString& applicationPath,
                                       bool bEdit);

    
    
    bool GetDllLaunchInfo(nsIFile * aDll,
                            nsILocalFile * aFile,
                            nsAString& args, bool bEdit);

    
    void ProcessPath(nsCOMPtr<nsIMutableArray>& appList,
                     nsTArray<nsString>& trackList,
                     const nsAString& appFilesystemCommand);

};

#endif
