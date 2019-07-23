





































#ifndef nsXBLInsertionPoint_h__
#define nsXBLInsertionPoint_h__

#include "nsCOMArray.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"

class nsXBLInsertionPoint
{
public:
  nsXBLInsertionPoint(nsIContent* aParentElement, PRUint32 aIndex, nsIContent* aDefContent);
  ~nsXBLInsertionPoint();

  nsrefcnt AddRef()
  {
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsXBLInsertionPoint",
                  sizeof(nsXBLInsertionPoint));
    return mRefCnt;
  }

  nsrefcnt Release();

  already_AddRefed<nsIContent> GetInsertionParent();
  PRInt32 GetInsertionIndex() { return mIndex; }

  void SetDefaultContent(nsIContent* aDefaultContent) { mDefaultContent = aDefaultContent; }
  already_AddRefed<nsIContent> GetDefaultContent();

  void SetDefaultContentTemplate(nsIContent* aDefaultContent) { mDefaultContentTemplate = aDefaultContent; }
  already_AddRefed<nsIContent> GetDefaultContentTemplate();

  void AddChild(nsIContent* aChildElement) { mElements.AppendObject(aChildElement); }
  void InsertChildAt(PRInt32 aIndex, nsIContent* aChildElement) { mElements.InsertObjectAt(aChildElement, aIndex); }
  void RemoveChild(nsIContent* aChildElement) { mElements.RemoveObject(aChildElement); }
  
  PRInt32 ChildCount() { return mElements.Count(); }

  already_AddRefed<nsIContent> ChildAt(PRUint32 aIndex);

  PRBool Matches(nsIContent* aContent, PRUint32 aIndex);

protected:
  nsAutoRefCnt mRefCnt;
  nsIContent* mParentElement;            
  PRInt32 mIndex;                        
  nsCOMArray<nsIContent> mElements;      
  nsCOMPtr<nsIContent> mDefaultContentTemplate ;           
                                                           
  nsCOMPtr<nsIContent> mDefaultContent;  
};

#endif
