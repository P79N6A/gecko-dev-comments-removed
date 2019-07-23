




































#ifndef nsIByteBuffer_h___
#define nsIByteBuffer_h___

#include "nscore.h"
#include "nsISupports.h"

class nsIInputStream;

#define NS_IBYTE_BUFFER_IID    \
{ 0xe4a6e4b0, 0x93b4, 0x11d1, \
  {0x89, 0x5b, 0x00, 0x60, 0x08, 0x91, 0x1b, 0x81} }
#define NS_IBYTEBUFFER_IID    \
{ 0xe4a6e4b0, 0x93b4, 0x11d1, \
  {0x89, 0x5b, 0x00, 0x60, 0x08, 0x91, 0x1b, 0x81} }
#define NS_BYTEBUFFER_CONTRACTID "@mozilla.org/byte-buffer;1"
#define NS_BYTEBUFFER_CLASSNAME "Byte Buffer"


class nsIByteBuffer : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBYTEBUFFER_IID)

  NS_IMETHOD Init(PRUint32 aBufferSize) = 0;

  
  NS_IMETHOD_(PRUint32) GetLength(void) const = 0;

  
  NS_IMETHOD_(PRUint32) GetBufferSize(void) const = 0;

  
  NS_IMETHOD_(char*) GetBuffer(void) const = 0;

  
  NS_IMETHOD_(PRBool) Grow(PRUint32 aNewSize) = 0;

  

  NS_IMETHOD_(PRInt32) Fill(nsresult* aErrorCode, nsIInputStream* aStream,
                            PRUint32 aKeep) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIByteBuffer, NS_IBYTEBUFFER_IID)

#define NS_BYTEBUFFER_CID                            \
{ /* a49d5280-0d6b-11d3-9331-00104ba0fd40 */         \
    0xa49d5280,                                      \
    0x0d6b,                                          \
    0x11d3,                                          \
    {0x93, 0x31, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}


extern NS_COM nsresult 
NS_NewByteBuffer(nsIByteBuffer** aInstancePtrResult,
                 nsISupports* aOuter,
                 PRUint32 aBufferSize = 0);

#endif 

