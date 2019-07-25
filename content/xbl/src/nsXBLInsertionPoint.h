





































#ifndef nsXBLInsertionPoint_h__
#define nsXBLInsertionPoint_h__

#include "nsCOMArray.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"

class nsXBLInsertionPoint
{
public:
  nsXBLInsertionPoint(nsIContent* aParentElement, PRUint32 aIndex, nsIContent* aDefContent);
  ~nsXBLInsertionPoint();

  NS_INLINE_DECL_REFCOUNTING(nsXBLInsertionPoint)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLInsertionPoint)

  nsIContent* GetInsertionParent();
  void ClearInsertionParent() { mParentElement = nsnull; }

  PRInt32 GetInsertionIndex() { return mIndex; }

  void SetDefaultContent(nsIContent* aDefaultContent) { mDefaultContent = aDefaultContent; }
  nsIContent* GetDefaultContent();

  void SetDefaultContentTemplate(nsIContent* aDefaultContent) { mDefaultContentTemplate = aDefaultContent; }
  nsIContent* GetDefaultContentTemplate();

  void AddChild(nsIContent* aChildElement) { mElements.AppendObject(aChildElement); }
  void InsertChildAt(PRInt32 aIndex, nsIContent* aChildElement) { mElements.InsertObjectAt(aChildElement, aIndex); }
  void RemoveChild(nsIContent* aChildElement) { mElements.RemoveObject(aChildElement); }
  
  PRInt32 ChildCount() { return mElements.Count(); }

  nsIContent* ChildAt(PRUint32 aIndex);

  PRInt32 IndexOf(nsIContent* aContent) { return mElements.IndexOf(aContent); }

  bool Matches(nsIContent* aContent, PRUint32 aIndex);

  
  
  void UnbindDefaultContent();

protected:
  nsIContent* mParentElement;            
  PRInt32 mIndex;                        
  nsCOMArray<nsIContent> mElements;      
  nsCOMPtr<nsIContent> mDefaultContentTemplate ;           
                                                           
  nsCOMPtr<nsIContent> mDefaultContent;  
};

#endif
