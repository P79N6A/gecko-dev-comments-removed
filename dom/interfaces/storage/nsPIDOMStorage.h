





#ifndef __nsPIDOMStorage_h_
#define __nsPIDOMStorage_h_

#include "nsISupports.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIPrincipal;

namespace mozilla {
namespace dom {

class DOMStorageCache;
class DOMStorageManager;

} 
} 


#define NS_PIDOMSTORAGE_IID \
  { 0x9198a51, 0x5d27, 0x4992, \
    { 0x97, 0xe4, 0x38, 0xa9, 0xce, 0xa2, 0xa6, 0x5d } }

class nsPIDOMStorage : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMSTORAGE_IID)

  enum StorageType {
    LocalStorage = 1,
    SessionStorage = 2
  };

  virtual StorageType GetType() const = 0;
  virtual mozilla::dom::DOMStorageManager* GetManager() const = 0;
  virtual const mozilla::dom::DOMStorageCache* GetCache() const = 0;

  virtual nsTArray<nsString>* GetKeys() = 0;

  virtual nsIPrincipal* GetPrincipal() = 0;
  virtual bool PrincipalEquals(nsIPrincipal* principal) = 0;
  virtual bool CanAccess(nsIPrincipal *aPrincipal) = 0;
  virtual bool IsPrivate() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMStorage, NS_PIDOMSTORAGE_IID)

#endif 
