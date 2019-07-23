





































#include "nsArray.h"
#include "nsArrayEnumerator.h"
#include "nsWeakReference.h"


struct findIndexOfClosure
{
    nsISupports *targetElement;
    PRUint32 startIndex;
    PRUint32 resultIndex;
};

PR_STATIC_CALLBACK(PRBool) FindElementCallback(void* aElement, void* aClosure);


NS_IMPL_ISUPPORTS2(nsArray, nsIArray, nsIMutableArray)

nsArray::~nsArray()
{
    Clear();
}
    
NS_IMETHODIMP
nsArray::GetLength(PRUint32* aLength)
{
    *aLength = mArray.Count();
    return NS_OK;
}

NS_IMETHODIMP
nsArray::QueryElementAt(PRUint32 aIndex,
                        const nsIID& aIID,
                        void ** aResult)
{
    nsISupports * obj = mArray.SafeObjectAt(aIndex);
    if (!obj) return NS_ERROR_ILLEGAL_VALUE;

    
    
    return obj->QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
nsArray::IndexOf(PRUint32 aStartIndex, nsISupports* aElement,
                 PRUint32* aResult)
{
    
    if (aStartIndex == 0) {
        *aResult = mArray.IndexOf(aElement);
        if (*aResult == PR_UINT32_MAX)
            return NS_ERROR_FAILURE;
        return NS_OK;
    }

    findIndexOfClosure closure = { aElement, aStartIndex, 0 };
    PRBool notFound = mArray.EnumerateForwards(FindElementCallback, &closure);
    if (notFound)
        return NS_ERROR_FAILURE;

    *aResult = closure.resultIndex;
    return NS_OK;
}

NS_IMETHODIMP
nsArray::Enumerate(nsISimpleEnumerator **aResult)
{
    return NS_NewArrayEnumerator(aResult, NS_STATIC_CAST(nsIArray*, this));
}



NS_IMETHODIMP
nsArray::AppendElement(nsISupports* aElement, PRBool aWeak)
{
    PRBool result;
    if (aWeak) {
        nsCOMPtr<nsISupports> elementRef =
            getter_AddRefs(NS_STATIC_CAST(nsISupports*,
                                          NS_GetWeakReference(aElement)));
        NS_ASSERTION(elementRef, "AppendElement: Trying to use weak references on an object that doesn't support it");
        if (!elementRef)
            return NS_ERROR_FAILURE;
        result = mArray.AppendObject(elementRef);
    }

    else {
        
        result = mArray.AppendObject(aElement);
    }
    return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsArray::RemoveElementAt(PRUint32 aIndex)
{
    PRBool result = mArray.RemoveObjectAt(aIndex);
    return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsArray::InsertElementAt(nsISupports* aElement, PRUint32 aIndex, PRBool aWeak)
{
    nsCOMPtr<nsISupports> elementRef;
    if (aWeak) {
        elementRef =
            getter_AddRefs(NS_STATIC_CAST(nsISupports*,
                                          NS_GetWeakReference(aElement)));
        NS_ASSERTION(elementRef, "InsertElementAt: Trying to use weak references on an object that doesn't support it");
        if (!elementRef)
            return NS_ERROR_FAILURE;
    } else {
        elementRef = aElement;
    }
    PRBool result = mArray.InsertObjectAt(elementRef, aIndex);
    return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsArray::ReplaceElementAt(nsISupports* aElement, PRUint32 aIndex, PRBool aWeak)
{
    nsCOMPtr<nsISupports> elementRef;
    if (aWeak) {
        elementRef =
            getter_AddRefs(NS_STATIC_CAST(nsISupports*,
                                          NS_GetWeakReference(aElement)));
        NS_ASSERTION(elementRef, "ReplaceElementAt: Trying to use weak references on an object that doesn't support it");
        if (!elementRef)
            return NS_ERROR_FAILURE;
    } else {
        elementRef = aElement;
    }
    PRBool result = mArray.ReplaceObjectAt(elementRef, aIndex);
    return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsArray::Clear()
{
    mArray.Clear();
    return NS_OK;
}




PRBool
FindElementCallback(void *aElement, void* aClosure)
{
    findIndexOfClosure* closure =
        NS_STATIC_CAST(findIndexOfClosure*, aClosure);

    nsISupports* element =
        NS_STATIC_CAST(nsISupports*, aElement);
    
    
    if (closure->resultIndex >= closure->startIndex &&
        element == closure->targetElement) {
        return PR_FALSE;    
    }
    closure->resultIndex++;

    return PR_TRUE;
}
