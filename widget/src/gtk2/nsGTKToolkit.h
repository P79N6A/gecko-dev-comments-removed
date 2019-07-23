






































#ifndef GTKTOOLKIT_H      
#define GTKTOOLKIT_H

#include "nsIToolkit.h"
#include "nsString.h"
#include <gtk/gtk.h>





 

class nsGTKToolkit : public nsIToolkit
{
public:
    nsGTKToolkit();
    virtual ~nsGTKToolkit();

    NS_DECL_ISUPPORTS

    NS_IMETHOD    Init(PRThread *aThread);

    void          CreateSharedGC(void);
    GdkGC         *GetSharedGC(void);
    
    



 
    void SetDesktopStartupID(const nsACString& aID) { mDesktopStartupID = aID; }
    void GetDesktopStartupID(nsACString* aID) { *aID = mDesktopStartupID; }

    



    void SetFocusTimestamp(PRUint32 aTimestamp) { mFocusTimestamp = aTimestamp; }
    PRUint32 GetFocusTimestamp() { return mFocusTimestamp; }

private:
    GdkGC         *mSharedGC;
    nsCString      mDesktopStartupID;
    PRUint32       mFocusTimestamp;
};

#endif  
