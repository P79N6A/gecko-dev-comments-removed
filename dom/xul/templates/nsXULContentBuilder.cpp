




#include "mozilla/ArrayUtils.h"

#include "nsContentCID.h"
#include "nsIDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULDocument.h"
#include "mozilla/dom/NodeInfo.h"
#include "nsIServiceManager.h"
#include "nsIXULDocument.h"

#include "nsContentSupportMap.h"
#include "nsRDFConMemberTestNode.h"
#include "nsRDFPropertyTestNode.h"
#include "nsXULSortService.h"
#include "nsTemplateRule.h"
#include "nsTemplateMap.h"
#include "nsTArray.h"
#include "nsXPIDLString.h"
#include "nsGkAtoms.h"
#include "nsXULContentUtils.h"
#include "nsXULElement.h"
#include "nsXULTemplateBuilder.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsAttrName.h"
#include "nsNodeUtils.h"
#include "mozAutoDocUpdate.h"
#include "nsTextNode.h"
#include "mozilla/dom/Element.h"

#include "pldhash.h"
#include "rdf.h"

using namespace mozilla;
using namespace mozilla::dom;





#define NS_ELEMENT_GOT_CREATED NS_RDF_NO_VALUE
#define NS_ELEMENT_WAS_THERE   NS_OK






















class nsXULContentBuilder : public nsXULTemplateBuilder
{
public:
    
    NS_IMETHOD CreateContents(nsIContent* aElement, bool aForceCreation) override;

    NS_IMETHOD HasGeneratedContent(nsIRDFResource* aResource,
                                   nsIAtom* aTag,
                                   bool* aGenerated) override;

    NS_IMETHOD GetResultForContent(nsIDOMElement* aContent,
                                   nsIXULTemplateResult** aResult) override;

    
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

protected:
    friend nsresult
    NS_NewXULContentBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    nsXULContentBuilder();

    void Traverse(nsCycleCollectionTraversalCallback& aCb) const override
    {
        mSortState.Traverse(aCb);
    }

    virtual void Uninit(bool aIsFinal) override;

    
    nsresult
    OpenContainer(nsIContent* aElement);

    nsresult
    CloseContainer(nsIContent* aElement);

    




    nsresult
    BuildContentFromTemplate(nsIContent *aTemplateNode,
                             nsIContent *aResourceNode,
                             nsIContent *aRealNode,
                             bool aIsUnique,
                             bool aIsSelfReference,
                             nsIXULTemplateResult* aChild,
                             bool aNotify,
                             nsTemplateMatch* aMatch,
                             nsIContent** aContainer,
                             int32_t* aNewIndexInContainer);

    








    nsresult
    CopyAttributesToElement(nsIContent* aTemplateNode,
                            nsIContent* aRealNode,
                            nsIXULTemplateResult* aResult,
                            bool aNotify);

    







    nsresult
    AddPersistentAttributes(Element* aTemplateNode,
                            nsIXULTemplateResult* aResult,
                            nsIContent* aRealNode);

    










    nsresult
    SynchronizeUsingTemplate(nsIContent *aTemplateNode,
                             nsIContent* aRealNode,
                             nsIXULTemplateResult* aResult);

    



    nsresult
    RemoveMember(nsIContent* aContent);

    






    nsresult
    CreateTemplateAndContainerContents(nsIContent* aElement,
                                       bool aForceCreation);

    









    nsresult
    CreateContainerContents(nsIContent* aElement,
                            nsIXULTemplateResult* aResult,
                            bool aForceCreation,
                            bool aNotify,
                            bool aNotifyAtEnd);

    








    nsresult
    CreateContainerContentsForQuerySet(nsIContent* aElement,
                                       nsIXULTemplateResult* aResult,
                                       bool aNotify,
                                       nsTemplateQuerySet* aQuerySet,
                                       nsIContent** aContainer,
                                       int32_t* aNewIndexInContainer);

    










    nsresult
    EnsureElementHasGenericChild(nsIContent* aParent,
                                 int32_t aNameSpaceID,
                                 nsIAtom* aTag,
                                 bool aNotify,
                                 nsIContent** aResult);

    bool
    IsOpen(nsIContent* aElement);

    nsresult
    RemoveGeneratedContent(nsIContent* aElement);

    nsresult
    GetElementsForResult(nsIXULTemplateResult* aResult,
                         nsCOMArray<nsIContent>& aElements);

    nsresult
    CreateElement(int32_t aNameSpaceID,
                  nsIAtom* aTag,
                  Element** aResult);

    










    nsresult
    SetContainerAttrs(nsIContent *aElement,
                      nsIXULTemplateResult* aResult,
                      bool aIgnoreNonContainers,
                      bool aNotify);

    virtual nsresult
    RebuildAll() override;

    
    

    




    virtual bool
    GetInsertionLocations(nsIXULTemplateResult* aOldResult,
                          nsCOMArray<nsIContent>** aLocations) override;

    



    virtual nsresult
    ReplaceMatch(nsIXULTemplateResult* aOldResult,
                 nsTemplateMatch* aNewMatch,
                 nsTemplateRule* aNewMatchRule,
                 void *aContext) override;

    




    virtual nsresult
    SynchronizeResult(nsIXULTemplateResult* aResult) override;

    




    nsresult
    CompareResultToNode(nsIXULTemplateResult* aResult, nsIContent* aContent,
                        int32_t* aSortOrder);

    




    nsresult
    InsertSortedNode(nsIContent* aContainer,
                     nsIContent* aNode,
                     nsIXULTemplateResult* aResult,
                     bool aNotify);

    



    nsContentSupportMap mContentSupportMap;

    



    nsTemplateMap mTemplateMap;

    


    nsSortState mSortState;
};

nsresult
NS_NewXULContentBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aOuter == nullptr, "no aggregation");
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsresult rv;
    nsXULContentBuilder* result = new nsXULContentBuilder();
    if (!result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result); 

    rv = result->InitGlobals();

    if (NS_SUCCEEDED(rv))
        rv = result->QueryInterface(aIID, aResult);

    NS_RELEASE(result);
    return rv;
}

