





































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
    
    NS_IMETHODIMP SetupDragSession(nsISupportsArray *aTransferables, PRUint32 aActionType);
    NS_IMETHODIMP SetDropActionType(PRUint32 aActionType);
    NS_IMETHODIMP ExecuteDrag();

    QDrag *mDrag;
    Qt::DropActions mDropAction;
    QWidget *mHiddenWidget;
};

#endif 
