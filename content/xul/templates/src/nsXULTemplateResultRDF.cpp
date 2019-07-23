



































#include "nsXULTemplateResultRDF.h"
#include "nsXULContentUtils.h"




NS_IMPL_CYCLE_COLLECTION_1(nsXULTemplateResultRDF, mQuery)

NS_INTERFACE_MAP_BEGIN(nsXULTemplateResultRDF)
  NS_INTERFACE_MAP_ENTRY(nsIXULTemplateResult)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsXULTemplateResultRDF)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULTemplateResultRDF)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULTemplateResultRDF)

nsXULTemplateResultRDF::nsXULTemplateResultRDF(nsIRDFResource* aNode)
    : mQuery(nsnull),
      mNode(aNode)
{
}

nsXULTemplateResultRDF::nsXULTemplateResultRDF(nsRDFQuery* aQuery,
                                               const Instantiation& aInst,
                                               nsIRDFResource *aNode)
    : mQuery(aQuery),
      mNode(aNode),
      mInst(aInst)
{
}

nsXULTemplateResultRDF::~nsXULTemplateResultRDF()
{
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetIsContainer(PRBool* aIsContainer)
{
    *aIsContainer = PR_FALSE;

    if (mNode) {
        nsXULTemplateQueryProcessorRDF* processor = GetProcessor();
        if (processor)
            return processor->CheckContainer(mNode, aIsContainer);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetIsEmpty(PRBool* aIsEmpty)
{
    *aIsEmpty = PR_TRUE;

    if (mNode) {
        nsXULTemplateQueryProcessorRDF* processor = GetProcessor();
        if (processor)
            return processor->CheckEmpty(mNode, aIsEmpty);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetMayProcessChildren(PRBool* aMayProcessChildren)
{
    
    *aMayProcessChildren = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetId(nsAString& aId)
{
    if (! mNode)
        return NS_ERROR_FAILURE;

    const char* uri;
    mNode->GetValueConst(&uri);

    CopyUTF8toUTF16(uri, aId);

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetResource(nsIRDFResource** aResource)
{
    *aResource = mNode;
    NS_IF_ADDREF(*aResource);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetType(nsAString& aType)
{
    aType.Truncate();

    nsresult rv = NS_OK;

    nsXULTemplateQueryProcessorRDF* processor = GetProcessor();
    if (processor) {
        PRBool found;
        rv = processor->CheckIsSeparator(mNode, &found);
        if (NS_SUCCEEDED(rv) && found)
            aType.AssignLiteral("separator");
    }

    return rv;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetBindingFor(nsIAtom* aVar, nsAString& aValue)
{
    nsCOMPtr<nsIRDFNode> val;
    GetAssignment(aVar, getter_AddRefs(val));

    return nsXULContentUtils::GetTextForNode(val, aValue);
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetBindingObjectFor(nsIAtom* aVar, nsISupports** aValue)
{
    GetAssignment(aVar, (nsIRDFNode **)aValue);

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::RuleMatched(nsISupports* aQuery, nsIDOMNode* aRuleNode)
{
    
    nsXULTemplateQueryProcessorRDF* processor = GetProcessor();
    if (processor) {
        RDFBindingSet* bindings = processor->GetBindingsForRule(aRuleNode);
        if (bindings) {
            nsresult rv = mBindingValues.SetBindingSet(bindings);
            if (NS_FAILED(rv))
                return rv;

            bindings->AddDependencies(mNode, this);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::HasBeenRemoved()
{
    
    
    mBindingValues.RemoveDependencies(mNode, this);

    nsXULTemplateQueryProcessorRDF* processor = GetProcessor();
    if (processor)
        processor->RemoveMemoryElements(mInst, this);

    return NS_OK;
}


void
nsXULTemplateResultRDF::GetAssignment(nsIAtom* aVar, nsIRDFNode** aValue)
{
    
    *aValue = nsnull;
    mInst.mAssignments.GetAssignmentFor(aVar, aValue);

    
    if (! *aValue)
        mBindingValues.GetAssignmentFor(this, aVar, aValue);
}


PRBool
nsXULTemplateResultRDF::SyncAssignments(nsIRDFResource* aSubject,
                                        nsIRDFResource* aPredicate,
                                        nsIRDFNode* aTarget)
{
    
    RDFBindingSet* bindingset = mBindingValues.GetBindingSet();
    if (bindingset) {
        return bindingset->SyncAssignments(aSubject, aPredicate, aTarget,
            (aSubject == mNode) ? mQuery->GetMemberVariable() : nsnull,
            this, mBindingValues);
    }

    return PR_FALSE;
}

PRBool
nsXULTemplateResultRDF::HasMemoryElement(const MemoryElement& aMemoryElement)
{
    MemoryElementSet::ConstIterator last = mInst.mSupport.Last();
    for (MemoryElementSet::ConstIterator element = mInst.mSupport.First();
                                         element != last; ++element) {
        if ((*element).Equals(aMemoryElement))
            return PR_TRUE;
    }

    return PR_FALSE;
}