nsXULContentBuilder::nsXULContentBuilder()
{
  mSortState.initialized = false;
}

void
nsXULContentBuilder::Uninit(bool aIsFinal)
{
    if (! aIsFinal && mRoot) {
        nsresult rv = RemoveGeneratedContent(mRoot);
        if (NS_FAILED(rv))
            return;
    }

    
    mContentSupportMap.Clear();
    mTemplateMap.Clear();

    mSortState.initialized = false;

    nsXULTemplateBuilder::Uninit(aIsFinal);
}

nsresult
nsXULContentBuilder::BuildContentFromTemplate(nsIContent *aTemplateNode,
                                              nsIContent *aResourceNode,
                                              nsIContent *aRealNode,
                                              bool aIsUnique,
                                              bool aIsSelfReference,
                                              nsIXULTemplateResult* aChild,
                                              bool aNotify,
                                              nsTemplateMatch* aMatch,
                                              nsIContent** aContainer,
                                              int32_t* aNewIndexInContainer)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("nsXULContentBuilder::BuildContentFromTemplate (is unique: %d)",
               aIsUnique));

        nsAutoString id;
        aChild->GetId(id);

        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("Tags: [Template: %s  Resource: %s  Real: %s] for id %s",
                nsAtomCString(aTemplateNode->NodeInfo()->NameAtom()).get(),
                nsAtomCString(aResourceNode->NodeInfo()->NameAtom()).get(),
                nsAtomCString(aRealNode->NodeInfo()->NameAtom()).get(), NS_ConvertUTF16toUTF8(id).get()));
    }
#endif

    
    
    for (nsIContent* tmplKid = aTemplateNode->GetFirstChild();
         tmplKid;
         tmplKid = tmplKid->GetNextSibling()) {

        int32_t nameSpaceID = tmplKid->GetNameSpaceID();

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        bool isGenerationElement = false;
        bool isUnique = aIsUnique;

        
        
        
        if (tmplKid->HasAttr(kNameSpaceID_None, nsGkAtoms::uri) && aMatch->IsActive()) {
            isGenerationElement = true;
            isUnique = false;
        }

        MOZ_ASSERT_IF(isGenerationElement, tmplKid->IsElement());

        nsIAtom *tag = tmplKid->NodeInfo()->NameAtom();

#ifdef PR_LOGGING
        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("xultemplate[%p]     building %s %s %s",
                    this, nsAtomCString(tag).get(),
                    (isGenerationElement ? "[resource]" : ""),
                    (isUnique ? "[unique]" : "")));
        }
#endif

        
        
        bool realKidAlreadyExisted = false;

        nsCOMPtr<nsIContent> realKid;
        if (isUnique) {
            
            
            
            
            
            rv = EnsureElementHasGenericChild(aRealNode, nameSpaceID, tag, aNotify, getter_AddRefs(realKid));
            if (NS_FAILED(rv))
                return rv;

            if (rv == NS_ELEMENT_WAS_THERE) {
                realKidAlreadyExisted = true;
            }
            else {
                
                
                
                if (aContainer && !*aContainer) {
                    *aContainer = aRealNode;
                    NS_ADDREF(*aContainer);

                    uint32_t indx = aRealNode->GetChildCount();

                    
                    
                    *aNewIndexInContainer = indx - 1;
                }
            }

            
            
            
            
            
            rv = BuildContentFromTemplate(tmplKid, aResourceNode, realKid, true,
                                          aIsSelfReference, aChild, aNotify, aMatch,
                                          aContainer, aNewIndexInContainer);

            if (NS_FAILED(rv))
                return rv;
        }
        else if (isGenerationElement) {
            
            
            nsCOMPtr<Element> element;
            rv = CreateElement(nameSpaceID, tag, getter_AddRefs(element));
            if (NS_FAILED(rv))
                return rv;
            realKid = element.forget();

            
            
            mContentSupportMap.Put(realKid, aMatch);

            
            nsAutoString id;
            rv = aChild->GetId(id);
            if (NS_FAILED(rv))
                return rv;

            rv = realKid->SetAttr(kNameSpaceID_None, nsGkAtoms::id, id, false);
            if (NS_FAILED(rv))
                return rv;

            
            SetContainerAttrs(realKid, aChild, true, false);
        }
        else if (tag == nsGkAtoms::textnode &&
                 nameSpaceID == kNameSpaceID_XUL) {
            
            
            
            
            
            char16_t attrbuf[128];
            nsFixedString attrValue(attrbuf, ArrayLength(attrbuf), 0);
            tmplKid->GetAttr(kNameSpaceID_None, nsGkAtoms::value, attrValue);
            if (!attrValue.IsEmpty()) {
                nsAutoString value;
                rv = SubstituteText(aChild, attrValue, value);
                if (NS_FAILED(rv)) return rv;

                nsRefPtr<nsTextNode> content =
                  new nsTextNode(mRoot->NodeInfo()->NodeInfoManager());

                content->SetText(value, false);

                rv = aRealNode->AppendChildTo(content, aNotify);
                if (NS_FAILED(rv)) return rv;

                
                
            }
        }
        else if (tmplKid->IsNodeOfType(nsINode::eTEXT)) {
            nsCOMPtr<nsIDOMNode> tmplTextNode = do_QueryInterface(tmplKid);
            if (!tmplTextNode) {
                NS_ERROR("textnode not implementing nsIDOMNode??");
                return NS_ERROR_FAILURE;
            }
            nsCOMPtr<nsIDOMNode> clonedNode;
            tmplTextNode->CloneNode(false, 1, getter_AddRefs(clonedNode));
            nsCOMPtr<nsIContent> clonedContent = do_QueryInterface(clonedNode);
            if (!clonedContent) {
                NS_ERROR("failed to clone textnode");
                return NS_ERROR_FAILURE;
            }
            rv = aRealNode->AppendChildTo(clonedContent, aNotify);
            if (NS_FAILED(rv)) return rv;
        }
        else {
            
            nsCOMPtr<Element> element;
            rv = CreateElement(nameSpaceID, tag, getter_AddRefs(element));
            if (NS_FAILED(rv)) return rv;
            realKid = element.forget();
        }

        if (realKid && !realKidAlreadyExisted) {
            
            
            if (aContainer && !*aContainer) {
                *aContainer = aRealNode;
                NS_ADDREF(*aContainer);

                uint32_t indx = aRealNode->GetChildCount();

                
                
                
                *aNewIndexInContainer = indx;
            }

            
            
            
            mTemplateMap.Put(realKid, tmplKid);

            rv = CopyAttributesToElement(tmplKid, realKid, aChild, false);
            if (NS_FAILED(rv)) return rv;

            
            if (isGenerationElement) {
                rv = AddPersistentAttributes(tmplKid->AsElement(), aChild,
                                             realKid);
                if (NS_FAILED(rv)) return rv;
            }

            
            
            
            if (!aIsSelfReference && !isUnique) {
                
                
                
                
                
                rv = BuildContentFromTemplate(tmplKid, aResourceNode, realKid, false,
                                              false, aChild, false, aMatch,
                                              nullptr ,
                                              nullptr );
                if (NS_FAILED(rv)) return rv;

                if (isGenerationElement) {
                    
                    rv = CreateContainerContents(realKid, aChild, false,
                                                 false, false);
                    if (NS_FAILED(rv)) return rv;
                }
            }

            
            
            
            if (! isUnique) {
                rv = NS_ERROR_UNEXPECTED;

                if (isGenerationElement)
                    rv = InsertSortedNode(aRealNode, realKid, aChild, aNotify);

                if (NS_FAILED(rv)) {
                    rv = aRealNode->AppendChildTo(realKid, aNotify);
                    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to insert element");
                }
            }
        }
    }

    return NS_OK;
}

