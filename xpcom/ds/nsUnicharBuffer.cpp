




#include "nsUnicharBuffer.h"
#include "nsCRT.h"
#include "nsAutoPtr.h"

#define MIN_BUFFER_SIZE 32

UnicharBufferImpl::UnicharBufferImpl()
  : mBuffer(NULL), mSpace(0), mLength(0)
{
}

NS_METHOD
UnicharBufferImpl::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsRefPtr<UnicharBufferImpl> it = new UnicharBufferImpl();

  return it->QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
UnicharBufferImpl::Init(uint32_t aBufferSize)
{
  if (aBufferSize < MIN_BUFFER_SIZE) {
    aBufferSize = MIN_BUFFER_SIZE;
  }
  mSpace = aBufferSize;
  mLength = 0;
  mBuffer = new PRUnichar[aBufferSize];
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(UnicharBufferImpl, nsIUnicharBuffer)

UnicharBufferImpl::~UnicharBufferImpl()
{
  if (nullptr != mBuffer) {
    delete[] mBuffer;
    mBuffer = nullptr;
  }
  mLength = 0;
}

NS_IMETHODIMP_(int32_t)
UnicharBufferImpl::GetLength() const
{
  return mLength;
}

NS_IMETHODIMP_(int32_t)
UnicharBufferImpl::GetBufferSize() const
{
  return mSpace;
}

NS_IMETHODIMP_(PRUnichar*)
UnicharBufferImpl::GetBuffer() const
{
  return mBuffer;
}

NS_IMETHODIMP_(bool)
UnicharBufferImpl::Grow(int32_t aNewSize)
{
  if (uint32_t(aNewSize) < MIN_BUFFER_SIZE) {
    aNewSize = MIN_BUFFER_SIZE;
  }
  PRUnichar* newbuf = new PRUnichar[aNewSize];
  if (nullptr != newbuf) {
    if (0 != mLength) {
      memcpy(newbuf, mBuffer, mLength * sizeof(PRUnichar));
    }
    delete[] mBuffer;
    mBuffer = newbuf;
    return true;
  }
  return false;
}

nsresult
NS_NewUnicharBuffer(nsIUnicharBuffer** aInstancePtrResult,
                    nsISupports* aOuter,
                    uint32_t aBufferSize)
{
  nsresult rv;
  nsIUnicharBuffer* buf;
  rv = UnicharBufferImpl::Create(aOuter, NS_GET_IID(nsIUnicharBuffer), 
                                 (void**)&buf);
  if (NS_FAILED(rv)) return rv;
  rv = buf->Init(aBufferSize);
  if (NS_FAILED(rv)) {
    NS_RELEASE(buf);
    return rv;
  }
  *aInstancePtrResult = buf;
  return rv;
}
