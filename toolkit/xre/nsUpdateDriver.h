






































#ifndef nsUpdateDriver_h__
#define nsUpdateDriver_h__

#include "nscore.h"

class nsIFile;














NS_HIDDEN_(nsresult) ProcessUpdates(nsIFile *greDir, nsIFile *appDir,
                                    nsIFile *updRootDir,
                                    int argc, char **argv);

#endif  
