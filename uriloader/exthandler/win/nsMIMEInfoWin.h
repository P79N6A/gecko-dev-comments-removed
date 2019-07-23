



































#ifndef nsMIMEInfoWin_h_
#define nsMIMEInfoWin_h_

#include "nsMIMEInfoImpl.h"
#include "nsIPropertyBag.h"

class nsMIMEInfoWin : public nsMIMEInfoBase, public nsIPropertyBag {
  public:
    nsMIMEInfoWin(const char* aType = "") : nsMIMEInfoBase(aType) {}
    nsMIMEInfoWin(const nsACString& aMIMEType) : nsMIMEInfoBase(aMIMEType) {}
    virtual ~nsMIMEInfoWin();

    NS_IMETHOD GetHasDefaultHandler(PRBool * _retval);

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIPROPERTYBAG

    void SetDefaultApplicationHandler(nsIFile* aDefaultApplication) 
    { 
      mDefaultApplication = aDefaultApplication; 
    }
  protected:
    virtual nsresult LaunchDefaultWithFile(nsIFile* aFile);
  
  private:
    nsCOMPtr<nsIFile>      mDefaultApplication;
};

#endif
