




































#ifndef nsUnicharBuffer_h__
#define nsUnicharBuffer_h__

#include "nsIUnicharBuffer.h"

class UnicharBufferImpl : public nsIUnicharBuffer {
public:
  UnicharBufferImpl();

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  NS_DECL_ISUPPORTS
  NS_IMETHOD Init(PRUint32 aBufferSize);
  NS_IMETHOD_(PRInt32) GetLength() const;
  NS_IMETHOD_(PRInt32) GetBufferSize() const;
  NS_IMETHOD_(PRUnichar*) GetBuffer() const;
  NS_IMETHOD_(PRBool) Grow(PRInt32 aNewSize);

  PRUnichar* mBuffer;
  PRUint32 mSpace;
  PRUint32 mLength;

private:
  ~UnicharBufferImpl();
};

#endif 
