





















































#include "nsStringStream.h"
#include "nsStreamUtils.h"
#include "nsReadableUtils.h"
#include "nsISeekableStream.h"
#include "nsISupportsPrimitives.h"
#include "nsInt64.h"
#include "nsCRT.h"
#include "prerror.h"
#include "plstr.h"
#include "nsIClassInfoImpl.h"





class nsStringInputStream : public nsIStringInputStream
                          , public nsISeekableStream
                          , public nsISupportsCString
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSISTRINGINPUTSTREAM
    NS_DECL_NSISEEKABLESTREAM
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSCSTRING

    nsStringInputStream()
        : mData(nsnull)
        , mOffset(0)
        , mLength(0)
        , mOwned(PR_FALSE)
    {}

private:
    ~nsStringInputStream()
    {
        if (mData)
            Clear();
    }

    PRInt32 LengthRemaining() const
    {
        return mLength - mOffset;
    }

    void Clear()
    {
        NS_ASSERTION(mData || !mOwned, "bad state");
        if (mOwned)
            NS_Free(NS_CONST_CAST(char*, mData));

        
        mOffset = 0;
    }

    const char*    mData;
    PRUint32       mOffset;
    PRUint32       mLength;
    PRPackedBool   mOwned;
};



NS_IMPL_THREADSAFE_ADDREF(nsStringInputStream)
NS_IMPL_THREADSAFE_RELEASE(nsStringInputStream)

NS_IMPL_QUERY_INTERFACE4_CI(nsStringInputStream,
                            nsIStringInputStream,
                            nsIInputStream,
                            nsISupportsCString,
                            nsISeekableStream)
NS_IMPL_CI_INTERFACE_GETTER4(nsStringInputStream,
                             nsIStringInputStream,
                             nsIInputStream,
                             nsISupportsCString,
                             nsISeekableStream)





