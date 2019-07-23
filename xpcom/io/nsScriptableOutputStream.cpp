




































#include "nsScriptableOutputStream.h"
#include "nsIProgrammingLanguage.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsIClassInfoImpl.h"


#include "nsIStreamBufferAccess.h"

NS_IMPL_THREADSAFE_ADDREF(nsScriptableOutputStream)
NS_IMPL_THREADSAFE_RELEASE(nsScriptableOutputStream)
NS_IMPL_QUERY_INTERFACE3_CI(nsScriptableOutputStream,
                            nsIOutputStream,
                            nsIScriptableIOOutputStream,
                            nsISeekableStream)
NS_IMPL_CI_INTERFACE_GETTER3(nsScriptableOutputStream,
                             nsIOutputStream,
                             nsIScriptableIOOutputStream,
                             nsISeekableStream)


NS_IMETHODIMP
nsScriptableOutputStream::Close(void)
{
  if (mUnicharOutputStream)
    return mUnicharOutputStream->Close();

  if (mOutputStream)
    return mOutputStream->Close();

  return NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP
nsScriptableOutputStream::Flush(void)
{
  if (mUnicharOutputStream)
    return mUnicharOutputStream->Flush();

  if (mOutputStream)
    return mOutputStream->Flush();

  return NS_ERROR_NOT_INITIALIZED;
}



NS_IMETHODIMP
nsScriptableOutputStream::InitWithStreams(nsIOutputStream* aOutputStream,
                                          nsIUnicharOutputStream *aCharStream)
{
  NS_ENSURE_ARG(aOutputStream);

  mOutputStream = aOutputStream;
  mUnicharOutputStream = aCharStream;
  return NS_OK;
}



NS_IMETHODIMP
nsScriptableOutputStream::IsNonBlocking(PRBool *aIsNonBlocking)
{
  if (mOutputStream)
    return mOutputStream->IsNonBlocking(aIsNonBlocking);

  if (mUnicharOutputStream) {
    *aIsNonBlocking = PR_FALSE;
    return NS_OK;
  }

  return NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP
nsScriptableOutputStream::Write(const char* aBuffer, PRUint32 aCount, PRUint32 *aWriteCount)
{
  if (mUnicharOutputStream) {
    nsAutoString str(NS_ConvertASCIItoUTF16(aBuffer, aCount));
    PRBool ok;
    mUnicharOutputStream->WriteString(str, &ok);
    *aWriteCount = ok ? aCount : 0;
    return NS_OK;
  }

  if (mOutputStream)
    return mOutputStream->Write(aBuffer, aCount, aWriteCount);

  return NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP
nsScriptableOutputStream::WriteFrom(nsIInputStream *aStream, PRUint32 aCount, PRUint32 *aWriteCount)
{
  if (mOutputStream)
    return mOutputStream->WriteFrom(aStream, aCount, aWriteCount);

  return NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP
nsScriptableOutputStream::WriteSegments(nsReadSegmentFun aFn,
                                        void* aClosure,
                                        PRUint32 aCount,
                                        PRUint32 *aReadCount)
{
  NS_ENSURE_TRUE(mOutputStream, NS_ERROR_NOT_INITIALIZED);
  return mOutputStream->WriteSegments(aFn, aClosure, aCount, aReadCount);
}

NS_IMETHODIMP
nsScriptableOutputStream::WriteString(const nsAString& aString, PRBool *aOK)
{
  if (mUnicharOutputStream)
    return mUnicharOutputStream->WriteString(aString, aOK);

  if (!mOutputStream)
    return NS_ERROR_NOT_INITIALIZED;

  
  nsCAutoString cstr = NS_LossyConvertUTF16toASCII(aString);
  PRUint32 count;
  nsresult rv = mOutputStream->Write(cstr.get(), (PRUint32)cstr.Length(), &count);
  NS_ENSURE_SUCCESS(rv, rv);
  *aOK = (count == cstr.Length());
  return NS_OK;
}

NS_IMETHODIMP
nsScriptableOutputStream::WriteBoolean(PRBool aBoolean)
{
  return Write8(aBoolean);
}

NS_IMETHODIMP
nsScriptableOutputStream::Write8(PRUint8 aByte)
{
  return WriteFully((const char *)&aByte, sizeof aByte);
}

NS_IMETHODIMP
nsScriptableOutputStream::Write16(PRUint16 a16)
{
  a16 = NS_SWAP16(a16);
  return WriteFully((const char *)&a16, sizeof a16);
}

NS_IMETHODIMP
nsScriptableOutputStream::Write32(PRUint32 a32)
{
  a32 = NS_SWAP32(a32);
  return WriteFully((const char *)&a32, sizeof a32);
}

NS_IMETHODIMP
nsScriptableOutputStream::WriteFloat(float aFloat)
{
  NS_ASSERTION(sizeof(float) == sizeof (PRUint32),
               "False assumption about sizeof(float)");
  return Write32(*NS_REINTERPRET_CAST(PRUint32*, &aFloat));
}

NS_IMETHODIMP
nsScriptableOutputStream::WriteDouble(double aDouble)
{
  NS_ASSERTION(sizeof(double) == sizeof(PRUint64),
               "False assumption about sizeof(double)");

  PRUint64 val = NS_SWAP64(*NS_REINTERPRET_CAST(PRUint64*, &aDouble));
  return WriteFully(NS_REINTERPRET_CAST(char*, &val), sizeof val);
}

NS_IMETHODIMP
nsScriptableOutputStream::WriteByteArray(PRUint8 *aBytes, PRUint32 aCount)
{
  return WriteFully((char *)aBytes, aCount);
}

nsresult
nsScriptableOutputStream::WriteFully(const char *aBuf, PRUint32 aCount)
{
  NS_ENSURE_TRUE(mOutputStream, NS_ERROR_NOT_INITIALIZED);

  PRUint32 bytesWritten;
  nsresult rv = mOutputStream->Write(aBuf, aCount, &bytesWritten);
  NS_ENSURE_SUCCESS(rv, rv);
  return (bytesWritten != aCount) ? NS_ERROR_FAILURE : NS_OK;
}


NS_IMETHODIMP
nsScriptableOutputStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
  nsCOMPtr<nsISeekableStream> cstream = do_QueryInterface(mOutputStream);
  NS_ENSURE_TRUE(cstream, NS_ERROR_NOT_AVAILABLE);
  return cstream->Seek(aWhence, aOffset);
}

NS_IMETHODIMP
nsScriptableOutputStream::Tell(PRInt64* aOffset)
{
  nsCOMPtr<nsISeekableStream> cstream = do_QueryInterface(mOutputStream);
  NS_ENSURE_TRUE(cstream, NS_ERROR_NOT_AVAILABLE);
  return cstream->Tell(aOffset);
}

NS_IMETHODIMP
nsScriptableOutputStream::SetEOF()
{
  nsCOMPtr<nsISeekableStream> cstream = do_QueryInterface(mOutputStream);
  NS_ENSURE_TRUE(cstream, NS_ERROR_NOT_AVAILABLE);
  return cstream->SetEOF();
}

NS_METHOD
nsScriptableOutputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter) return NS_ERROR_NO_AGGREGATION;

  nsRefPtr<nsScriptableOutputStream> sos = new nsScriptableOutputStream();
  NS_ENSURE_TRUE(sos, NS_ERROR_OUT_OF_MEMORY);
  return sos->QueryInterface(aIID, aResult);
}
