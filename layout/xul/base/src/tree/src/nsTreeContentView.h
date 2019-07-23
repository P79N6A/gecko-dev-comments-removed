




































#ifndef nsTreeContentView_h__
#define nsTreeContentView_h__

#include "nsFixedSizeAllocator.h"
#include "nsVoidArray.h"
#include "nsIDocument.h"
#include "nsStubDocumentObserver.h"
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"
#include "nsITreeView.h"
#include "nsITreeContentView.h"
#include "nsITreeSelection.h"

nsresult NS_NewTreeContentView(nsITreeView** aResult);

class nsTreeContentView : public nsINativeTreeView,
                          public nsITreeContentView,
                          public nsStubDocumentObserver
{
  public:
    nsTreeContentView(void);

    ~nsTreeContentView(void);

    NS_DECL_ISUPPORTS

    NS_DECL_NSITREEVIEW
    
    NS_IMETHOD EnsureNative() { return NS_OK; }

    NS_DECL_NSITREECONTENTVIEW

    
    virtual void ContentStatesChanged(nsIDocument* aDocument,
                                      nsIContent* aContent1,
                                      nsIContent* aContent2,
                                      PRInt32 aStateMask);
    virtual void AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType);
    virtual void ContentAppended(nsIDocument *aDocument,
                                 nsIContent* aContainer,
                                 PRInt32 aNewIndexInContainer);
    virtual void ContentInserted(nsIDocument *aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 PRInt32 aIndexInContainer);
    virtual void ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                                nsIContent* aChild, PRInt32 aIndexInContainer);
    virtual void NodeWillBeDestroyed(const nsINode* aNode);

  protected:
    
    void Serialize(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex, nsVoidArray& aRows);

    void SerializeItem(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex, nsVoidArray& aRows);

    void SerializeSeparator(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex, nsVoidArray& aRows);

    void SerializeOption(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex,
                         nsVoidArray& aRows);

    void SerializeOptGroup(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex,
                           nsVoidArray& aRows);

    void GetIndexInSubtree(nsIContent* aContainer, nsIContent* aContent, PRInt32* aResult);
    
    
    PRInt32 EnsureSubtree(PRInt32 aIndex);

    PRInt32 RemoveSubtree(PRInt32 aIndex);

    PRInt32 InsertRow(PRInt32 aParentIndex, PRInt32 aIndex, nsIContent* aContent);

    void InsertRowFor(nsIContent* aParent, nsIContent* aChild);

    PRInt32 RemoveRow(PRInt32 aIndex);

    void ClearRows();
    
    void OpenContainer(PRInt32 aIndex);

    void CloseContainer(PRInt32 aIndex);

    PRInt32 FindContent(nsIContent* aContent);

    void UpdateSubtreeSizes(PRInt32 aIndex, PRInt32 aCount);

    void UpdateParentIndexes(PRInt32 aIndex, PRInt32 aSkip, PRInt32 aCount);

    
    nsIContent* GetCell(nsIContent* aContainer, nsITreeColumn* aCol);

  private:
    nsCOMPtr<nsITreeBoxObject>          mBoxObject;
    nsCOMPtr<nsITreeSelection>          mSelection;
    nsCOMPtr<nsIContent>                mRoot;
    nsCOMPtr<nsIContent>                mBody;
    nsIDocument*                        mDocument;      
    nsFixedSizeAllocator                mAllocator;
    nsVoidArray                         mRows;

    PRPackedBool                        mUpdateSelection;
};

#endif 
