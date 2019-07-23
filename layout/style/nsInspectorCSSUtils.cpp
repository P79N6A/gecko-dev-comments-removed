








































#include "nsInspectorCSSUtils.h"
#include "nsIStyleRule.h"
#include "nsRuleNode.h"
#include "nsString.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsAutoPtr.h"
#include "nsIFrame.h"
#include "nsStyleSet.h"
#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIDOMElement.h"
#include "nsIMutableArray.h"
#include "nsBindingManager.h"

nsInspectorCSSUtils::nsInspectorCSSUtils()
{
    nsCSSProps::AddRefTable();
}

nsInspectorCSSUtils::~nsInspectorCSSUtils()
{
    nsCSSProps::ReleaseTable();
}

NS_IMPL_ISUPPORTS1(nsInspectorCSSUtils, nsIInspectorCSSUtils)

NS_IMETHODIMP
nsInspectorCSSUtils::LookupCSSProperty(const nsAString& aName, nsCSSProperty *aProp)
{
    *aProp = nsCSSProps::LookupProperty(aName);
    return NS_OK;
}

NS_IMETHODIMP
nsInspectorCSSUtils::GetRuleNodeParent(nsRuleNode *aNode, nsRuleNode **aParent)
{
    *aParent = aNode->GetParent();
    return NS_OK;
}

NS_IMETHODIMP
nsInspectorCSSUtils::GetRuleNodeRule(nsRuleNode *aNode, nsIStyleRule **aRule)
{
    *aRule = aNode->GetRule();
    NS_IF_ADDREF(*aRule);
    return NS_OK;
}

NS_IMETHODIMP
nsInspectorCSSUtils::IsRuleNodeRoot(nsRuleNode *aNode, PRBool *aIsRoot)
{
    *aIsRoot = aNode->IsRoot();
    return NS_OK;
}


nsStyleContext*
nsInspectorCSSUtils::GetStyleContextForFrame(nsIFrame* aFrame)
{
    nsStyleContext* styleContext = aFrame->GetStyleContext();

    




    if (aFrame->GetType() == nsGkAtoms::tableOuterFrame)
        return styleContext->GetParent();

    return styleContext;
}    


already_AddRefed<nsStyleContext>
nsInspectorCSSUtils::GetStyleContextForContent(nsIContent* aContent,
                                               nsIAtom* aPseudo,
                                               nsIPresShell* aPresShell)
{
    if (!aPseudo) {
        aPresShell->FlushPendingNotifications(Flush_Style);
        nsIFrame* frame = aPresShell->GetPrimaryFrameFor(aContent);
        if (frame) {
            nsStyleContext* result = GetStyleContextForFrame(frame);
            
            
            
            if (!result->HasPseudoElementData()) {
                
                result->AddRef();
                return result;
            }
        }
    }

    
    
    nsRefPtr<nsStyleContext> parentContext;
    nsIContent* parent = aPseudo ? aContent : aContent->GetParent();
    if (parent)
        parentContext = GetStyleContextForContent(parent, nsnull, aPresShell);

    nsPresContext *presContext = aPresShell->GetPresContext();
    if (!presContext)
        return nsnull;

    nsStyleSet *styleSet = aPresShell->StyleSet();

    if (!aContent->IsNodeOfType(nsINode::eELEMENT)) {
        NS_ASSERTION(!aPseudo, "Shouldn't have a pseudo for a non-element!");
        return styleSet->ResolveStyleForNonElement(parentContext);
    }

    if (aPseudo) {
        return styleSet->ResolvePseudoStyleFor(aContent, aPseudo, parentContext);
    }
    
    return styleSet->ResolveStyleFor(aContent, parentContext);
}

NS_IMETHODIMP
nsInspectorCSSUtils::GetRuleNodeForContent(nsIContent* aContent,
                                           nsRuleNode** aRuleNode)
{
    *aRuleNode = nsnull;

    nsIDocument* doc = aContent->GetDocument();
    NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

    nsIPresShell *presShell = doc->GetPrimaryShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_UNEXPECTED);

    nsRefPtr<nsStyleContext> sContext =
        GetStyleContextForContent(aContent, nsnull, presShell);
    *aRuleNode = sContext->GetRuleNode();
    return NS_OK;
}

NS_IMETHODIMP
nsInspectorCSSUtils::GetBindingURLs(nsIDOMElement *aElement,
                                    nsIArray **aResult)
{
    *aResult = nsnull;

    nsCOMPtr<nsIMutableArray> urls = do_CreateInstance(NS_ARRAY_CONTRACTID);
    if (!urls)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
    NS_ASSERTION(content, "elements must implement nsIContent");

    nsIDocument *ownerDoc = content->GetOwnerDoc();
    if (ownerDoc) {
        nsXBLBinding *binding =
            ownerDoc->BindingManager()->GetBinding(content);

        while (binding) {
            urls->AppendElement(binding->PrototypeBinding()->BindingURI(),
                                PR_FALSE);
            binding = binding->GetBaseBinding();
        }
    }

    NS_ADDREF(*aResult = urls);
    return NS_OK;
}
