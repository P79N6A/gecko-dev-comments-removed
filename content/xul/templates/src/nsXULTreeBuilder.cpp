








































#include "nscore.h"
#include "nsIContent.h"
#include "nsINodeInfo.h"
#include "nsIDOMElement.h"
#include "nsILocalStore.h"
#include "nsIBoxObject.h"
#include "nsITreeBoxObject.h"
#include "nsITreeSelection.h"
#include "nsITreeColumns.h"
#include "nsITreeView.h"
#include "nsTreeUtils.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsQuickSort.h"
#include "nsTreeRows.h"
#include "nsTemplateRule.h"
#include "nsTemplateMatch.h"
#include "nsGkAtoms.h"
#include "nsXULContentUtils.h"
#include "nsXULTemplateBuilder.h"
#include "nsVoidArray.h"
#include "nsUnicharUtils.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMClassInfo.h"


#include "nsIDocument.h"





class nsXULTreeBuilder : public nsXULTemplateBuilder,
                         public nsIXULTreeBuilder,
                         public nsINativeTreeView
{
public:
    
    NS_DECL_ISUPPORTS_INHERITED

    
    NS_DECL_NSIXULTREEBUILDER

    
    NS_DECL_NSITREEVIEW
    
    NS_IMETHOD EnsureNative() { return NS_OK; }

    virtual void NodeWillBeDestroyed(const nsINode* aNode);

protected:
    friend NS_IMETHODIMP
    NS_NewXULTreeBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    nsXULTreeBuilder();

    


    virtual void Uninit(PRBool aIsFinal);

    


    nsresult
    EnsureSortVariables();

    virtual nsresult
    RebuildAll();

    



    nsresult
    GetTemplateActionRowFor(PRInt32 aRow, nsIContent** aResult);

    



    nsresult
    GetTemplateActionCellFor(PRInt32 aRow, nsITreeColumn* aCol, nsIContent** aResult);

    


    nsresult
    GetResourceFor(PRInt32 aRow, nsIRDFResource** aResource);

    



    nsresult
    OpenContainer(PRInt32 aIndex, nsIXULTemplateResult* aResult);

    



    nsresult
    OpenSubtreeOf(nsTreeRows::Subtree* aSubtree,
                  PRInt32 aIndex,
                  nsIXULTemplateResult *aResult,
                  PRInt32* aDelta);

    nsresult
    OpenSubtreeForQuerySet(nsTreeRows::Subtree* aSubtree,
                           PRInt32 aIndex,
                           nsIXULTemplateResult *aResult,
                           nsTemplateQuerySet* aQuerySet,
                           PRInt32* aDelta,
                           nsAutoVoidArray& open);

    



    nsresult
    CloseContainer(PRInt32 aIndex);

    


    nsresult
    RemoveMatchesFor(nsTreeRows::Subtree& subtree);

    


    nsresult
    IsContainerOpen(nsIXULTemplateResult *aResult, PRBool* aOpen);

    nsresult
    IsContainerOpen(nsIRDFResource* aResource, PRBool* aOpen);

    


    static int PR_CALLBACK
    Compare(const void* aLeft, const void* aRight, void* aClosure);

    


    PRInt32
    CompareResults(nsIXULTemplateResult* aLeft, nsIXULTemplateResult* aRight);

    



    nsresult
    SortSubtree(nsTreeRows::Subtree* aSubtree);

    NS_IMETHOD
    HasGeneratedContent(nsIRDFResource* aResource,
                        nsIAtom* aTag,
                        PRBool* aGenerated);

    
    

    



    PRBool
    GetInsertionLocations(nsIXULTemplateResult* aResult,
                          nsCOMArray<nsIContent>** aLocations);

    


    virtual nsresult
    ReplaceMatch(nsIXULTemplateResult* aOldResult,
                 nsTemplateMatch* aNewMatch,
                 nsTemplateRule* aNewMatchRule,
                 void *aContext);

    


    virtual nsresult
    SynchronizeResult(nsIXULTemplateResult* aResult);

    


    nsCOMPtr<nsITreeBoxObject> mBoxObject;

    


    nsCOMPtr<nsITreeSelection> mSelection;

    


    nsCOMPtr<nsIRDFDataSource> mPersistStateStore;

    


    nsTreeRows mRows;

    


    nsCOMPtr<nsIAtom> mSortVariable;

    enum Direction {
        eDirection_Descending = -1,
        eDirection_Natural    =  0,
        eDirection_Ascending  = +1
    };

    


    Direction mSortDirection;

    


    nsCOMPtr<nsISupportsArray> mObservers;
};



NS_IMETHODIMP
NS_NewXULTreeBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    *aResult = nsnull;

    NS_PRECONDITION(aOuter == nsnull, "no aggregation");
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsresult rv;
    nsXULTreeBuilder* result = new nsXULTreeBuilder();
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result); 

    rv = result->InitGlobals();

    if (NS_SUCCEEDED(rv))
        rv = result->QueryInterface(aIID, aResult);

    NS_RELEASE(result);
    return rv;
}

NS_IMPL_ADDREF_INHERITED(nsXULTreeBuilder, nsXULTemplateBuilder)
NS_IMPL_RELEASE_INHERITED(nsXULTreeBuilder, nsXULTemplateBuilder)

NS_INTERFACE_MAP_BEGIN(nsXULTreeBuilder)
  NS_INTERFACE_MAP_ENTRY(nsIXULTreeBuilder)
  NS_INTERFACE_MAP_ENTRY(nsITreeView)
  NS_INTERFACE_MAP_ENTRY_DOM_CLASSINFO(XULTreeBuilder)
NS_INTERFACE_MAP_END_INHERITING(nsXULTemplateBuilder)


nsXULTreeBuilder::nsXULTreeBuilder()
    : mSortDirection(eDirection_Natural)
{
}