nsresult
nsXULContentBuilder::CopyAttributesToElement(nsIContent* aTemplateNode,
                                             nsIContent* aRealNode,
                                             nsIXULTemplateResult* aResult,
                                             bool aNotify)
{
    nsresult rv;

    
    uint32_t numAttribs = aTemplateNode->GetAttrCount();

    for (uint32_t attr = 0; attr < numAttribs; attr++) {
        const nsAttrName* name = aTemplateNode->GetAttrNameAt(attr);
        int32_t attribNameSpaceID = name->NamespaceID();
        
        
        nsCOMPtr<nsIAtom> attribName = name->LocalName();

        
        if (attribName != nsGkAtoms::id && attribName != nsGkAtoms::uri) {
            
            
            
            char16_t attrbuf[128];
            nsFixedString attribValue(attrbuf, ArrayLength(attrbuf), 0);
            aTemplateNode->GetAttr(attribNameSpaceID, attribName, attribValue);
            if (!attribValue.IsEmpty()) {
                nsAutoString value;
                rv = SubstituteText(aResult, attribValue, value);
                if (NS_FAILED(rv))
                    return rv;

                
                
                if (!value.IsEmpty()) {
                    rv = aRealNode->SetAttr(attribNameSpaceID,
                                            attribName,
                                            name->GetPrefix(),
                                            value,
                                            aNotify);
                }
                else {
                    rv = aRealNode->UnsetAttr(attribNameSpaceID,
                                              attribName,
                                              aNotify);
                }

                if (NS_FAILED(rv))
                    return rv;
            }
        }
    }

    return NS_OK;
}

