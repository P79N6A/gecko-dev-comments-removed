









#ifndef __nsNullPrincipalURI_h__
#define __nsNullPrincipalURI_h__

#include "nsIURI.h"
#include "nsISizeOf.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"


#define NS_NULLPRINCIPALURI_IMPLEMENTATION_CID \
  {0x51fcd543, 0x3b52, 0x41f7, \
    {0xb9, 0x1b, 0x6b, 0x54, 0x10, 0x22, 0x36, 0xe6} }

class nsNullPrincipalURI MOZ_FINAL : public nsIURI
                                   , public nsISizeOf
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURI

  
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  explicit nsNullPrincipalURI(const nsCString &aSpec);

private:
  ~nsNullPrincipalURI() {}

  nsCString mScheme;
  nsCString mPath;
};

#endif 