void
nsXULTreeBuilder::Uninit(PRBool aIsFinal)
{
    PRInt32 count = mRows.Count();
    mRows.Clear();

    if (mBoxObject) {
        mBoxObject->BeginUpdateBatch();
        mBoxObject->RowCountChanged(0, -count);
    }

    nsXULTemplateBuilder::Uninit(aIsFinal);
}







NS_IMETHODIMP
nsXULTreeBuilder::GetResourceAtIndex(PRInt32 aRowIndex, nsIRDFResource** aResult)
{
    if (aRowIndex < 0 || aRowIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    return GetResourceFor(aRowIndex, aResult);
}

NS_IMETHODIMP
nsXULTreeBuilder::GetIndexOfResource(nsIRDFResource* aResource, PRInt32* aResult)
{
    NS_ENSURE_ARG_POINTER(aResource);
    nsTreeRows::iterator iter = mRows.FindByResource(aResource);
    if (iter == mRows.Last())
        *aResult = -1;
    else
        *aResult = iter.GetRowIndex();
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::AddObserver(nsIXULTreeBuilderObserver* aObserver)
{
    nsresult rv;  
    if (!mObservers) {
        rv = NS_NewISupportsArray(getter_AddRefs(mObservers));
        if (NS_FAILED(rv))
            return rv;
    }

    return mObservers->AppendElement(aObserver);
}

NS_IMETHODIMP
nsXULTreeBuilder::RemoveObserver(nsIXULTreeBuilderObserver* aObserver)
{
    return mObservers ? mObservers->RemoveElement(aObserver) : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULTreeBuilder::Sort(nsIDOMElement* aElement)
{
    nsCOMPtr<nsIContent> header = do_QueryInterface(aElement);
    if (! header)
        return NS_ERROR_FAILURE;

    if (header->AttrValueIs(kNameSpaceID_None, nsGkAtoms::sortLocked,
                            nsGkAtoms::_true, eCaseMatters))
        return NS_OK;

    nsAutoString sort;
    header->GetAttr(kNameSpaceID_None, nsGkAtoms::sort, sort);

    if (sort.IsEmpty())
        return NS_OK;

    
    mSortVariable = do_GetAtom(sort);

    
    nsAutoString dir;
    header->GetAttr(kNameSpaceID_None, nsGkAtoms::sortDirection, dir);

    if (dir.EqualsLiteral("ascending")) {
        dir.AssignLiteral("descending");
        mSortDirection = eDirection_Descending;
    }
    else if (dir.EqualsLiteral("descending")) {
        dir.AssignLiteral("natural");
        mSortDirection = eDirection_Natural;
    }
    else {
        dir.AssignLiteral("ascending");
        mSortDirection = eDirection_Ascending;
    }

    
    SortSubtree(mRows.GetRoot());
    mRows.InvalidateCachedRow();
    if (mBoxObject) 
        mBoxObject->Invalidate();

    nsTreeUtils::UpdateSortIndicators(header, dir);

    return NS_OK;
}






NS_IMETHODIMP
nsXULTreeBuilder::GetRowCount(PRInt32* aRowCount)
{
    *aRowCount = mRows.Count();
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetSelection(nsITreeSelection** aSelection)
{
    NS_IF_ADDREF(*aSelection = mSelection.get());
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::SetSelection(nsITreeSelection* aSelection)
{
    mSelection = aSelection;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetRowProperties(PRInt32 aIndex, nsISupportsArray* aProperties)
{
    NS_ENSURE_ARG_POINTER(aProperties);
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIContent> row;
    GetTemplateActionRowFor(aIndex, getter_AddRefs(row));
    if (row) {
        nsAutoString raw;
        row->GetAttr(kNameSpaceID_None, nsGkAtoms::properties, raw);

        if (!raw.IsEmpty()) {
            nsAutoString cooked;
            SubstituteText(mRows[aIndex]->mMatch->mResult, raw, cooked);

            nsTreeUtils::TokenizeProperties(cooked, aProperties);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetCellProperties(PRInt32 aRow, nsITreeColumn* aCol, nsISupportsArray* aProperties)
{
    NS_ENSURE_ARG_POINTER(aCol);
    NS_ENSURE_ARG_POINTER(aProperties);
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad row");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::properties, raw);

        if (!raw.IsEmpty()) {
            nsAutoString cooked;
            SubstituteText(mRows[aRow]->mMatch->mResult, raw, cooked);

            nsTreeUtils::TokenizeProperties(cooked, aProperties);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetColumnProperties(nsITreeColumn* aCol,
                                      nsISupportsArray* aProperties)
{
    NS_ENSURE_ARG_POINTER(aCol);
    NS_ENSURE_ARG_POINTER(aProperties);
    
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsContainer(PRInt32 aIndex, PRBool* aResult)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::iterator iter = mRows[aIndex];

    if (iter->mContainerType == nsTreeRows::eContainerType_Unknown) {
        PRBool isContainer;
        iter->mMatch->mResult->GetIsContainer(&isContainer);

        iter->mContainerType = isContainer
            ? nsTreeRows::eContainerType_Container
            : nsTreeRows::eContainerType_Noncontainer;
    }

    *aResult = (iter->mContainerType == nsTreeRows::eContainerType_Container);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsContainerOpen(PRInt32 aIndex, PRBool* aOpen)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::iterator iter = mRows[aIndex];

    if (iter->mContainerState == nsTreeRows::eContainerState_Unknown) {
        PRBool isOpen;
        IsContainerOpen(iter->mMatch->mResult, &isOpen);

        iter->mContainerState = isOpen
            ? nsTreeRows::eContainerState_Open
            : nsTreeRows::eContainerState_Closed;
    }

    *aOpen = (iter->mContainerState == nsTreeRows::eContainerState_Open);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsContainerEmpty(PRInt32 aIndex, PRBool* aResult)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::iterator iter = mRows[aIndex];
    NS_ASSERTION(iter->mContainerType == nsTreeRows::eContainerType_Container,
                 "asking for empty state on non-container");

    
    
    
    if ((mFlags & eDontRecurse) && (iter->mMatch->mResult != mRootResult)) {
        *aResult = PR_TRUE;
        return NS_OK;
    }

    if (iter->mContainerFill == nsTreeRows::eContainerFill_Unknown) {
        PRBool isEmpty;
        iter->mMatch->mResult->GetIsEmpty(&isEmpty);

        iter->mContainerFill = isEmpty
            ? nsTreeRows::eContainerFill_Empty
            : nsTreeRows::eContainerFill_Nonempty;
    }

    *aResult = (iter->mContainerFill == nsTreeRows::eContainerFill_Empty);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsSeparator(PRInt32 aIndex, PRBool* aResult)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsAutoString type;
    nsTreeRows::Row& row = *(mRows[aIndex]);
    row.mMatch->mResult->GetType(type);

    *aResult = type.EqualsLiteral("separator");

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetParentIndex(PRInt32 aRowIndex, PRInt32* aResult)
{
    NS_PRECONDITION(aRowIndex >= 0 && aRowIndex < mRows.Count(), "bad row");
    if (aRowIndex < 0 || aRowIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    nsTreeRows::iterator iter = mRows[aRowIndex];

    
    nsTreeRows::Subtree* parent = iter.GetParent();

    
    
    PRInt32 index = iter.GetChildIndex();
    while (--index >= 0)
        aRowIndex -= mRows.GetSubtreeSizeFor(parent, index) + 1;

    
    *aResult = aRowIndex - 1;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::HasNextSibling(PRInt32 aRowIndex, PRInt32 aAfterIndex, PRBool* aResult)
{
    NS_PRECONDITION(aRowIndex >= 0 && aRowIndex < mRows.Count(), "bad row");
    if (aRowIndex < 0 || aRowIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    nsTreeRows::iterator iter = mRows[aRowIndex];

    
    nsTreeRows::Subtree* parent = iter.GetParent();

    
    
    *aResult = iter.GetChildIndex() != parent->Count() - 1;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetLevel(PRInt32 aRowIndex, PRInt32* aResult)
{
    NS_PRECONDITION(aRowIndex >= 0 && aRowIndex < mRows.Count(), "bad row");
    if (aRowIndex < 0 || aRowIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    
    nsTreeRows::iterator iter = mRows[aRowIndex];
    *aResult = iter.GetDepth() - 1;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetImageSrc(PRInt32 aRow, nsITreeColumn* aCol, nsAString& aResult)
{
    NS_ENSURE_ARG_POINTER(aCol);
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad index");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::src, raw);

        SubstituteText(mRows[aRow]->mMatch->mResult, raw, aResult);
    }
    else
        aResult.Truncate();

    return NS_OK;
}


NS_IMETHODIMP
nsXULTreeBuilder::GetProgressMode(PRInt32 aRow, nsITreeColumn* aCol, PRInt32* aResult)
{
    NS_ENSURE_ARG_POINTER(aCol);
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad index");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    *aResult = nsITreeView::PROGRESS_NONE;

    
    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::mode, raw);

        nsAutoString mode;
        SubstituteText(mRows[aRow]->mMatch->mResult, raw, mode);

        if (mode.EqualsLiteral("normal"))
            *aResult = nsITreeView::PROGRESS_NORMAL;
        else if (mode.EqualsLiteral("undetermined"))
            *aResult = nsITreeView::PROGRESS_UNDETERMINED;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetCellValue(PRInt32 aRow, nsITreeColumn* aCol, nsAString& aResult)
{
    NS_ENSURE_ARG_POINTER(aCol);
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad index");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::value, raw);

        SubstituteText(mRows[aRow]->mMatch->mResult, raw, aResult);
    }
    else
        aResult.Truncate();

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetCellText(PRInt32 aRow, nsITreeColumn* aCol, nsAString& aResult)
{
    NS_ENSURE_ARG_POINTER(aCol);
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad index");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::label, raw);

        SubstituteText(mRows[aRow]->mMatch->mResult, raw, aResult);

    }
    else
        aResult.Truncate();

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::SetTree(nsITreeBoxObject* aTree)
{
    NS_PRECONDITION(mRoot, "not initialized");

    mBoxObject = aTree;

    
    if (!mBoxObject) {
        Uninit(PR_FALSE);
        return NS_OK;
    }

    
    PRBool isTrusted = PR_FALSE;
    nsresult rv = IsSystemPrincipal(mRoot->NodePrincipal(), &isTrusted);
    if (NS_SUCCEEDED(rv) && isTrusted) {
        
        nsAutoString datasourceStr;
        mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::statedatasource, datasourceStr);

        
        
        
        if (! datasourceStr.IsEmpty()) {
            gRDFService->GetDataSource(NS_ConvertUTF16toUTF8(datasourceStr).get(),
                                       getter_AddRefs(mPersistStateStore));
        }
        else {
            gRDFService->GetDataSource("rdf:local-store",
                                       getter_AddRefs(mPersistStateStore));
        }
    }

    
    
    
    
    
    
    if (! mPersistStateStore) {
        mPersistStateStore =
            do_CreateInstance("@mozilla.org/rdf/datasource;1?name=in-memory-datasource");
    }

    NS_ASSERTION(mPersistStateStore, "failed to get a persistent state store");
    if (! mPersistStateStore)
        return NS_ERROR_FAILURE;

    Rebuild();

    EnsureSortVariables();
    if (mSortVariable)
        SortSubtree(mRows.GetRoot());

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::ToggleOpenState(PRInt32 aIndex)
{
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsIXULTemplateResult* result = mRows[aIndex]->mMatch->mResult;
    if (! result)
        return NS_ERROR_FAILURE;

    if (mFlags & eDontRecurse)
        return NS_OK;

    if (result && result != mRootResult) {
        
        PRBool mayProcessChildren;
        nsresult rv = result->GetMayProcessChildren(&mayProcessChildren);
        if (NS_FAILED(rv) || !mayProcessChildren)
            return rv;
    }

    if (mObservers) {
        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer)
                observer->OnToggleOpenState(aIndex);
        }
    }

    if (mPersistStateStore) {
        PRBool isOpen;
        IsContainerOpen(aIndex, &isOpen);

        nsCOMPtr<nsIRDFResource> container;
        GetResourceFor(aIndex, getter_AddRefs(container));
        if (! container)
            return NS_ERROR_FAILURE;

        PRBool hasProperty;
        IsContainerOpen(container, &hasProperty);

        if (isOpen) {
            if (hasProperty) {
                mPersistStateStore->Unassert(container,
                                             nsXULContentUtils::NC_open,
                                             nsXULContentUtils::true_);
            }

            CloseContainer(aIndex);
        }
        else {
            if (! hasProperty) {
                mPersistStateStore->Assert(container,
                                           nsXULContentUtils::NC_open,
                                           nsXULContentUtils::true_,
                                           PR_TRUE);
            }

            OpenContainer(aIndex, result);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::CycleHeader(nsITreeColumn* aCol)
{
    NS_ENSURE_ARG_POINTER(aCol);
    nsCOMPtr<nsIDOMElement> element;
    aCol->GetElement(getter_AddRefs(element));

    if (mObservers) {
        nsAutoString id;
        aCol->GetId(id);

        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer)
                observer->OnCycleHeader(id.get(), element);
        }
    }

    return Sort(element);
}

NS_IMETHODIMP
nsXULTreeBuilder::SelectionChanged()
{
    if (mObservers) {
        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer)
                observer->OnSelectionChanged();
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::CycleCell(PRInt32 aRow, nsITreeColumn* aCol)
{
    NS_ENSURE_ARG_POINTER(aCol);
    if (mObservers) {
        nsAutoString id;
        aCol->GetId(id);

        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer)
                observer->OnCycleCell(aRow, id.get());
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsEditable(PRInt32 aRow, nsITreeColumn* aCol, PRBool* _retval)
{
    *_retval = PR_TRUE;
    NS_ENSURE_ARG_POINTER(aCol);
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad index");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::editable, raw);

        nsAutoString editable;
        SubstituteText(mRows[aRow]->mMatch->mResult, raw, editable);

        if (editable.EqualsLiteral("false"))
            *_retval = PR_FALSE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsSelectable(PRInt32 aRow, nsITreeColumn* aCol, PRBool* _retval)
{
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad index");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    *_retval = PR_TRUE;

    
    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::selectable, raw);

        nsAutoString selectable;
        SubstituteText(mRows[aRow]->mMatch->mResult, raw, selectable);

        if (selectable.EqualsLiteral("false"))
            *_retval = PR_FALSE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::SetCellValue(PRInt32 aRow, nsITreeColumn* aCol, const nsAString& aValue)
{
    NS_ENSURE_ARG_POINTER(aCol);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::SetCellText(PRInt32 aRow, nsITreeColumn* aCol, const nsAString& aValue)
{
    NS_ENSURE_ARG_POINTER(aCol);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::PerformAction(const PRUnichar* aAction)
{
    if (mObservers) {  
        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer)
                observer->OnPerformAction(aAction);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::PerformActionOnRow(const PRUnichar* aAction, PRInt32 aRow)
{
    if (mObservers) {  
        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer)
                observer->OnPerformActionOnRow(aAction, aRow);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::PerformActionOnCell(const PRUnichar* aAction, PRInt32 aRow, nsITreeColumn* aCol)
{
    NS_ENSURE_ARG_POINTER(aCol);
    if (mObservers) {  
        nsAutoString id;
        aCol->GetId(id);

        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer)
                observer->OnPerformActionOnCell(aAction, aRow, id.get());
        }
    }

    return NS_OK;
}


void
nsXULTreeBuilder::NodeWillBeDestroyed(const nsINode* aNode)
{
    if (mObservers)
        mObservers->Clear();

    nsXULTemplateBuilder::NodeWillBeDestroyed(aNode);
}

NS_IMETHODIMP
nsXULTreeBuilder::HasGeneratedContent(nsIRDFResource* aResource,
                                      nsIAtom* aTag,
                                      PRBool* aGenerated)
{
    *aGenerated = PR_FALSE;
    NS_ENSURE_ARG_POINTER(aResource);

    if (!mRootResult)
        return NS_OK;

    nsCOMPtr<nsIRDFResource> rootresource;
    nsresult rv = mRootResult->GetResource(getter_AddRefs(rootresource));
    if (NS_FAILED(rv))
        return rv;

    if (aResource == rootresource ||
        mRows.FindByResource(aResource) != mRows.Last())
        *aGenerated = PR_TRUE;

    return NS_OK;
}

PRBool
nsXULTreeBuilder::GetInsertionLocations(nsIXULTemplateResult* aResult,
                                        nsCOMArray<nsIContent>** aLocations)
{
    *aLocations = nsnull;

    
    

    nsAutoString ref;
    nsresult rv = aResult->GetBindingFor(mRefVariable, ref);
    if (NS_FAILED(rv) || ref.IsEmpty())
        return PR_FALSE;

    nsCOMPtr<nsIRDFResource> container;
    rv = gRDFService->GetUnicodeResource(ref, getter_AddRefs(container));
    if (NS_FAILED(rv))
        return PR_FALSE;

    
    if (container == mRows.GetRootResource())
        return PR_TRUE;

    nsTreeRows::iterator iter = mRows.FindByResource(container);
    if (iter == mRows.Last())
        return PR_FALSE;

    return (iter->mContainerState == nsTreeRows::eContainerState_Open);
}

nsresult
nsXULTreeBuilder::ReplaceMatch(nsIXULTemplateResult* aOldResult,
                               nsTemplateMatch* aNewMatch,
                               nsTemplateRule* aNewMatchRule,
                               void *aLocation)
{
    if (! mBoxObject)
        return NS_OK;

    if (aOldResult) {
        
        nsTreeRows::iterator iter = mRows.Find(aOldResult);

        NS_ASSERTION(iter != mRows.Last(), "couldn't find row");
        if (iter == mRows.Last())
            return NS_ERROR_FAILURE;

        
        PRInt32 row = iter.GetRowIndex();

        
        
        
        PRInt32 delta = mRows.GetSubtreeSizeFor(iter);
        if (delta)
            RemoveMatchesFor(*(iter->mSubtree));

        if (mRows.RemoveRowAt(iter) == 0 && iter.GetRowIndex() >= 0) {

            
            
            iter->mContainerFill = nsTreeRows::eContainerFill_Unknown;

            nsCOMPtr<nsITreeColumns> cols;
            mBoxObject->GetColumns(getter_AddRefs(cols));
            if (cols) {
                nsCOMPtr<nsITreeColumn> primaryCol;
                cols->GetPrimaryColumn(getter_AddRefs(primaryCol));
                if (primaryCol)
                    mBoxObject->InvalidateCell(iter.GetRowIndex(), primaryCol);
            }
        }

        
        mBoxObject->RowCountChanged(row, -delta - 1);
    }

    if (aNewMatch && aNewMatch->mResult) {
        
        PRInt32 row = -1;
        nsTreeRows::Subtree* parent = nsnull;
        nsIXULTemplateResult* result = aNewMatch->mResult;

        nsAutoString ref;
        nsresult rv = result->GetBindingFor(mRefVariable, ref);
        if (NS_FAILED(rv) || ref.IsEmpty())
            return rv;

        nsCOMPtr<nsIRDFResource> container;
        rv = gRDFService->GetUnicodeResource(ref, getter_AddRefs(container));
        if (NS_FAILED(rv))
            return rv;

        if (container != mRows.GetRootResource()) {
            nsTreeRows::iterator iter = mRows.FindByResource(container);
            row = iter.GetRowIndex();

            NS_ASSERTION(iter != mRows.Last(), "couldn't find container row");
            if (iter == mRows.Last())
                return NS_ERROR_FAILURE;

            
            
            PRBool open = PR_FALSE;
            IsContainerOpen(row, &open);

            
            if (open)
                parent = mRows.EnsureSubtreeFor(iter);

            
            
            
            if ((iter->mContainerType != nsTreeRows::eContainerType_Container) ||
                (iter->mContainerFill != nsTreeRows::eContainerFill_Nonempty)) {
                iter->mContainerType  = nsTreeRows::eContainerType_Container;
                iter->mContainerFill = nsTreeRows::eContainerFill_Nonempty;
                mBoxObject->InvalidateRow(iter.GetRowIndex());
            }
        }
        else {
            parent = mRows.GetRoot();
        }

        if (parent) {
            
            
            
            PRInt32 index = parent->Count();

            if (mSortVariable) {
                
                
                PRInt32 left = 0;
                PRInt32 right = index;

                while (left < right) {
                    index = (left + right) / 2;
                    PRInt32 cmp = CompareResults((*parent)[index].mMatch->mResult, result);
                    if (cmp < 0)
                        left = ++index;
                    else if (cmp > 0)
                        right = index;
                    else
                        break;
                }
            }

            nsTreeRows::iterator iter =
                mRows.InsertRowAt(aNewMatch, parent, index);

            mBoxObject->RowCountChanged(iter.GetRowIndex(), +1);

            
            

            if (mFlags & eDontRecurse)
                return NS_OK;

            if (result && (result != mRootResult)) {
                
                PRBool mayProcessChildren;
                nsresult rv = result->GetMayProcessChildren(&mayProcessChildren);
                if (NS_FAILED(rv) || ! mayProcessChildren) return NS_OK;
            }

            PRBool open;
            IsContainerOpen(result, &open);
            if (open)
                OpenContainer(iter.GetRowIndex(), result);
        }
    }

    return NS_OK;
}

nsresult
nsXULTreeBuilder::SynchronizeResult(nsIXULTemplateResult* aResult)
{
    if (mBoxObject) {
        
        

        nsTreeRows::iterator iter = mRows.Find(aResult);

        NS_ASSERTION(iter != mRows.Last(), "couldn't find row");
        if (iter == mRows.Last())
            return NS_ERROR_FAILURE;

        PRInt32 row = iter.GetRowIndex();
        if (row >= 0)
            mBoxObject->InvalidateRow(row);

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("xultemplate[%p]   => row %d", this, row));
    }

    return NS_OK;
}



nsresult
nsXULTreeBuilder::EnsureSortVariables()
{
    
    
    nsCOMPtr<nsIContent> treecols;
 
    nsXULContentUtils::FindChildByTag(mRoot, kNameSpaceID_XUL,
                                      nsGkAtoms::treecols,
                                      getter_AddRefs(treecols));

    if (!treecols)
        return NS_OK;

    PRUint32 count = treecols->GetChildCount();
    for (PRUint32 i = 0; i < count; ++i) {
        nsIContent *child = treecols->GetChildAt(i);

        if (child->NodeInfo()->Equals(nsGkAtoms::treecol,
                                      kNameSpaceID_XUL)) {
            if (child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::sortActive,
                                   nsGkAtoms::_true, eCaseMatters)) {
                nsAutoString sort;
                child->GetAttr(kNameSpaceID_None, nsGkAtoms::sort, sort);
                if (! sort.IsEmpty()) {
                    mSortVariable = do_GetAtom(sort);

                    static nsIContent::AttrValuesArray strings[] =
                      {&nsGkAtoms::ascending, &nsGkAtoms::descending, nsnull};
                    switch (child->FindAttrValueIn(kNameSpaceID_None,
                                                   nsGkAtoms::sortDirection,
                                                   strings, eCaseMatters)) {
                       case 0: mSortDirection = eDirection_Ascending; break;
                       case 1: mSortDirection = eDirection_Descending; break;
                       default: mSortDirection = eDirection_Natural; break;
                    }
                }
                break;
            }
        }
    }

    return NS_OK;
}

nsresult
nsXULTreeBuilder::RebuildAll()
{
    NS_PRECONDITION(mRoot != nsnull, "not initialized");
    if (! mRoot)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsIDocument> doc = mRoot->GetDocument();

    
    if (!doc)
        return NS_OK;

    if (! mQueryProcessor)
        return NS_OK;

    if (mQueriesCompiled) {
        Uninit(PR_FALSE);
    }
    else if (mBoxObject) {
        PRInt32 count = mRows.Count();
        mRows.Clear();
        mBoxObject->BeginUpdateBatch();
        mBoxObject->RowCountChanged(0, -count);
    }

    nsresult rv = CompileQueries();
    if (NS_FAILED(rv))
        return rv;

    if (mQuerySets.Length() == 0)
        return NS_OK;

    
    nsAutoString ref;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::ref, ref);

    if (! ref.IsEmpty()) {
        rv = mQueryProcessor->TranslateRef(mDB, ref, getter_AddRefs(mRootResult));
        if (NS_FAILED(rv))
            return rv;

        if (mRootResult) {
            OpenContainer(-1, mRootResult);

            nsCOMPtr<nsIRDFResource> rootResource;
            GetResultResource(mRootResult, getter_AddRefs(rootResource));

            mRows.SetRootResource(rootResource);
        }
    }

    if (mBoxObject) {
        mBoxObject->EndUpdateBatch();
    }

    return NS_OK;
}

nsresult
nsXULTreeBuilder::GetTemplateActionRowFor(PRInt32 aRow, nsIContent** aResult)
{
    
    
    nsTreeRows::Row& row = *(mRows[aRow]);

    nsCOMPtr<nsIContent> action;

    
    
    
    PRInt16 ruleindex = row.mMatch->RuleIndex();
    if (ruleindex >= 0) {
        nsTemplateQuerySet* qs = mQuerySets[row.mMatch->QuerySetPriority()];
        nsTemplateRule* rule = qs->GetRuleAt(ruleindex);
        if (rule) {
            rule->GetAction(getter_AddRefs(action));

            nsCOMPtr<nsIContent> children;
            nsXULContentUtils::FindChildByTag(action, kNameSpaceID_XUL,
                                              nsGkAtoms::treechildren,
                                              getter_AddRefs(children));
            if (children) {
                nsCOMPtr<nsIContent> item;
                nsXULContentUtils::FindChildByTag(children, kNameSpaceID_XUL,
                                                  nsGkAtoms::treeitem,
                                                  getter_AddRefs(item));
                if (item)
                    return nsXULContentUtils::FindChildByTag(item,
                                                             kNameSpaceID_XUL,
                                                             nsGkAtoms::treerow,
                                                             aResult);
            }
        }
    }

    *aResult = nsnull;
    return NS_OK;
}

nsresult
nsXULTreeBuilder::GetTemplateActionCellFor(PRInt32 aRow,
                                           nsITreeColumn* aCol,
                                           nsIContent** aResult)
{
    *aResult = nsnull;

    if (!aCol) return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIContent> row;
    GetTemplateActionRowFor(aRow, getter_AddRefs(row));
    if (row) {
        nsCOMPtr<nsIAtom> colAtom;
        PRInt32 colIndex;
        aCol->GetAtom(getter_AddRefs(colAtom));
        aCol->GetIndex(&colIndex);

        PRUint32 count = row->GetChildCount();
        PRUint32 j = 0;
        for (PRUint32 i = 0; i < count; ++i) {
            nsIContent *child = row->GetChildAt(i);

            if (child->NodeInfo()->Equals(nsGkAtoms::treecell,
                                          kNameSpaceID_XUL)) {
                if (colAtom &&
                    child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::ref,
                                       colAtom, eCaseMatters)) {
                    *aResult = child;
                    break;
                }
                else if (j == (PRUint32)colIndex)
                    *aResult = child;
                j++;
            }
        }
    }
    NS_IF_ADDREF(*aResult);

    return NS_OK;
}

nsresult
nsXULTreeBuilder::GetResourceFor(PRInt32 aRow, nsIRDFResource** aResource)
{
    nsTreeRows::Row& row = *(mRows[aRow]);
    return GetResultResource(row.mMatch->mResult, aResource);
}

nsresult
nsXULTreeBuilder::OpenContainer(PRInt32 aIndex, nsIXULTemplateResult* aResult)
{
    
    NS_ASSERTION(aIndex >= -1 && aIndex < mRows.Count(), "bad row");
    if (aIndex < -1 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::Subtree* container;

    if (aIndex >= 0) {
        nsTreeRows::iterator iter = mRows[aIndex];
        container = mRows.EnsureSubtreeFor(iter.GetParent(),
                                           iter.GetChildIndex());

        iter->mContainerState = nsTreeRows::eContainerState_Open;
    }
    else
        container = mRows.GetRoot();

    if (! container)
        return NS_ERROR_OUT_OF_MEMORY;

    PRInt32 count;
    OpenSubtreeOf(container, aIndex, aResult, &count);

    
    if (mBoxObject) {
        if (aIndex >= 0)
            mBoxObject->InvalidateRow(aIndex);

        if (count)
            mBoxObject->RowCountChanged(aIndex + 1, count);
    }

    return NS_OK;
}

nsresult
nsXULTreeBuilder::OpenSubtreeOf(nsTreeRows::Subtree* aSubtree,
                                PRInt32 aIndex,
                                nsIXULTemplateResult *aResult,
                                PRInt32* aDelta)
{
    nsAutoVoidArray open;
    PRInt32 count = 0;

    PRInt32 rulecount = mQuerySets.Length();

    for (PRInt32 r = 0; r < rulecount; r++) {
        nsTemplateQuerySet* queryset = mQuerySets[r];
        OpenSubtreeForQuerySet(aSubtree, aIndex, aResult, queryset, &count, open);
    }

    
    
    for (PRInt32 i = open.Count() - 1; i >= 0; --i) {
        PRInt32 index = NS_PTR_TO_INT32(open[i]);

        nsTreeRows::Subtree* child =
            mRows.EnsureSubtreeFor(aSubtree, index);

        nsIXULTemplateResult* result = (*aSubtree)[index].mMatch->mResult;

        PRInt32 delta;
        OpenSubtreeOf(child, aIndex + index, result, &delta);
        count += delta;
    }

    
    if (mSortVariable) {
        NS_QuickSort(mRows.GetRowsFor(aSubtree),
                     aSubtree->Count(),
                     sizeof(nsTreeRows::Row),
                     Compare,
                     this);
    }

    *aDelta = count;
    return NS_OK;
}

nsresult
nsXULTreeBuilder::OpenSubtreeForQuerySet(nsTreeRows::Subtree* aSubtree,
                                         PRInt32 aIndex,
                                         nsIXULTemplateResult* aResult,
                                         nsTemplateQuerySet* aQuerySet,
                                         PRInt32* aDelta,
                                         nsAutoVoidArray& open)
{
    PRInt32 count = *aDelta;
    
    nsCOMPtr<nsISimpleEnumerator> results;
    nsresult rv = mQueryProcessor->GenerateResults(mDB, aResult,
                                                   aQuerySet->mCompiledQuery,
                                                   getter_AddRefs(results));
    if (NS_FAILED(rv))
        return rv;

    PRBool hasMoreResults;
    rv = results->HasMoreElements(&hasMoreResults);

    for (; NS_SUCCEEDED(rv) && hasMoreResults;
           rv = results->HasMoreElements(&hasMoreResults)) {
        nsCOMPtr<nsISupports> nr;
        rv = results->GetNext(getter_AddRefs(nr));
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsIXULTemplateResult> nextresult = do_QueryInterface(nr);
        if (!nextresult)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIRDFResource> resultid;
        rv = GetResultResource(nextresult, getter_AddRefs(resultid));
        if (NS_FAILED(rv))
            return rv;

        if (! resultid)
            continue;

        
        
        

        PRBool generateContent = PR_TRUE;

        nsTemplateMatch* prevmatch = nsnull;
        nsTemplateMatch* existingmatch = nsnull;
        if (mMatchMap.Get(resultid, &existingmatch)){
            
            while (existingmatch) {
                if (existingmatch->IsActive())
                    generateContent = PR_FALSE;
                prevmatch = existingmatch;
                existingmatch = existingmatch->mNext;
            }
        }

        nsTemplateMatch *newmatch =
            nsTemplateMatch::Create(mPool, aQuerySet->Priority(),
                                    nextresult, nsnull);
        if (!newmatch)
            return NS_ERROR_OUT_OF_MEMORY;

        if (generateContent) {
            
            PRBool cyclic = PR_FALSE;

            if (aIndex >= 0) {
                for (nsTreeRows::iterator iter = mRows[aIndex]; iter.GetDepth() > 0; iter.Pop()) {
                    nsCOMPtr<nsIRDFResource> parentid;
                    rv = GetResultResource(iter->mMatch->mResult, getter_AddRefs(parentid));
                    if (NS_FAILED(rv)) {
                        nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                        return rv;
                    }

                    if (resultid == parentid) {
                        cyclic = PR_TRUE;
                        break;
                    }
                }
            }

            if (cyclic) {
                NS_WARNING("tree cannot handle cyclic graphs");
                nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                continue;
            }

            PRInt16 ruleindex;
            nsTemplateRule* matchedrule = nsnull;
            rv = DetermineMatchedRule(nsnull, nextresult, aQuerySet,
                                      &matchedrule, &ruleindex);
            if (NS_FAILED(rv)) {
                nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                return rv;
            }

            if (matchedrule) {
                rv = newmatch->RuleMatched(aQuerySet, matchedrule, ruleindex,
                                           nextresult);
                if (NS_FAILED(rv)) {
                    nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                    return rv;
                }

                
                mRows.InsertRowAt(newmatch, aSubtree, count);

                
                
                PRBool isOpen = PR_FALSE;
                IsContainerOpen(nextresult, &isOpen);
                if (isOpen) {
                    if (!open.AppendElement(NS_INT32_TO_PTR(count)))
                        return NS_ERROR_OUT_OF_MEMORY;
                }

                ++count;
            }
        }

        if (prevmatch) {
            prevmatch->mNext = newmatch;
        }
        else if (!mMatchMap.Put(resultid, newmatch)) {
            nsTemplateMatch::Destroy(mPool, newmatch, PR_TRUE);
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    *aDelta = count;
    return rv;
}

nsresult
nsXULTreeBuilder::CloseContainer(PRInt32 aIndex)
{
    NS_ASSERTION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::iterator iter = mRows[aIndex];

    nsTreeRows::Subtree& subtree = *(iter->mSubtree);

    RemoveMatchesFor(subtree);

    
    iter = mRows[aIndex];

    PRInt32 count = mRows.GetSubtreeSizeFor(iter);
    mRows.RemoveSubtreeFor(iter);

    iter->mContainerState = nsTreeRows::eContainerState_Closed;

    if (mBoxObject) {
        mBoxObject->InvalidateRow(aIndex);

        if (count)
            mBoxObject->RowCountChanged(aIndex + 1, -count);
    }

    return NS_OK;
}

nsresult
nsXULTreeBuilder::RemoveMatchesFor(nsTreeRows::Subtree& subtree)
{
    for (PRInt32 i = subtree.Count() - 1; i >= 0; --i) {
        nsTreeRows::Row& row = subtree[i];

        nsTemplateMatch* match = row.mMatch;

        nsCOMPtr<nsIRDFResource> id;
        nsresult rv = GetResultResource(match->mResult, getter_AddRefs(id));
        if (NS_FAILED(rv))
            return rv;

        nsTemplateMatch* existingmatch;
        if (mMatchMap.Get(id, &existingmatch)) {
            while (existingmatch) {
                nsTemplateMatch* nextmatch = existingmatch->mNext;
                nsTemplateMatch::Destroy(mPool, existingmatch, PR_TRUE);
                existingmatch = nextmatch;
            }

            mMatchMap.Remove(id);
        }

        if ((row.mContainerState == nsTreeRows::eContainerState_Open) && row.mSubtree)
            RemoveMatchesFor(*(row.mSubtree));
    }

    return NS_OK;
}

nsresult
nsXULTreeBuilder::IsContainerOpen(nsIXULTemplateResult *aResult, PRBool* aOpen)
{
    
    if ((mFlags & eDontRecurse) && aResult != mRootResult) {
        *aOpen = PR_FALSE;
        return NS_OK;
    }

    nsCOMPtr<nsIRDFResource> id;
    nsresult rv = GetResultResource(aResult, getter_AddRefs(id));
    if (NS_FAILED(rv))
        return rv;

    return IsContainerOpen(id, aOpen);
}

nsresult
nsXULTreeBuilder::IsContainerOpen(nsIRDFResource* aResource, PRBool* aOpen)
{
    if (mPersistStateStore)
        mPersistStateStore->HasAssertion(aResource,
                                         nsXULContentUtils::NC_open,
                                         nsXULContentUtils::true_,
                                         PR_TRUE,
                                         aOpen);
    else
        *aOpen = PR_FALSE;

    return NS_OK;
}

int
nsXULTreeBuilder::Compare(const void* aLeft, const void* aRight, void* aClosure)
{
    nsXULTreeBuilder* self = NS_STATIC_CAST(nsXULTreeBuilder*, aClosure);

    nsTreeRows::Row* left = NS_STATIC_CAST(nsTreeRows::Row*,
                                               NS_CONST_CAST(void*, aLeft));

    nsTreeRows::Row* right = NS_STATIC_CAST(nsTreeRows::Row*,
                                                NS_CONST_CAST(void*, aRight));

    return self->CompareResults(left->mMatch->mResult, right->mMatch->mResult);
}

PRInt32
nsXULTreeBuilder::CompareResults(nsIXULTemplateResult* aLeft, nsIXULTemplateResult* aRight)
{
    if (mSortDirection == eDirection_Natural) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<nsISupports> ref;
        nsresult rv = aLeft->GetBindingObjectFor(mRefVariable, getter_AddRefs(ref));
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsIRDFResource> container = do_QueryInterface(ref);
        if (container) {
            PRBool isSequence = PR_FALSE;
            gRDFContainerUtils->IsSeq(mDB, container, &isSequence);
            if (isSequence) {
                
                
                PRInt32 lindex = 0, rindex = 0;

                nsCOMPtr<nsIRDFResource> leftitem;
                aLeft->GetResource(getter_AddRefs(leftitem));
                if (leftitem) {
                    gRDFContainerUtils->IndexOf(mDB, container, leftitem, &lindex);
                    if (lindex < 0)
                        return 0;
                }

                nsCOMPtr<nsIRDFResource> rightitem;
                aRight->GetResource(getter_AddRefs(rightitem));
                if (rightitem) {
                    gRDFContainerUtils->IndexOf(mDB, container, rightitem, &rindex);
                    if (rindex < 0)
                        return 0;
                }

                return lindex - rindex;
            }
        }
    }

    PRInt32 sortorder;
    mQueryProcessor->CompareResults(aLeft, aRight, mSortVariable, &sortorder);

    if (sortorder)
        sortorder = sortorder * mSortDirection;
    return sortorder;
}

nsresult
nsXULTreeBuilder::SortSubtree(nsTreeRows::Subtree* aSubtree)
{
    NS_QuickSort(mRows.GetRowsFor(aSubtree),
                 aSubtree->Count(),
                 sizeof(nsTreeRows::Row),
                 Compare,
                 this);

    for (PRInt32 i = aSubtree->Count() - 1; i >= 0; --i) {
        nsTreeRows::Subtree* child = (*aSubtree)[i].mSubtree;
        if (child)
            SortSubtree(child);
    }

    return NS_OK;
}



NS_IMETHODIMP
nsXULTreeBuilder::CanDrop(PRInt32 index, PRInt32 orientation, PRBool *_retval)
{
    *_retval = PR_FALSE;
    if (mObservers) {
        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer) {
                observer->CanDrop(index, orientation, _retval);
                if (*_retval)
                    break;
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::Drop(PRInt32 row, PRInt32 orient)
{
    if (mObservers) {
        PRUint32 count;
        mObservers->Count(&count);
        for (PRUint32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIXULTreeBuilderObserver> observer = do_QueryElementAt(mObservers, i);
            if (observer) {
                PRBool canDrop = PR_FALSE;
                observer->CanDrop(row, orient, &canDrop);
                if (canDrop)
                    observer->OnDrop(row, orient);
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsSorted(PRBool *_retval)
{
  *_retval = (mSortVariable != nsnull);
  return NS_OK;
}

