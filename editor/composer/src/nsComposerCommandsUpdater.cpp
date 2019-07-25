








































#include "nsPIDOMWindow.h"
#include "nsComposerCommandsUpdater.h"
#include "nsComponentManagerUtils.h"
#include "nsIDOMDocument.h"
#include "nsISelection.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsString.h"

#include "nsICommandManager.h"

#include "nsIDocShell.h"
#include "nsITransactionManager.h"

nsComposerCommandsUpdater::nsComposerCommandsUpdater()
:  mDirtyState(eStateUninitialized)
,  mSelectionCollapsed(eStateUninitialized)
,  mFirstDoOfFirstUndo(true)
{
}

nsComposerCommandsUpdater::~nsComposerCommandsUpdater()
{
  
  if (mUpdateTimer)
  {
    mUpdateTimer->Cancel();
  }
}

NS_IMPL_ISUPPORTS4(nsComposerCommandsUpdater, nsISelectionListener,
                   nsIDocumentStateListener, nsITransactionListener, nsITimerCallback)

#if 0
#pragma mark -
#endif

NS_IMETHODIMP
nsComposerCommandsUpdater::NotifyDocumentCreated()
{
  
  UpdateOneCommand("obs_documentCreated");
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::NotifyDocumentWillBeDestroyed()
{
  
  if (mUpdateTimer)
  {
    mUpdateTimer->Cancel();
    mUpdateTimer = nsnull;
  }
  
  
  
#if 0
  
  UpdateOneCommand("obs_documentWillBeDestroyed");
#endif
  return NS_OK;
}


NS_IMETHODIMP
nsComposerCommandsUpdater::NotifyDocumentStateChanged(bool aNowDirty)
{
  
  return UpdateDirtyState(aNowDirty);
}

NS_IMETHODIMP
nsComposerCommandsUpdater::NotifySelectionChanged(nsIDOMDocument *,
                                                  nsISelection *, PRInt16)
{
  return PrimeUpdateTimer();
}

#if 0
#pragma mark -
#endif

NS_IMETHODIMP
nsComposerCommandsUpdater::WillDo(nsITransactionManager *aManager,
                                  nsITransaction *aTransaction, bool *aInterrupt)
{
  *aInterrupt = false;
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::DidDo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, nsresult aDoResult)
{
  
  PRInt32 undoCount;
  aManager->GetNumberOfUndoItems(&undoCount);
  if (undoCount == 1)
  {
    if (mFirstDoOfFirstUndo)
      UpdateCommandGroup(NS_LITERAL_STRING("undo"));
    mFirstDoOfFirstUndo = false;
  }
	
  return NS_OK;
}

NS_IMETHODIMP 
nsComposerCommandsUpdater::WillUndo(nsITransactionManager *aManager,
                                    nsITransaction *aTransaction,
                                    bool *aInterrupt)
{
  *aInterrupt = false;
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::DidUndo(nsITransactionManager *aManager,
                                   nsITransaction *aTransaction,
                                   nsresult aUndoResult)
{
  PRInt32 undoCount;
  aManager->GetNumberOfUndoItems(&undoCount);
  if (undoCount == 0)
    mFirstDoOfFirstUndo = true;    

  UpdateCommandGroup(NS_LITERAL_STRING("undo"));
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::WillRedo(nsITransactionManager *aManager,
                                    nsITransaction *aTransaction,
                                    bool *aInterrupt)
{
  *aInterrupt = false;
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::DidRedo(nsITransactionManager *aManager,  
                                   nsITransaction *aTransaction,
                                   nsresult aRedoResult)
{
  UpdateCommandGroup(NS_LITERAL_STRING("undo"));
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::WillBeginBatch(nsITransactionManager *aManager,
                                          bool *aInterrupt)
{
  *aInterrupt = false;
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::DidBeginBatch(nsITransactionManager *aManager,
                                         nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::WillEndBatch(nsITransactionManager *aManager,
                                        bool *aInterrupt)
{
  *aInterrupt = false;
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::DidEndBatch(nsITransactionManager *aManager, 
                                       nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::WillMerge(nsITransactionManager *aManager,
                                     nsITransaction *aTopTransaction,
                                     nsITransaction *aTransactionToMerge,
                                     bool *aInterrupt)
{
  *aInterrupt = false;
  return NS_OK;
}

NS_IMETHODIMP
nsComposerCommandsUpdater::DidMerge(nsITransactionManager *aManager,
                                    nsITransaction *aTopTransaction,
                                    nsITransaction *aTransactionToMerge,
                                    bool aDidMerge, nsresult aMergeResult)
{
  return NS_OK;
}

#if 0
#pragma mark -
#endif

nsresult
nsComposerCommandsUpdater::Init(nsIDOMWindow* aDOMWindow)
{
  NS_ENSURE_ARG(aDOMWindow);
  mDOMWindow = do_GetWeakReference(aDOMWindow);

  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aDOMWindow));
  if (window)
  {
    mDocShell = do_GetWeakReference(window->GetDocShell());
  }
  return NS_OK;
}

nsresult
nsComposerCommandsUpdater::PrimeUpdateTimer()
{
  if (!mUpdateTimer)
  {
    nsresult rv = NS_OK;
    mUpdateTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  const PRUint32 kUpdateTimerDelay = 150;
  return mUpdateTimer->InitWithCallback(static_cast<nsITimerCallback*>(this),
                                        kUpdateTimerDelay,
                                        nsITimer::TYPE_ONE_SHOT);
}


void nsComposerCommandsUpdater::TimerCallback()
{
  
  bool isCollapsed = SelectionIsCollapsed();
  if (isCollapsed != mSelectionCollapsed)
  {
    UpdateCommandGroup(NS_LITERAL_STRING("select"));
    mSelectionCollapsed = isCollapsed;
  }
  
  
  
  UpdateCommandGroup(NS_LITERAL_STRING("style"));
}

nsresult
nsComposerCommandsUpdater::UpdateDirtyState(bool aNowDirty)
{
  if (mDirtyState != aNowDirty)
  {
    UpdateCommandGroup(NS_LITERAL_STRING("save"));
    UpdateCommandGroup(NS_LITERAL_STRING("undo"));
    mDirtyState = aNowDirty;
  }
  
  return NS_OK;  
}

nsresult
nsComposerCommandsUpdater::UpdateCommandGroup(const nsAString& aCommandGroup)
{
  nsCOMPtr<nsPICommandUpdater> commandUpdater = GetCommandUpdater();
  NS_ENSURE_TRUE(commandUpdater, NS_ERROR_FAILURE);

  
  
  
  if (aCommandGroup.EqualsLiteral("undo"))
  {
    commandUpdater->CommandStatusChanged("cmd_undo");
    commandUpdater->CommandStatusChanged("cmd_redo");
  }
  else if (aCommandGroup.EqualsLiteral("select") ||
           aCommandGroup.EqualsLiteral("style"))
  {
    commandUpdater->CommandStatusChanged("cmd_bold");
    commandUpdater->CommandStatusChanged("cmd_italic");
    commandUpdater->CommandStatusChanged("cmd_underline");
    commandUpdater->CommandStatusChanged("cmd_tt");

    commandUpdater->CommandStatusChanged("cmd_strikethrough");
    commandUpdater->CommandStatusChanged("cmd_superscript");
    commandUpdater->CommandStatusChanged("cmd_subscript");
    commandUpdater->CommandStatusChanged("cmd_nobreak");

    commandUpdater->CommandStatusChanged("cmd_em");
    commandUpdater->CommandStatusChanged("cmd_strong");
    commandUpdater->CommandStatusChanged("cmd_cite");
    commandUpdater->CommandStatusChanged("cmd_abbr");
    commandUpdater->CommandStatusChanged("cmd_acronym");
    commandUpdater->CommandStatusChanged("cmd_code");
    commandUpdater->CommandStatusChanged("cmd_samp");
    commandUpdater->CommandStatusChanged("cmd_var");
   
    commandUpdater->CommandStatusChanged("cmd_increaseFont");
    commandUpdater->CommandStatusChanged("cmd_decreaseFont");

    commandUpdater->CommandStatusChanged("cmd_paragraphState");
    commandUpdater->CommandStatusChanged("cmd_fontFace");
    commandUpdater->CommandStatusChanged("cmd_fontColor");
    commandUpdater->CommandStatusChanged("cmd_backgroundColor");
    commandUpdater->CommandStatusChanged("cmd_highlight");
  }  
  else if (aCommandGroup.EqualsLiteral("save"))
  {
    
    commandUpdater->CommandStatusChanged("cmd_setDocumentModified");
    commandUpdater->CommandStatusChanged("cmd_save");
  }
  return NS_OK;  
}

nsresult
nsComposerCommandsUpdater::UpdateOneCommand(const char *aCommand)
{
  nsCOMPtr<nsPICommandUpdater> commandUpdater = GetCommandUpdater();
  NS_ENSURE_TRUE(commandUpdater, NS_ERROR_FAILURE);

  commandUpdater->CommandStatusChanged(aCommand);

  return NS_OK;  
}

bool
nsComposerCommandsUpdater::SelectionIsCollapsed()
{
  nsCOMPtr<nsIDOMWindow> domWindow = do_QueryReferent(mDOMWindow);
  NS_ENSURE_TRUE(domWindow, true);

  nsCOMPtr<nsISelection> domSelection;
  if (NS_SUCCEEDED(domWindow->GetSelection(getter_AddRefs(domSelection))) && domSelection)
  {
    bool selectionCollapsed = false;
    domSelection->GetIsCollapsed(&selectionCollapsed);
    return selectionCollapsed;
  }

  NS_WARNING("nsComposerCommandsUpdater::SelectionIsCollapsed - no domSelection");

  return false;
}

already_AddRefed<nsPICommandUpdater>
nsComposerCommandsUpdater::GetCommandUpdater()
{
  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocShell);
  NS_ENSURE_TRUE(docShell, nsnull);
  nsCOMPtr<nsICommandManager> manager = do_GetInterface(docShell);
  nsCOMPtr<nsPICommandUpdater> updater = do_QueryInterface(manager);
  nsPICommandUpdater* retVal = nsnull;
  updater.swap(retVal);
  return retVal;
}

#if 0
#pragma mark -
#endif

nsresult
nsComposerCommandsUpdater::Notify(nsITimer *timer)
{
  NS_ASSERTION(timer == mUpdateTimer.get(), "Hey, this ain't my timer!");
  TimerCallback();
  return NS_OK;
}

#if 0
#pragma mark -
#endif


nsresult
NS_NewComposerCommandsUpdater(nsISelectionListener** aInstancePtrResult)
{
  nsComposerCommandsUpdater* newThang = new nsComposerCommandsUpdater;
  NS_ENSURE_TRUE(newThang, NS_ERROR_OUT_OF_MEMORY);

  return newThang->QueryInterface(NS_GET_IID(nsISelectionListener),
                                  (void **)aInstancePtrResult);
}
