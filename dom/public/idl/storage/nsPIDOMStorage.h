






































#ifndef __nsPIDOMStorage_h_
#define __nsPIDOMStorage_h_

#include "nsISupports.h"
#include "nsTArray.h"

class nsIDOMStorage;
class nsIURI;

#define NS_PIDOMSTORAGE_IID                                 \
  { 0x2fdbb82e, 0x4b47, 0x406a,                             \
      { 0xb1, 0x17, 0x6d, 0x67, 0x58, 0xc1, 0xee, 0x6b } }

class nsPIDOMStorage : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMSTORAGE_IID)

  virtual void Init(nsIURI* aURI, const nsAString &aDomain, PRBool aUseDB) = 0;

  virtual already_AddRefed<nsIDOMStorage> Clone(nsIURI* aURI) = 0;

  virtual nsTArray<nsString> *GetKeys() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMStorage, NS_PIDOMSTORAGE_IID)

#endif 
