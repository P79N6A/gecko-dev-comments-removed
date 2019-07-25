






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
      gToolkit = nullptr;
    }

    void          CreateSharedGC(void);
    GdkGC         *GetSharedGC(void);
    
    



 
    void SetDesktopStartupID(const nsACString& aID) { mDesktopStartupID = aID; }
    void GetDesktopStartupID(nsACString* aID) { *aID = mDesktopStartupID; }

    



    void SetFocusTimestamp(uint32_t aTimestamp) { mFocusTimestamp = aTimestamp; }
    uint32_t GetFocusTimestamp() { return mFocusTimestamp; }

private:
    static nsGTKToolkit* gToolkit;

    GdkGC         *mSharedGC;
    nsCString      mDesktopStartupID;
    uint32_t       mFocusTimestamp;
};

#endif  