nsresult
nsXULContentBuilder::AddPersistentAttributes(Element* aTemplateNode,
                                             nsIXULTemplateResult* aResult,
                                             nsIContent* aRealNode)
{
    if (!mRoot)
        return NS_OK;

    nsCOMPtr<nsIRDFResource> resource;
    nsresult rv = GetResultResource(aResult, getter_AddRefs(resource));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString attribute, persist;
    aTemplateNode->GetAttr(kNameSpaceID_None, nsGkAtoms::persist, persist);

    while (!persist.IsEmpty()) {
        attribute.Truncate();

        int32_t offset = persist.FindCharInSet(" ,");
        if (offset > 0) {
            persist.Left(attribute, offset);
            persist.Cut(0, offset + 1);
        }
        else {
            attribute = persist;
            persist.Truncate();
        }

        attribute.Trim(" ");

        if (attribute.IsEmpty())
            break;

        nsCOMPtr<nsIAtom> tag;
        int32_t nameSpaceID;

        nsRefPtr<mozilla::dom::NodeInfo> ni =
            aTemplateNode->GetExistingAttrNameFromQName(attribute);
        if (ni) {
            tag = ni->NameAtom();
            nameSpaceID = ni->NamespaceID();
        }
        else {
            tag = do_GetAtom(attribute);
            NS_ENSURE_TRUE(tag, NS_ERROR_OUT_OF_MEMORY);

            nameSpaceID = kNameSpaceID_None;
        }

        nsCOMPtr<nsIRDFResource> property;
        rv = nsXULContentUtils::GetResource(nameSpaceID, tag, getter_AddRefs(property));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIRDFNode> target;
        rv = mDB->GetTarget(resource, property, true, getter_AddRefs(target));
        NS_ENSURE_SUCCESS(rv, rv);

        if (! target)
            continue;

        nsCOMPtr<nsIRDFLiteral> value = do_QueryInterface(target);
        NS_ASSERTION(value != nullptr, "unable to stomach that sort of node");
        if (! value)
            continue;

        const char16_t* valueStr;
        rv = value->GetValueConst(&valueStr);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aRealNode->SetAttr(nameSpaceID, tag, nsDependentString(valueStr),
                                false);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
nsXULContentBuilder::SynchronizeUsingTemplate(nsIContent* aTemplateNode,
                                              nsIContent* aRealElement,
                                              nsIXULTemplateResult* aResult)
{
    
    
    nsresult rv;
    rv = CopyAttributesToElement(aTemplateNode, aRealElement, aResult, true);
    if (NS_FAILED(rv))
        return rv;

    uint32_t count = aTemplateNode->GetChildCount();

    for (uint32_t loop = 0; loop < count; ++loop) {
        nsIContent *tmplKid = aTemplateNode->GetChildAt(loop);

        if (! tmplKid)
            break;

        nsIContent *realKid = aRealElement->GetChildAt(loop);
        if (! realKid)
            break;

        
        
        if (tmplKid->NodeInfo()->Equals(nsGkAtoms::textnode,
                                        kNameSpaceID_XUL)) {
            char16_t attrbuf[128];
            nsFixedString attrValue(attrbuf, ArrayLength(attrbuf), 0);
            tmplKid->GetAttr(kNameSpaceID_None, nsGkAtoms::value, attrValue);
            if (!attrValue.IsEmpty()) {
                nsAutoString value;
                rv = SubstituteText(aResult, attrValue, value);
                if (NS_FAILED(rv)) return rv;
                realKid->SetText(value, true);
            }
        }

        rv = SynchronizeUsingTemplate(tmplKid, realKid, aResult);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsXULContentBuilder::RemoveMember(nsIContent* aContent)
{
    nsCOMPtr<nsIContent> parent = aContent->GetParent();
    if (parent) {
        int32_t pos = parent->IndexOf(aContent);

        NS_ASSERTION(pos >= 0, "parent doesn't think this child has an index");
        if (pos < 0) return NS_OK;

        
        
        
        parent->RemoveChildAt(pos, true);
    }

    
    mContentSupportMap.Remove(aContent);

    
    mTemplateMap.Remove(aContent);

    return NS_OK;
}

nsresult
nsXULContentBuilder::CreateTemplateAndContainerContents(nsIContent* aElement,
                                                        bool aForceCreation)
{
    
    
    

    PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
           ("nsXULContentBuilder::CreateTemplateAndContainerContents start - flags: %d",
            mFlags));

    if (! mQueryProcessor)
        return NS_OK;

    
    if (aElement == mRoot) {
        if (! mRootResult) {
            nsAutoString ref;
            mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::ref, ref);

            if (! ref.IsEmpty()) {
                nsresult rv = mQueryProcessor->TranslateRef(mDataSource, ref,
                                                            getter_AddRefs(mRootResult));
                if (NS_FAILED(rv))
                    return rv;
            }
        }

        if (mRootResult) {
            CreateContainerContents(aElement, mRootResult, aForceCreation,
                                    false, true);
        }
    }
    else if (!(mFlags & eDontRecurse)) {
        
        
        
        nsTemplateMatch *match = nullptr;
        if (mContentSupportMap.Get(aElement, &match))
            CreateContainerContents(aElement, match->mResult, aForceCreation,
                                    false, true);
    }

    PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
           ("nsXULContentBuilder::CreateTemplateAndContainerContents end"));

    return NS_OK;
}

nsresult
nsXULContentBuilder::CreateContainerContents(nsIContent* aElement,
                                             nsIXULTemplateResult* aResult,
                                             bool aForceCreation,
                                             bool aNotify,
                                             bool aNotifyAtEnd)
{
    if (!aForceCreation && !IsOpen(aElement))
        return NS_OK;

    
    if (aResult != mRootResult) {
        if (mFlags & eDontRecurse)
            return NS_OK;

        bool mayProcessChildren;
        nsresult rv = aResult->GetMayProcessChildren(&mayProcessChildren);
        if (NS_FAILED(rv) || !mayProcessChildren)
            return rv;
    }

    nsCOMPtr<nsIRDFResource> refResource;
    GetResultResource(aResult, getter_AddRefs(refResource));
    if (! refResource)
        return NS_ERROR_FAILURE;

    
    if (IsActivated(refResource))
        return NS_OK;

    ActivationEntry entry(refResource, &mTop);

    
    if (! mQueriesCompiled) {
        nsresult rv = CompileQueries();
        if (NS_FAILED(rv))
            return rv;
    }

    if (mQuerySets.Length() == 0)
        return NS_OK;

    
    
    
    nsXULElement *xulcontent = nsXULElement::FromContent(aElement);
    if (xulcontent) {
        if (xulcontent->GetTemplateGenerated())
            return NS_OK;

        
        
        xulcontent->SetTemplateGenerated();
    }

    int32_t newIndexInContainer = -1;
    nsIContent* container = nullptr;

    int32_t querySetCount = mQuerySets.Length();

    for (int32_t r = 0; r < querySetCount; r++) {
        nsTemplateQuerySet* queryset = mQuerySets[r];

        nsIAtom* tag = queryset->GetTag();
        if (tag && tag != aElement->NodeInfo()->NameAtom())
            continue;

        CreateContainerContentsForQuerySet(aElement, aResult, aNotify, queryset,
                                           &container, &newIndexInContainer);
    }

    if (aNotifyAtEnd && container) {
        MOZ_AUTO_DOC_UPDATE(container->GetUncomposedDoc(), UPDATE_CONTENT_MODEL,
                            true);
        nsNodeUtils::ContentAppended(container,
                                     container->GetChildAt(newIndexInContainer),
                                     newIndexInContainer);
    }

    NS_IF_RELEASE(container);

    return NS_OK;
}

nsresult
nsXULContentBuilder::CreateContainerContentsForQuerySet(nsIContent* aElement,
                                                        nsIXULTemplateResult* aResult,
                                                        bool aNotify,
                                                        nsTemplateQuerySet* aQuerySet,
                                                        nsIContent** aContainer,
                                                        int32_t* aNewIndexInContainer)
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        nsAutoString id;
        aResult->GetId(id);
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("nsXULContentBuilder::CreateContainerContentsForQuerySet start for ref %s\n",
               NS_ConvertUTF16toUTF8(id).get()));
    }
