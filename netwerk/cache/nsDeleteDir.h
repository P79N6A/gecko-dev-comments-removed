





































#ifndef nsDeleteDir_h__
#define nsDeleteDir_h__

#include "nsCOMPtr.h"

class nsIFile;


















NS_HIDDEN_(nsresult) DeleteDir(nsIFile *dir, PRBool moveToTrash, PRBool sync);





NS_HIDDEN_(nsresult) GetTrashDir(nsIFile *dir, nsCOMPtr<nsIFile> *result);

#endif  
