




#include "nscore.h"
#include "nsError.h"
#include "nsIContent.h"
#include "mozilla/dom/NodeInfo.h"
#include "nsIDOMElement.h"
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
#include "nsIXULSortService.h"
#include "nsTArray.h"
#include "nsUnicharUtils.h"
#include "nsNameSpaceManager.h"
#include "nsDOMClassInfoID.h"
#include "nsWhitespaceTokenizer.h"
#include "nsTreeContentView.h"
#include "nsIXULStore.h"
#include "mozilla/BinarySearch.h"


#include "nsIDocument.h"





class nsXULTreeBuilder : public nsXULTemplateBuilder,
                         public nsIXULTreeBuilder,
                         public nsINativeTreeView
{
public:
    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeBuilder, nsXULTemplateBuilder)

    
    NS_DECL_NSIXULTREEBUILDER

    
    NS_DECL_NSITREEVIEW
    
    NS_IMETHOD EnsureNative() { return NS_OK; }

    
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

protected:
    friend nsresult
    NS_NewXULTreeBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    friend struct ResultComparator;

    nsXULTreeBuilder();
    ~nsXULTreeBuilder();

    


    virtual void Uninit(bool aIsFinal);

    


    nsresult
    EnsureSortVariables();

    virtual nsresult
    RebuildAll();

    



    nsresult
    GetTemplateActionRowFor(int32_t aRow, nsIContent** aResult);

    



    nsresult
    GetTemplateActionCellFor(int32_t aRow, nsITreeColumn* aCol, nsIContent** aResult);

    


    nsresult
    GetResourceFor(int32_t aRow, nsIRDFResource** aResource);

    



    nsresult
    OpenContainer(int32_t aIndex, nsIXULTemplateResult* aResult);

    



    nsresult
    OpenSubtreeOf(nsTreeRows::Subtree* aSubtree,
                  int32_t aIndex,
                  nsIXULTemplateResult *aResult,
                  int32_t* aDelta);

    nsresult
    OpenSubtreeForQuerySet(nsTreeRows::Subtree* aSubtree,
                           int32_t aIndex,
                           nsIXULTemplateResult *aResult,
                           nsTemplateQuerySet* aQuerySet,
                           int32_t* aDelta,
                           nsTArray<int32_t>& open);

    



    nsresult
    CloseContainer(int32_t aIndex);

    


    nsresult
    RemoveMatchesFor(nsTreeRows::Subtree& subtree);

    


    bool
    IsContainerOpen(nsIXULTemplateResult* aResource);

    


    static int
    Compare(const void* aLeft, const void* aRight, void* aClosure);

    


    int32_t
    CompareResults(nsIXULTemplateResult* aLeft, nsIXULTemplateResult* aRight);

    



    nsresult
    SortSubtree(nsTreeRows::Subtree* aSubtree);

    NS_IMETHOD
    HasGeneratedContent(nsIRDFResource* aResource,
                        nsIAtom* aTag,
                        bool* aGenerated);

    
    

    



    bool
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

    


    uint32_t mSortHints;

    


    nsCOMArray<nsIXULTreeBuilderObserver> mObservers;

    


    nsCOMPtr<nsIXULStore> mLocalStore;
};



nsresult
NS_NewXULTreeBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    *aResult = nullptr;

    NS_PRECONDITION(aOuter == nullptr, "no aggregation");
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

NS_IMPL_CYCLE_COLLECTION_INHERITED(nsXULTreeBuilder, nsXULTemplateBuilder,
                                   mBoxObject,
                                   mSelection,
                                   mPersistStateStore,
                                   mLocalStore,
                                   mObservers)

DOMCI_DATA(XULTreeBuilder, nsXULTreeBuilder)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXULTreeBuilder)
    NS_INTERFACE_MAP_ENTRY(nsIXULTreeBuilder)
    NS_INTERFACE_MAP_ENTRY(nsITreeView)
    NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XULTreeBuilder)
NS_INTERFACE_MAP_END_INHERITING(nsXULTemplateBuilder)