#endif

    if (! mQueryProcessor)
        return NS_OK;

    nsCOMPtr<nsISimpleEnumerator> results;
    nsresult rv = mQueryProcessor->GenerateResults(mDataSource, aResult,
                                                   aQuerySet->mCompiledQuery,
                                                   getter_AddRefs(results));
    if (NS_FAILED(rv) || !results)
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

        if (!resultid)
            continue;

        nsTemplateMatch *newmatch =
            nsTemplateMatch::Create(aQuerySet->Priority(),
                                    nextresult, aElement);
        if (!newmatch)
            return NS_ERROR_OUT_OF_MEMORY;

        
        
        

        bool generateContent = true;

        nsTemplateMatch* prevmatch = nullptr;
        nsTemplateMatch* existingmatch = nullptr;
        nsTemplateMatch* removematch = nullptr;
        if (mMatchMap.Get(resultid, &existingmatch)){
            
            while (existingmatch) {
                
                
                
                int32_t priority = existingmatch->QuerySetPriority();
                if (priority > aQuerySet->Priority())
                    break;

                
                if (existingmatch->GetContainer() == aElement) {
                    
                    
                    if (priority == aQuerySet->Priority()) {
                        removematch = existingmatch;
                        break;
                    }

                    if (existingmatch->IsActive())
                        generateContent = false;
                }

                prevmatch = existingmatch;
                existingmatch = existingmatch->mNext;
            }
        }

        if (removematch) {
            
            rv = ReplaceMatch(removematch->mResult, nullptr, nullptr, aElement);
            if (NS_FAILED(rv))
                return rv;

            if (mFlags & eLoggingEnabled)
                OutputMatchToLog(resultid, removematch, false);
        }

        if (generateContent) {
            
            

            int16_t ruleindex;
            nsTemplateRule* matchedrule = nullptr;
            rv = DetermineMatchedRule(aElement, nextresult, aQuerySet,
                                      &matchedrule, &ruleindex);
            if (NS_FAILED(rv)) {
                nsTemplateMatch::Destroy(newmatch, false);
                return rv;
            }

            if (matchedrule) {
                rv = newmatch->RuleMatched(aQuerySet, matchedrule,
                                           ruleindex, nextresult);
                if (NS_FAILED(rv)) {
                    nsTemplateMatch::Destroy(newmatch, false);
                    return rv;
                }

                
                nsCOMPtr<nsIContent> action = matchedrule->GetAction();
                BuildContentFromTemplate(action, aElement, aElement, true,
                                         mRefVariable == matchedrule->GetMemberVariable(),
                                         nextresult, aNotify, newmatch,
                                         aContainer, aNewIndexInContainer);
            }
        }

        if (mFlags & eLoggingEnabled)
            OutputMatchToLog(resultid, newmatch, true);

        if (prevmatch) {
            prevmatch->mNext = newmatch;
        }
        else {
            mMatchMap.Put(resultid, newmatch);
        }

        if (removematch) {
            newmatch->mNext = removematch->mNext;
            nsTemplateMatch::Destroy(removematch, true);
        }
        else {
            newmatch->mNext = existingmatch;
        }
    }

    return rv;
}

nsresult
nsXULContentBuilder::EnsureElementHasGenericChild(nsIContent* parent,
                                                  int32_t nameSpaceID,
                                                  nsIAtom* tag,
                                                  bool aNotify,
                                                  nsIContent** result)
{
    nsresult rv;

    rv = nsXULContentUtils::FindChildByTag(parent, nameSpaceID, tag, result);
    if (NS_FAILED(rv))
        return rv;

    if (rv == NS_RDF_NO_VALUE) {
        
        nsCOMPtr<Element> element;

        rv = CreateElement(nameSpaceID, tag, getter_AddRefs(element));
        if (NS_FAILED(rv))
            return rv;

        
        rv = parent->AppendChildTo(element, aNotify);
        if (NS_FAILED(rv))
            return rv;

        element.forget(result);
        return NS_ELEMENT_GOT_CREATED;
    }
    else {
        return NS_ELEMENT_WAS_THERE;
    }
}

bool
nsXULContentBuilder::IsOpen(nsIContent* aElement)
{
    

    
    if (aElement->IsAnyOfXULElements(nsGkAtoms::menu,
                                     nsGkAtoms::menubutton,
                                     nsGkAtoms::toolbarbutton,
                                     nsGkAtoms::button,
                                     nsGkAtoms::treeitem))
        return aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::open,
                                     nsGkAtoms::_true, eCaseMatters);
    return true;
}

nsresult
nsXULContentBuilder::RemoveGeneratedContent(nsIContent* aElement)
{
    
    
    nsAutoTArray<nsIContent*, 8> ungenerated;
    if (ungenerated.AppendElement(aElement) == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;

    uint32_t count;
    while (0 != (count = ungenerated.Length())) {
        
        uint32_t last = count - 1;
        nsCOMPtr<nsIContent> element = ungenerated[last];
        ungenerated.RemoveElementAt(last);

        uint32_t i = element->GetChildCount();

        while (i-- > 0) {
            nsCOMPtr<nsIContent> child = element->GetChildAt(i);

            
            
            
            
            
            if (element->NodeInfo()->Equals(nsGkAtoms::_template,
                                            kNameSpaceID_XUL) ||
                !element->IsElement())
                continue;

            
            
            nsCOMPtr<nsIContent> tmpl;
            mTemplateMap.GetTemplateFor(child, getter_AddRefs(tmpl));

            if (! tmpl) {
                
                
                if (ungenerated.AppendElement(child) == nullptr)
                    return NS_ERROR_OUT_OF_MEMORY;
                continue;
            }

            
            element->RemoveChildAt(i, true);

            
            mContentSupportMap.Remove(child);

            
            mTemplateMap.Remove(child);
        }
    }

    return NS_OK;
}

nsresult
nsXULContentBuilder::GetElementsForResult(nsIXULTemplateResult* aResult,
                                          nsCOMArray<nsIContent>& aElements)
{
    
    
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mRoot->GetComposedDoc());
    if (! xuldoc)
        return NS_OK;

    nsAutoString id;
    aResult->GetId(id);

    xuldoc->GetElementsForID(id, aElements);

    return NS_OK;
}

