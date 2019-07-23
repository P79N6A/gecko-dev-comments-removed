






































#ifndef __nsPIDOMStorage_h_
#define __nsPIDOMStorage_h_

#include "nsISupports.h"
#include "nsTArray.h"

class nsIDOMStorageObsolete;
class nsIURI;
class nsIPrincipal;

#define NS_PIDOMSTORAGE_IID                                 \
  { 0x2cbaea60, 0x69e7, 0x4b49,                             \
      { 0xa2, 0xe2, 0x99, 0x53, 0xf4, 0x11, 0xd0, 0x8f } }

class nsPIDOMStorage : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMSTORAGE_IID)

  virtual nsresult InitAsLocalStorage(nsIPrincipal *aPrincipal) = 0;
  virtual nsresult InitAsGlobalStorage(const nsACString &aDomainDemanded) = 0;
  virtual nsresult InitAsSessionStorage(nsIURI* aURI) = 0;

  virtual already_AddRefed<nsIDOMStorageObsolete> Clone() = 0;

  virtual nsTArray<nsString> *GetKeys() = 0;

  virtual const nsCString &Domain() = 0;
  virtual PRBool CanAccess(nsIPrincipal *aPrincipal) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMStorage, NS_PIDOMSTORAGE_IID)

#endif 
