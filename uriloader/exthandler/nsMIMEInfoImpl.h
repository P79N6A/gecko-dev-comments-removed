




#ifndef __nsmimeinfoimpl_h___
#define __nsmimeinfoimpl_h___

#include "nsIMIMEInfo.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsIMutableArray.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIProcess.h"





#define PROPERTY_DEFAULT_APP_ICON_URL "defaultApplicationIconURL"




#define PROPERTY_CUSTOM_APP_ICON_URL "customApplicationIconURL"






class nsMIMEInfoBase : public nsIMIMEInfo {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS

    
    NS_IMETHOD GetFileExtensions(nsIUTF8StringEnumerator **_retval) MOZ_OVERRIDE;
    NS_IMETHOD SetFileExtensions(const nsACString & aExtensions) MOZ_OVERRIDE;
    NS_IMETHOD ExtensionExists(const nsACString & aExtension, bool *_retval) MOZ_OVERRIDE;
    NS_IMETHOD AppendExtension(const nsACString & aExtension) MOZ_OVERRIDE;
    NS_IMETHOD GetPrimaryExtension(nsACString & aPrimaryExtension) MOZ_OVERRIDE;
    NS_IMETHOD SetPrimaryExtension(const nsACString & aPrimaryExtension) MOZ_OVERRIDE;
    NS_IMETHOD GetType(nsACString & aType) MOZ_OVERRIDE;
    NS_IMETHOD GetMIMEType(nsACString & aMIMEType) MOZ_OVERRIDE;
    NS_IMETHOD GetDescription(nsAString & aDescription) MOZ_OVERRIDE;
    NS_IMETHOD SetDescription(const nsAString & aDescription) MOZ_OVERRIDE;
    NS_IMETHOD Equals(nsIMIMEInfo *aMIMEInfo, bool *_retval) MOZ_OVERRIDE;
    NS_IMETHOD GetPreferredApplicationHandler(nsIHandlerApp * *aPreferredAppHandler) MOZ_OVERRIDE;
    NS_IMETHOD SetPreferredApplicationHandler(nsIHandlerApp * aPreferredAppHandler) MOZ_OVERRIDE;
    NS_IMETHOD GetPossibleApplicationHandlers(nsIMutableArray * *aPossibleAppHandlers) MOZ_OVERRIDE;
    NS_IMETHOD GetDefaultDescription(nsAString & aDefaultDescription) MOZ_OVERRIDE;
    NS_IMETHOD LaunchWithFile(nsIFile *aFile) MOZ_OVERRIDE;
    NS_IMETHOD LaunchWithURI(nsIURI *aURI,
                             nsIInterfaceRequestor *aWindowContext) MOZ_OVERRIDE;
    NS_IMETHOD GetPreferredAction(nsHandlerInfoAction *aPreferredAction) MOZ_OVERRIDE;
    NS_IMETHOD SetPreferredAction(nsHandlerInfoAction aPreferredAction) MOZ_OVERRIDE;
    NS_IMETHOD GetAlwaysAskBeforeHandling(bool *aAlwaysAskBeforeHandling) MOZ_OVERRIDE;
    NS_IMETHOD SetAlwaysAskBeforeHandling(bool aAlwaysAskBeforeHandling) MOZ_OVERRIDE; 
    NS_IMETHOD GetPossibleLocalHandlers(nsIArray **_retval) MOZ_OVERRIDE; 

    enum HandlerClass {
      eMIMEInfo,
      eProtocolInfo
    };

    
    explicit nsMIMEInfoBase(const char *aMIMEType = "");
    explicit nsMIMEInfoBase(const nsACString& aMIMEType);
    nsMIMEInfoBase(const nsACString& aType, HandlerClass aClass);

    void SetMIMEType(const nsACString & aMIMEType) { mSchemeOrType = aMIMEType; }

    void SetDefaultDescription(const nsString& aDesc) { mDefaultAppDescription = aDesc; }

    







    void CopyBasicDataTo(nsMIMEInfoBase* aOther);

    


    bool HasExtensions() const { return mExtensions.Length() != 0; }

  protected:
    virtual ~nsMIMEInfoBase();        

    






    virtual nsresult LaunchDefaultWithFile(nsIFile* aFile) = 0;

    




    virtual nsresult LoadUriInternal(nsIURI *aURI) = 0;

    static already_AddRefed<nsIProcess> InitProcess(nsIFile* aApp,
                                                    nsresult* aResult);

    








    static nsresult LaunchWithIProcess(nsIFile* aApp,
                                                   const nsCString &aArg);
    static nsresult LaunchWithIProcess(nsIFile* aApp,
                                                   const nsString &aArg);

    





    static nsresult GetLocalFileFromURI(nsIURI *aURI,
                                                    nsIFile **aFile);

    
    nsTArray<nsCString>    mExtensions; 
    nsString               mDescription; 
    nsCString              mSchemeOrType;
    HandlerClass           mClass;
    nsCOMPtr<nsIHandlerApp> mPreferredApplication;
    nsCOMPtr<nsIMutableArray> mPossibleApplications;
    nsHandlerInfoAction    mPreferredAction; 
    nsString               mPreferredAppDescription;
    nsString               mDefaultAppDescription;
    bool                   mAlwaysAskBeforeHandling;
};










class nsMIMEInfoImpl : public nsMIMEInfoBase {
  public:
    explicit nsMIMEInfoImpl(const char *aMIMEType = "") : nsMIMEInfoBase(aMIMEType) {}
    explicit nsMIMEInfoImpl(const nsACString& aMIMEType) : nsMIMEInfoBase(aMIMEType) {}
    nsMIMEInfoImpl(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoBase(aType, aClass) {}
    virtual ~nsMIMEInfoImpl() {}

    
    NS_IMETHOD GetHasDefaultHandler(bool *_retval);
    NS_IMETHOD GetDefaultDescription(nsAString& aDefaultDescription);

    
    



    void SetDefaultApplication(nsIFile* aApp) { if (!mDefaultApplication) mDefaultApplication = aApp; }

  protected:
    
    



    virtual nsresult LaunchDefaultWithFile(nsIFile* aFile);

    



    virtual nsresult LoadUriInternal(nsIURI *aURI) = 0;

    nsCOMPtr<nsIFile>      mDefaultApplication; 
};

#endif 