nsresult
nsXULContentBuilder::CreateElement(int32_t aNameSpaceID,
                                   nsIAtom* aTag,
                                   Element** aResult)
{
    nsCOMPtr<nsIDocument> doc = mRoot->GetComposedDoc();
    NS_ASSERTION(doc != nullptr, "not initialized");
    if (! doc)
        return NS_ERROR_NOT_INITIALIZED;

    nsRefPtr<mozilla::dom::NodeInfo> nodeInfo =
        doc->NodeInfoManager()->GetNodeInfo(aTag, nullptr, aNameSpaceID,
                                            nsIDOMNode::ELEMENT_NODE);

    return NS_NewElement(aResult, nodeInfo.forget(), NOT_FROM_PARSER);
}

nsresult
nsXULContentBuilder::SetContainerAttrs(nsIContent *aElement,
                                       nsIXULTemplateResult* aResult,
                                       bool aIgnoreNonContainers,
                                       bool aNotify)
{
    NS_PRECONDITION(aResult != nullptr, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    bool iscontainer;
    aResult->GetIsContainer(&iscontainer);

    if (aIgnoreNonContainers && !iscontainer)
        return NS_OK;

    NS_NAMED_LITERAL_STRING(true_, "true");
    NS_NAMED_LITERAL_STRING(false_, "false");

    const nsAString& newcontainer =
        iscontainer ? true_ : false_;

    aElement->SetAttr(kNameSpaceID_None, nsGkAtoms::container,
                      newcontainer, aNotify);

    if (iscontainer && !(mFlags & eDontTestEmpty)) {
        bool isempty;
        aResult->GetIsEmpty(&isempty);

        const nsAString& newempty =
            (iscontainer && isempty) ? true_ : false_;

        aElement->SetAttr(kNameSpaceID_None, nsGkAtoms::empty,
                          newempty, aNotify);
    }

    return NS_OK;
}







NS_IMETHODIMP
nsXULContentBuilder::CreateContents(nsIContent* aElement, bool aForceCreation)
{
    NS_PRECONDITION(aElement != nullptr, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    
    
    if (!aForceCreation && !IsOpen(aElement))
        return NS_OK;

    return CreateTemplateAndContainerContents(aElement, aForceCreation);
}

NS_IMETHODIMP
nsXULContentBuilder::HasGeneratedContent(nsIRDFResource* aResource,
                                         nsIAtom* aTag,
                                         bool* aGenerated)
{
    *aGenerated = false;
    NS_ENSURE_TRUE(mRoot, NS_ERROR_NOT_INITIALIZED);
    NS_ENSURE_STATE(mRootResult);

    nsCOMPtr<nsIRDFResource> rootresource;
    nsresult rv = mRootResult->GetResource(getter_AddRefs(rootresource));
    if (NS_FAILED(rv))
        return rv;

    
    if (aResource == rootresource) {
        if (!aTag || mRoot->NodeInfo()->NameAtom() == aTag)
            *aGenerated = true;
    }
    else {
        const char* uri;
        aResource->GetValueConst(&uri);

        NS_ConvertUTF8toUTF16 refID(uri);

        
        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mRoot->GetComposedDoc());
        if (! xuldoc)
            return NS_OK;

        nsCOMArray<nsIContent> elements;
        xuldoc->GetElementsForID(refID, elements);

        uint32_t cnt = elements.Count();

        for (int32_t i = int32_t(cnt) - 1; i >= 0; --i) {
            nsCOMPtr<nsIContent> content = elements.SafeObjectAt(i);

            do {
                nsTemplateMatch* match;
                if (content == mRoot || mContentSupportMap.Get(content, &match)) {
                    
                    if (!aTag || content->NodeInfo()->NameAtom() == aTag) {
                        *aGenerated = true;
                        return NS_OK;
                    }
                }

                content = content->GetParent();
            } while (content);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULContentBuilder::GetResultForContent(nsIDOMElement* aElement,
                                         nsIXULTemplateResult** aResult)
{
    NS_ENSURE_ARG_POINTER(aElement);
    NS_ENSURE_ARG_POINTER(aResult);

    nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
    if (content == mRoot) {
        *aResult = mRootResult;
    }
    else {
        nsTemplateMatch *match = nullptr;
        if (mContentSupportMap.Get(content, &match))
            *aResult = match->mResult;
        else
            *aResult = nullptr;
    }

    NS_IF_ADDREF(*aResult);
    return NS_OK;
}






void
nsXULContentBuilder::AttributeChanged(nsIDocument* aDocument,
                                      Element*     aElement,
                                      int32_t      aNameSpaceID,
                                      nsIAtom*     aAttribute,
                                      int32_t      aModType)
{
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);

    
    
    
    if (aElement->GetNameSpaceID() == kNameSpaceID_XUL &&
        aAttribute == nsGkAtoms::open) {
        
        if (aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::open,
                                  nsGkAtoms::_true, eCaseMatters))
            OpenContainer(aElement);
        else
            CloseContainer(aElement);
    }

    if ((aNameSpaceID == kNameSpaceID_XUL) &&
        ((aAttribute == nsGkAtoms::sort) ||
         (aAttribute == nsGkAtoms::sortDirection) ||
         (aAttribute == nsGkAtoms::sortResource) ||
         (aAttribute == nsGkAtoms::sortResource2)))
        mSortState.initialized = false;

    
    nsXULTemplateBuilder::AttributeChanged(aDocument, aElement, aNameSpaceID,
                                           aAttribute, aModType);
}

void
nsXULContentBuilder::NodeWillBeDestroyed(const nsINode* aNode)
{
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);
    
    mContentSupportMap.Clear();

    nsXULTemplateBuilder::NodeWillBeDestroyed(aNode);
}







bool
nsXULContentBuilder::GetInsertionLocations(nsIXULTemplateResult* aResult,
                                           nsCOMArray<nsIContent>** aLocations)
{
    *aLocations = nullptr;

    nsAutoString ref;
    nsresult rv = aResult->GetBindingFor(mRefVariable, ref);
    if (NS_FAILED(rv))
        return false;

    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mRoot->GetComposedDoc());
    if (! xuldoc)
        return false;

    *aLocations = new nsCOMArray<nsIContent>;
    NS_ENSURE_TRUE(*aLocations, false);

    xuldoc->GetElementsForID(ref, **aLocations);
    uint32_t count = (*aLocations)->Count();

    bool found = false;

    for (uint32_t t = 0; t < count; t++) {
        nsCOMPtr<nsIContent> content = (*aLocations)->SafeObjectAt(t);

        nsTemplateMatch* refmatch;
        if (content == mRoot || mContentSupportMap.Get(content, &refmatch)) {
            
            
            
            
            
            nsXULElement *xulcontent = nsXULElement::FromContent(content);
            if (!xulcontent || xulcontent->GetTemplateGenerated()) {
                found = true;
                continue;
            }
        }

        
        (*aLocations)->ReplaceObjectAt(nullptr, t);
    }

    return found;
}

