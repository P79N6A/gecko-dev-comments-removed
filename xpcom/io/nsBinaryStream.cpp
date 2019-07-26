


















#include <string.h>
#include "nsBinaryStream.h"
#include "nsCRT.h"
#include "prlong.h"
#include "nsString.h"
#include "nsISerializable.h"
#include "nsIClassInfo.h"
#include "nsComponentManagerUtils.h"
#include "nsIURI.h" 
#include "mozilla/Endian.h"

#include "jsapi.h"
#include "jsfriendapi.h"

NS_IMPL_ISUPPORTS3(nsBinaryOutputStream, nsIObjectOutputStream, nsIBinaryOutputStream, nsIOutputStream)

NS_IMETHODIMP
nsBinaryOutputStream::Flush() 
{ 
    NS_ENSURE_STATE(mOutputStream);
    return mOutputStream->Flush(); 
}

NS_IMETHODIMP
nsBinaryOutputStream::Close() 
{ 
    NS_ENSURE_STATE(mOutputStream);
    return mOutputStream->Close(); 
}

NS_IMETHODIMP
nsBinaryOutputStream::Write(const char *aBuf, uint32_t aCount, uint32_t *aActualBytes)
{
    NS_ENSURE_STATE(mOutputStream);
    return mOutputStream->Write(aBuf, aCount, aActualBytes);
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteFrom(nsIInputStream *inStr, uint32_t count, uint32_t *_retval)
{
    NS_NOTREACHED("WriteFrom");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteSegments(nsReadSegmentFun reader, void * closure, uint32_t count, uint32_t *_retval)
{
    NS_NOTREACHED("WriteSegments");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinaryOutputStream::IsNonBlocking(bool *aNonBlocking)
{
    NS_ENSURE_STATE(mOutputStream);
    return mOutputStream->IsNonBlocking(aNonBlocking);
}

nsresult
nsBinaryOutputStream::WriteFully(const char *aBuf, uint32_t aCount)
{
    NS_ENSURE_STATE(mOutputStream);

    nsresult rv;
    uint32_t bytesWritten;

    rv = mOutputStream->Write(aBuf, aCount, &bytesWritten);
    if (NS_FAILED(rv)) return rv;
    if (bytesWritten != aCount)
        return NS_ERROR_FAILURE;
    return NS_OK;
}

NS_IMETHODIMP
nsBinaryOutputStream::SetOutputStream(nsIOutputStream *aOutputStream)
{
    NS_ENSURE_ARG_POINTER(aOutputStream);
    mOutputStream = aOutputStream;
    mBufferAccess = do_QueryInterface(aOutputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteBoolean(bool aBoolean)
{
    return Write8(aBoolean);
}

NS_IMETHODIMP
nsBinaryOutputStream::Write8(uint8_t aByte)
{
    return WriteFully((const char*)&aByte, sizeof aByte);
}

NS_IMETHODIMP
nsBinaryOutputStream::Write16(uint16_t a16)
{
    a16 = mozilla::NativeEndian::swapToBigEndian(a16);
    return WriteFully((const char*)&a16, sizeof a16);
}

NS_IMETHODIMP
nsBinaryOutputStream::Write32(uint32_t a32)
{
    a32 = mozilla::NativeEndian::swapToBigEndian(a32);
    return WriteFully((const char*)&a32, sizeof a32);
}

NS_IMETHODIMP
nsBinaryOutputStream::Write64(uint64_t a64)
{
    nsresult rv;
    uint32_t bytesWritten;

    a64 = mozilla::NativeEndian::swapToBigEndian(a64);
    rv = Write(reinterpret_cast<char*>(&a64), sizeof a64, &bytesWritten);
    if (NS_FAILED(rv)) return rv;
    if (bytesWritten != sizeof a64)
        return NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteFloat(float aFloat)
{
    NS_ASSERTION(sizeof(float) == sizeof (uint32_t),
                 "False assumption about sizeof(float)");
    return Write32(*reinterpret_cast<uint32_t*>(&aFloat));
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteDouble(double aDouble)
{
    NS_ASSERTION(sizeof(double) == sizeof(uint64_t),
                 "False assumption about sizeof(double)");
    return Write64(*reinterpret_cast<uint64_t*>(&aDouble));
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteStringZ(const char *aString)
{
    uint32_t length;
    nsresult rv;

    length = strlen(aString);
    rv = Write32(length);
    if (NS_FAILED(rv)) return rv;
    return WriteFully(aString, length);
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteWStringZ(const PRUnichar* aString)
{
    uint32_t length, byteCount;
    nsresult rv;

    length = NS_strlen(aString);
    rv = Write32(length);
    if (NS_FAILED(rv)) return rv;

    if (length == 0)
        return NS_OK;
    byteCount = length * sizeof(PRUnichar);

#ifdef IS_BIG_ENDIAN
    rv = WriteBytes(reinterpret_cast<const char*>(aString), byteCount);
#else
    
    PRUnichar *copy, temp[64];
    if (length <= 64) {
        copy = temp;
    } else {
        copy = reinterpret_cast<PRUnichar*>(moz_malloc(byteCount));
        if (!copy)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ASSERTION((uintptr_t(aString) & 0x1) == 0, "aString not properly aligned");
    mozilla::NativeEndian::copyAndSwapToBigEndian(copy, aString, length);
    rv = WriteBytes(reinterpret_cast<const char*>(copy), byteCount);
    if (copy != temp)
        moz_free(copy);
#endif

    return rv;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteUtf8Z(const PRUnichar* aString)
{
    return WriteStringZ(NS_ConvertUTF16toUTF8(aString).get());
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteBytes(const char *aString, uint32_t aLength)
{
    nsresult rv;
    uint32_t bytesWritten;

    rv = Write(aString, aLength, &bytesWritten);
    if (NS_FAILED(rv)) return rv;
    if (bytesWritten != aLength)
        return NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteByteArray(uint8_t *aBytes, uint32_t aLength)
{
    return WriteBytes(reinterpret_cast<char *>(aBytes), aLength);
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteObject(nsISupports* aObject, bool aIsStrongRef)
{
    return WriteCompoundObject(aObject, NS_GET_IID(nsISupports),
                               aIsStrongRef);
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteSingleRefObject(nsISupports* aObject)
{
    return WriteCompoundObject(aObject, NS_GET_IID(nsISupports),
                               true);
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteCompoundObject(nsISupports* aObject,
                                          const nsIID& aIID,
                                          bool aIsStrongRef)
{
    
    NS_ENSURE_TRUE(aIsStrongRef, NS_ERROR_UNEXPECTED);
    
    nsCOMPtr<nsIClassInfo> classInfo = do_QueryInterface(aObject);
    NS_ENSURE_TRUE(classInfo, NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<nsISerializable> serializable = do_QueryInterface(aObject);
    NS_ENSURE_TRUE(serializable, NS_ERROR_NOT_AVAILABLE);

    nsCID cid;
    classInfo->GetClassIDNoAlloc(&cid);

    nsresult rv = WriteID(cid);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = WriteID(aIID);
    NS_ENSURE_SUCCESS(rv, rv);

    return serializable->Write(this);
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteID(const nsIID& aIID)
{
    nsresult rv = Write32(aIID.m0);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = Write16(aIID.m1);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = Write16(aIID.m2);
    NS_ENSURE_SUCCESS(rv, rv);

    for (int i = 0; i < 8; ++i) {
        rv = Write8(aIID.m3[i]);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

NS_IMETHODIMP_(char*)
nsBinaryOutputStream::GetBuffer(uint32_t aLength, uint32_t aAlignMask)
{
    if (mBufferAccess)
        return mBufferAccess->GetBuffer(aLength, aAlignMask);
    return nullptr;
}

NS_IMETHODIMP_(void)
nsBinaryOutputStream::PutBuffer(char* aBuffer, uint32_t aLength)
{
    if (mBufferAccess)
        mBufferAccess->PutBuffer(aBuffer, aLength);
}

NS_IMPL_ISUPPORTS3(nsBinaryInputStream, nsIObjectInputStream, nsIBinaryInputStream, nsIInputStream)

NS_IMETHODIMP
nsBinaryInputStream::Available(uint64_t* aResult)
{
    NS_ENSURE_STATE(mInputStream);
    return mInputStream->Available(aResult);
}

NS_IMETHODIMP
nsBinaryInputStream::Read(char* aBuffer, uint32_t aCount, uint32_t *aNumRead)
{
    NS_ENSURE_STATE(mInputStream);

    
    uint32_t totalRead = 0;

    uint32_t bytesRead;
    do {
        nsresult rv = mInputStream->Read(aBuffer, aCount, &bytesRead);
        if (rv == NS_BASE_STREAM_WOULD_BLOCK && totalRead != 0) {
            
            break;
        }
        
        if (NS_FAILED(rv)) {
            return rv;
        }

        totalRead += bytesRead;
        aBuffer += bytesRead;
        aCount -= bytesRead;
    } while (aCount != 0 && bytesRead != 0);

    *aNumRead = totalRead;
    
    return NS_OK;
}







struct ReadSegmentsClosure {
    nsIInputStream* mRealInputStream;
    void* mRealClosure;
    nsWriteSegmentFun mRealWriter;
    nsresult mRealResult;
    uint32_t mBytesRead;  
};


static NS_METHOD
ReadSegmentForwardingThunk(nsIInputStream* aStream,
                           void *aClosure,
                           const char* aFromSegment,
                           uint32_t aToOffset,
                           uint32_t aCount,
                           uint32_t *aWriteCount)
{
    ReadSegmentsClosure* thunkClosure =
        reinterpret_cast<ReadSegmentsClosure*>(aClosure);

    NS_ASSERTION(NS_SUCCEEDED(thunkClosure->mRealResult),
                 "How did this get to be a failure status?");

    thunkClosure->mRealResult =
        thunkClosure->mRealWriter(thunkClosure->mRealInputStream,
                                  thunkClosure->mRealClosure,
                                  aFromSegment,
                                  thunkClosure->mBytesRead + aToOffset,
                                  aCount, aWriteCount);

    return thunkClosure->mRealResult;
}


NS_IMETHODIMP
nsBinaryInputStream::ReadSegments(nsWriteSegmentFun writer, void * closure, uint32_t count, uint32_t *_retval)
{
    NS_ENSURE_STATE(mInputStream);

    ReadSegmentsClosure thunkClosure = { this, closure, writer, NS_OK, 0 };
    
    
    uint32_t bytesRead;
    do {
        nsresult rv = mInputStream->ReadSegments(ReadSegmentForwardingThunk,
                                                 &thunkClosure,
                                                 count, &bytesRead);

        if (rv == NS_BASE_STREAM_WOULD_BLOCK && thunkClosure.mBytesRead != 0) {
            
            break;
        }
        
        if (NS_FAILED(rv)) {
            return rv;
        }

        thunkClosure.mBytesRead += bytesRead;
        count -= bytesRead;
    } while (count != 0 && bytesRead != 0 &&
             NS_SUCCEEDED(thunkClosure.mRealResult));

    *_retval = thunkClosure.mBytesRead;

    return NS_OK;
}

NS_IMETHODIMP
nsBinaryInputStream::IsNonBlocking(bool *aNonBlocking)
{
    NS_ENSURE_STATE(mInputStream);
    return mInputStream->IsNonBlocking(aNonBlocking);
}

NS_IMETHODIMP
nsBinaryInputStream::Close() 
{ 
    NS_ENSURE_STATE(mInputStream);
    return mInputStream->Close(); 
}

NS_IMETHODIMP
nsBinaryInputStream::SetInputStream(nsIInputStream *aInputStream)
{
    NS_ENSURE_ARG_POINTER(aInputStream);
    mInputStream = aInputStream;
    mBufferAccess = do_QueryInterface(aInputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadBoolean(bool* aBoolean)
{
    uint8_t byteResult;
    nsresult rv = Read8(&byteResult);
    if (NS_FAILED(rv)) return rv;
    *aBoolean = !!byteResult;
    return rv;
}

NS_IMETHODIMP
nsBinaryInputStream::Read8(uint8_t* aByte)
{
    nsresult rv;
    uint32_t bytesRead;

    rv = Read(reinterpret_cast<char*>(aByte), sizeof(*aByte), &bytesRead);
    if (NS_FAILED(rv)) return rv;
    if (bytesRead != 1)
        return NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsBinaryInputStream::Read16(uint16_t* a16)
{
    nsresult rv;
    uint32_t bytesRead;

    rv = Read(reinterpret_cast<char*>(a16), sizeof *a16, &bytesRead);
    if (NS_FAILED(rv)) return rv;
    if (bytesRead != sizeof *a16)
        return NS_ERROR_FAILURE;
    *a16 = mozilla::NativeEndian::swapFromBigEndian(*a16);
    return rv;
}

NS_IMETHODIMP
nsBinaryInputStream::Read32(uint32_t* a32)
{
    nsresult rv;
    uint32_t bytesRead;

    rv = Read(reinterpret_cast<char*>(a32), sizeof *a32, &bytesRead);
    if (NS_FAILED(rv)) return rv;
    if (bytesRead != sizeof *a32)
        return NS_ERROR_FAILURE;
    *a32 = mozilla::NativeEndian::swapFromBigEndian(*a32);
    return rv;
}

NS_IMETHODIMP
nsBinaryInputStream::Read64(uint64_t* a64)
{
    nsresult rv;
    uint32_t bytesRead;

    rv = Read(reinterpret_cast<char*>(a64), sizeof *a64, &bytesRead);
    if (NS_FAILED(rv)) return rv;
    if (bytesRead != sizeof *a64)
        return NS_ERROR_FAILURE;
    *a64 = mozilla::NativeEndian::swapFromBigEndian(*a64);
    return rv;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadFloat(float* aFloat)
{
    NS_ASSERTION(sizeof(float) == sizeof (uint32_t),
                 "False assumption about sizeof(float)");
    return Read32(reinterpret_cast<uint32_t*>(aFloat));
}

NS_IMETHODIMP
nsBinaryInputStream::ReadDouble(double* aDouble)
{
    NS_ASSERTION(sizeof(double) == sizeof(uint64_t),
                 "False assumption about sizeof(double)");
    return Read64(reinterpret_cast<uint64_t*>(aDouble));
}

static NS_METHOD
WriteSegmentToCString(nsIInputStream* aStream,
                      void *aClosure,
                      const char* aFromSegment,
                      uint32_t aToOffset,
                      uint32_t aCount,
                      uint32_t *aWriteCount)
{
    nsACString* outString = static_cast<nsACString*>(aClosure);

    outString->Append(aFromSegment, aCount);

    *aWriteCount = aCount;
    
    return NS_OK;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadCString(nsACString& aString)
{
    nsresult rv;
    uint32_t length, bytesRead;

    rv = Read32(&length);
    if (NS_FAILED(rv)) return rv;

    aString.Truncate();
    rv = ReadSegments(WriteSegmentToCString, &aString, length, &bytesRead);
    if (NS_FAILED(rv)) return rv;
    
    if (bytesRead != length)
        return NS_ERROR_FAILURE;

    return NS_OK;
}




struct WriteStringClosure {
    PRUnichar *mWriteCursor;
    bool mHasCarryoverByte;
    char mCarryoverByte;
};

















static NS_METHOD
WriteSegmentToString(nsIInputStream* aStream,
                     void *aClosure,
                     const char* aFromSegment,
                     uint32_t aToOffset,
                     uint32_t aCount,
                     uint32_t *aWriteCount)
{
    NS_PRECONDITION(aCount > 0, "Why are we being told to write 0 bytes?");
    NS_PRECONDITION(sizeof(PRUnichar) == 2, "We can't handle other sizes!");

    WriteStringClosure* closure = static_cast<WriteStringClosure*>(aClosure);
    PRUnichar *cursor = closure->mWriteCursor;

    
    
    
    *aWriteCount = aCount;

    
    if (closure->mHasCarryoverByte) {
        
        char bytes[2] = { closure->mCarryoverByte, *aFromSegment };
        *cursor = *(PRUnichar*)bytes;
        
        mozilla::NativeEndian::swapToBigEndianInPlace(cursor, 1);
        ++cursor;
        
        
        
        
        ++aFromSegment;
        --aCount;

        closure->mHasCarryoverByte = false;
    }
    
    
    const PRUnichar *unicodeSegment =
        reinterpret_cast<const PRUnichar*>(aFromSegment);

    
    uint32_t segmentLength = aCount / sizeof(PRUnichar);

    
    
    memcpy(cursor, unicodeSegment, segmentLength * sizeof(PRUnichar));
    PRUnichar *end = cursor + segmentLength;
    mozilla::NativeEndian::swapToBigEndianInPlace(cursor, segmentLength);
    closure->mWriteCursor = end;

    
    
    
    if (aCount % sizeof(PRUnichar) != 0) {
        
        
        closure->mCarryoverByte = aFromSegment[aCount - 1];
        closure->mHasCarryoverByte = true;
    }
    
    return NS_OK;
}


NS_IMETHODIMP
nsBinaryInputStream::ReadString(nsAString& aString)
{
    nsresult rv;
    uint32_t length, bytesRead;

    rv = Read32(&length);
    if (NS_FAILED(rv)) return rv;

    if (length == 0) {
      aString.Truncate();
      return NS_OK;
    }

    
    if (!aString.SetLength(length, mozilla::fallible_t()))
        return NS_ERROR_OUT_OF_MEMORY;

    nsAString::iterator start;
    aString.BeginWriting(start);
    
    WriteStringClosure closure;
    closure.mWriteCursor = start.get();
    closure.mHasCarryoverByte = false;
    
    rv = ReadSegments(WriteSegmentToString, &closure,
                      length*sizeof(PRUnichar), &bytesRead);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(!closure.mHasCarryoverByte, "some strange stream corruption!");
    
    if (bytesRead != length*sizeof(PRUnichar))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadBytes(uint32_t aLength, char* *_rval)
{
    nsresult rv;
    uint32_t bytesRead;
    char* s;

    s = reinterpret_cast<char*>(moz_malloc(aLength));
    if (!s)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = Read(s, aLength, &bytesRead);
    if (NS_FAILED(rv)) {
        moz_free(s);
        return rv;
    }
    if (bytesRead != aLength) {
        moz_free(s);
        return NS_ERROR_FAILURE;
    }

    *_rval = s;
    return NS_OK;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadByteArray(uint32_t aLength, uint8_t* *_rval)
{
    return ReadBytes(aLength, reinterpret_cast<char **>(_rval));
}

NS_IMETHODIMP
nsBinaryInputStream::ReadArrayBuffer(uint32_t aLength, const JS::Value& aBuffer, JSContext* cx)
{
    if (!aBuffer.isObject()) {
        return NS_ERROR_FAILURE;
    }
    JS::RootedObject buffer(cx, &aBuffer.toObject());
    if (!JS_IsArrayBufferObject(buffer) ||
        JS_GetArrayBufferByteLength(buffer) < aLength) {
        return NS_ERROR_FAILURE;
    }
    uint8_t* data = JS_GetArrayBufferData(&aBuffer.toObject());
    if (!data) {
        return NS_ERROR_FAILURE;
    }

    uint32_t bytesRead;
    nsresult rv = Read(reinterpret_cast<char*>(data), aLength, &bytesRead);
    NS_ENSURE_SUCCESS(rv, rv);
    if (bytesRead != aLength) {
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadObject(bool aIsStrongRef, nsISupports* *aObject)
{
    nsCID cid;
    nsIID iid;
    nsresult rv = ReadID(&cid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ReadID(&iid);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    static const nsIID oldURIiid =
        { 0x7a22cc0, 0xce5, 0x11d3,
          { 0x93, 0x31, 0x0, 0x10, 0x4b, 0xa0, 0xfd, 0x40 }};

    
    static const nsIID oldURIiid2 =
        { 0xd6d04c36, 0x0fa4, 0x4db3,
          { 0xbe, 0x05, 0x4a, 0x18, 0x39, 0x71, 0x03, 0xe2 }};

    
    static const nsIID oldURIiid3 =
        { 0x12120b20, 0x0929, 0x40e9,
          { 0x88, 0xcf, 0x6e, 0x08, 0x76, 0x6e, 0x8b, 0x23 }};

    if (iid.Equals(oldURIiid) ||
        iid.Equals(oldURIiid2) ||
        iid.Equals(oldURIiid3)) {
        const nsIID newURIiid = NS_IURI_IID;
        iid = newURIiid;
    }
    

    nsCOMPtr<nsISupports> object = do_CreateInstance(cid, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISerializable> serializable = do_QueryInterface(object);
    NS_ENSURE_TRUE(serializable, NS_ERROR_UNEXPECTED);

    rv = serializable->Read(this);
    NS_ENSURE_SUCCESS(rv, rv);    

    return object->QueryInterface(iid, reinterpret_cast<void**>(aObject));
}

NS_IMETHODIMP
nsBinaryInputStream::ReadID(nsID *aResult)
{
    nsresult rv = Read32(&aResult->m0);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = Read16(&aResult->m1);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = Read16(&aResult->m2);
    NS_ENSURE_SUCCESS(rv, rv);

    for (int i = 0; i < 8; ++i) {
        rv = Read8(&aResult->m3[i]);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

NS_IMETHODIMP_(char*)
nsBinaryInputStream::GetBuffer(uint32_t aLength, uint32_t aAlignMask)
{
    if (mBufferAccess)
        return mBufferAccess->GetBuffer(aLength, aAlignMask);
    return nullptr;
}

NS_IMETHODIMP_(void)
nsBinaryInputStream::PutBuffer(char* aBuffer, uint32_t aLength)
{
    if (mBufferAccess)
        mBufferAccess->PutBuffer(aBuffer, aLength);
}
