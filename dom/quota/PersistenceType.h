





#ifndef mozilla_dom_quota_persistencetype_h__
#define mozilla_dom_quota_persistencetype_h__

#include "mozilla/dom/quota/QuotaCommon.h"

#include "mozilla/dom/StorageTypeBinding.h"

BEGIN_QUOTA_NAMESPACE

enum PersistenceType
{
  PERSISTENCE_TYPE_PERSISTENT = 0,
  PERSISTENCE_TYPE_TEMPORARY,

  
  PERSISTENCE_TYPE_INVALID
};

inline void
PersistenceTypeToText(PersistenceType aPersistenceType, nsACString& aText)
{
  switch (aPersistenceType) {
    case PERSISTENCE_TYPE_PERSISTENT:
      aText.AssignLiteral("persistent");
      return;
    case PERSISTENCE_TYPE_TEMPORARY:
      aText.AssignLiteral("temporary");
      return;

    case PERSISTENCE_TYPE_INVALID:
    default:
      MOZ_CRASH("Bad persistence type value!");
  }

  MOZ_ASSUME_UNREACHABLE("Should never get here!");
}

inline PersistenceType
PersistenceTypeFromText(const nsACString& aText)
{
  if (aText.EqualsLiteral("persistent")) {
    return PERSISTENCE_TYPE_PERSISTENT;
  }

  if (aText.EqualsLiteral("temporary")) {
    return PERSISTENCE_TYPE_TEMPORARY;
  }

  MOZ_ASSUME_UNREACHABLE("Should never get here!");
}

inline nsresult
NullablePersistenceTypeFromText(const nsACString& aText,
                                Nullable<PersistenceType> *aPersistenceType)
{
  if (aText.IsVoid()) {
    *aPersistenceType = Nullable<PersistenceType>();
    return NS_OK;
  }

  if (aText.EqualsLiteral("persistent")) {
    *aPersistenceType = Nullable<PersistenceType>(PERSISTENCE_TYPE_PERSISTENT);
    return NS_OK;
  }

  if (aText.EqualsLiteral("temporary")) {
    *aPersistenceType = Nullable<PersistenceType>(PERSISTENCE_TYPE_TEMPORARY);
    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
}

inline mozilla::dom::StorageType
PersistenceTypeToStorage(PersistenceType aPersistenceType)
{
  return mozilla::dom::StorageType(static_cast<int>(aPersistenceType));
}

inline PersistenceType
PersistenceTypeFromStorage(const Optional<mozilla::dom::StorageType>& aStorage,
                           PersistenceType aDefaultPersistenceType)
{
  if (aStorage.WasPassed()) {
    return PersistenceType(static_cast<int>(aStorage.Value()));
  }

  return aDefaultPersistenceType;
}

END_QUOTA_NAMESPACE

#endif