nsresult
nsXULContentBuilder::ReplaceMatch(nsIXULTemplateResult* aOldResult,
                                  nsTemplateMatch* aNewMatch,
                                  nsTemplateRule* aNewMatchRule,
                                  void *aContext)

{
    nsresult rv;
    nsIContent* content = static_cast<nsIContent*>(aContext);

    
    if (content) {
        nsAutoString ref;
        if (aNewMatch)
            rv = aNewMatch->mResult->GetBindingFor(mRefVariable, ref);
        else
            rv = aOldResult->GetBindingFor(mRefVariable, ref);
        if (NS_FAILED(rv))
            return rv;

        if (!ref.IsEmpty()) {
            nsCOMPtr<nsIXULTemplateResult> refResult;
            rv = GetResultForId(ref, getter_AddRefs(refResult));
            if (NS_FAILED(rv))
                return rv;

            if (refResult)
                SetContainerAttrs(content, refResult, false, true);
        }
    }

    if (aOldResult) {
        nsCOMArray<nsIContent> elements;
        rv = GetElementsForResult(aOldResult, elements);
        if (NS_FAILED(rv))
            return rv;

        uint32_t count = elements.Count();

        for (int32_t e = int32_t(count) - 1; e >= 0; --e) {
            nsCOMPtr<nsIContent> child = elements.SafeObjectAt(e);

            nsTemplateMatch* match;
            if (mContentSupportMap.Get(child, &match)) {
                if (content == match->GetContainer())
                    RemoveMember(child);
            }
        }
    }

    if (aNewMatch) {
        nsCOMPtr<nsIContent> action = aNewMatchRule->GetAction();
        return BuildContentFromTemplate(action, content, content, true,
                                        mRefVariable == aNewMatchRule->GetMemberVariable(),
                                        aNewMatch->mResult, true, aNewMatch,
                                        nullptr, nullptr);
    }

    return NS_OK;
}


nsresult
nsXULContentBuilder::SynchronizeResult(nsIXULTemplateResult* aResult)
{
    nsCOMArray<nsIContent> elements;
    GetElementsForResult(aResult, elements);

    uint32_t cnt = elements.Count();

    for (int32_t i = int32_t(cnt) - 1; i >= 0; --i) {
        nsCOMPtr<nsIContent> element = elements.SafeObjectAt(i);

        nsTemplateMatch* match;
        if (! mContentSupportMap.Get(element, &match))
            continue;

        nsCOMPtr<nsIContent> templateNode;
        mTemplateMap.GetTemplateFor(element, getter_AddRefs(templateNode));

        NS_ASSERTION(templateNode, "couldn't find template node for element");
        if (! templateNode)
            continue;

        
        SynchronizeUsingTemplate(templateNode, element, aResult);
    }

    return NS_OK;
}






nsresult
nsXULContentBuilder::OpenContainer(nsIContent* aElement)
{
    if (aElement != mRoot) {
        if (mFlags & eDontRecurse)
            return NS_OK;

        bool rightBuilder = false;

        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(aElement->GetComposedDoc());
        if (! xuldoc)
            return NS_OK;

        
        nsIContent* content = aElement;
        do {
            nsCOMPtr<nsIXULTemplateBuilder> builder;
            xuldoc->GetTemplateBuilderFor(content, getter_AddRefs(builder));
            if (builder) {
                if (builder == this)
                    rightBuilder = true;
                break;
            }

            content = content->GetParent();
        } while (content);

        if (! rightBuilder)
            return NS_OK;
    }

    CreateTemplateAndContainerContents(aElement, false);

    return NS_OK;
}

nsresult
nsXULContentBuilder::CloseContainer(nsIContent* aElement)
{
    return NS_OK;
}

nsresult
nsXULContentBuilder::RebuildAll()
{
    NS_ENSURE_TRUE(mRoot, NS_ERROR_NOT_INITIALIZED);

    
    nsCOMPtr<nsIDocument> doc = mRoot->GetComposedDoc();
    if (!doc)
        return NS_OK;

    if (mQueriesCompiled)
        Uninit(false);

    nsresult rv = CompileQueries();
    if (NS_FAILED(rv))
        return rv;

    if (mQuerySets.Length() == 0)
        return NS_OK;

    nsXULElement *xulcontent = nsXULElement::FromContent(mRoot);
    if (xulcontent)
        xulcontent->ClearTemplateGenerated();

    
    
    CreateTemplateAndContainerContents(mRoot, false);

    return NS_OK;
}



