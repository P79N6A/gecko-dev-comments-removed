




#ifndef nsUnicharBuffer_h__
#define nsUnicharBuffer_h__

#include "nsIUnicharBuffer.h"
#include "mozilla/Attributes.h"

class UnicharBufferImpl MOZ_FINAL : public nsIUnicharBuffer {
public:
  UnicharBufferImpl();

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  NS_DECL_ISUPPORTS
  NS_IMETHOD Init(uint32_t aBufferSize);
  NS_IMETHOD_(int32_t) GetLength() const;
  NS_IMETHOD_(int32_t) GetBufferSize() const;
  NS_IMETHOD_(PRUnichar*) GetBuffer() const;
  NS_IMETHOD_(bool) Grow(int32_t aNewSize);

  PRUnichar* mBuffer;
  uint32_t mSpace;
  uint32_t mLength;

private:
  ~UnicharBufferImpl();
};

#endif 
