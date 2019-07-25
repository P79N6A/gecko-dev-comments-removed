





































#ifndef nsCommandManager_h__
#define nsCommandManager_h__


#include "nsString.h"
#include "nsClassHashtable.h"
#include "nsWeakReference.h"

#include "nsICommandManager.h"
#include "nsPICommandUpdater.h"
#include "nsCycleCollectionParticipant.h"

class nsIController;
template<class E> class nsCOMArray;


class nsCommandManager :  public nsICommandManager,
                          public nsPICommandUpdater,
                       
                          public nsSupportsWeakReference

{
public:
                        nsCommandManager();
  virtual               ~nsCommandManager();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsCommandManager, nsICommandManager)
  
  
  NS_DECL_NSICOMMANDMANAGER
  
  
  NS_DECL_NSPICOMMANDUPDATER


protected:


  nsresult  IsCallerChrome(PRBool *aIsCallerChrome);
  nsresult  GetControllerForCommand(const char * aCommand,
                                    nsIDOMWindow *aDirectedToThisWindow,
                                    nsIController** outController);


protected:

  nsClassHashtable<nsCharPtrHashKey, nsCOMArray<nsIObserver> > mObserversTable;

  nsIDOMWindow*         mWindow;      
};


#endif 