nsresult
nsXULContentBuilder::CompareResultToNode(nsIXULTemplateResult* aResult,
                                         nsIContent* aContent,
                                         int32_t* aSortOrder)
{
    NS_ASSERTION(aSortOrder, "CompareResultToNode: null out param aSortOrder");
  
    *aSortOrder = 0;

    nsTemplateMatch *match = nullptr;
    if (!mContentSupportMap.Get(aContent, &match)) {
        *aSortOrder = mSortState.sortStaticsLast ? -1 : 1;
        return NS_OK;
    }

    if (!mQueryProcessor)
        return NS_OK;

    if (mSortState.direction == nsSortState_natural) {
        
        nsresult rv = mQueryProcessor->CompareResults(aResult, match->mResult,
                                                      nullptr, mSortState.sortHints,
                                                      aSortOrder);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        
        
        int32_t length = mSortState.sortKeys.Count();
        for (int32_t t = 0; t < length; t++) {
            nsresult rv = mQueryProcessor->CompareResults(aResult, match->mResult,
                                                          mSortState.sortKeys[t],
                                                          mSortState.sortHints, aSortOrder);
            NS_ENSURE_SUCCESS(rv, rv);

            if (*aSortOrder)
                break;
        }
    }

    
    if (mSortState.direction == nsSortState_descending)
        *aSortOrder = -*aSortOrder;

    return NS_OK;
}

nsresult
nsXULContentBuilder::InsertSortedNode(nsIContent* aContainer,
                                      nsIContent* aNode,
                                      nsIXULTemplateResult* aResult,
                                      bool aNotify)
{
    nsresult rv;

    if (!mSortState.initialized) {
        nsAutoString sort, sortDirection, sortHints;
        mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::sort, sort);
        mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::sortDirection, sortDirection);
        mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::sorthints, sortHints);
        sortDirection += ' ';
        sortDirection += sortHints;
        rv = XULSortServiceImpl::InitializeSortState(mRoot, aContainer,
                                                     sort, sortDirection, &mSortState);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    
    mSortState.isContainerRDFSeq = false;
    if (mSortState.direction == nsSortState_natural) {
        nsCOMPtr<nsISupports> ref;
        nsresult rv = aResult->GetBindingObjectFor(mRefVariable, getter_AddRefs(ref));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIRDFResource> container = do_QueryInterface(ref);

        if (container) {
            rv = gRDFContainerUtils->IsSeq(mDB, container, &mSortState.isContainerRDFSeq);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    bool childAdded = false;
    uint32_t numChildren = aContainer->GetChildCount();

    if (mSortState.direction != nsSortState_natural ||
        (mSortState.direction == nsSortState_natural && mSortState.isContainerRDFSeq))
    {
        
        int32_t realNumChildren = numChildren;
        nsIContent *child = nullptr;

        
        int32_t staticCount = 0;

        nsAutoString staticValue;
        aContainer->GetAttr(kNameSpaceID_None, nsGkAtoms::staticHint, staticValue);
        if (!staticValue.IsEmpty())
        {
            
            nsresult strErr = NS_OK;
            staticCount = staticValue.ToInteger(&strErr);
            if (NS_FAILED(strErr))
                staticCount = 0;
        } else {
            
            for (nsIContent* child = aContainer->GetFirstChild();
                 child;
                 child = child->GetNextSibling()) {
                 
                if (nsContentUtils::HasNonEmptyAttr(child, kNameSpaceID_None,
                                                    nsGkAtoms::_template))
                    break;
                else
                    ++staticCount;
            }

            if (mSortState.sortStaticsLast) {
                
                
                staticCount = -staticCount;
            }

            
            nsAutoString valueStr;
            valueStr.AppendInt(staticCount);
            aContainer->SetAttr(kNameSpaceID_None, nsGkAtoms::staticHint, valueStr, false);
        }

        if (staticCount <= 0) {
            numChildren += staticCount;
            staticCount = 0;
        } else if (staticCount > (int32_t)numChildren) {
            staticCount = numChildren;
            numChildren -= staticCount;
        }

        
        if (numChildren > 0) {
            nsIContent *temp;
            int32_t direction;

            
            

            if (mSortState.lastWasFirst) {
                child = aContainer->GetChildAt(staticCount);
                temp = child;
                rv = CompareResultToNode(aResult, temp, &direction);
                if (direction < 0) {
                    aContainer->InsertChildAt(aNode, staticCount, aNotify);
                    childAdded = true;
                } else
                    mSortState.lastWasFirst = false;
            } else if (mSortState.lastWasLast) {
                child = aContainer->GetChildAt(realNumChildren - 1);
                temp = child;
                rv = CompareResultToNode(aResult, temp, &direction);
                if (direction > 0) {
                    aContainer->InsertChildAt(aNode, realNumChildren, aNotify);
                    childAdded = true;
                } else
                    mSortState.lastWasLast = false;
            }

            int32_t left = staticCount + 1, right = realNumChildren, x;
            while (!childAdded && right >= left) {
                x = (left + right) / 2;
                child = aContainer->GetChildAt(x - 1);
                temp = child;

                rv = CompareResultToNode(aResult, temp, &direction);
                if ((x == left && direction < 0) ||
                    (x == right && direction >= 0) ||
                    left == right)
                {
                    int32_t thePos = (direction > 0 ? x : x - 1);
                    aContainer->InsertChildAt(aNode, thePos, aNotify);
                    childAdded = true;

                    mSortState.lastWasFirst = (thePos == staticCount);
                    mSortState.lastWasLast = (thePos >= realNumChildren);

                    break;
                }
                if (direction < 0)
                    right = x - 1;
                else
                    left = x + 1;
            }
        }
    }

    
    
    if (!childAdded)
        aContainer->InsertChildAt(aNode, numChildren, aNotify);

    return NS_OK;
}
