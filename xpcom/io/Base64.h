




































#ifndef mozilla_Base64_h__
#define mozilla_Base64_h__

#include "nsString.h"

class nsIInputStream;

namespace mozilla {

nsresult
Base64EncodeInputStream(nsIInputStream *aInputStream, 
                        nsACString &aDest,
                        PRUint32 aCount,
                        PRUint32 aOffset = 0);
nsresult
Base64EncodeInputStream(nsIInputStream *aInputStream, 
                        nsAString &aDest,
                        PRUint32 aCount,
                        PRUint32 aOffset = 0);

} 

#endif
