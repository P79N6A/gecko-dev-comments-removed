





















































#include "IPC/IPCMessageUtils.h"

#include "nsStringStream.h"
#include "nsStreamUtils.h"
#include "nsReadableUtils.h"
#include "nsISeekableStream.h"
#include "nsISupportsPrimitives.h"
#include "nsCRT.h"
#include "prerror.h"
#include "plstr.h"
#include "nsIClassInfoImpl.h"
#include "nsIIPCSerializable.h"





class nsStringInputStream : public nsIStringInputStream
                          , public nsISeekableStream
                          , public nsISupportsCString
                          , public nsIIPCSerializable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSISTRINGINPUTSTREAM
    NS_DECL_NSISEEKABLESTREAM
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSCSTRING
    NS_DECL_NSIIPCSERIALIZABLE

    nsStringInputStream()
    {
        Clear();
    }

private:
    ~nsStringInputStream()
    {}

    PRUint32 Length() const
    {
        return mData.Length();
    }

    PRUint32 LengthRemaining() const
    {
        return Length() - mOffset;
    }

    void Clear()
    {
        mData.SetIsVoid(true);
    }

    bool Closed()
    {
        return mData.IsVoid();
    }

    nsDependentCSubstring mData;
    PRUint32 mOffset;
};



NS_IMPL_THREADSAFE_ADDREF(nsStringInputStream)
NS_IMPL_THREADSAFE_RELEASE(nsStringInputStream)

NS_IMPL_CLASSINFO(nsStringInputStream, NULL, nsIClassInfo::THREADSAFE,
                  NS_STRINGINPUTSTREAM_CID)
NS_IMPL_QUERY_INTERFACE5_CI(nsStringInputStream,
                            nsIStringInputStream,
                            nsIInputStream,
                            nsISupportsCString,
                            nsISeekableStream,
                            nsIIPCSerializable)
NS_IMPL_CI_INTERFACE_GETTER5(nsStringInputStream,
                             nsIStringInputStream,
                             nsIInputStream,
                             nsISupportsCString,
                             nsISeekableStream,
                             nsIIPCSerializable)





NS_IMETHODIMP
nsStringInputStream::GetType(PRUint16 *type)
{
    *type = TYPE_CSTRING;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::GetData(nsACString &data)
{
    
    
    
    NS_ENSURE_TRUE(!Closed(), NS_BASE_STREAM_CLOSED);

    data.Assign(mData);
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::SetData(const nsACString &data)
{
    mData.Assign(data);
    mOffset = 0;
    return NS_OK;
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
    mData.Assign(data, dataLen);
    mOffset = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::AdoptData(char *data, PRInt32 dataLen)
{
    NS_ENSURE_ARG_POINTER(data);
    mData.Adopt(data, dataLen);
    mOffset = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::ShareData(const char *data, PRInt32 dataLen)
{
    NS_ENSURE_ARG_POINTER(data);

    if (dataLen < 0)
        dataLen = strlen(data);

    mData.Rebind(data, data + dataLen);
    mOffset = 0;
    return NS_OK;
}





NS_IMETHODIMP
nsStringInputStream::Close()
{
    Clear();
    return NS_OK;
}
    
NS_IMETHODIMP
nsStringInputStream::Available(PRUint32 *aLength)
{
    NS_ASSERTION(aLength, "null ptr");

    if (Closed())
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
    NS_ASSERTION(Length() >= mOffset, "bad stream state");

    if (Closed())
        return NS_BASE_STREAM_CLOSED;

    
    PRUint32 maxCount = LengthRemaining();
    if (maxCount == 0) {
        *result = 0;
        return NS_OK;
    }

    if (aCount > maxCount)
        aCount = maxCount;
    nsresult rv = writer(this, closure, mData.BeginReading() + mOffset, 0, aCount, result);
    if (NS_SUCCEEDED(rv)) {
        NS_ASSERTION(*result <= aCount,
                     "writer should not write more than we asked it to write");
        mOffset += *result;
    }

    
    return NS_OK;
}
    
NS_IMETHODIMP
nsStringInputStream::IsNonBlocking(bool *aNonBlocking)
{
    *aNonBlocking = true;
    return NS_OK;
}





NS_IMETHODIMP 
nsStringInputStream::Seek(PRInt32 whence, PRInt64 offset)
{
    if (Closed())
        return NS_BASE_STREAM_CLOSED;

    
 
    PRInt64 newPos = offset;
    switch (whence) {
    case NS_SEEK_SET:
        break;
    case NS_SEEK_CUR:
        newPos += mOffset;
        break;
    case NS_SEEK_END:
        newPos += Length();
        break;
    default:
        NS_ERROR("invalid whence");
        return NS_ERROR_INVALID_ARG;
    }

    NS_ENSURE_ARG(newPos >= 0);
    NS_ENSURE_ARG(newPos <= Length());

    mOffset = (PRUint32)newPos;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::Tell(PRInt64* outWhere)
{
    if (Closed())
        return NS_BASE_STREAM_CLOSED;

    *outWhere = mOffset;
    return NS_OK;
}

NS_IMETHODIMP
nsStringInputStream::SetEOF()
{
    if (Closed())
        return NS_BASE_STREAM_CLOSED;

    mOffset = Length();
    return NS_OK;
}





bool
nsStringInputStream::Read(const IPC::Message *aMsg, void **aIter)
{
    using IPC::ReadParam;

    nsCString value;

    if (!ReadParam(aMsg, aIter, &value))
        return false;

    nsresult rv = SetData(value);
    if (NS_FAILED(rv))
        return false;

    return true;
}

void
nsStringInputStream::Write(IPC::Message *aMsg)
{
    using IPC::WriteParam;

    WriteParam(aMsg, static_cast<const nsCString&>(PromiseFlatCString(mData)));
}

nsresult
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
        rv = stream->AdoptData(const_cast<char*>(aStringToRead), aLength);
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

nsresult
NS_NewStringInputStream(nsIInputStream** aStreamResult,
                        const nsAString& aStringToRead)
{
    NS_LossyConvertUTF16toASCII data(aStringToRead); 
    return NS_NewCStringInputStream(aStreamResult, data);
}

nsresult
NS_NewCStringInputStream(nsIInputStream** aStreamResult,
                         const nsACString& aStringToRead)
{
    NS_PRECONDITION(aStreamResult, "null out ptr");

    nsStringInputStream* stream = new nsStringInputStream();
    if (! stream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(stream);

    stream->SetData(aStringToRead);

    *aStreamResult = stream;
    return NS_OK;
}


nsresult
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
