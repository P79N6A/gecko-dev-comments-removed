








#ifndef nsComposerCommandsUpdater_h__
#define nsComposerCommandsUpdater_h__

#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsWeakPtr.h"
#include "nsPICommandUpdater.h"

#include "nsISelectionListener.h"
#include "nsIDocumentStateListener.h"
#include "nsITransactionListener.h"

class nsIDocShell;
class nsITransactionManager;

class nsComposerCommandsUpdater : public nsISelectionListener,
                                  public nsIDocumentStateListener,
                                  public nsITransactionListener,
                                  public nsITimerCallback
{
public:

                                  nsComposerCommandsUpdater();
  virtual                         ~nsComposerCommandsUpdater();

  
  NS_DECL_ISUPPORTS
  
  
  NS_DECL_NSISELECTIONLISTENER
  
  
  NS_DECL_NSIDOCUMENTSTATELISTENER

  
  NS_DECL_NSITIMERCALLBACK

  
  
  NS_IMETHOD WillDo(nsITransactionManager *aManager, nsITransaction *aTransaction, bool *aInterrupt);
  NS_IMETHOD DidDo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aDoResult);
  NS_IMETHOD WillUndo(nsITransactionManager *aManager, nsITransaction *aTransaction, bool *aInterrupt);
  NS_IMETHOD DidUndo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aUndoResult);
  NS_IMETHOD WillRedo(nsITransactionManager *aManager, nsITransaction *aTransaction, bool *aInterrupt);
  NS_IMETHOD DidRedo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aRedoResult);
  NS_IMETHOD WillBeginBatch(nsITransactionManager *aManager, bool *aInterrupt);
  NS_IMETHOD DidBeginBatch(nsITransactionManager *aManager, nsresult aResult);
  NS_IMETHOD WillEndBatch(nsITransactionManager *aManager, bool *aInterrupt);
  NS_IMETHOD DidEndBatch(nsITransactionManager *aManager, nsresult aResult);
  NS_IMETHOD WillMerge(nsITransactionManager *aManager, nsITransaction *aTopTransaction,
                       nsITransaction *aTransactionToMerge, bool *aInterrupt);
  NS_IMETHOD DidMerge(nsITransactionManager *aManager, nsITransaction *aTopTransaction,
                      nsITransaction *aTransactionToMerge,
                      bool aDidMerge, nsresult aMergeResult);


  nsresult   Init(nsIDOMWindow* aDOMWindow);

protected:

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
  PRInt8        mDirtyState;  
  PRInt8        mSelectionCollapsed;  
  bool          mFirstDoOfFirstUndo;
    

};

extern "C" nsresult NS_NewComposerCommandsUpdater(nsISelectionListener** aInstancePtrResult);


#endif
