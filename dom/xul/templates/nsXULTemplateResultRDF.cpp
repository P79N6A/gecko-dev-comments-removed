




#include "nsXULTemplateResultRDF.h"
#include "nsXULContentUtils.h"




NS_IMPL_CYCLE_COLLECTION(nsXULTemplateResultRDF, mQuery)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXULTemplateResultRDF)
  NS_INTERFACE_MAP_ENTRY(nsIXULTemplateResult)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULTemplateResultRDF)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULTemplateResultRDF)

nsXULTemplateResultRDF::nsXULTemplateResultRDF(nsIRDFResource* aNode)
    : mQuery(nullptr),
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
nsXULTemplateResultRDF::GetIsContainer(bool* aIsContainer)
{
    *aIsContainer = false;

    if (mNode) {
        nsXULTemplateQueryProcessorRDF* processor = GetProcessor();
        if (processor)
            return processor->CheckContainer(mNode, aIsContainer);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetIsEmpty(bool* aIsEmpty)
{
    *aIsEmpty = true;

    if (mNode) {
        nsXULTemplateQueryProcessorRDF* processor = GetProcessor();
        if (processor)
            return processor->CheckEmpty(mNode, aIsEmpty);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultRDF::GetMayProcessChildren(bool* aMayProcessChildren)
{
    
    *aMayProcessChildren = true;
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
        bool found;
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
    
    *aValue = nullptr;
    mInst.mAssignments.GetAssignmentFor(aVar, aValue);

    
    if (! *aValue)
        mBindingValues.GetAssignmentFor(this, aVar, aValue);
}


bool
nsXULTemplateResultRDF::SyncAssignments(nsIRDFResource* aSubject,
                                        nsIRDFResource* aPredicate,
                                        nsIRDFNode* aTarget)
{
    
    RDFBindingSet* bindingset = mBindingValues.GetBindingSet();
    if (bindingset) {
        return bindingset->SyncAssignments(aSubject, aPredicate, aTarget,
            (aSubject == mNode) ? mQuery->GetMemberVariable() : nullptr,
            this, mBindingValues);
    }

    return false;
}

bool
nsXULTemplateResultRDF::HasMemoryElement(const MemoryElement& aMemoryElement)
{
    MemoryElementSet::ConstIterator last = mInst.mSupport.Last();
    for (MemoryElementSet::ConstIterator element = mInst.mSupport.First();
                                         element != last; ++element) {
        if ((*element).Equals(aMemoryElement))
            return true;
    }

    return false;
}
