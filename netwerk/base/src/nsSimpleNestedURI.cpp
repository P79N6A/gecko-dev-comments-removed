





































#include "nsSimpleNestedURI.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsNetUtil.h"

NS_IMPL_ISUPPORTS_INHERITED1(nsSimpleNestedURI, nsSimpleURI, nsINestedURI)

nsSimpleNestedURI::nsSimpleNestedURI(nsIURI* innerURI)
    : nsSimpleURI(nsnull),
      mInnerURI(innerURI)
{
    NS_ASSERTION(innerURI, "Must have inner URI");
    NS_TryToSetImmutable(innerURI);
}
    


NS_IMETHODIMP
nsSimpleNestedURI::Read(nsIObjectInputStream* aStream)
{
    nsresult rv = nsSimpleURI::Read(aStream);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(!mMutable, "How did that happen?");

    rv = aStream->ReadObject(PR_TRUE, getter_AddRefs(mInnerURI));
    if (NS_FAILED(rv)) return rv;

    NS_TryToSetImmutable(mInnerURI);

    return rv;
}

NS_IMETHODIMP
nsSimpleNestedURI::Write(nsIObjectOutputStream* aStream)
{
    nsCOMPtr<nsISerializable> serializable = do_QueryInterface(mInnerURI);
    if (!serializable) {
        
        return NS_ERROR_NOT_AVAILABLE;
    }

    nsresult rv = nsSimpleURI::Write(aStream);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->WriteCompoundObject(mInnerURI, NS_GET_IID(nsIURI),
                                      PR_TRUE);
    return rv;
}



NS_IMETHODIMP
nsSimpleNestedURI::GetInnerURI(nsIURI** uri)
{
    NS_ENSURE_TRUE(mInnerURI, NS_ERROR_NOT_INITIALIZED);
    
    return NS_EnsureSafeToReturn(mInnerURI, uri);
}

NS_IMETHODIMP
nsSimpleNestedURI::GetInnermostURI(nsIURI** uri)
{
    return NS_ImplGetInnermostURI(this, uri);
}



NS_IMETHODIMP
nsSimpleNestedURI::Equals(nsIURI* other, PRBool *result)
{
    *result = PR_FALSE;
    NS_ENSURE_TRUE(mInnerURI, NS_ERROR_NOT_INITIALIZED);
    
    if (other) {
        PRBool correctScheme;
        nsresult rv = other->SchemeIs(mScheme.get(), &correctScheme);
        NS_ENSURE_SUCCESS(rv, rv);

        if (correctScheme) {
            nsCOMPtr<nsINestedURI> nest = do_QueryInterface(other);
            if (nest) {
                nsCOMPtr<nsIURI> otherInner;
                rv = nest->GetInnerURI(getter_AddRefs(otherInner));
                NS_ENSURE_SUCCESS(rv, rv);

                return otherInner->Equals(mInnerURI, result);
            }
        }
    }

    return NS_OK;
}

 nsSimpleURI*
nsSimpleNestedURI::StartClone()
{
    NS_ENSURE_TRUE(mInnerURI, nsnull);
    
    nsCOMPtr<nsIURI> innerClone;
    nsresult rv = mInnerURI->Clone(getter_AddRefs(innerClone));
    if (NS_FAILED(rv)) {
        return nsnull;
    }

    nsSimpleNestedURI* url = new nsSimpleNestedURI(innerClone);
    if (url) {
        url->SetMutable(PR_FALSE);
    }

    return url;
}



NS_IMETHODIMP 
nsSimpleNestedURI::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    static NS_DEFINE_CID(kSimpleNestedURICID, NS_SIMPLENESTEDURI_CID);

    *aClassIDNoAlloc = kSimpleNestedURICID;
    return NS_OK;
}
