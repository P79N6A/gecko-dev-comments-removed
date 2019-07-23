




































#ifndef NSAUTHINFORMATIONHOLDER_H_
#define NSAUTHINFORMATIONHOLDER_H_

#include "nsIAuthInformation.h"
#include "nsString.h"

class nsAuthInformationHolder : public nsIAuthInformation {
public:
    
    nsAuthInformationHolder(PRUint32 aFlags, const nsString& aRealm,
                            const nsCString& aAuthType)
        : mFlags(aFlags), mRealm(aRealm), mAuthType(aAuthType) {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHINFORMATION

    const nsString& User() const { return mUser; }
    const nsString& Password() const { return mPassword; }
    const nsString& Domain() const { return mDomain; }

    



    void SetUserInternal(const nsString& aUsername) {
      mUser = aUsername;
    }
private:
    nsString mUser;
    nsString mPassword;
    nsString mDomain;

    PRUint32 mFlags;
    nsString mRealm;
    nsCString mAuthType;
};


#endif
