




#ifndef nsXBLInsertionPoint_h__
#define nsXBLInsertionPoint_h__

#include "nsCOMArray.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"

class nsXBLInsertionPoint
{
public:
  nsXBLInsertionPoint(nsIContent* aParentElement, uint32_t aIndex, nsIContent* aDefContent);
  ~nsXBLInsertionPoint();

  NS_INLINE_DECL_REFCOUNTING(nsXBLInsertionPoint)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLInsertionPoint)

  nsIContent* GetInsertionParent();
  void ClearInsertionParent() { mParentElement = nullptr; }

  int32_t GetInsertionIndex() { return mIndex; }

  void SetDefaultContent(nsIContent* aDefaultContent) { mDefaultContent = aDefaultContent; }
  nsIContent* GetDefaultContent();

  void SetDefaultContentTemplate(nsIContent* aDefaultContent) { mDefaultContentTemplate = aDefaultContent; }
  nsIContent* GetDefaultContentTemplate();

  void AddChild(nsIContent* aChildElement) { mElements.AppendObject(aChildElement); }
  void InsertChildAt(int32_t aIndex, nsIContent* aChildElement) { mElements.InsertObjectAt(aChildElement, aIndex); }
  void RemoveChild(nsIContent* aChildElement) { mElements.RemoveObject(aChildElement); }
  
  int32_t ChildCount() { return mElements.Count(); }

  nsIContent* ChildAt(uint32_t aIndex);

  int32_t IndexOf(nsIContent* aContent) { return mElements.IndexOf(aContent); }

  bool Matches(nsIContent* aContent, uint32_t aIndex);

  
  
  void UnbindDefaultContent();

protected:
  nsIContent* mParentElement;            
  int32_t mIndex;                        
  nsCOMArray<nsIContent> mElements;      
  nsCOMPtr<nsIContent> mDefaultContentTemplate ;           
                                                           
  nsCOMPtr<nsIContent> mDefaultContent;  
};

#endif
