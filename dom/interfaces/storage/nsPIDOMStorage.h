






































#ifndef __nsPIDOMStorage_h_
#define __nsPIDOMStorage_h_

#include "nsISupports.h"
#include "nsTArray.h"

class nsIDOMStorageObsolete;
class nsIURI;
class nsIPrincipal;


#define NS_PIDOMSTORAGE_IID                                 \
  { 0xbaffceb1, 0xfd40, 0x4ea9,  \
    { 0x83, 0x78, 0x35, 0x9, 0xdd, 0x79, 0x20, 0x4a } }

class nsPIDOMStorage : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMSTORAGE_IID)

  typedef enum {
    Unknown = 0,
    GlobalStorage = 1,
    LocalStorage = 2,
    SessionStorage = 3
  } nsDOMStorageType;

  virtual nsresult InitAsSessionStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI) = 0;
  virtual nsresult InitAsLocalStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI) = 0;
  virtual nsresult InitAsGlobalStorage(const nsACString &aDomainDemanded) = 0;

  virtual already_AddRefed<nsIDOMStorage> Clone() = 0;
  virtual already_AddRefed<nsIDOMStorage> Fork(const nsSubstring &aDocumentURI) = 0;
  virtual bool IsForkOf(nsIDOMStorage* aThat) = 0;

  virtual nsTArray<nsString> *GetKeys() = 0;

  virtual nsIPrincipal* Principal() = 0;
  virtual bool CanAccess(nsIPrincipal *aPrincipal) = 0;

  virtual nsDOMStorageType StorageType() = 0;

  virtual void BroadcastChangeNotification(const nsSubstring &aKey,
                                           const nsSubstring &aOldValue,
                                           const nsSubstring &aNewValue) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMStorage, NS_PIDOMSTORAGE_IID)

#endif 
