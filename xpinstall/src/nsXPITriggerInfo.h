






































#ifndef nsXPITriggerInfo_h
#define nsXPITriggerInfo_h

#include "nsString.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsISupportsUtils.h"
#include "nsILocalFile.h"
#include "nsIOutputStream.h"
#include "jsapi.h"
#include "prthread.h"
#include "nsIXPConnect.h"
#include "nsICryptoHash.h"
#include "nsIPrincipal.h"
#include "nsThreadUtils.h"

struct XPITriggerEvent : public nsRunnable {
    nsString    URL;
    PRInt32     status;
    JSContext*  cx;
    jsval       cbval;
    nsCOMPtr<nsISupports> ref;
    nsCOMPtr<nsIPrincipal> princ;

    virtual ~XPITriggerEvent();
    NS_IMETHOD Run();
};

class nsXPITriggerItem
{
  public:
    nsXPITriggerItem( const PRUnichar* name,
                      const PRUnichar* URL,
                      const PRUnichar* iconURL,
                      const char* hash = nsnull,
                      PRInt32 flags = 0);
    ~nsXPITriggerItem();

    nsString    mName;
    nsString    mURL;
    nsString    mIconURL;
    nsString    mArguments;
    nsString    mCertName;

    PRBool      mHashFound; 
    nsCString   mHash;
    nsCOMPtr<nsICryptoHash> mHasher;
    PRInt32     mFlags;

    nsCOMPtr<nsILocalFile>      mFile;
    nsCOMPtr<nsIOutputStream>   mOutStream;
    nsCOMPtr<nsIPrincipal>      mPrincipal;

    void    SetPrincipal(nsIPrincipal* aPrincipal);

    PRBool  IsFileURL() { return StringBeginsWith(mURL, NS_LITERAL_STRING("file:/")); }

    const PRUnichar* GetSafeURLString();

  private:
    
    nsXPITriggerItem& operator=(const nsXPITriggerItem& rhs);
    nsXPITriggerItem(const nsXPITriggerItem& rhs);

    nsString    mSafeURL;
};



class nsXPITriggerInfo
{
  public:
    nsXPITriggerInfo();
    ~nsXPITriggerInfo();

    void                Add( nsXPITriggerItem *aItem )
                        { if ( aItem ) mItems.AppendElement( aItem ); }

    nsXPITriggerItem*   Get( PRUint32 aIndex )
                        { return mItems.ElementAt(aIndex);}

    void                SaveCallback( JSContext *aCx, jsval aVal );

    PRUint32            Size() { return mItems.Length(); }

    void                SendStatus(const PRUnichar* URL, PRInt32 status);

    void                SetPrincipal(nsIPrincipal* aPrinc) { mPrincipal = aPrinc; }


  private:
    nsTArray<nsXPITriggerItem*> mItems;
    JSContext   *mCx;
    nsCOMPtr<nsISupports> mContextWrapper;
    jsval       mCbval;
    nsCOMPtr<nsIThread> mThread;

    nsCOMPtr<nsIPrincipal>      mPrincipal;

    
    nsXPITriggerInfo& operator=(const nsXPITriggerInfo& rhs);
    nsXPITriggerInfo(const nsXPITriggerInfo& rhs);
};

#endif 
