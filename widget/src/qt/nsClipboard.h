




































#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIClipboardOwner.h"
#include "nsCOMPtr.h"

#include <qcstring.h>
#include <qmime.h>


class nsClipboard : public nsIClipboard
{
public:
    nsClipboard();
    virtual ~nsClipboard();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSICLIPBOARD

protected:
    NS_IMETHOD SetNativeClipboardData(PRInt32 aWhichClipboard);
    NS_IMETHOD GetNativeClipboardData(nsITransferable *aTransferable,
                                      PRInt32 aWhichClipboard);

    nsITransferable *GetTransferable(PRInt32 aWhichClipboard);

    PRBool mIgnoreEmptyNotification;

    nsCOMPtr<nsIClipboardOwner> mSelectionOwner;
    nsCOMPtr<nsIClipboardOwner> mGlobalOwner;
    nsCOMPtr<nsITransferable>   mSelectionTransferable;
    nsCOMPtr<nsITransferable>   mGlobalTransferable;
};

#endif 
