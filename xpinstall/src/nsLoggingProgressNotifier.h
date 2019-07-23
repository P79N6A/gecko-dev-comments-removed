









































#ifndef nsLoggingProgressNotifier_H__
#define nsLoggingProgressNotifier_H__

#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIOutputStream.h"
#include "nsIXPINotifier.h"

class nsLoggingProgressListener : public nsIXPIListener
{
    public:
        
        nsLoggingProgressListener();
        ~nsLoggingProgressListener();
        
        NS_DECL_ISUPPORTS

        
        NS_DECL_NSIXPILISTENER
   
     private:
        void GetTime(char** aString);
        nsCOMPtr<nsIOutputStream>  mLogStream;
};

#endif
