









#ifndef __nsNullPrincipalURI_h__
#define __nsNullPrincipalURI_h__

#include "nsIURI.h"
#include "nsISizeOf.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "nsIIPCSerializableURI.h"
#include "mozilla/MemoryReporting.h"
#include "nsNullPrincipal.h"
#include "nsID.h"


#define NS_NULLPRINCIPALURI_IMPLEMENTATION_CID \
  {0x51fcd543, 0x3b52, 0x41f7, \
    {0xb9, 0x1b, 0x6b, 0x54, 0x10, 0x22, 0x36, 0xe6} }

class nsNullPrincipalURI final : public nsIURI
                               , public nsISizeOf
                               , public nsIIPCSerializableURI
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURI
  NS_DECL_NSIIPCSERIALIZABLEURI

  
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const override;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const override;

  
  
  nsNullPrincipalURI();

  
  static already_AddRefed<nsNullPrincipalURI> Create();

private:
  nsNullPrincipalURI(const nsNullPrincipalURI& aOther);

  ~nsNullPrincipalURI() {}

  nsresult Init();

  char mPathBytes[NSID_LENGTH];
  nsFixedCString mPath;
};

#endif 