NS_IMETHODIMP
nsStringInputStream::GetType(PRUint16 *type)
{
    *type = TYPE_CSTRING;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::GetData(nsACString &data)
{
    
    
    
    NS_ENSURE_TRUE(mData, NS_BASE_STREAM_CLOSED);

    data.Assign(mData, mLength);
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::SetData(const nsACString &data)
{
    nsACString::const_iterator iter;
    data.BeginReading(iter);
    return SetData(iter.get(), iter.size_forward());
}

NS_IMETHODIMP
nsStringInputStream::ToString(char **result)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP
nsStringInputStream::SetData(const char *data, PRInt32 dataLen)
{
    NS_ENSURE_ARG_POINTER(data);

    if (dataLen < 0)
        dataLen = strlen(data);

    
    
 
    char *copy = NS_STATIC_CAST(char *, NS_Alloc(dataLen));
    if (!copy)
        return NS_ERROR_OUT_OF_MEMORY;
    memcpy(copy, data, dataLen);

    return AdoptData(copy, dataLen);
}

NS_IMETHODIMP
nsStringInputStream::AdoptData(char *data, PRInt32 dataLen)
{
    NS_ENSURE_ARG_POINTER(data);

    if (dataLen < 0)
        dataLen = strlen(data);

    Clear();
    
    mData = data;
    mLength = dataLen;
    mOwned = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::ShareData(const char *data, PRInt32 dataLen)
{
    NS_ENSURE_ARG_POINTER(data);

    if (dataLen < 0)
        dataLen = strlen(data);

    Clear();
    
    mData = data;
    mLength = dataLen;
    mOwned = PR_FALSE;
    return NS_OK;
}





NS_IMETHODIMP
nsStringInputStream::Close()
{
    Clear();
    mData = nsnull;
    mLength = 0;
    mOwned = PR_FALSE;
    return NS_OK;
}
    
NS_IMETHODIMP
nsStringInputStream::Available(PRUint32 *aLength)
{
    NS_ASSERTION(aLength, "null ptr");

    if (!mData)
        return NS_BASE_STREAM_CLOSED;

    *aLength = LengthRemaining();
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::Read(char* aBuf, PRUint32 aCount, PRUint32 *aReadCount)
{
    NS_ASSERTION(aBuf, "null ptr");
    return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, aReadCount);
}

NS_IMETHODIMP
nsStringInputStream::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                  PRUint32 aCount, PRUint32 *result)
{
    NS_ASSERTION(result, "null ptr");
    NS_ASSERTION(mLength >= mOffset, "bad stream state");

    
    PRUint32 maxCount = LengthRemaining();
    if (maxCount == 0) {
        *result = 0;
        return NS_OK;
    }
    NS_ASSERTION(mData, "must have data if maxCount != 0");

    if (aCount > maxCount)
        aCount = maxCount;
    nsresult rv = writer(this, closure, mData + mOffset, 0, aCount, result);
    if (NS_SUCCEEDED(rv)) {
        NS_ASSERTION(*result <= aCount,
                     "writer should not write more than we asked it to write");
        mOffset += *result;
    }

    
    return NS_OK;
}
    
NS_IMETHODIMP
nsStringInputStream::IsNonBlocking(PRBool *aNonBlocking)
{
    *aNonBlocking = PR_TRUE;
    return NS_OK;
}





NS_IMETHODIMP 
nsStringInputStream::Seek(PRInt32 whence, PRInt64 offset)
{
    if (!mData)
        return NS_BASE_STREAM_CLOSED;

    
 
    PRInt64 newPos = offset;
    switch (whence) {
    case NS_SEEK_SET:
        break;
    case NS_SEEK_CUR:
        newPos += (PRInt32) mOffset;
        break;
    case NS_SEEK_END:
        newPos += (PRInt32) mLength;
        break;
    default:
        NS_ERROR("invalid whence");
        return NS_ERROR_INVALID_ARG;
    }

    

    NS_ENSURE_ARG(newPos >= 0);
    NS_ENSURE_ARG(newPos <= (PRInt32) mLength);

    mOffset = (PRInt32) newPos;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::Tell(PRInt64* outWhere)
{
    if (!mData)
        return NS_BASE_STREAM_CLOSED;

    *outWhere = mOffset;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::SetEOF()
{
    if (!mData)
        return NS_BASE_STREAM_CLOSED;

    mLength = mOffset;
    return NS_OK;
}

NS_COM nsresult
NS_NewByteInputStream(nsIInputStream** aStreamResult,
                      const char* aStringToRead, PRInt32 aLength,
                      nsAssignmentType aAssignment)
{
    NS_PRECONDITION(aStreamResult, "null out ptr");

    nsStringInputStream* stream = new nsStringInputStream();
    if (! stream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(stream);

    nsresult rv;
    switch (aAssignment) {
    case NS_ASSIGNMENT_COPY:
        rv = stream->SetData(aStringToRead, aLength);
        break;
    case NS_ASSIGNMENT_DEPEND:
        rv = stream->ShareData(aStringToRead, aLength);
        break;
    case NS_ASSIGNMENT_ADOPT:
        rv = stream->AdoptData(NS_CONST_CAST(char*, aStringToRead), aLength);
        break;
    default:
        NS_ERROR("invalid assignment type");
        rv = NS_ERROR_INVALID_ARG;
    }
    
    if (NS_FAILED(rv)) {
        NS_RELEASE(stream);
        return rv;
    }
    
    *aStreamResult = stream;
    return NS_OK;
}

NS_COM nsresult
NS_NewStringInputStream(nsIInputStream** aStreamResult,
                        const nsAString& aStringToRead)
{
    char* data = ToNewCString(aStringToRead);  
    if (!data)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = NS_NewByteInputStream(aStreamResult, data,
                                        aStringToRead.Length(),
                                        NS_ASSIGNMENT_ADOPT);
    if (NS_FAILED(rv))
        NS_Free(data);
    return rv;
}

NS_COM nsresult
NS_NewCStringInputStream(nsIInputStream** aStreamResult,
                         const nsACString& aStringToRead)
{
    nsACString::const_iterator data;
    aStringToRead.BeginReading(data);

    return NS_NewByteInputStream(aStreamResult, data.get(), data.size_forward(),
                                 NS_ASSIGNMENT_COPY);
}


NS_METHOD
nsStringInputStreamConstructor(nsISupports *outer, REFNSIID iid, void **result)
{
    *result = nsnull;

    NS_ENSURE_TRUE(!outer, NS_ERROR_NO_AGGREGATION);

    nsStringInputStream *inst = new nsStringInputStream();
    if (!inst)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(inst);
    nsresult rv = inst->QueryInterface(iid, result);
    NS_RELEASE(inst);

    return rv;
}
