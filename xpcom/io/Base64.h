




#ifndef mozilla_Base64_h__
#define mozilla_Base64_h__

#include "nsString.h"

class nsIInputStream;

namespace mozilla {

nsresult
Base64EncodeInputStream(nsIInputStream* aInputStream,
                        nsACString& aDest,
                        uint32_t aCount,
                        uint32_t aOffset = 0);
nsresult
Base64EncodeInputStream(nsIInputStream* aInputStream,
                        nsAString& aDest,
                        uint32_t aCount,
                        uint32_t aOffset = 0);

nsresult
Base64Encode(const nsACString& aString, nsACString& aBinary);
nsresult
Base64Encode(const nsAString& aString, nsAString& aBinaryData);

nsresult
Base64Decode(const nsACString& aBinaryData, nsACString& aString);
nsresult
Base64Decode(const nsAString& aBinaryData, nsAString& aString);

} 

#endif
