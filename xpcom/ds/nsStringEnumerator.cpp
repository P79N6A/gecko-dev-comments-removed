






































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
    nsStringEnumerator(const nsTArray<nsString>* aArray, bool aOwnsArray) :
        mArray(aArray), mIndex(0), mOwnsArray(aOwnsArray), mIsUnicode(true)
    {}
    
    nsStringEnumerator(const nsTArray<nsCString>* aArray, bool aOwnsArray) :
        mCArray(aArray), mIndex(0), mOwnsArray(aOwnsArray), mIsUnicode(false)
    {}

    nsStringEnumerator(const nsTArray<nsString>* aArray, nsISupports* aOwner) :
        mArray(aArray), mIndex(0), mOwner(aOwner), mOwnsArray(false), mIsUnicode(true)
    {}
    
    nsStringEnumerator(const nsTArray<nsCString>* aArray, nsISupports* aOwner) :
        mCArray(aArray), mIndex(0), mOwner(aOwner), mOwnsArray(false), mIsUnicode(false)
    {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIUTF8STRINGENUMERATOR

    
    
    NS_IMETHOD GetNext(nsAString& aResult);
    NS_DECL_NSISIMPLEENUMERATOR

private:
    ~nsStringEnumerator() {
        if (mOwnsArray) {
            
            
            
            if (mIsUnicode)
                delete const_cast<nsTArray<nsString>*>(mArray);
            else
                delete const_cast<nsTArray<nsCString>*>(mCArray);
        }
    }

    union {
        const nsTArray<nsString>* mArray;
        const nsTArray<nsCString>* mCArray;
    };

    inline PRUint32 Count() {
        return mIsUnicode ? mArray->Length() : mCArray->Length();
    }
    
    PRUint32 mIndex;

    
    
    
    
    nsCOMPtr<nsISupports> mOwner;
    bool mOwnsArray;
    bool mIsUnicode;
};

NS_IMPL_ISUPPORTS3(nsStringEnumerator,
                   nsIStringEnumerator,
                   nsIUTF8StringEnumerator,
                   nsISimpleEnumerator)

NS_IMETHODIMP
nsStringEnumerator::HasMore(bool* aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = mIndex < Count();
    return NS_OK;
}

NS_IMETHODIMP
nsStringEnumerator::HasMoreElements(bool* aResult)
{
    return HasMore(aResult);
}

NS_IMETHODIMP
nsStringEnumerator::GetNext(nsISupports** aResult)
{
    if (mIsUnicode) {
        nsSupportsStringImpl* stringImpl = new nsSupportsStringImpl();
        if (!stringImpl) return NS_ERROR_OUT_OF_MEMORY;
        
        stringImpl->SetData(mArray->ElementAt(mIndex++));
        *aResult = stringImpl;
    }
    else {
        nsSupportsCStringImpl* cstringImpl = new nsSupportsCStringImpl();
        if (!cstringImpl) return NS_ERROR_OUT_OF_MEMORY;

        cstringImpl->SetData(mCArray->ElementAt(mIndex++));
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
        aResult = mArray->ElementAt(mIndex++);
    else
        CopyUTF8toUTF16(mCArray->ElementAt(mIndex++), aResult);
    
    return NS_OK;
}

NS_IMETHODIMP
nsStringEnumerator::GetNext(nsACString& aResult)
{
    NS_ENSURE_TRUE(mIndex < Count(), NS_ERROR_UNEXPECTED);
    
    if (mIsUnicode)
        CopyUTF16toUTF8(mArray->ElementAt(mIndex++), aResult);
    else
        aResult = mCArray->ElementAt(mIndex++);
    
    return NS_OK;
}

template<class T>
static inline nsresult
StringEnumeratorTail(T** aResult NS_INPARAM)
{
    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aResult);
    return NS_OK;
}





nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsTArray<nsString>* aArray, nsISupports* aOwner)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, aOwner);
    return StringEnumeratorTail(aResult);
}


nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsTArray<nsCString>* aArray, nsISupports* aOwner)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, aOwner);
    return StringEnumeratorTail(aResult);
}

nsresult
NS_NewAdoptingStringEnumerator(nsIStringEnumerator** aResult,
                               nsTArray<nsString>* aArray)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, true);
    return StringEnumeratorTail(aResult);
}

nsresult
NS_NewAdoptingUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                                   nsTArray<nsCString>* aArray)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, true);
    return StringEnumeratorTail(aResult);
}


nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsTArray<nsString>* aArray)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, false);
    return StringEnumeratorTail(aResult);
}

nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsTArray<nsCString>* aArray)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aArray);
    
    *aResult = new nsStringEnumerator(aArray, false);
    return StringEnumeratorTail(aResult);
}

