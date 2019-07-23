





































#include "nsIStringEnumerator.h"
#include "nsVoidArray.h"
#include "nsISimpleEnumerator.h"
#include "nsCOMPtr.h"




































NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsCStringArray* aArray);

NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsStringArray* aArray);










NS_COM nsresult
NS_NewAdoptingStringEnumerator(nsIStringEnumerator** aResult,
                               nsStringArray* aArray);

NS_COM nsresult
NS_NewAdoptingUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                                   nsCStringArray* aArray);














NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsStringArray* aArray,
                       nsISupports* aOwner);
NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsCStringArray* aArray,
                           nsISupports* aOwner);

class nsStringEnumerator : public nsIStringEnumerator,
                           public nsIUTF8StringEnumerator,
                           public nsISimpleEnumerator
{
public:
    nsStringEnumerator(const nsStringArray* aArray, PRBool aOwnsArray) :
        mArray(aArray), mIndex(0), mOwnsArray(aOwnsArray), mIsUnicode(PR_TRUE)
    {}
    
    nsStringEnumerator(const nsCStringArray* aArray, PRBool aOwnsArray) :
        mCArray(aArray), mIndex(0), mOwnsArray(aOwnsArray), mIsUnicode(PR_FALSE)
    {}

    nsStringEnumerator(const nsStringArray* aArray, nsISupports* aOwner) :
        mArray(aArray), mIndex(0), mOwner(aOwner), mOwnsArray(PR_FALSE), mIsUnicode(PR_TRUE)
    {}
    
    nsStringEnumerator(const nsCStringArray* aArray, nsISupports* aOwner) :
        mCArray(aArray), mIndex(0), mOwner(aOwner), mOwnsArray(PR_FALSE), mIsUnicode(PR_FALSE)
    {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIUTF8STRINGENUMERATOR

    
    
    NS_IMETHOD GetNext(nsAString& aResult);
    NS_DECL_NSISIMPLEENUMERATOR

private:
    ~nsStringEnumerator() {
        if (mOwnsArray) {
            
            
            
            if (mIsUnicode)
                delete const_cast<nsStringArray*>(mArray);
            else
                delete const_cast<nsCStringArray*>(mCArray);
        }
    }

    union {
        const nsStringArray* mArray;
        const nsCStringArray* mCArray;
    };

    inline PRUint32 Count() {
        return mIsUnicode ? mArray->Count() : mCArray->Count();
    }
    
    PRUint32 mIndex;

    
    
    
    
    nsCOMPtr<nsISupports> mOwner;
    PRPackedBool mOwnsArray;
    PRPackedBool mIsUnicode;
};
