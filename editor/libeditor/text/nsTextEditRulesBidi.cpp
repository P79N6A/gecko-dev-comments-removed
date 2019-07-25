




































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
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(shell, NS_ERROR_NULL_POINTER);
  
  nsPresContext *context = shell->GetPresContext();
  NS_ENSURE_TRUE(context, NS_ERROR_NULL_POINTER);
  
  if (!context->BidiEnabled())
    return NS_OK;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aSelNode);
  NS_ENSURE_TRUE(content, NS_ERROR_NULL_POINTER);

  PRUint8 levelBefore;
  PRUint8 levelAfter;

  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(aSelection));
  NS_ENSURE_TRUE(privateSelection, NS_ERROR_NULL_POINTER);
  
  nsRefPtr<nsFrameSelection> frameSelection;
  privateSelection->GetFrameSelection(getter_AddRefs(frameSelection));
  NS_ENSURE_TRUE(frameSelection, NS_ERROR_NULL_POINTER);
  
  nsPrevNextBidiLevels levels = frameSelection->
    GetPrevNextBidiLevels(content, aSelOffset, PR_TRUE);
    
  levelBefore = levels.mLevelBefore;
  levelAfter = levels.mLevelAfter;

  PRUint8 currentCaretLevel = frameSelection->GetCaretBidiLevel();

  PRUint8 levelOfDeletion;
  levelOfDeletion =
    (nsIEditor::eNext==aAction || nsIEditor::eNextWord==aAction) ?
    levelAfter : levelBefore;

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

