




#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsFrameSelection.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIEditor.h"
#include "nsIPresShell.h"
#include "mozilla/dom/Selection.h"
#include "nsISelectionPrivate.h"
#include "nsISupportsImpl.h"
#include "nsPlaintextEditor.h"
#include "nsPresContext.h"
#include "nsTextEditRules.h"
#include "nscore.h"

using namespace mozilla;
using namespace mozilla::dom;


nsresult
nsTextEditRules::CheckBidiLevelForDeletion(nsISelection         *aSelection,
                                           nsIDOMNode           *aSelNode, 
                                           int32_t               aSelOffset, 
                                           nsIEditor::EDirection aAction,
                                           bool                 *aCancel)
{
  NS_ENSURE_ARG_POINTER(aCancel);
  *aCancel = false;

  nsCOMPtr<nsIPresShell> shell = mEditor->GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_NOT_INITIALIZED);
  
  nsPresContext *context = shell->GetPresContext();
  NS_ENSURE_TRUE(context, NS_ERROR_NULL_POINTER);
  
  if (!context->BidiEnabled())
    return NS_OK;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aSelNode);
  NS_ENSURE_TRUE(content, NS_ERROR_NULL_POINTER);

  uint8_t levelBefore;
  uint8_t levelAfter;
  nsRefPtr<nsFrameSelection> frameSelection =
    static_cast<Selection*>(aSelection)->GetFrameSelection();
  NS_ENSURE_TRUE(frameSelection, NS_ERROR_NULL_POINTER);
  
  nsPrevNextBidiLevels levels = frameSelection->
    GetPrevNextBidiLevels(content, aSelOffset, true);
    
  levelBefore = levels.mLevelBefore;
  levelAfter = levels.mLevelAfter;

  uint8_t currentCaretLevel = frameSelection->GetCaretBidiLevel();

  uint8_t levelOfDeletion;
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
      *aCancel = true;

    
    
    frameSelection->SetCaretBidiLevel(levelOfDeletion);
  }
  return NS_OK;
}

