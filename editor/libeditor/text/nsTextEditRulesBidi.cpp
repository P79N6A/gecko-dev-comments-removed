




































#include "nsTextEditRules.h"
#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsISelectionPrivate.h"
#include "nsFrameSelection.h"


nsresult
nsTextEditRules::CheckBidiLevelForDeletion(nsISelection         *aSelection,
                                           nsIDOMNode           *aSelNode, 
                                           PRInt32               aSelOffset, 
                                           nsIEditor::EDirection aAction,
                                           PRBool               *aCancel)
{
  NS_ENSURE_ARG_POINTER(aCancel);
  *aCancel = PR_FALSE;

  nsCOMPtr<nsIPresShell> shell;
  nsresult res = mEditor->GetPresShell(getter_AddRefs(shell));
  if (NS_FAILED(res))
    return res;
  if (!shell)
    return NS_ERROR_NULL_POINTER;
  
  nsPresContext *context = shell->GetPresContext();
  if (!context)
    return NS_ERROR_NULL_POINTER;
  
  if (!context->BidiEnabled())
    return NS_OK;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aSelNode);
  if (!content)
    return NS_ERROR_NULL_POINTER;

  PRUint8 levelBefore;
  PRUint8 levelAfter;

  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(aSelection));
  if (!privateSelection)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsFrameSelection> frameSelection;
  privateSelection->GetFrameSelection(getter_AddRefs(frameSelection));
  if (!frameSelection)
    return NS_ERROR_NULL_POINTER;
  
  nsPrevNextBidiLevels levels = frameSelection->
    GetPrevNextBidiLevels(content, aSelOffset, PR_TRUE);
    
  levelBefore = levels.mLevelBefore;
  levelAfter = levels.mLevelAfter;

  PRUint8 currentCaretLevel = frameSelection->GetCaretBidiLevel();

  PRUint8 levelOfDeletion;
  levelOfDeletion = (nsIEditor::eNext==aAction) ? levelAfter : levelBefore;

  if (currentCaretLevel == levelOfDeletion)
    ; 
  else
  {
    if (mDeleteBidiImmediately || levelBefore == levelAfter)
      ; 
    else
      *aCancel = PR_TRUE;

    
    
    frameSelection->SetCaretBidiLevel(levelOfDeletion);
  }
  return NS_OK;
}

