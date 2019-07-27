



#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIClipboardOwner.h"
#include "nsCOMPtr.h"

#include <qclipboard.h>


class nsClipboard : public nsIClipboard
{
public:
    nsClipboard();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSICLIPBOARD

protected:
    virtual ~nsClipboard();

    NS_IMETHOD SetNativeClipboardData(nsITransferable *aTransferable,
                                      QClipboard::Mode cbMode);
    NS_IMETHOD GetNativeClipboardData(nsITransferable *aTransferable,
                                      QClipboard::Mode cbMode);

    nsCOMPtr<nsIClipboardOwner> mSelectionOwner;
    nsCOMPtr<nsIClipboardOwner> mGlobalOwner;
    nsCOMPtr<nsITransferable>   mSelectionTransferable;
    nsCOMPtr<nsITransferable>   mGlobalTransferable;
};

#endif 
