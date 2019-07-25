





































#ifndef nsDeleteDir_h__
#define nsDeleteDir_h__

#include "nsCOMPtr.h"

class nsIFile;
























NS_HIDDEN_(nsresult) DeleteDir(nsIFile *dir, bool moveToTrash, bool sync, 
                               PRUint32 delay = 0);





NS_HIDDEN_(nsresult) GetTrashDir(nsIFile *dir, nsCOMPtr<nsIFile> *result);

#endif  
