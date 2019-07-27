





#ifndef mozilla_dom_DataStoreCallbacks_h
#define mozilla_dom_DataStoreCallbacks_h

#include "nsISupports.h"

namespace mozilla {
namespace dom {

class DataStoreDB;

class DataStoreDBCallback
{
public:
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) = 0;

  enum RunStatus {
    Success,
    CreatedSchema,
    Error
  };

  virtual void Run(DataStoreDB* aDb, RunStatus aStatus) = 0;

protected:
  virtual ~DataStoreDBCallback()
  {
  }
};

class DataStoreRevisionCallback
{
public:
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) = 0;

  virtual void Run(const nsAString& aRevisionID) = 0;

protected:
  virtual ~DataStoreRevisionCallback()
  {
  }
};

} 
} 

#endif 
