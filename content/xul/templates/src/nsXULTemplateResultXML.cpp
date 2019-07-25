



































#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIContent.h"

#include "nsIRDFService.h"

#include "nsXULTemplateResultXML.h"
#include "nsXMLBinding.h"

static PRUint32 sTemplateId = 0;

NS_IMPL_ISUPPORTS1(nsXULTemplateResultXML, nsIXULTemplateResult)

nsXULTemplateResultXML::nsXULTemplateResultXML(nsXMLQuery* aQuery,
                                               nsIDOMNode* aNode,
                                               nsXMLBindingSet* aBindings)
    : mQuery(aQuery), mNode(aNode)
{
    nsCOMPtr<nsIContent> content = do_QueryInterface(mNode);

    
    
    nsCOMPtr<nsIAtom> id = content->GetID();
    if (id) {
      nsCOMPtr<nsIURI> uri = content->GetBaseURI();
      nsCAutoString spec;
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
nsXULTemplateResultXML::GetIsContainer(PRBool* aIsContainer)
{
    
    if (mNode)
        mNode->HasChildNodes(aIsContainer);
    else
        *aIsContainer = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetIsEmpty(PRBool* aIsEmpty)
{
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(mNode);
    if (content) {
        for (nsIContent* child = content->GetFirstChild();
             child;
             child = child->GetNextSibling()) {
            if (child->IsElement()) {
                *aIsEmpty = PR_FALSE;
                return NS_OK;
            }
        }
    }

    *aIsEmpty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::GetMayProcessChildren(PRBool* aMayProcessChildren)
{
    *aMayProcessChildren = PR_TRUE;
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
    *aResource = nsnull;
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

    PRInt32 idx = mRequiredValues.LookupTargetIndex(aVar, &binding);
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
    nsCOMPtr<nsIDOMNode> node;

    if (mQuery && aVar == mQuery->GetMemberVariable()) {
        node = mNode;
    }
    else {
        PRInt32 idx = mRequiredValues.LookupTargetIndex(aVar, &binding);
        if (idx > 0) {
            mRequiredValues.GetNodeAssignmentFor(this, binding, idx,
                                                 getter_AddRefs(node));
        }
        else {
            idx = mOptionalValues.LookupTargetIndex(aVar, &binding);
            if (idx > 0) {
                mOptionalValues.GetNodeAssignmentFor(this, binding, idx,
                                                     getter_AddRefs(node));
            }
        }
    }

    *aValue = node;
    NS_IF_ADDREF(*aValue);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultXML::RuleMatched(nsISupports* aQueryNode,
                                    nsIDOMNode* aRuleNode)
{
    
    nsXULTemplateQueryProcessorXML* processor = mQuery ? mQuery->Processor() :
                                                         nsnull;
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

void
nsXULTemplateResultXML::GetNode(nsIDOMNode** aNode)
{
    *aNode = mNode;
    NS_IF_ADDREF(*aNode);
}
