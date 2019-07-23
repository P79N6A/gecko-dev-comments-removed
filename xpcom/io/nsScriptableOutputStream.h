




































#ifndef ___nsscriptableoutputstream___h_
#define ___nsscriptableoutputstream___h_

#include "nsIScriptableStreams.h"
#include "nsIOutputStream.h"
#include "nsISeekableStream.h"
#include "nsIUnicharOutputStream.h"
#include "nsCOMPtr.h"

#define NS_SCRIPTABLEOUTPUTSTREAM_CID        \
{ 0xaea1cfe2, 0xf727, 0x4b94, { 0x93, 0xff, 0x41, 0x8d, 0x96, 0x87, 0x94, 0xd1 } }

#define NS_SCRIPTABLEOUTPUTSTREAM_CONTRACTID "@mozilla.org/scriptableoutputstream;1"
#define NS_SCRIPTABLEOUTPUTSTREAM_CLASSNAME "Scriptable Output Stream"

class nsScriptableOutputStream : public nsIScriptableIOOutputStream,
                                 public nsISeekableStream,
                                 public nsIOutputStream
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOUTPUTSTREAM

  
  NS_DECL_NSISCRIPTABLEIOOUTPUTSTREAM

  
  NS_DECL_NSISEEKABLESTREAM

  
  nsScriptableOutputStream() {}

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
  ~nsScriptableOutputStream() {}

  nsresult WriteFully(const char *aBuf, PRUint32 aCount);

  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCOMPtr<nsIUnicharOutputStream> mUnicharOutputStream;
};

#endif 
