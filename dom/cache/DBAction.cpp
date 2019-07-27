





#include "mozilla/dom/cache/DBAction.h"

#include "mozilla/dom/quota/PersistenceType.h"
#include "mozilla/net/nsFileProtocolHandler.h"
#include "mozIStorageConnection.h"
#include "mozIStorageService.h"
#include "mozStorageCID.h"
#include "nsIFile.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "DBSchema.h"
#include "FileUtils.h"

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::dom::quota::PERSISTENCE_TYPE_DEFAULT;
using mozilla::dom::quota::PersistenceType;

DBAction::DBAction(Mode aMode)
  : mMode(aMode)
{
}

DBAction::~DBAction()
{
}

void
DBAction::RunOnTarget(Resolver* aResolver, const QuotaInfo& aQuotaInfo)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aResolver);
  MOZ_ASSERT(aQuotaInfo.mDir);

  if (IsCanceled()) {
    aResolver->Resolve(NS_ERROR_ABORT);
    return;
  }

  nsCOMPtr<nsIFile> dbDir;
  nsresult rv = aQuotaInfo.mDir->Clone(getter_AddRefs(dbDir));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aResolver->Resolve(rv);
    return;
  }

  rv = dbDir->Append(NS_LITERAL_STRING("cache"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aResolver->Resolve(rv);
    return;
  }

  nsCOMPtr<mozIStorageConnection> conn;
  rv = OpenConnection(aQuotaInfo, dbDir, getter_AddRefs(conn));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aResolver->Resolve(rv);
    return;
  }
  MOZ_ASSERT(conn);

  RunWithDBOnTarget(aResolver, aQuotaInfo, dbDir, conn);
}

nsresult
DBAction::OpenConnection(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                         mozIStorageConnection** aConnOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aDBDir);
  MOZ_ASSERT(aConnOut);

  nsCOMPtr<mozIStorageConnection> conn;

  bool exists;
  nsresult rv = aDBDir->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (!exists) {
    if (NS_WARN_IF(mMode != Create)) {  return NS_ERROR_FILE_NOT_FOUND; }
    rv = aDBDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  nsCOMPtr<nsIFile> dbFile;
  rv = aDBDir->Clone(getter_AddRefs(dbFile));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = dbFile->Append(NS_LITERAL_STRING("caches.sqlite"));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = dbFile->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  
  
  
  nsRefPtr<nsFileProtocolHandler> handler = new nsFileProtocolHandler();
  rv = handler->Init();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsCOMPtr<nsIURI> uri;
  rv = handler->NewFileURI(dbFile, getter_AddRefs(uri));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsCOMPtr<nsIFileURL> dbFileUrl = do_QueryInterface(uri);
  if (NS_WARN_IF(!dbFileUrl)) { return NS_ERROR_UNEXPECTED; }

  nsAutoCString type;
  PersistenceTypeToText(PERSISTENCE_TYPE_DEFAULT, type);

  rv = dbFileUrl->SetQuery(
    NS_LITERAL_CSTRING("persistenceType=") + type +
    NS_LITERAL_CSTRING("&group=") + aQuotaInfo.mGroup +
    NS_LITERAL_CSTRING("&origin=") + aQuotaInfo.mOrigin);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsCOMPtr<mozIStorageService> ss =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  if (NS_WARN_IF(!ss)) { return NS_ERROR_UNEXPECTED; }

  rv = ss->OpenDatabaseWithFileURL(dbFileUrl, getter_AddRefs(conn));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    NS_WARNING("Cache database corrupted. Recreating empty database.");

    conn = nullptr;

    
    
    rv = WipeDatabase(dbFile, aDBDir);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = ss->OpenDatabaseWithFileURL(dbFileUrl, getter_AddRefs(conn));
  }
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  int32_t schemaVersion = 0;
  rv = conn->GetSchemaVersion(&schemaVersion);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  if (schemaVersion > 0 && schemaVersion < DBSchema::kMaxWipeSchemaVersion) {
    conn = nullptr;
    rv = WipeDatabase(dbFile, aDBDir);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = ss->OpenDatabaseWithFileURL(dbFileUrl, getter_AddRefs(conn));
  }

  rv = DBSchema::InitializeConnection(conn);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  conn.forget(aConnOut);

  return rv;
}

nsresult
DBAction::WipeDatabase(nsIFile* aDBFile, nsIFile* aDBDir)
{
  nsresult rv = aDBFile->Remove(false);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  rv = FileUtils::BodyDeleteDir(aDBDir);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return rv;
}

SyncDBAction::SyncDBAction(Mode aMode)
  : DBAction(aMode)
{
}

SyncDBAction::~SyncDBAction()
{
}

void
SyncDBAction::RunWithDBOnTarget(Resolver* aResolver,
                                const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                                mozIStorageConnection* aConn)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aResolver);
  MOZ_ASSERT(aDBDir);
  MOZ_ASSERT(aConn);

  nsresult rv = RunSyncWithDBOnTarget(aQuotaInfo, aDBDir, aConn);
  aResolver->Resolve(rv);
}

} 
} 
} 
