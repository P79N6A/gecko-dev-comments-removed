




#ifndef NSDRAGSERVICEPROXY_H
#define NSDRAGSERVICEPROXY_H

#include "nsBaseDragService.h"

class nsDragServiceProxy : public nsBaseDragService
{
public:
  nsDragServiceProxy();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD InvokeDragSession(nsIDOMNode* aDOMNode,
                               nsISupportsArray* anArrayTransferables,
                               nsIScriptableRegion* aRegion,
                               uint32_t aActionType);
private:
  virtual ~nsDragServiceProxy();
};

#endif 