nsXULTreeBuilder::nsXULTreeBuilder()
    : mSortDirection(eDirection_Natural), mSortHints(0)
{
}

nsXULTreeBuilder::~nsXULTreeBuilder()
{
}

void
nsXULTreeBuilder::Uninit(bool aIsFinal)
{
    int32_t count = mRows.Count();
    mRows.Clear();

    if (mBoxObject) {
        mBoxObject->BeginUpdateBatch();
        mBoxObject->RowCountChanged(0, -count);
        if (mBoxObject) {
            mBoxObject->EndUpdateBatch();
        }
    }

    nsXULTemplateBuilder::Uninit(aIsFinal);
}







NS_IMETHODIMP
nsXULTreeBuilder::GetResourceAtIndex(int32_t aRowIndex, nsIRDFResource** aResult)
{
    if (aRowIndex < 0 || aRowIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    return GetResourceFor(aRowIndex, aResult);
}

NS_IMETHODIMP
nsXULTreeBuilder::GetIndexOfResource(nsIRDFResource* aResource, int32_t* aResult)
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
    return mObservers.AppendObject(aObserver) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULTreeBuilder::RemoveObserver(nsIXULTreeBuilderObserver* aObserver)
{
    return mObservers.RemoveObject(aObserver) ? NS_OK : NS_ERROR_FAILURE;
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

    nsAutoString hints;
    header->GetAttr(kNameSpaceID_None, nsGkAtoms::sorthints, hints);

    bool hasNaturalState = true;
    nsWhitespaceTokenizer tokenizer(hints);
    while (tokenizer.hasMoreTokens()) {
      const nsDependentSubstring& token(tokenizer.nextToken());
      if (token.EqualsLiteral("comparecase"))
        mSortHints |= nsIXULSortService::SORT_COMPARECASE;
      else if (token.EqualsLiteral("integer"))
        mSortHints |= nsIXULSortService::SORT_INTEGER;
      else if (token.EqualsLiteral("twostate"))
        hasNaturalState = false;
    }

    
    nsAutoString dir;
    header->GetAttr(kNameSpaceID_None, nsGkAtoms::sortDirection, dir);

    if (dir.EqualsLiteral("ascending")) {
        dir.AssignLiteral("descending");
        mSortDirection = eDirection_Descending;
    }
    else if (hasNaturalState && dir.EqualsLiteral("descending")) {
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
nsXULTreeBuilder::GetRowCount(int32_t* aRowCount)
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
    NS_ENSURE_TRUE(!aSelection ||
                   nsTreeContentView::CanTrustTreeSelection(aSelection),
                   NS_ERROR_DOM_SECURITY_ERR);
    mSelection = aSelection;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetRowProperties(int32_t aIndex, nsAString& aProps)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIContent> row;
    GetTemplateActionRowFor(aIndex, getter_AddRefs(row));
    if (row) {
        nsAutoString raw;
        row->GetAttr(kNameSpaceID_None, nsGkAtoms::properties, raw);

        if (!raw.IsEmpty()) {
            SubstituteText(mRows[aIndex]->mMatch->mResult, raw, aProps);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetCellProperties(int32_t aRow, nsITreeColumn* aCol,
                                    nsAString& aProps)
{
    NS_ENSURE_ARG_POINTER(aCol);
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad row");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::properties, raw);

        if (!raw.IsEmpty()) {
            SubstituteText(mRows[aRow]->mMatch->mResult, raw, aProps);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetColumnProperties(nsITreeColumn* aCol, nsAString& aProps)
{
    NS_ENSURE_ARG_POINTER(aCol);
    
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsContainer(int32_t aIndex, bool* aResult)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::iterator iter = mRows[aIndex];

    bool isContainer;
    iter->mMatch->mResult->GetIsContainer(&isContainer);

    iter->mContainerType = isContainer
        ? nsTreeRows::eContainerType_Container
        : nsTreeRows::eContainerType_Noncontainer;

    *aResult = (iter->mContainerType == nsTreeRows::eContainerType_Container);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsContainerOpen(int32_t aIndex, bool* aOpen)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::iterator iter = mRows[aIndex];

    if (iter->mContainerState == nsTreeRows::eContainerState_Unknown) {
        bool isOpen = IsContainerOpen(iter->mMatch->mResult);

        iter->mContainerState = isOpen
            ? nsTreeRows::eContainerState_Open
            : nsTreeRows::eContainerState_Closed;
    }

    *aOpen = (iter->mContainerState == nsTreeRows::eContainerState_Open);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsContainerEmpty(int32_t aIndex, bool* aResult)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::iterator iter = mRows[aIndex];
    NS_ASSERTION(iter->mContainerType == nsTreeRows::eContainerType_Container,
                 "asking for empty state on non-container");

    
    
    
    if ((mFlags & eDontRecurse) && (iter->mMatch->mResult != mRootResult)) {
        *aResult = true;
        return NS_OK;
    }

    if (iter->mContainerFill == nsTreeRows::eContainerFill_Unknown) {
        bool isEmpty;
        iter->mMatch->mResult->GetIsEmpty(&isEmpty);

        iter->mContainerFill = isEmpty
            ? nsTreeRows::eContainerFill_Empty
            : nsTreeRows::eContainerFill_Nonempty;
    }

    *aResult = (iter->mContainerFill == nsTreeRows::eContainerFill_Empty);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsSeparator(int32_t aIndex, bool* aResult)
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
nsXULTreeBuilder::GetParentIndex(int32_t aRowIndex, int32_t* aResult)
{
    NS_PRECONDITION(aRowIndex >= 0 && aRowIndex < mRows.Count(), "bad row");
    if (aRowIndex < 0 || aRowIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    nsTreeRows::iterator iter = mRows[aRowIndex];

    
    nsTreeRows::Subtree* parent = iter.GetParent();

    
    
    int32_t index = iter.GetChildIndex();
    while (--index >= 0)
        aRowIndex -= mRows.GetSubtreeSizeFor(parent, index) + 1;

    
    *aResult = aRowIndex - 1;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::HasNextSibling(int32_t aRowIndex, int32_t aAfterIndex, bool* aResult)
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
nsXULTreeBuilder::GetLevel(int32_t aRowIndex, int32_t* aResult)
{
    NS_PRECONDITION(aRowIndex >= 0 && aRowIndex < mRows.Count(), "bad row");
    if (aRowIndex < 0 || aRowIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    
    
    nsTreeRows::iterator iter = mRows[aRowIndex];
    *aResult = iter.GetDepth() - 1;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::GetImageSrc(int32_t aRow, nsITreeColumn* aCol, nsAString& aResult)
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
nsXULTreeBuilder::GetProgressMode(int32_t aRow, nsITreeColumn* aCol, int32_t* aResult)
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
nsXULTreeBuilder::GetCellValue(int32_t aRow, nsITreeColumn* aCol, nsAString& aResult)
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
nsXULTreeBuilder::GetCellText(int32_t aRow, nsITreeColumn* aCol, nsAString& aResult)
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
    mBoxObject = aTree;

    
    if (!mBoxObject) {
        Uninit(false);
        return NS_OK;
    }
    NS_ENSURE_TRUE(mRoot, NS_ERROR_NOT_INITIALIZED);

    
    bool isTrusted = false;
    nsresult rv = IsSystemPrincipal(mRoot->NodePrincipal(), &isTrusted);
    if (NS_SUCCEEDED(rv) && isTrusted) {
        mLocalStore = do_GetService("@mozilla.org/xul/xulstore;1");
        if(NS_WARN_IF(!mLocalStore)){
            return NS_ERROR_NOT_INITIALIZED;
        }
    }

    Rebuild();

    EnsureSortVariables();
    if (mSortVariable)
        SortSubtree(mRows.GetRoot());

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::ToggleOpenState(int32_t aIndex)
{
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsIXULTemplateResult* result = mRows[aIndex]->mMatch->mResult;
    if (! result)
        return NS_ERROR_FAILURE;

    if (mFlags & eDontRecurse)
        return NS_OK;

    if (result && result != mRootResult) {
        
        bool mayProcessChildren;
        nsresult rv = result->GetMayProcessChildren(&mayProcessChildren);
        if (NS_FAILED(rv) || !mayProcessChildren)
            return rv;
    }

    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer)
            observer->OnToggleOpenState(aIndex);
    }

    if (mLocalStore && mRoot) {
        bool isOpen;
        IsContainerOpen(aIndex, &isOpen);

        nsIDocument* doc = mRoot->GetComposedDoc();
        if (!doc) {
            return NS_ERROR_FAILURE;
        }

        nsIURI* docURI = doc->GetDocumentURI();
        nsTreeRows::Row& row = *(mRows[aIndex]);
        nsAutoString nodeid;
        nsresult rv = row.mMatch->mResult->GetId(nodeid);
        if (NS_FAILED(rv)) {
            return rv;
        }

        nsAutoCString utf8uri;
        rv = docURI->GetSpec(utf8uri);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
        NS_ConvertUTF8toUTF16 uri(utf8uri);

        if (isOpen) {
            mLocalStore->RemoveValue(uri, nodeid, NS_LITERAL_STRING("open"));
            CloseContainer(aIndex);
        } else {
            mLocalStore->SetValue(uri, nodeid, NS_LITERAL_STRING("open"),
                NS_LITERAL_STRING("true"));

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

    nsAutoString id;
    aCol->GetId(id);

    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer)
            observer->OnCycleHeader(id.get(), element);
    }

    return Sort(element);
}

NS_IMETHODIMP
nsXULTreeBuilder::SelectionChanged()
{
    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer)
            observer->OnSelectionChanged();
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::CycleCell(int32_t aRow, nsITreeColumn* aCol)
{
    NS_ENSURE_ARG_POINTER(aCol);

    nsAutoString id;
    aCol->GetId(id);

    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer)
            observer->OnCycleCell(aRow, id.get());
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsEditable(int32_t aRow, nsITreeColumn* aCol, bool* _retval)
{
    *_retval = true;
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
            *_retval = false;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsSelectable(int32_t aRow, nsITreeColumn* aCol, bool* _retval)
{
    NS_PRECONDITION(aRow >= 0 && aRow < mRows.Count(), "bad index");
    if (aRow < 0 || aRow >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    *_retval = true;

    
    nsCOMPtr<nsIContent> cell;
    GetTemplateActionCellFor(aRow, aCol, getter_AddRefs(cell));
    if (cell) {
        nsAutoString raw;
        cell->GetAttr(kNameSpaceID_None, nsGkAtoms::selectable, raw);

        nsAutoString selectable;
        SubstituteText(mRows[aRow]->mMatch->mResult, raw, selectable);

        if (selectable.EqualsLiteral("false"))
            *_retval = false;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::SetCellValue(int32_t aRow, nsITreeColumn* aCol, const nsAString& aValue)
{
    NS_ENSURE_ARG_POINTER(aCol);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::SetCellText(int32_t aRow, nsITreeColumn* aCol, const nsAString& aValue)
{
    NS_ENSURE_ARG_POINTER(aCol);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::PerformAction(const char16_t* aAction)
{
    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer)
            observer->OnPerformAction(aAction);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::PerformActionOnRow(const char16_t* aAction, int32_t aRow)
{
    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer)
            observer->OnPerformActionOnRow(aAction, aRow);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::PerformActionOnCell(const char16_t* aAction, int32_t aRow, nsITreeColumn* aCol)
{
    NS_ENSURE_ARG_POINTER(aCol);
    nsAutoString id;
    aCol->GetId(id);

    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer)
            observer->OnPerformActionOnCell(aAction, aRow, id.get());
    }

    return NS_OK;
}


void
nsXULTreeBuilder::NodeWillBeDestroyed(const nsINode* aNode)
{
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);
    mObservers.Clear();

    nsXULTemplateBuilder::NodeWillBeDestroyed(aNode);
}

NS_IMETHODIMP
nsXULTreeBuilder::HasGeneratedContent(nsIRDFResource* aResource,
                                      nsIAtom* aTag,
                                      bool* aGenerated)
{
    *aGenerated = false;
    NS_ENSURE_ARG_POINTER(aResource);

    if (!mRootResult)
        return NS_OK;

    nsCOMPtr<nsIRDFResource> rootresource;
    nsresult rv = mRootResult->GetResource(getter_AddRefs(rootresource));
    if (NS_FAILED(rv))
        return rv;

    if (aResource == rootresource ||
        mRows.FindByResource(aResource) != mRows.Last())
        *aGenerated = true;

    return NS_OK;
}

bool
nsXULTreeBuilder::GetInsertionLocations(nsIXULTemplateResult* aResult,
                                        nsCOMArray<nsIContent>** aLocations)
{
    *aLocations = nullptr;

    
    

    nsAutoString ref;
    nsresult rv = aResult->GetBindingFor(mRefVariable, ref);
    if (NS_FAILED(rv) || ref.IsEmpty())
        return false;

    nsCOMPtr<nsIRDFResource> container;
    rv = gRDFService->GetUnicodeResource(ref, getter_AddRefs(container));
    if (NS_FAILED(rv))
        return false;

    
    if (container == mRows.GetRootResource())
        return true;

    nsTreeRows::iterator iter = mRows.FindByResource(container);
    if (iter == mRows.Last())
        return false;

    return (iter->mContainerState == nsTreeRows::eContainerState_Open);
}

struct ResultComparator
{
    nsXULTreeBuilder* const mTreebuilder;
    nsIXULTemplateResult* const mResult;
    ResultComparator(nsXULTreeBuilder* aTreebuilder, nsIXULTemplateResult* aResult)
      : mTreebuilder(aTreebuilder), mResult(aResult) {}
    int operator()(const nsTreeRows::Row& aSubtree) const {
        return mTreebuilder->CompareResults(mResult, aSubtree.mMatch->mResult);
    }
};

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

        
        int32_t row = iter.GetRowIndex();

        
        
        
        int32_t delta = mRows.GetSubtreeSizeFor(iter);
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
        
        int32_t row = -1;
        nsTreeRows::Subtree* parent = nullptr;
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

            
            
            bool open = false;
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
            
            
            
            size_t index = parent->Count();

            if (mSortVariable) {
                
                
                mozilla::BinarySearchIf(*parent, 0, parent->Count(),
                                        ResultComparator(this, result), &index);
            }

            nsTreeRows::iterator iter =
                mRows.InsertRowAt(aNewMatch, parent, index);

            mBoxObject->RowCountChanged(iter.GetRowIndex(), +1);

            
            

            if (mFlags & eDontRecurse)
                return NS_OK;

            if (result != mRootResult) {
                
                bool mayProcessChildren;
                nsresult rv = result->GetMayProcessChildren(&mayProcessChildren);
                if (NS_FAILED(rv) || ! mayProcessChildren) return NS_OK;
            }

            if (IsContainerOpen(result)) {
                OpenContainer(iter.GetRowIndex(), result);
            }
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

        int32_t row = iter.GetRowIndex();
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
        
    for (nsIContent* child = treecols->GetFirstChild();
         child;
         child = child->GetNextSibling()) {

        if (child->NodeInfo()->Equals(nsGkAtoms::treecol,
                                      kNameSpaceID_XUL)) {
            if (child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::sortActive,
                                   nsGkAtoms::_true, eCaseMatters)) {
                nsAutoString sort;
                child->GetAttr(kNameSpaceID_None, nsGkAtoms::sort, sort);
                if (! sort.IsEmpty()) {
                    mSortVariable = do_GetAtom(sort);

                    static nsIContent::AttrValuesArray strings[] =
                      {&nsGkAtoms::ascending, &nsGkAtoms::descending, nullptr};
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
    NS_ENSURE_TRUE(mRoot, NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsIDocument> doc = mRoot->GetComposedDoc();

    
    if (!doc)
        return NS_OK;

    if (! mQueryProcessor)
        return NS_OK;

    if (mBoxObject) {
        mBoxObject->BeginUpdateBatch();
    }

    if (mQueriesCompiled) {
        Uninit(false);
    }
    else if (mBoxObject) {
        int32_t count = mRows.Count();
        mRows.Clear();
        mBoxObject->RowCountChanged(0, -count);
    }

    nsresult rv = CompileQueries();
    if (NS_SUCCEEDED(rv) && mQuerySets.Length() > 0) {
        
        nsAutoString ref;
        mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::ref, ref);
        if (!ref.IsEmpty()) {
            rv = mQueryProcessor->TranslateRef(mDataSource, ref,
                                               getter_AddRefs(mRootResult));
            if (NS_SUCCEEDED(rv) && mRootResult) {
                OpenContainer(-1, mRootResult);

                nsCOMPtr<nsIRDFResource> rootResource;
                GetResultResource(mRootResult, getter_AddRefs(rootResource));

                mRows.SetRootResource(rootResource);
            }
        }
    }

    if (mBoxObject) {
        mBoxObject->EndUpdateBatch();
    }

    return rv;
}

nsresult
nsXULTreeBuilder::GetTemplateActionRowFor(int32_t aRow, nsIContent** aResult)
{
    
    
    nsTreeRows::Row& row = *(mRows[aRow]);

    
    
    
    int16_t ruleindex = row.mMatch->RuleIndex();
    if (ruleindex >= 0) {
        nsTemplateQuerySet* qs = mQuerySets[row.mMatch->QuerySetPriority()];
        nsTemplateRule* rule = qs->GetRuleAt(ruleindex);
        if (rule) {
            nsCOMPtr<nsIContent> children;
            nsXULContentUtils::FindChildByTag(rule->GetAction(), kNameSpaceID_XUL,
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

    *aResult = nullptr;
    return NS_OK;
}

nsresult
nsXULTreeBuilder::GetTemplateActionCellFor(int32_t aRow,
                                           nsITreeColumn* aCol,
                                           nsIContent** aResult)
{
    *aResult = nullptr;

    if (!aCol) return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIContent> row;
    GetTemplateActionRowFor(aRow, getter_AddRefs(row));
    if (row) {
        nsCOMPtr<nsIAtom> colAtom;
        int32_t colIndex;
        aCol->GetAtom(getter_AddRefs(colAtom));
        aCol->GetIndex(&colIndex);

        uint32_t j = 0;
        for (nsIContent* child = row->GetFirstChild();
             child;
             child = child->GetNextSibling()) {
            
            if (child->NodeInfo()->Equals(nsGkAtoms::treecell,
                                          kNameSpaceID_XUL)) {
                if (colAtom &&
                    child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::ref,
                                       colAtom, eCaseMatters)) {
                    *aResult = child;
                    break;
                }
                else if (j == (uint32_t)colIndex)
                    *aResult = child;
                j++;
            }
        }
    }
    NS_IF_ADDREF(*aResult);

    return NS_OK;
}

nsresult
nsXULTreeBuilder::GetResourceFor(int32_t aRow, nsIRDFResource** aResource)
{
    nsTreeRows::Row& row = *(mRows[aRow]);
    return GetResultResource(row.mMatch->mResult, aResource);
}

nsresult
nsXULTreeBuilder::OpenContainer(int32_t aIndex, nsIXULTemplateResult* aResult)
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

    int32_t count;
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
                                int32_t aIndex,
                                nsIXULTemplateResult *aResult,
                                int32_t* aDelta)
{
    nsAutoTArray<int32_t, 8> open;
    int32_t count = 0;

    int32_t rulecount = mQuerySets.Length();

    for (int32_t r = 0; r < rulecount; r++) {
        nsTemplateQuerySet* queryset = mQuerySets[r];
        OpenSubtreeForQuerySet(aSubtree, aIndex, aResult, queryset, &count, open);
    }

    
    
    for (int32_t i = open.Length() - 1; i >= 0; --i) {
        int32_t index = open[i];

        nsTreeRows::Subtree* child =
            mRows.EnsureSubtreeFor(aSubtree, index);

        nsIXULTemplateResult* result = (*aSubtree)[index].mMatch->mResult;

        int32_t delta;
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
                                         int32_t aIndex,
                                         nsIXULTemplateResult* aResult,
                                         nsTemplateQuerySet* aQuerySet,
                                         int32_t* aDelta,
                                         nsTArray<int32_t>& open)
{
    int32_t count = *aDelta;
    
    nsCOMPtr<nsISimpleEnumerator> results;
    nsresult rv = mQueryProcessor->GenerateResults(mDataSource, aResult,
                                                   aQuerySet->mCompiledQuery,
                                                   getter_AddRefs(results));
    if (NS_FAILED(rv))
        return rv;

    bool hasMoreResults;
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

        
        
        

        bool generateContent = true;

        nsTemplateMatch* prevmatch = nullptr;
        nsTemplateMatch* existingmatch = nullptr;
        if (mMatchMap.Get(resultid, &existingmatch)){
            
            while (existingmatch) {
                if (existingmatch->IsActive())
                    generateContent = false;
                prevmatch = existingmatch;
                existingmatch = existingmatch->mNext;
            }
        }

        nsTemplateMatch *newmatch =
            nsTemplateMatch::Create(aQuerySet->Priority(), nextresult, nullptr);
        if (!newmatch)
            return NS_ERROR_OUT_OF_MEMORY;

        if (generateContent) {
            
            bool cyclic = false;

            if (aIndex >= 0) {
                for (nsTreeRows::iterator iter = mRows[aIndex]; iter.GetDepth() > 0; iter.Pop()) {
                    nsCOMPtr<nsIRDFResource> parentid;
                    rv = GetResultResource(iter->mMatch->mResult, getter_AddRefs(parentid));
                    if (NS_FAILED(rv)) {
                        nsTemplateMatch::Destroy(newmatch, false);
                        return rv;
                    }

                    if (resultid == parentid) {
                        cyclic = true;
                        break;
                    }
                }
            }

            if (cyclic) {
                NS_WARNING("tree cannot handle cyclic graphs");
                nsTemplateMatch::Destroy(newmatch, false);
                continue;
            }

            int16_t ruleindex;
            nsTemplateRule* matchedrule = nullptr;
            rv = DetermineMatchedRule(nullptr, nextresult, aQuerySet,
                                      &matchedrule, &ruleindex);
            if (NS_FAILED(rv)) {
                nsTemplateMatch::Destroy(newmatch, false);
                return rv;
            }

            if (matchedrule) {
                rv = newmatch->RuleMatched(aQuerySet, matchedrule, ruleindex,
                                           nextresult);
                if (NS_FAILED(rv)) {
                    nsTemplateMatch::Destroy(newmatch, false);
                    return rv;
                }

                
                mRows.InsertRowAt(newmatch, aSubtree, count);

                
                
                if (IsContainerOpen(nextresult)) {
                    if (open.AppendElement(count) == nullptr)
                        return NS_ERROR_OUT_OF_MEMORY;
                }

                ++count;
            }

            if (mFlags & eLoggingEnabled)
                OutputMatchToLog(resultid, newmatch, true);

        }

        if (prevmatch) {
            prevmatch->mNext = newmatch;
        }
        else {
            mMatchMap.Put(resultid, newmatch);
        }
    }

    *aDelta = count;
    return rv;
}

nsresult
nsXULTreeBuilder::CloseContainer(int32_t aIndex)
{
    NS_ASSERTION(aIndex >= 0 && aIndex < mRows.Count(), "bad row");
    if (aIndex < 0 || aIndex >= mRows.Count())
        return NS_ERROR_INVALID_ARG;

    nsTreeRows::iterator iter = mRows[aIndex];

    if (iter->mSubtree)
        RemoveMatchesFor(*iter->mSubtree);


    int32_t count = mRows.GetSubtreeSizeFor(iter);
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
    for (int32_t i = subtree.Count() - 1; i >= 0; --i) {
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
                nsTemplateMatch::Destroy(existingmatch, true);
                existingmatch = nextmatch;
            }

            mMatchMap.Remove(id);
        }

        if ((row.mContainerState == nsTreeRows::eContainerState_Open) && row.mSubtree)
            RemoveMatchesFor(*(row.mSubtree));
    }

    return NS_OK;
}


bool
nsXULTreeBuilder::IsContainerOpen(nsIXULTemplateResult *aResult)
{
  
  if ((mFlags & eDontRecurse) && aResult != mRootResult) {
    return false;
  }

  if (!mLocalStore) {
    return false;
  }

  nsIDocument* doc = mRoot->GetComposedDoc();
  if (!doc) {
    return false;
  }

  nsIURI* docURI = doc->GetDocumentURI();

  nsAutoString nodeid;
  nsresult rv = aResult->GetId(nodeid);
  if (NS_FAILED(rv)) {
    return false;
  }

  nsAutoCString utf8uri;
  rv = docURI->GetSpec(utf8uri);
  if (NS_WARN_IF(NS_FAILED(rv))) {
      return false;
  }
  NS_ConvertUTF8toUTF16 uri(utf8uri);

  nsAutoString val;
  mLocalStore->GetValue(uri, nodeid, NS_LITERAL_STRING("open"), val);
  return val.EqualsLiteral("true");
}

int
nsXULTreeBuilder::Compare(const void* aLeft, const void* aRight, void* aClosure)
{
    nsXULTreeBuilder* self = static_cast<nsXULTreeBuilder*>(aClosure);

    nsTreeRows::Row* left = static_cast<nsTreeRows::Row*>
                                       (const_cast<void*>(aLeft));

    nsTreeRows::Row* right = static_cast<nsTreeRows::Row*>
                                        (const_cast<void*>(aRight));

    return self->CompareResults(left->mMatch->mResult, right->mMatch->mResult);
}

int32_t
nsXULTreeBuilder::CompareResults(nsIXULTemplateResult* aLeft, nsIXULTemplateResult* aRight)
{
    
    
    if (mSortDirection == eDirection_Natural && mDB) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<nsISupports> ref;
        nsresult rv = aLeft->GetBindingObjectFor(mRefVariable, getter_AddRefs(ref));
        if (NS_FAILED(rv))
            return 0;

        nsCOMPtr<nsIRDFResource> container = do_QueryInterface(ref);
        if (container) {
            bool isSequence = false;
            gRDFContainerUtils->IsSeq(mDB, container, &isSequence);
            if (isSequence) {
                
                
                int32_t lindex = 0, rindex = 0;

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

    int32_t sortorder;
    if (!mQueryProcessor)
        return 0;

    mQueryProcessor->CompareResults(aLeft, aRight, mSortVariable, mSortHints, &sortorder);

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

    for (int32_t i = aSubtree->Count() - 1; i >= 0; --i) {
        nsTreeRows::Subtree* child = (*aSubtree)[i].mSubtree;
        if (child)
            SortSubtree(child);
    }

    return NS_OK;
}



NS_IMETHODIMP
nsXULTreeBuilder::CanDrop(int32_t index, int32_t orientation,
                          nsIDOMDataTransfer* dataTransfer, bool *_retval)
{
    *_retval = false;
    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer) {
            observer->CanDrop(index, orientation, dataTransfer, _retval);
            if (*_retval)
                break;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::Drop(int32_t row, int32_t orient, nsIDOMDataTransfer* dataTransfer)
{
    uint32_t count = mObservers.Count();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIXULTreeBuilderObserver> observer = mObservers.SafeObjectAt(i);
        if (observer) {
            bool canDrop = false;
            observer->CanDrop(row, orient, dataTransfer, &canDrop);
            if (canDrop)
                observer->OnDrop(row, orient, dataTransfer);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTreeBuilder::IsSorted(bool *_retval)
{
  *_retval = (mSortVariable != nullptr);
  return NS_OK;
}

