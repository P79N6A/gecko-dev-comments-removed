








#ifndef nsComposerCommandsUpdater_h__
#define nsComposerCommandsUpdater_h__

#include "nsCOMPtr.h"                   
#include "nsIDocumentStateListener.h"
#include "nsISelectionListener.h"
#include "nsISupportsImpl.h"            
#include "nsITimer.h"                   
#include "nsITransactionListener.h"     
#include "nsIWeakReferenceUtils.h"      
#include "nscore.h"                     

class nsIDOMWindow;
class nsITransaction;
class nsITransactionManager;
class nsPICommandUpdater;

class nsComposerCommandsUpdater : public nsISelectionListener,
                                  public nsIDocumentStateListener,
                                  public nsITransactionListener,
                                  public nsITimerCallback
{
public:

                                  nsComposerCommandsUpdater();

  
  NS_DECL_ISUPPORTS
  
  
  NS_DECL_NSISELECTIONLISTENER
  
  
  NS_DECL_NSIDOCUMENTSTATELISTENER

  
  NS_DECL_NSITIMERCALLBACK

  
  
  NS_IMETHOD WillDo(nsITransactionManager *aManager, nsITransaction *aTransaction, bool *aInterrupt) override;
  NS_IMETHOD DidDo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aDoResult) override;
  NS_IMETHOD WillUndo(nsITransactionManager *aManager, nsITransaction *aTransaction, bool *aInterrupt) override;
  NS_IMETHOD DidUndo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aUndoResult) override;
  NS_IMETHOD WillRedo(nsITransactionManager *aManager, nsITransaction *aTransaction, bool *aInterrupt) override;
  NS_IMETHOD DidRedo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aRedoResult) override;
  NS_IMETHOD WillBeginBatch(nsITransactionManager *aManager, bool *aInterrupt) override;
  NS_IMETHOD DidBeginBatch(nsITransactionManager *aManager, nsresult aResult) override;
  NS_IMETHOD WillEndBatch(nsITransactionManager *aManager, bool *aInterrupt) override;
  NS_IMETHOD DidEndBatch(nsITransactionManager *aManager, nsresult aResult) override;
  NS_IMETHOD WillMerge(nsITransactionManager *aManager, nsITransaction *aTopTransaction,
                       nsITransaction *aTransactionToMerge, bool *aInterrupt) override;
  NS_IMETHOD DidMerge(nsITransactionManager *aManager, nsITransaction *aTopTransaction,
                      nsITransaction *aTransactionToMerge,
                      bool aDidMerge, nsresult aMergeResult) override;


  nsresult   Init(nsIDOMWindow* aDOMWindow);

protected:

  virtual ~nsComposerCommandsUpdater();

  enum {
    eStateUninitialized   = -1,
    eStateOff             = false,
    eStateOn              = true
  };
  
  bool          SelectionIsCollapsed();
  nsresult      UpdateDirtyState(bool aNowDirty);  
  nsresult      UpdateOneCommand(const char* aCommand);
  nsresult      UpdateCommandGroup(const nsAString& aCommandGroup);

  already_AddRefed<nsPICommandUpdater> GetCommandUpdater();
  
  nsresult      PrimeUpdateTimer();
  void          TimerCallback();
  nsCOMPtr<nsITimer>  mUpdateTimer;

  nsWeakPtr     mDOMWindow;
  nsWeakPtr     mDocShell;
  int8_t        mDirtyState;  
  int8_t        mSelectionCollapsed;  
  bool          mFirstDoOfFirstUndo;
    

};

extern "C" nsresult NS_NewComposerCommandsUpdater(nsISelectionListener** aInstancePtrResult);


#endif 
