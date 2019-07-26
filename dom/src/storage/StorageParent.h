





#ifndef mozilla_dom_StorageParent_h
#define mozilla_dom_StorageParent_h

#include "mozilla/dom/PStorageParent.h"
#include "nsDOMStorage.h"

namespace mozilla {
namespace dom {

class StorageConstructData;

class StorageParent : public PStorageParent
{
public:
  StorageParent(const StorageConstructData& aData);

private:
  bool RecvGetKeys(const bool& aCallerSecure, InfallibleTArray<nsString>* aKeys);
  bool RecvGetLength(const bool& aCallerSecure, const bool& aSessionOnly,
                     uint32_t* aLength, nsresult* rv);
  bool RecvGetKey(const bool& aCallerSecure, const bool& aSessionOnly,
                  const uint32_t& aIndex,nsString* aKey, nsresult* rv);
  bool RecvGetValue(const bool& aCallerSecure, const bool& aSessionOnly,
                    const nsString& aKey, StorageItem* aItem, nsresult* rv);
  bool RecvSetValue(const bool& aCallerSecure, const bool& aSessionOnly,
                    const nsString& aKey, const nsString& aData,
                    nsString* aOldValue, nsresult* rv);
  bool RecvRemoveValue(const bool& aCallerSecure, const bool& aSessionOnly,
                       const nsString& aKey, nsString* aOldData, nsresult* rv);
  bool RecvClear(const bool& aCallerSecure, const bool& aSessionOnly,
                 int32_t* aOldCount, nsresult* rv);

  bool RecvGetDBValue(const nsString& aKey, nsString* aValue, bool* aSecure,
                      nsresult* rv);
  bool RecvSetDBValue(const nsString& aKey, const nsString& aValue,
                      const bool& aSecure, nsresult* rv);
  bool RecvSetSecure(const nsString& aKey, const bool& aSecure, nsresult* rv);

  bool RecvInit(const bool& aUseDB,
                const bool& aCanUseChromePersist,
                const bool& aSessionOnly,
                const bool& aPrivate,
                const nsCString& aDomain,
                const nsCString& aScopeDBKey,
                const nsCString& aQuotaDomainDBKey,
                const nsCString& aQuotaETLDplus1DomainDBKey,
                const uint32_t& aStorageType);

  bool RecvUpdatePrivateState(const bool& aEnabled);

  nsRefPtr<DOMStorageImpl> mStorage;
};

}
}

#endif
