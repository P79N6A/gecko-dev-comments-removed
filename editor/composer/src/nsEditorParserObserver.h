





































#include "nsIElementObserver.h"
#include "nsIObserver.h"

#include "nsWeakReference.h"
#include "nsString.h"
#include "nsHTMLTags.h"

class nsEditorParserObserver : public nsSupportsWeakReference,
                               public nsIElementObserver,
                               public nsIObserver
{
public:

                            nsEditorParserObserver();
  virtual                   ~nsEditorParserObserver();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD                Notify(PRUint32 aDocumentID, eHTMLTags aTag, PRUint32 numOfAttributes, 
                                    const PRUnichar* nameArray[], const PRUnichar* valueArray[]);
  NS_IMETHOD                Notify(PRUint32 aDocumentID, const PRUnichar* aTag, PRUint32 numOfAttributes, 
                                    const PRUnichar* nameArray[], const PRUnichar* valueArray[]);
  NS_IMETHOD                Notify(nsISupports* aWebShell, 
                                   nsISupports* aChannel,
                                   const PRUnichar* aTag, 
                                   const nsStringArray* aKeys, 
                                   const nsStringArray* aValues,
                                   const PRUint32 aFlags);

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD                Start(eHTMLTags* aWatchTags);
  NS_IMETHOD                End();

  
  NS_IMETHOD                GetBadTagFound(PRBool *aFound);

protected:

  virtual void              Notify();
  
protected:

  PRBool                    mBadTagFound;
};


