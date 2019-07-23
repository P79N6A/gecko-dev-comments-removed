




































#ifndef nsToolkit_h__      
#define nsToolkit_h__

#include "nsIToolkit.h"
#include "nsString.h"
#include <QPixmap>





 
class nsToolkit : public nsIToolkit
{
public:
    nsToolkit();
    virtual ~nsToolkit();

    void          CreateSharedGC(void);
    Qt::HANDLE    GetSharedGC(void);

    



 
    void SetDesktopStartupID(const nsACString& aID) { mDesktopStartupID = aID; }
    void GetDesktopStartupID(nsACString* aID) { *aID = mDesktopStartupID; }

    



    void SetFocusTimestamp(PRUint32 aTimestamp) { mFocusTimestamp = aTimestamp; }
    PRUint32 GetFocusTimestamp() { return mFocusTimestamp; }

    NS_DECL_ISUPPORTS
    NS_IMETHOD Init(PRThread *aThread);

private:
    nsCString      mDesktopStartupID;
    PRUint32       mFocusTimestamp;
    QPixmap        *mSharedGC;
};

#endif  
