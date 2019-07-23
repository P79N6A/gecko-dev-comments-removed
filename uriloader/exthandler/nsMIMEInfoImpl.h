





































#ifndef __nsmimeinfoimpl_h___
#define __nsmimeinfoimpl_h___

#include "nsIMIMEInfo.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"





#define PROPERTY_DEFAULT_APP_ICON_URL "defaultApplicationIconURL"




#define PROPERTY_CUSTOM_APP_ICON_URL "customApplicationIconURL"






class nsMIMEInfoBase : public nsIMIMEInfo {
  public:
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD GetFileExtensions(nsIUTF8StringEnumerator **_retval);
    NS_IMETHOD SetFileExtensions(const nsACString & aExtensions);
    NS_IMETHOD ExtensionExists(const nsACString & aExtension, PRBool *_retval);
    NS_IMETHOD AppendExtension(const nsACString & aExtension);
    NS_IMETHOD GetPrimaryExtension(nsACString & aPrimaryExtension);
    NS_IMETHOD SetPrimaryExtension(const nsACString & aPrimaryExtension);
    NS_IMETHOD GetType(nsACString & aType);
    NS_IMETHOD GetMIMEType(nsACString & aMIMEType);
    NS_IMETHOD GetDescription(nsAString & aDescription);
    NS_IMETHOD SetDescription(const nsAString & aDescription);
    NS_IMETHOD GetMacType(PRUint32 *aMacType);
    NS_IMETHOD SetMacType(PRUint32 aMacType);
    NS_IMETHOD GetMacCreator(PRUint32 *aMacCreator);
    NS_IMETHOD SetMacCreator(PRUint32 aMacCreator);
    NS_IMETHOD Equals(nsIMIMEInfo *aMIMEInfo, PRBool *_retval);
    NS_IMETHOD GetPreferredApplicationHandler(nsIHandlerApp * *aPreferredApplicationHandler);
    NS_IMETHOD SetPreferredApplicationHandler(nsIHandlerApp * aPreferredApplicationHandler);
    NS_IMETHOD GetDefaultDescription(nsAString & aDefaultDescription);
    NS_IMETHOD LaunchWithURI(nsIURI *aURI);
    NS_IMETHOD GetPreferredAction(nsHandlerInfoAction *aPreferredAction);
    NS_IMETHOD SetPreferredAction(nsHandlerInfoAction aPreferredAction);
    NS_IMETHOD GetAlwaysAskBeforeHandling(PRBool *aAlwaysAskBeforeHandling);
    NS_IMETHOD SetAlwaysAskBeforeHandling(PRBool aAlwaysAskBeforeHandling); 

    enum HandlerClass {
      eMIMEInfo,
      eProtocolInfo
    };

    
    nsMIMEInfoBase(const char *aMIMEType = "") NS_HIDDEN;
    nsMIMEInfoBase(const nsACString& aMIMEType) NS_HIDDEN;
    nsMIMEInfoBase(const nsACString& aType, HandlerClass aClass) NS_HIDDEN;
    virtual ~nsMIMEInfoBase();        

    void SetMIMEType(const nsACString & aMIMEType) { mType = aMIMEType; }

    void SetDefaultDescription(const nsString& aDesc) { mDefaultAppDescription = aDesc; }

    







    void CopyBasicDataTo(nsMIMEInfoBase* aOther);

    


    PRBool HasExtensions() const { return mExtensions.Count() != 0; }

  protected:
    






    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile) = 0;

    




    virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI) = 0;

    








    static NS_HIDDEN_(nsresult) LaunchWithIProcess(nsIFile* aApp,
                                                   const nsCString &aArg);

    




    static NS_HIDDEN_(nsresult) LaunchWithWebHandler(nsIWebHandlerApp *aApp, 
                                                     nsIURI *aURI);

    





    static NS_HIDDEN_(nsresult) GetLocalFileFromURI(nsIURI *aURI,
                                                    nsILocalFile **aFile);

    
    nsCStringArray         mExtensions; 
    nsString               mDescription; 
    PRUint32               mMacType, mMacCreator; 
    nsCString              mType;
    HandlerClass           mClass;
    nsCOMPtr<nsIHandlerApp> mPreferredApplication;
    nsHandlerInfoAction    mPreferredAction; 
    nsString               mPreferredAppDescription;
    nsString               mDefaultAppDescription;
    PRBool                 mAlwaysAskBeforeHandling;
};










class nsMIMEInfoImpl : public nsMIMEInfoBase {
  public:
    nsMIMEInfoImpl(const char *aMIMEType = "") : nsMIMEInfoBase(aMIMEType) {}
    nsMIMEInfoImpl(const nsACString& aMIMEType) : nsMIMEInfoBase(aMIMEType) {}
    nsMIMEInfoImpl(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoBase(aType, aClass) {}
    virtual ~nsMIMEInfoImpl() {}

    
    NS_IMETHOD GetHasDefaultHandler(PRBool *_retval);
    NS_IMETHOD GetDefaultDescription(nsAString& aDefaultDescription);

    
    



    void SetDefaultApplication(nsIFile* aApp) { if (!mDefaultApplication) mDefaultApplication = aApp; }

  protected:
    
    



    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile);

    



    virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI) = 0;

    nsCOMPtr<nsIFile>      mDefaultApplication; 
};

#endif 
