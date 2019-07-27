




#ifndef mozilla_dom_workers_WorkerStructuredClone_h
#define mozilla_dom_workers_WorkerStructuredClone_h

#include "Workers.h"
#include "mozilla/dom/PMessagePort.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class MessagePortBase;

namespace workers {


class WorkerStructuredCloneClosure final
{
private:
  WorkerStructuredCloneClosure(const WorkerStructuredCloneClosure&) = delete;
  WorkerStructuredCloneClosure & operator=(const WorkerStructuredCloneClosure&) = delete;

public:
  WorkerStructuredCloneClosure();
  ~WorkerStructuredCloneClosure();

  void Clear();

  
  nsCOMPtr<nsPIDOMWindow> mParentWindow;

  nsTArray<nsCOMPtr<nsISupports>> mClonedObjects;

  
  nsTArray<nsRefPtr<MessagePortBase>> mMessagePorts;

  
  nsTArray<MessagePortIdentifier> mMessagePortIdentifiers;

  
  nsTArray<nsRefPtr<MessagePortBase>> mTransferredPorts;
};

} 
} 
} 

#endif 
