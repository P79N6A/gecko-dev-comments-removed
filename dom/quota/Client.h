





#ifndef mozilla_dom_quota_client_h__
#define mozilla_dom_quota_client_h__

#include "mozilla/dom/quota/QuotaCommon.h"

class nsIOfflineStorage;
class nsIRunnable;

BEGIN_QUOTA_NAMESPACE

class UsageRunnable;




class Client
{
public:
  NS_IMETHOD_(nsrefcnt)
  AddRef() = 0;

  NS_IMETHOD_(nsrefcnt)
  Release() = 0;

  enum Type {
    IDB = 0,
    
    
    TYPE_MAX
  };

  virtual Type
  GetType() = 0;

  static nsresult
  TypeToText(Type aType, nsAString& aText)
  {
    switch (aType) {
      case IDB:
        aText.AssignLiteral("idb");
        break;

      case TYPE_MAX:
      default:
        NS_NOTREACHED("Bad id value!");
        return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
  }

  static nsresult
  TypeFromText(const nsAString& aText, Type& aType)
  {
    if (aText.EqualsLiteral("idb")) {
      aType = IDB;
    }
    else {
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

  virtual nsresult
  InitOrigin(const nsACString& aOrigin, UsageRunnable* aUsageRunnable) = 0;

  virtual nsresult
  GetUsageForOrigin(const nsACString& aOrigin,
                    UsageRunnable* aUsageRunnable) = 0;

  virtual bool
  IsFileServiceUtilized() = 0;

  virtual bool
  IsTransactionServiceActivated() = 0;

  virtual void
  WaitForStoragesToComplete(nsTArray<nsIOfflineStorage*>& aStorages,
                            nsIRunnable* aCallback) = 0;

  virtual void
  AbortTransactionsForStorage(nsIOfflineStorage* aStorage) = 0;

  virtual bool
  HasTransactionsForStorage(nsIOfflineStorage* aStorage) = 0;

  virtual void
  OnOriginClearCompleted(const nsACString& aPattern) = 0;

  virtual void
  ShutdownTransactionService() = 0;

  virtual void
  OnShutdownCompleted() = 0;

protected:
  virtual ~Client()
  { }
};

END_QUOTA_NAMESPACE

#endif 
