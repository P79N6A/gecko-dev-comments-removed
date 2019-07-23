






































#ifndef _nsHyperTextAccessible_H_
#define _nsHyperTextAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsIAccessibleText.h"
#include "nsIAccessibleHyperText.h"
#include "nsIAccessibleEditableText.h"
#include "nsAccessibleEventData.h"
#include "nsIEditActionListener.h"
#include "nsIEditor.h"
#include "nsFrameSelection.h"
#include "nsISelectionController.h"

enum EGetTextType { eGetBefore=-1, eGetAt=0, eGetAfter=1 };



const PRUnichar kEmbeddedObjectChar = 0xfffc;
const PRUnichar kForcedNewLineChar = '\n';




class nsHyperTextAccessible : public nsAccessibleWrap,
                              public nsIAccessibleText,
                              public nsIAccessibleHyperText,
                              public nsIAccessibleEditableText,
                              public nsIEditActionListener
{
public:
  nsHyperTextAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETEXT
  NS_DECL_NSIACCESSIBLEHYPERTEXT
  NS_DECL_NSIACCESSIBLEEDITABLETEXT
  NS_DECL_NSIEDITACTIONLISTENER

  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  void CacheChildren();

protected:
  PRBool IsHyperText();

  









  nsresult GetTextHelper(EGetTextType aType, nsAccessibleTextBoundary aBoundaryType,
                         PRInt32 aOffset, PRInt32 *aStartOffset, PRInt32 *aEndOffset,
                         nsAString & aText);

  









  PRInt32 GetRelativeOffset(nsIPresShell *aPresShell, nsIFrame *aFromFrame, PRInt32 aFromOffset,
                            nsSelectionAmount aAmount, nsDirection aDirection, PRBool aNeedsStart);
  











  nsIFrame* GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset, nsAString *aText = nsnull,
                          nsIFrame **aEndFrame = nsnull, nsIntRect *aBoundsRect = nsnull);
  











  nsresult DOMPointToOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset, PRInt32 *aResultOffset,
                            nsIAccessible **aFinalAccessible = nsnull);
  nsIntRect GetBoundsForString(nsIFrame *aFrame, PRInt32 aStartOffset, PRInt32 aLength);

  
  virtual void SetEditor(nsIEditor *aEditor) { return; }
  virtual already_AddRefed<nsIEditor> GetEditor() { return nsnull; }

  
  nsresult GetSelections(nsISelectionController **aSelCon, nsISelection **aDomSel);
  nsresult SetSelectionRange(PRInt32 aStartPos, PRInt32 aEndPos);

  
  nsresult FireTextChangeEvent(AtkTextChange *aTextData);
};
#endif  
