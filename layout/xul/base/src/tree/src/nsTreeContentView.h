




































#ifndef nsTreeContentView_h__
#define nsTreeContentView_h__

#include "nsFixedSizeAllocator.h"
#include "nsTArray.h"
#include "nsIDocument.h"
#include "nsStubDocumentObserver.h"
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"
#include "nsITreeView.h"
#include "nsITreeContentView.h"
#include "nsITreeSelection.h"

class Row;

nsresult NS_NewTreeContentView(nsITreeView** aResult);

class nsTreeContentView : public nsINativeTreeView,
                          public nsITreeContentView,
                          public nsStubDocumentObserver
{
  public:
    nsTreeContentView(void);

    ~nsTreeContentView(void);

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsTreeContentView,
                                             nsINativeTreeView)

    NS_DECL_NSITREEVIEW
    
    NS_IMETHOD EnsureNative() { return NS_OK; }

    NS_DECL_NSITREECONTENTVIEW

    
    virtual void ContentStatesChanged(nsIDocument* aDocument,
                                      nsIContent* aContent1,
                                      nsIContent* aContent2,
                                      PRInt32 aStateMask);
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

  protected:
    
    void Serialize(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex,
                   nsTArray<Row*>& aRows);

    void SerializeItem(nsIContent* aContent, PRInt32 aParentIndex,
                       PRInt32* aIndex, nsTArray<Row*>& aRows);

    void SerializeSeparator(nsIContent* aContent, PRInt32 aParentIndex,
                            PRInt32* aIndex, nsTArray<Row*>& aRows);

    void SerializeOption(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex,
                         nsTArray<Row*>& aRows);

    void SerializeOptGroup(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex,
                           nsTArray<Row*>& aRows);

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
    nsTArray<Row*>                      mRows;

    PRPackedBool                        mUpdateSelection;
};

#endif 
