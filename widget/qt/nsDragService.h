




#ifndef nsDragService_h__
#define nsDragService_h__

#include <qdrag.h>

#include "nsBaseDragService.h"


class nsDragService : public nsBaseDragService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDRAGSERVICE

    nsDragService();

private:
    ~nsDragService();

protected:
    
    NS_IMETHODIMP SetupDragSession(nsISupportsArray *aTransferables, uint32_t aActionType);
    NS_IMETHODIMP SetDropActionType(uint32_t aActionType);
    NS_IMETHODIMP ExecuteDrag();

    QDrag *mDrag;
    Qt::DropActions mDropAction;
    QWidget *mHiddenWidget;
};

#endif 
