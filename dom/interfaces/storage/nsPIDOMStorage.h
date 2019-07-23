






































#ifndef __nsPIDOMStorage_h_
#define __nsPIDOMStorage_h_

#include "nsISupports.h"
#include "nsTArray.h"

class nsIDOMStorageObsolete;
class nsIURI;
class nsIPrincipal;

#define NS_PIDOMSTORAGE_IID                                 \
  { 0x3231d539, 0xdd51, 0x4451,                             \
      { 0xba, 0xdc, 0xf7, 0x72, 0xf5, 0xda, 0x1c, 0x8a } }

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
