






































#include "nsStringEnumerator.h"
#include "prtypes.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsISimpleEnumerator.h"
#include "nsSupportsPrimitives.h"





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

NS_IMPL_ISUPPORTS3(nsStringEnumerator,
                   nsIStringEnumerator,
                   nsIUTF8StringEnumerator,
                   nsISimpleEnumerator)

NS_IMETHODIMP
nsStringEnumerator::HasMore(PRBool* aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = mIndex < Count();
    return NS_OK;
}

NS_IMETHODIMP
nsStringEnumerator::HasMoreElements(PRBool* aResult)
{
    return HasMore(aResult);
}

NS_IMETHODIMP
nsStringEnumerator::GetNext(nsISupports** aResult)
{
    if (mIsUnicode) {
        nsSupportsStringImpl* stringImpl = new nsSupportsStringImpl();
        if (!stringImpl) return NS_ERROR_OUT_OF_MEMORY;
        
        stringImpl->SetData(*mArray->StringAt(mIndex++));
        *aResult = stringImpl;
    }
    else {
        nsSupportsCStringImpl* cstringImpl = new nsSupportsCStringImpl();
        if (!cstringImpl) return NS_ERROR_OUT_OF_MEMORY;

        cstringImpl->SetData(*mCArray->CStringAt(mIndex++));
        *aResult = cstringImpl;
    }
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsStringEnumerator::GetNext(nsAString& aResult)
{
    NS_ENSURE_TRUE(mIndex < Count(), NS_ERROR_UNEXPECTED);

    if (mIsUnicode)
        aResult = *mArray->StringAt(mIndex++);
    else
        CopyUTF8toUTF16(*mCArray->CStringAt(mIndex++), aResult);
    
    return NS_OK;
}

NS_IMETHODIMP
nsStringEnumerator::GetNext(nsACString& aResult)
{
    NS_ENSURE_TRUE(mIndex < Count(), NS_ERROR_UNEXPECTED);
    
    if (mIsUnicode)
        CopyUTF16toUTF8(*mArray->StringAt(mIndex++), aResult);
    else
        aResult = *mCArray->CStringAt(mIndex++);
    
    return NS_OK;
}

template<class T>
static inline nsresult
StringEnumeratorTail(T** aResult)
{
    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aResult);
    return NS_OK;
}





NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsStringArray* aArray, nsISupports* aOwner)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, aOwner);
    return StringEnumeratorTail(aResult);
}


NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsCStringArray* aArray, nsISupports* aOwner)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, aOwner);
    return StringEnumeratorTail(aResult);
}

NS_COM nsresult
NS_NewAdoptingStringEnumerator(nsIStringEnumerator** aResult,
                               nsStringArray* aArray)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, PR_TRUE);
    return StringEnumeratorTail(aResult);
}

NS_COM nsresult
NS_NewAdoptingUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                                   nsCStringArray* aArray)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, PR_TRUE);
    return StringEnumeratorTail(aResult);
}


NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsStringArray* aArray)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, PR_FALSE);
    return StringEnumeratorTail(aResult);
}

NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsCStringArray* aArray)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, PR_FALSE);
    return StringEnumeratorTail(aResult);
}

