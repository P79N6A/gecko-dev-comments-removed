




































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
    NS_IMETHOD GetMIMEType(nsACString & aMIMEType);
    NS_IMETHOD GetDescription(nsAString & aDescription);
    NS_IMETHOD SetDescription(const nsAString & aDescription);
    NS_IMETHOD GetMacType(PRUint32 *aMacType);
    NS_IMETHOD SetMacType(PRUint32 aMacType);
    NS_IMETHOD GetMacCreator(PRUint32 *aMacCreator);
    NS_IMETHOD SetMacCreator(PRUint32 aMacCreator);
    NS_IMETHOD Equals(nsIMIMEInfo *aMIMEInfo, PRBool *_retval);
    NS_IMETHOD GetPreferredApplicationHandler(nsIFile * *aPreferredApplicationHandler);
    NS_IMETHOD SetPreferredApplicationHandler(nsIFile * aPreferredApplicationHandler);
    NS_IMETHOD GetApplicationDescription(nsAString & aApplicationDescription);
    NS_IMETHOD SetApplicationDescription(const nsAString & aApplicationDescription);
    NS_IMETHOD GetDefaultDescription(nsAString & aDefaultDescription);
    NS_IMETHOD LaunchWithFile(nsIFile *aFile);
    NS_IMETHOD GetPreferredAction(nsMIMEInfoHandleAction *aPreferredAction);
    NS_IMETHOD SetPreferredAction(nsMIMEInfoHandleAction aPreferredAction);
    NS_IMETHOD GetAlwaysAskBeforeHandling(PRBool *aAlwaysAskBeforeHandling);
    NS_IMETHOD SetAlwaysAskBeforeHandling(PRBool aAlwaysAskBeforeHandling); 

    
    nsMIMEInfoBase(const char *aMIMEType = "") NS_HIDDEN;
    nsMIMEInfoBase(const nsACString& aMIMEType) NS_HIDDEN;
    virtual ~nsMIMEInfoBase();        

    void SetMIMEType(const nsACString & aMIMEType) { mMIMEType = aMIMEType; }

    void SetDefaultDescription(const nsString& aDesc) { mDefaultAppDescription = aDesc; }

    







    void CopyBasicDataTo(nsMIMEInfoBase* aOther);

    


    PRBool HasExtensions() const { return mExtensions.Count() != 0; }

  protected:
    






    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile) = 0;

    









    static NS_HIDDEN_(nsresult) LaunchWithIProcess(nsIFile* aApp, nsIFile* aFile);

    
    nsCStringArray         mExtensions; 
    nsString               mDescription; 
    PRUint32               mMacType, mMacCreator; 
    nsCString              mMIMEType;
    nsCOMPtr<nsIFile>      mPreferredApplication; 
    nsMIMEInfoHandleAction mPreferredAction; 
    nsString               mPreferredAppDescription;
    nsString               mDefaultAppDescription;
    PRBool                 mAlwaysAskBeforeHandling;
};









class nsMIMEInfoImpl : public nsMIMEInfoBase {
  public:
    nsMIMEInfoImpl(const char *aMIMEType = "") : nsMIMEInfoBase(aMIMEType) {}
    nsMIMEInfoImpl(const nsACString& aMIMEType) : nsMIMEInfoBase(aMIMEType) {}
    virtual ~nsMIMEInfoImpl() {}

    
    NS_IMETHOD GetHasDefaultHandler(PRBool *_retval);
    NS_IMETHOD GetDefaultDescription(nsAString& aDefaultDescription);

    
    



    void SetDefaultApplication(nsIFile* aApp) { if (!mDefaultApplication) mDefaultApplication = aApp; }
  protected:
    
    



    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile);


    nsCOMPtr<nsIFile>      mDefaultApplication; 
};

#endif 
