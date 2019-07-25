






































#include "nsStringBundleTextOverride.h"
#include "nsString.h"
#include "nsEscape.h"

#include "nsNetUtil.h"
#include "nsAppDirectoryServiceDefs.h"

static NS_DEFINE_CID(kPersistentPropertiesCID, NS_IPERSISTENTPROPERTIES_CID);




class URLPropertyElement : public nsIPropertyElement
{
public:
    URLPropertyElement(nsIPropertyElement *aRealElement, PRUint32 aURLLength) :
        mRealElement(aRealElement),
        mURLLength(aURLLength)
    { }
    virtual ~URLPropertyElement() {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROPERTYELEMENT
    
private:
    nsCOMPtr<nsIPropertyElement> mRealElement;
    PRUint32 mURLLength;
};

NS_IMPL_ISUPPORTS1(URLPropertyElement, nsIPropertyElement)


NS_IMETHODIMP
URLPropertyElement::GetKey(nsACString& aKey)
{
    nsresult rv =  mRealElement->GetKey(aKey);
    if (NS_FAILED(rv)) return rv;

    
    aKey.Cut(0, mURLLength);
    
    return NS_OK;
}


NS_IMETHODIMP
URLPropertyElement::GetValue(nsAString& aValue)
{
    return mRealElement->GetValue(aValue);
}


NS_IMETHODIMP
URLPropertyElement::SetKey(const nsACString& aKey)
{
    
    
    
    
    NS_ERROR("This makes no sense!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
URLPropertyElement::SetValue(const nsAString& aValue)
{
    return mRealElement->SetValue(aValue);
}




class nsPropertyEnumeratorByURL : public nsISimpleEnumerator
{
public:
    nsPropertyEnumeratorByURL(const nsACString& aURL,
                              nsISimpleEnumerator* aOuter) :
        mOuter(aOuter),
        mURL(aURL)
    {
        
        
        
        mURL.ReplaceSubstring(":", "%3A");
        
        mURL.Append('#');
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR
    
    virtual ~nsPropertyEnumeratorByURL() {}
private:

    
    nsCOMPtr<nsISimpleEnumerator> mOuter;

    
    nsCOMPtr<nsIPropertyElement> mCurrent;

    
    nsCString mURL;
};




NS_IMPL_ISUPPORTS1(nsStringBundleTextOverride,
                   nsIStringBundleOverride)

nsresult
nsStringBundleTextOverride::Init()
{
    nsresult rv;

    

    nsCOMPtr<nsIFile> customStringsFile;
    rv = NS_GetSpecialDirectory(NS_APP_CHROME_DIR,
                                getter_AddRefs(customStringsFile));

    if (NS_FAILED(rv)) return rv;

    
    

    customStringsFile->AppendNative(NS_LITERAL_CSTRING("custom-strings.txt"));

    PRBool exists;
    rv = customStringsFile->Exists(&exists);
    if (NS_FAILED(rv) || !exists)
        return NS_ERROR_FAILURE;

    NS_WARNING("Using custom-strings.txt to override string bundles.");
    
    

    nsCAutoString customStringsURLSpec;
    rv = NS_GetURLSpecFromFile(customStringsFile, customStringsURLSpec);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), customStringsURLSpec);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIInputStream> in;
    rv = NS_OpenURI(getter_AddRefs(in), uri);
    if (NS_FAILED(rv)) return rv;

    mValues = do_CreateInstance(kPersistentPropertiesCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = mValues->Load(in);

    
#ifdef DEBUG_alecf
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    mValues->Enumerate(getter_AddRefs(enumerator));
    NS_ASSERTION(enumerator, "no enumerator!\n");
    
    printf("custom-strings.txt contains:\n");
    printf("----------------------------\n");

    PRBool hasMore;
    enumerator->HasMoreElements(&hasMore);
    do {
        nsCOMPtr<nsISupports> sup;
        enumerator->GetNext(getter_AddRefs(sup));

        nsCOMPtr<nsIPropertyElement> prop = do_QueryInterface(sup);

        nsCAutoString key;
        nsAutoString value;
        prop->GetKey(key);
        prop->GetValue(value);

        printf("%s = '%s'\n", key.get(), NS_ConvertUTF16toUTF8(value).get());

        enumerator->HasMoreElements(&hasMore);
    } while (hasMore);
#endif
    
    return rv;
}

NS_IMETHODIMP
nsStringBundleTextOverride::GetStringFromName(const nsACString& aURL,
                                              const nsACString& key,
                                              nsAString& aResult)
{
    
    nsCAutoString combinedURL(aURL + NS_LITERAL_CSTRING("#") + key);

    
    combinedURL.ReplaceSubstring(":", "%3A");

    return mValues->GetStringProperty(combinedURL, aResult);
}

NS_IMETHODIMP
nsStringBundleTextOverride::EnumerateKeysInBundle(const nsACString& aURL,
                                                  nsISimpleEnumerator** aResult)
{
    
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    mValues->Enumerate(getter_AddRefs(enumerator));

    
    nsPropertyEnumeratorByURL* propEnum =
        new nsPropertyEnumeratorByURL(aURL, enumerator);

    if (!propEnum) return NS_ERROR_OUT_OF_MEMORY;
    
    NS_ADDREF(*aResult = propEnum);
    
    return NS_OK;
}







NS_IMPL_ISUPPORTS1(nsPropertyEnumeratorByURL, nsISimpleEnumerator)

NS_IMETHODIMP
nsPropertyEnumeratorByURL::GetNext(nsISupports **aResult)
{
    if (!mCurrent) return NS_ERROR_UNEXPECTED;

    
    *aResult = new URLPropertyElement(mCurrent, mURL.Length());
    NS_ADDREF(*aResult);

    
    mCurrent = nsnull;
    
    return NS_OK;
}

NS_IMETHODIMP
nsPropertyEnumeratorByURL::HasMoreElements(PRBool * aResult)
{
    PRBool hasMore;
    mOuter->HasMoreElements(&hasMore);
    while (hasMore) {

        nsCOMPtr<nsISupports> supports;
        mOuter->GetNext(getter_AddRefs(supports));

        mCurrent = do_QueryInterface(supports);

        if (mCurrent) {
            nsCAutoString curKey;
            mCurrent->GetKey(curKey);
        
            if (StringBeginsWith(curKey, mURL))
                break;
        }
        
        mOuter->HasMoreElements(&hasMore);
    }

    if (!hasMore)
        mCurrent = PR_FALSE;
    
    *aResult = mCurrent ? PR_TRUE : PR_FALSE;
    
    return NS_OK;
}
