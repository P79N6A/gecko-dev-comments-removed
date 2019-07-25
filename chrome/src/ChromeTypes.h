





































#ifndef ChromeTypes_h__
#define ChromeTypes_h__

#ifndef MOZILLA_INTERNAL_API
#include "nsStringAPI.h"
#else
#include "nsString.h"
#endif

#include "nsCOMPtr.h"
#include "nsIURI.h"

struct ChromePackage {
  nsCString package;
  nsCOMPtr<nsIURI> baseURI;
  PRUint32 flags;
};

struct ChromeResource {
  nsCString package;
  nsCOMPtr<nsIURI> resolvedURI;
};

#endif 
