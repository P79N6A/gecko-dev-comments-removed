






































#ifndef GTKTOOLKIT_H      
#define GTKTOOLKIT_H

#include "nsString.h"
#include <gtk/gtk.h>





 

class nsGTKToolkit
{
public:
    nsGTKToolkit();
    virtual ~nsGTKToolkit();

    static nsGTKToolkit* GetToolkit();

    static void Shutdown() {
      delete gToolkit;
      gToolkit = nsnull;
    }

    void          CreateSharedGC(void);
    GdkGC         *GetSharedGC(void);
    
    



 
    void SetDesktopStartupID(const nsACString& aID) { mDesktopStartupID = aID; }
    void GetDesktopStartupID(nsACString* aID) { *aID = mDesktopStartupID; }

    



    void SetFocusTimestamp(PRUint32 aTimestamp) { mFocusTimestamp = aTimestamp; }
    PRUint32 GetFocusTimestamp() { return mFocusTimestamp; }

private:
    static nsGTKToolkit* gToolkit;

    GdkGC         *mSharedGC;
    nsCString      mDesktopStartupID;
    PRUint32       mFocusTimestamp;
};

#endif  
