




#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIContent.h"

#include "nsIRDFService.h"

#include "nsXULTemplateResultXML.h"
#include "nsXMLBinding.h"

static uint32_t sTemplateId = 0;

NS_IMPL_ISUPPORTS(nsXULTemplateResultXML, nsIXULTemplateResult)

nsXULTemplateResultXML::nsXULTemplateResultXML(nsXMLQuery* aQuery,
                                               nsIContent* aNode,
                                               nsXMLBindingSet* aBindings)
    : mQuery(aQuery), mNode(aNode)
{
    
    
    nsCOMPtr<nsIAtom> id = mNode->GetID();
    if (id) {
      nsCOMPtr<nsIURI> uri = mNode->GetBaseURI();
      nsAutoCString spec;
      uri->GetSpec(spec);

      mId = NS_ConvertUTF8toUTF16(spec);

      nsAutoString idstr;
      id->ToString(idstr);
      mId += NS_LITERAL_STRING("#") + idstr;
    }
    else {
      nsAutoString rowid(NS_LITERAL_STRING("row"));
      rowid.AppendInt(++sTemplateId);
      mId.Assign(rowid);
    }

    if (aBindings)
        mRequiredValues.SetBindingSet(aBindings);
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetIsContainer(bool* aIsContainer)
{
    
    *aIsContainer = mNode && mNode->HasChildNodes();
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetIsEmpty(bool* aIsEmpty)
{
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(mNode);
    if (content) {
        for (nsIContent* child = content->GetFirstChild();
             child;
             child = child->GetNextSibling()) {
            if (child->IsElement()) {
                *aIsEmpty = false;
                return NS_OK;
            }
        }
    }

    *aIsEmpty = true;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetMayProcessChildren(bool* aMayProcessChildren)
{
    *aMayProcessChildren = true;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetId(nsAString& aId)
{
    aId = mId;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetResource(nsIRDFResource** aResource)
{
    *aResource = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetType(nsAString& aType)
{
    aType.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetBindingFor(nsIAtom* aVar, nsAString& aValue)
{
    NS_ENSURE_ARG_POINTER(aVar);

    
    nsXMLBinding* binding;

    int32_t idx = mRequiredValues.LookupTargetIndex(aVar, &binding);
    if (idx >= 0) {
        mRequiredValues.GetStringAssignmentFor(this, binding, idx, aValue);
        return NS_OK;
    }

    idx = mOptionalValues.LookupTargetIndex(aVar, &binding);
    if (idx >= 0) {
        mOptionalValues.GetStringAssignmentFor(this, binding, idx, aValue);
        return NS_OK;
    }

    
    
    nsAutoString attr;
    aVar->ToString(attr);

    if (attr.Length() > 1) {
        nsCOMPtr<nsIDOMElement> element = do_QueryInterface(mNode);
        if (element)
            return element->GetAttribute(Substring(attr, 1), aValue);
    }

    aValue.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetBindingObjectFor(nsIAtom* aVar, nsISupports** aValue)
{
    NS_ENSURE_ARG_POINTER(aVar);

    nsXMLBinding* binding;
    nsCOMPtr<nsISupports> node;

    if (mQuery && aVar == mQuery->GetMemberVariable()) {
        node = mNode;
    }
    else {
        int32_t idx = mRequiredValues.LookupTargetIndex(aVar, &binding);
        if (idx > 0) {
            node = mRequiredValues.GetNodeAssignmentFor(this, binding, idx);
        }
        else {
            idx = mOptionalValues.LookupTargetIndex(aVar, &binding);
            if (idx > 0) {
                node = mOptionalValues.GetNodeAssignmentFor(this, binding, idx);
            }
        }
    }

    node.forget(aValue);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::RuleMatched(nsISupports* aQueryNode,
                                    nsIDOMNode* aRuleNode)
{
    
    nsXULTemplateQueryProcessorXML* processor = mQuery ? mQuery->Processor() :
                                                         nullptr;
    if (processor) {
        nsXMLBindingSet* bindings =
            processor->GetOptionalBindingsForRule(aRuleNode);
        if (bindings)
            mOptionalValues.SetBindingSet(bindings);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::HasBeenRemoved()
{
    return NS_OK;
}
