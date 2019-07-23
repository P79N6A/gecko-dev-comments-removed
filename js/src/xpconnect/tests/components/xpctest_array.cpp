









































#include "xpctest_private.h"

class xpcarraytest : public nsIXPCTestArray
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTARRAY

    xpcarraytest();
    virtual ~xpcarraytest();
private:
    nsIXPCTestArray* mReceiver;
};

xpcarraytest::xpcarraytest()
    : mReceiver(NULL)
{
    NS_ADDREF_THIS();
}

xpcarraytest::~xpcarraytest()
{
    NS_IF_RELEASE(mReceiver);
}

NS_IMPL_ISUPPORTS1(xpcarraytest, nsIXPCTestArray)

NS_IMETHODIMP xpcarraytest::SetReceiver(nsIXPCTestArray* aReceiver)
{
    NS_IF_ADDREF(aReceiver);
    NS_IF_RELEASE(mReceiver);
    mReceiver = aReceiver;

    
    if(mReceiver)
    {
        nsCOMPtr<nsIEcho> echo = do_QueryInterface(mReceiver);
    }

    return NS_OK;
}



NS_IMETHODIMP
xpcarraytest::PrintIntegerArray(PRUint32 count, PRInt32 *valueArray)
{
    if(mReceiver)
        return mReceiver->PrintIntegerArray(count, valueArray);
    if(valueArray && count)
    {
        for(PRUint32 i = 0; i < count; i++)
            printf("%d%s", valueArray[i], i == count -1 ? "\n" : ",");
    }
    else
        printf("empty array\n");

    return NS_OK;
}


NS_IMETHODIMP
xpcarraytest::PrintStringArray(PRUint32 count, const char **valueArray)
{
    if(mReceiver)
        return mReceiver->PrintStringArray(count, valueArray);
    if(valueArray && count)
    {
        for(PRUint32 i = 0; i < count; i++)
            printf("\"%s\"%s", valueArray[i], i == count -1 ? "\n" : ",");
    }
    else
        printf("empty array\n");

    return NS_OK;
}


