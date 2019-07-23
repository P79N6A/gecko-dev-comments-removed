





































#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsIDragService.h"
#include <qdrag.h>


class nsDragService : public nsIDragService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDRAGSERVICE

    nsDragService();

private:
    ~nsDragService();

protected:
  
};

#endif
