





#ifndef mozilla_dom_quota_client_h__
#define mozilla_dom_quota_client_h__

#include "mozilla/dom/quota/QuotaCommon.h"

#include "mozilla/dom/ipc/IdType.h"

#include "PersistenceType.h"

class nsIRunnable;

#define IDB_DIRECTORY_NAME "idb"
#define ASMJSCACHE_DIRECTORY_NAME "asmjs"
#define DOMCACHE_DIRECTORY_NAME "cache"

BEGIN_QUOTA_NAMESPACE

class UsageInfo;




class Client
{
public:
  NS_IMETHOD_(MozExternalRefCountType)
  AddRef() = 0;

  NS_IMETHOD_(MozExternalRefCountType)
  Release() = 0;

  enum Type {
    IDB = 0,
    
    
    ASMJS,
    DOMCACHE,
    TYPE_MAX
  };

  virtual Type
  GetType() = 0;

  static nsresult
  TypeToText(Type aType, nsAString& aText)
  {
    switch (aType) {
      case IDB:
        aText.AssignLiteral(IDB_DIRECTORY_NAME);
        break;

      case ASMJS:
        aText.AssignLiteral(ASMJSCACHE_DIRECTORY_NAME);
        break;

      case DOMCACHE:
        aText.AssignLiteral(DOMCACHE_DIRECTORY_NAME);
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
    if (aText.EqualsLiteral(IDB_DIRECTORY_NAME)) {
      aType = IDB;
    }
    else if (aText.EqualsLiteral(ASMJSCACHE_DIRECTORY_NAME)) {
      aType = ASMJS;
    }
    else if (aText.EqualsLiteral(DOMCACHE_DIRECTORY_NAME)) {
      aType = DOMCACHE;
    }
    else {
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

  
  virtual nsresult
  InitOrigin(PersistenceType aPersistenceType,
             const nsACString& aGroup,
             const nsACString& aOrigin,
             UsageInfo* aUsageInfo) = 0;

  virtual nsresult
  GetUsageForOrigin(PersistenceType aPersistenceType,
                    const nsACString& aGroup,
                    const nsACString& aOrigin,
                    UsageInfo* aUsageInfo) = 0;

  virtual void
  OnOriginClearCompleted(PersistenceType aPersistenceType,
                         const nsACString& aOrigin) = 0;

  virtual void
  ReleaseIOThreadObjects() = 0;

  
  virtual void
  AbortOperations(const nsACString& aOrigin) = 0;

  virtual void
  AbortOperationsForProcess(ContentParentId aContentParentId) = 0;

  virtual void
  PerformIdleMaintenance() = 0;

  virtual void
  ShutdownWorkThreads() = 0;

protected:
  virtual ~Client()
  { }
};

END_QUOTA_NAMESPACE

#endif 
