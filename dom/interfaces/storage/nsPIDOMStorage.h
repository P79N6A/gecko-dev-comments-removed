






































#ifndef __nsPIDOMStorage_h_
#define __nsPIDOMStorage_h_

#include "nsISupports.h"
#include "nsTArray.h"

class nsIDOMStorageObsolete;
class nsIURI;
class nsIPrincipal;

#define NS_PIDOMSTORAGE_IID                                 \
  { 0x5ffbee8d, 0x9a86, 0x4a57,                           \
      { 0x8c, 0x63, 0x76, 0x56, 0x18, 0x9c, 0xb2, 0xbc } }

class nsPIDOMStorage : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMSTORAGE_IID)

  virtual nsresult InitAsSessionStorage(nsIPrincipal *aPrincipal) = 0;
  virtual nsresult InitAsLocalStorage(nsIPrincipal *aPrincipal) = 0;
  virtual nsresult InitAsGlobalStorage(const nsACString &aDomainDemanded) = 0;

  virtual already_AddRefed<nsIDOMStorage> Clone() = 0;

  virtual nsTArray<nsString> *GetKeys() = 0;

  virtual nsIPrincipal* Principal() = 0;
  virtual PRBool CanAccess(nsIPrincipal *aPrincipal) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMStorage, NS_PIDOMSTORAGE_IID)

#endif 
