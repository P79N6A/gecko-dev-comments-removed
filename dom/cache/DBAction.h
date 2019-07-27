





#ifndef mozilla_dom_cache_DBAction_h
#define mozilla_dom_cache_DBAction_h

#include "mozilla/dom/cache/Action.h"
#include "mozilla/dom/cache/CacheInitData.h"
#include "nsRefPtr.h"
#include "nsString.h"

class mozIStorageConnection;
class nsIFile;

namespace mozilla {
namespace dom {
namespace cache {

class DBAction : public Action
{
protected:
  
  
  enum Mode
  {
    Existing,
    Create
  };

  explicit DBAction(Mode aMode);

  
  virtual ~DBAction();

  
  
  
  virtual void
  RunWithDBOnTarget(Resolver* aResolver, const QuotaInfo& aQuotaInfo,
                    nsIFile* aDBDir, mozIStorageConnection* aConn) = 0;

private:
  virtual void
  RunOnTarget(Resolver* aResolver, const QuotaInfo& aQuotaInfo) override;

  nsresult OpenConnection(const QuotaInfo& aQuotaInfo, nsIFile* aQuotaDir,
                          mozIStorageConnection** aConnOut);

  nsresult WipeDatabase(nsIFile* aDBFile, nsIFile* aDBDir);

  const Mode mMode;
};

class SyncDBAction : public DBAction
{
protected:
  explicit SyncDBAction(Mode aMode);

  
  virtual ~SyncDBAction();

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) = 0;

private:
  virtual void
  RunWithDBOnTarget(Resolver* aResolver, const QuotaInfo& aQuotaInfo,
                    nsIFile* aDBDir, mozIStorageConnection* aConn) override;
};

} 
} 
} 

#endif
