




































#ifndef nsByteBuffer_h__
#define nsByteBuffer_h__

#include "nsIByteBuffer.h"

class ByteBufferImpl : public nsIByteBuffer {
public:
  ByteBufferImpl(void);

  NS_DECL_ISUPPORTS

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  NS_IMETHOD Init(PRUint32 aBufferSize);
  NS_IMETHOD_(PRUint32) GetLength(void) const;
  NS_IMETHOD_(PRUint32) GetBufferSize(void) const;
  NS_IMETHOD_(char*) GetBuffer() const;
  NS_IMETHOD_(PRBool) Grow(PRUint32 aNewSize);
  NS_IMETHOD_(PRInt32) Fill(nsresult* aErrorCode, nsIInputStream* aStream,
                            PRUint32 aKeep);

  char* mBuffer;
  PRUint32 mSpace;
  PRUint32 mLength;
private:
  ~ByteBufferImpl();
};

#endif 