NS_IMETHODIMP
xpcarraytest::MultiplyEachItemInIntegerArray(PRInt32 val, PRUint32 count, PRInt32 **valueArray)
{
    if(mReceiver)
        return mReceiver->MultiplyEachItemInIntegerArray(val, count, valueArray);
    PRInt32* a;
    if(valueArray && count && nsnull != (a = *valueArray))
    {
        for(PRUint32 i = 0; i < count; i++)
            a[i] *= val;
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}        


NS_IMETHODIMP
xpcarraytest::MultiplyEachItemInIntegerArrayAndAppend(PRInt32 val, PRUint32 *count, PRInt32 **valueArray)
{
    if(mReceiver)
        return mReceiver->MultiplyEachItemInIntegerArrayAndAppend(val, count, valueArray);
    PRInt32* in;
    PRUint32 in_count;
    if(valueArray && count && 0 != (in_count = *count) && nsnull != (in = *valueArray))
    {
        PRInt32* out = 
            (PRInt32*) nsMemory::Alloc(in_count * 2 * sizeof(PRUint32));

        if(!out)
            return NS_ERROR_OUT_OF_MEMORY;

        for(PRUint32 i = 0; i < in_count; i++)
        {
            out[i*2]   = in[i];
            out[i*2+1] = in[i] * val;
        }
        nsMemory::Free(in);
        *valueArray = out;
        *count = in_count * 2;
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}        


NS_IMETHODIMP
xpcarraytest::CallEchoMethodOnEachInArray(nsIID * *uuid, PRUint32 *count, void * **result)
{
    NS_ENSURE_ARG_POINTER(uuid);
    NS_ENSURE_ARG_POINTER(count);
    NS_ENSURE_ARG_POINTER(result);

    if(mReceiver)
        return mReceiver->CallEchoMethodOnEachInArray(uuid, count, result);

    
    if(!(*uuid)->Equals(NS_GET_IID(nsIEcho)))
        return NS_ERROR_FAILURE;

    
    nsIEcho** ifaceArray = (nsIEcho**) *result;
    for(PRUint32 i = 0; i < *count; i++)
    {
        ifaceArray[i]->SendOneString("print this from C++");
        NS_RELEASE(ifaceArray[i]);
    }

    
    nsMemory::Free(*uuid);
    nsMemory::Free(*result);

    

    *uuid = (nsIID*) nsMemory::Clone(&NS_GET_IID(nsIXPCTestArray), 
                                        sizeof(nsIID));
    NS_ENSURE_TRUE(*uuid, NS_ERROR_OUT_OF_MEMORY);

    nsISupports** outArray = (nsISupports**) 
            nsMemory::Alloc(2 * sizeof(nsISupports*));
    if (!outArray) {
      nsMemory::Free(*uuid);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    outArray[0] = outArray[1] = this;    
    NS_ADDREF(this);
    NS_ADDREF(this);
    *result = (void**) outArray;

    *count = 2;

    return NS_OK;
}        


NS_IMETHODIMP
xpcarraytest::CallEchoMethodOnEachInArray2(PRUint32 *count, nsIEcho ***result)
{
    NS_ENSURE_ARG_POINTER(count);
    NS_ENSURE_ARG_POINTER(result);

    if(mReceiver)
        return mReceiver->CallEchoMethodOnEachInArray2(count, result);

    
    nsIEcho** ifaceArray =  *result;
    for(PRUint32 i = 0; i < *count; i++)
    {
        ifaceArray[i]->SendOneString("print this from C++");
        NS_RELEASE(ifaceArray[i]);
    }

    
    nsMemory::Free(*result);

    
    *count = 0;
    *result = nsnull;
    return NS_OK;
}        




NS_IMETHODIMP
xpcarraytest::DoubleStringArray(PRUint32 *count, char ***valueArray)
{
    NS_ENSURE_ARG_POINTER(valueArray);
    if(mReceiver)
        return mReceiver->DoubleStringArray(count, valueArray);
    if(!count || !*count)
        return NS_OK;

    char** outArray = (char**) nsMemory::Alloc(*count * 2 * sizeof(char*));
    if(!outArray)
        return NS_ERROR_OUT_OF_MEMORY;
    
    char** p = *valueArray;
    for(PRUint32 i = 0; i < *count; i++)
    {
        int len = strlen(p[i]);
        outArray[i*2] = (char*)nsMemory::Alloc(((len * 2)+1) * sizeof(char));                        
        outArray[(i*2)+1] = (char*)nsMemory::Alloc(((len * 2)+1) * sizeof(char));                        

        for(int k = 0; k < len; k++)
        {
            outArray[i*2][k*2] = outArray[i*2][(k*2)+1] =
            outArray[(i*2)+1][k*2] = outArray[(i*2)+1][(k*2)+1] = p[i][k];
        }
        outArray[i*2][len*2] = outArray[(i*2)+1][len*2] = '\0';
        nsMemory::Free(p[i]);
    }

    nsMemory::Free(p);
    *valueArray = outArray; 
    *count = *count * 2;
    return NS_OK;
}        


NS_IMETHODIMP
xpcarraytest::ReverseStringArray(PRUint32 count, char ***valueArray)
{
    NS_ENSURE_ARG_POINTER(valueArray);
    if(mReceiver)
        return mReceiver->ReverseStringArray(count, valueArray);
    if(!count)
        return NS_OK;

    char** p = *valueArray;
    for(PRUint32 i = 0; i < count/2; i++)
    {
        char* temp = p[i];
        p[i] = p[count-1-i];
        p[count-1-i] = temp;
    }
    return NS_OK;
}


NS_IMETHODIMP
xpcarraytest::PrintStringWithSize(PRUint32 count, const char *str)
{
    if(mReceiver)
        return mReceiver->PrintStringWithSize(count, str);
    printf("\"%s\" : %d\n", str, count);
    return NS_OK;
}        


NS_IMETHODIMP
xpcarraytest::DoubleString(PRUint32 *count, char **str)
{
    NS_ENSURE_ARG_POINTER(str);
    if(mReceiver)
        return mReceiver->DoubleString(count, str);
    if(!count || !*count)
        return NS_OK;

    char* out = (char*) nsMemory::Alloc(((*count * 2)+1) * sizeof(char));                        
    if(!out)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 k;
    for(k = 0; k < *count; k++)
        out[k*2] = out[(k*2)+1] = (*str)[k];
    out[k*2] = '\0';
    nsMemory::Free(*str);
    *str = out; 
    *count = *count * 2;
    return NS_OK;
}        


NS_IMETHODIMP 
xpcarraytest::GetStrings(PRUint32 *count, char ***str)
{
    const static char *strings[] = {"one", "two", "three", "four"};
    const static PRUint32 scount = sizeof(strings)/sizeof(strings[0]);

    if(mReceiver)
        return mReceiver->GetStrings(count, str);

    char** out = (char**) nsMemory::Alloc(scount * sizeof(char*));
    if(!out)
        return NS_ERROR_OUT_OF_MEMORY;
    for(PRUint32 i = 0; i < scount; ++i)
    {
        out[i] = (char*) nsMemory::Clone(strings[i], strlen(strings[i])+1);
        
        if(!out[i])
            return NS_ERROR_OUT_OF_MEMORY;
    }

    *count = scount;
    *str = out;
    return NS_OK;
}




NS_IMETHODIMP
xpctest::ConstructArrayTest(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcarraytest* obj = new xpcarraytest();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}





