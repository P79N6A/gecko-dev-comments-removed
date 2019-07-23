









































#include "nsContentCID.h"
#include "nsIDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULDocument.h"
#include "nsINodeInfo.h"
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

#include "jsapi.h"
#include "pldhash.h"
#include "rdf.h"





#define NS_ELEMENT_GOT_CREATED NS_RDF_NO_VALUE
#define NS_ELEMENT_WAS_THERE   NS_OK






















class nsXULContentBuilder : public nsXULTemplateBuilder
{
public:
    
    NS_IMETHOD CreateContents(nsIContent* aElement, PRBool aForceCreation);

    NS_IMETHOD HasGeneratedContent(nsIRDFResource* aResource,
                                   nsIAtom* aTag,
                                   PRBool* aGenerated);

    NS_IMETHOD GetResultForContent(nsIDOMElement* aContent,
                                   nsIXULTemplateResult** aResult);

    
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

protected:
    friend NS_IMETHODIMP
    NS_NewXULContentBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    nsXULContentBuilder();

    void Traverse(nsCycleCollectionTraversalCallback &cb) const
    {
        mSortState.Traverse(cb);
    }

    virtual void Uninit(PRBool aIsFinal);

    
    nsresult
    OpenContainer(nsIContent* aElement);

    nsresult
    CloseContainer(nsIContent* aElement);

    




    nsresult
    BuildContentFromTemplate(nsIContent *aTemplateNode,
                             nsIContent *aResourceNode,
                             nsIContent *aRealNode,
                             PRBool aIsUnique,
                             PRBool aIsSelfReference,
                             nsIXULTemplateResult* aChild,
                             PRBool aNotify,
                             nsTemplateMatch* aMatch,
                             nsIContent** aContainer,
                             PRInt32* aNewIndexInContainer);

    








    nsresult
    CopyAttributesToElement(nsIContent* aTemplateNode,
                            nsIContent* aRealNode,
                            nsIXULTemplateResult* aResult,
                            PRBool aNotify);

    







    nsresult
    AddPersistentAttributes(nsIContent* aTemplateNode,
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
                                       PRBool aForceCreation);

    









    nsresult
    CreateContainerContents(nsIContent* aElement,
                            nsIXULTemplateResult* aResult,
                            PRBool aForceCreation,
                            PRBool aNotify,
                            PRBool aNotifyAtEnd);

    








    nsresult
    CreateContainerContentsForQuerySet(nsIContent* aElement,
                                       nsIXULTemplateResult* aResult,
                                       PRBool aNotify,
                                       nsTemplateQuerySet* aQuerySet,
                                       nsIContent** aContainer,
                                       PRInt32* aNewIndexInContainer);

    










    nsresult
    EnsureElementHasGenericChild(nsIContent* aParent,
                                 PRInt32 aNameSpaceID,
                                 nsIAtom* aTag,
                                 PRBool aNotify,
                                 nsIContent** aResult);

    PRBool
    IsOpen(nsIContent* aElement);

    nsresult
    RemoveGeneratedContent(nsIContent* aElement);

    nsresult
    GetElementsForResult(nsIXULTemplateResult* aResult,
                         nsCOMArray<nsIContent>& aElements);

    nsresult
    CreateElement(PRInt32 aNameSpaceID,
                  nsIAtom* aTag,
                  nsIContent** aResult);

    










    nsresult
    SetContainerAttrs(nsIContent *aElement,
                      nsIXULTemplateResult* aResult,
                      PRBool aIgnoreNonContainers,
                      PRBool aNotify);

    virtual nsresult
    RebuildAll();

    
    

    




    virtual PRBool
    GetInsertionLocations(nsIXULTemplateResult* aOldResult,
                          nsCOMArray<nsIContent>** aLocations);

    



    virtual nsresult
    ReplaceMatch(nsIXULTemplateResult* aOldResult,
                 nsTemplateMatch* aNewMatch,
                 nsTemplateRule* aNewMatchRule,
                 void *aContext);

    




    virtual nsresult
    SynchronizeResult(nsIXULTemplateResult* aResult);

    




    nsresult
    CompareResultToNode(nsIXULTemplateResult* aResult, nsIContent* aContent,
                        PRInt32* aSortOrder);

    




    nsresult
    InsertSortedNode(nsIContent* aContainer,
                     nsIContent* aNode,
                     nsIXULTemplateResult* aResult,
                     PRBool aNotify);

    



    nsContentSupportMap mContentSupportMap;

    



    nsTemplateMap mTemplateMap;

    


    nsSortState mSortState;
};

NS_IMETHODIMP
NS_NewXULContentBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aOuter == nsnull, "no aggregation");
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
  mSortState.initialized = PR_FALSE;
}

void
nsXULContentBuilder::Uninit(PRBool aIsFinal)
{
    if (! aIsFinal && mRoot) {
        nsresult rv = RemoveGeneratedContent(mRoot);
        if (NS_FAILED(rv))
            return;
    }

    
    mContentSupportMap.Clear();
    mTemplateMap.Clear();

    mSortState.initialized = PR_FALSE;

    nsXULTemplateBuilder::Uninit(aIsFinal);
}

nsresult
nsXULContentBuilder::BuildContentFromTemplate(nsIContent *aTemplateNode,
                                              nsIContent *aResourceNode,
                                              nsIContent *aRealNode,
                                              PRBool aIsUnique,
                                              PRBool aIsSelfReference,
                                              nsIXULTemplateResult* aChild,
                                              PRBool aNotify,
                                              nsTemplateMatch* aMatch,
                                              nsIContent** aContainer,
                                              PRInt32* aNewIndexInContainer)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("nsXULContentBuilder::BuildContentFromTemplate (is unique: %d)",
               aIsUnique));

        const char *tmpln, *resn, *realn;
        aTemplateNode->Tag()->GetUTF8String(&tmpln);
        aResourceNode->Tag()->GetUTF8String(&resn);
        aRealNode->Tag()->GetUTF8String(&realn);

        nsAutoString id;
        aChild->GetId(id);

        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("Tags: [Template: %s  Resource: %s  Real: %s] for id %s",
               tmpln, resn, realn, NS_ConvertUTF16toUTF8(id).get()));
    }
#endif

    
    
    PRUint32 count = aTemplateNode->GetChildCount();

    for (PRUint32 kid = 0; kid < count; kid++) {
        nsIContent *tmplKid = aTemplateNode->GetChildAt(kid);

        PRInt32 nameSpaceID = tmplKid->GetNameSpaceID();

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        PRBool isGenerationElement = PR_FALSE;
        PRBool isUnique = aIsUnique;

        {
            
            
            
            if (tmplKid->HasAttr(kNameSpaceID_None, nsGkAtoms::uri) && aMatch->IsActive()) {
                isGenerationElement = PR_TRUE;
                isUnique = PR_FALSE;
            }
        }

        nsIAtom *tag = tmplKid->Tag();

#ifdef PR_LOGGING
        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
            const char *tagname;
            tag->GetUTF8String(&tagname);
            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("xultemplate[%p]     building %s %s %s",
                    this, tagname,
                    (isGenerationElement ? "[resource]" : ""),
                    (isUnique ? "[unique]" : "")));
        }
#endif

        
        
        PRBool realKidAlreadyExisted = PR_FALSE;

        nsCOMPtr<nsIContent> realKid;
        if (isUnique) {
            
            
            
            
            
            rv = EnsureElementHasGenericChild(aRealNode, nameSpaceID, tag, aNotify, getter_AddRefs(realKid));
            if (NS_FAILED(rv))
                return rv;

            if (rv == NS_ELEMENT_WAS_THERE) {
                realKidAlreadyExisted = PR_TRUE;
            }
            else {
                
                
                
                if (aContainer && !*aContainer) {
                    *aContainer = aRealNode;
                    NS_ADDREF(*aContainer);

                    PRUint32 indx = aRealNode->GetChildCount();

                    
                    
                    *aNewIndexInContainer = indx - 1;
                }
            }

            
            
            
            
            
            rv = BuildContentFromTemplate(tmplKid, aResourceNode, realKid, PR_TRUE,
                                          aIsSelfReference, aChild, aNotify, aMatch,
                                          aContainer, aNewIndexInContainer);

            if (NS_FAILED(rv))
                return rv;
        }
        else if (isGenerationElement) {
            
            
            rv = CreateElement(nameSpaceID, tag, getter_AddRefs(realKid));
            if (NS_FAILED(rv))
                return rv;

            
            
            mContentSupportMap.Put(realKid, aMatch);

            
            nsAutoString id;
            rv = aChild->GetId(id);
            if (NS_FAILED(rv))
                return rv;

            rv = realKid->SetAttr(kNameSpaceID_None, nsGkAtoms::id, id, PR_FALSE);
            if (NS_FAILED(rv))
                return rv;

            if (! aNotify) {
                
                
                
                
                nsCOMPtr<nsIXULDocument> xuldoc =
                    do_QueryInterface(mRoot->GetDocument());
                if (xuldoc)
                    xuldoc->AddElementForID(realKid);
            }

            
            SetContainerAttrs(realKid, aChild, PR_TRUE, PR_FALSE);
        }
        else if (tag == nsGkAtoms::textnode &&
                 nameSpaceID == kNameSpaceID_XUL) {
            
            
            
            
            
            PRUnichar attrbuf[128];
            nsFixedString attrValue(attrbuf, NS_ARRAY_LENGTH(attrbuf), 0);
            tmplKid->GetAttr(kNameSpaceID_None, nsGkAtoms::value, attrValue);
            if (!attrValue.IsEmpty()) {
                nsAutoString value;
                rv = SubstituteText(aChild, attrValue, value);
                if (NS_FAILED(rv)) return rv;

                nsCOMPtr<nsIContent> content;
                rv = NS_NewTextNode(getter_AddRefs(content),
                                    mRoot->NodeInfo()->NodeInfoManager());
                if (NS_FAILED(rv)) return rv;

                content->SetText(value, PR_FALSE);

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
            tmplTextNode->CloneNode(PR_FALSE, getter_AddRefs(clonedNode));
            nsCOMPtr<nsIContent> clonedContent = do_QueryInterface(clonedNode);
            if (!clonedContent) {
                NS_ERROR("failed to clone textnode");
                return NS_ERROR_FAILURE;
            }
            rv = aRealNode->AppendChildTo(clonedContent, aNotify);
            if (NS_FAILED(rv)) return rv;
        }
        else {
            
            rv = CreateElement(nameSpaceID, tag, getter_AddRefs(realKid));
            if (NS_FAILED(rv)) return rv;
        }

        if (realKid && !realKidAlreadyExisted) {
            
            
            if (aContainer && !*aContainer) {
                *aContainer = aRealNode;
                NS_ADDREF(*aContainer);

                PRUint32 indx = aRealNode->GetChildCount();

                
                
                
                *aNewIndexInContainer = indx;
            }

            
            
            
            mTemplateMap.Put(realKid, tmplKid);

            rv = CopyAttributesToElement(tmplKid, realKid, aChild, PR_FALSE);
            if (NS_FAILED(rv)) return rv;

            
            if (isGenerationElement) {
                rv = AddPersistentAttributes(tmplKid, aChild, realKid);
                if (NS_FAILED(rv)) return rv;
            }

            
            
            
            if (!aIsSelfReference && !isUnique) {
                
                
                
                
                
                rv = BuildContentFromTemplate(tmplKid, aResourceNode, realKid, PR_FALSE,
                                              PR_FALSE, aChild, PR_FALSE, aMatch,
                                              nsnull ,
                                              nsnull );
                if (NS_FAILED(rv)) return rv;

                if (isGenerationElement) {
                    
                    rv = CreateContainerContents(realKid, aChild, PR_FALSE,
                                                 PR_FALSE, PR_FALSE);
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
                                             PRBool aNotify)
{
    nsresult rv;

    
    PRUint32 numAttribs = aTemplateNode->GetAttrCount();

    for (PRUint32 attr = 0; attr < numAttribs; attr++) {
        const nsAttrName* name = aTemplateNode->GetAttrNameAt(attr);
        PRInt32 attribNameSpaceID = name->NamespaceID();
        
        
        nsCOMPtr<nsIAtom> attribName = name->LocalName();

        
        if (attribName != nsGkAtoms::id && attribName != nsGkAtoms::uri) {
            
            
            
            PRUnichar attrbuf[128];
            nsFixedString attribValue(attrbuf, NS_ARRAY_LENGTH(attrbuf), 0);
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
nsXULContentBuilder::AddPersistentAttributes(nsIContent* aTemplateNode,
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

        PRInt32 offset = persist.FindCharInSet(" ,");
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
        PRInt32 nameSpaceID;

        nsCOMPtr<nsINodeInfo> ni =
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
        rv = mDB->GetTarget(resource, property, PR_TRUE, getter_AddRefs(target));
        NS_ENSURE_SUCCESS(rv, rv);

        if (! target)
            continue;

        nsCOMPtr<nsIRDFLiteral> value = do_QueryInterface(target);
        NS_ASSERTION(value != nsnull, "unable to stomach that sort of node");
        if (! value)
            continue;

        const PRUnichar* valueStr;
        rv = value->GetValueConst(&valueStr);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aRealNode->SetAttr(nameSpaceID, tag, nsDependentString(valueStr),
                                PR_FALSE);
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
    rv = CopyAttributesToElement(aTemplateNode, aRealElement, aResult, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 count = aTemplateNode->GetChildCount();

    for (PRUint32 loop = 0; loop < count; ++loop) {
        nsIContent *tmplKid = aTemplateNode->GetChildAt(loop);

        if (! tmplKid)
            break;

        nsIContent *realKid = aRealElement->GetChildAt(loop);
        if (! realKid)
            break;

        
        
        if (tmplKid->NodeInfo()->Equals(nsGkAtoms::textnode,
                                        kNameSpaceID_XUL)) {
            PRUnichar attrbuf[128];
            nsFixedString attrValue(attrbuf, NS_ARRAY_LENGTH(attrbuf), 0);
            tmplKid->GetAttr(kNameSpaceID_None, nsGkAtoms::value, attrValue);
            if (!attrValue.IsEmpty()) {
                nsAutoString value;
                rv = SubstituteText(aResult, attrValue, value);
                if (NS_FAILED(rv)) return rv;
                realKid->SetText(value, PR_TRUE);
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
        PRInt32 pos = parent->IndexOf(aContent);

        NS_ASSERTION(pos >= 0, "parent doesn't think this child has an index");
        if (pos < 0) return NS_OK;

        
        
        
        nsresult rv = parent->RemoveChildAt(pos, PR_TRUE);
        if (NS_FAILED(rv)) return rv;
    }

    
    mContentSupportMap.Remove(aContent);

    
    mTemplateMap.Remove(aContent);

    return NS_OK;
}

nsresult
nsXULContentBuilder::CreateTemplateAndContainerContents(nsIContent* aElement,
                                                        PRBool aForceCreation)
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
                                    PR_FALSE, PR_TRUE);
        }
    }
    else if (!(mFlags & eDontRecurse)) {
        
        
        
        nsTemplateMatch *match = nsnull;
        if (mContentSupportMap.Get(aElement, &match))
            CreateContainerContents(aElement, match->mResult, aForceCreation,
                                    PR_FALSE, PR_TRUE);
    }

    PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
           ("nsXULContentBuilder::CreateTemplateAndContainerContents end"));

    return NS_OK;
}

nsresult
nsXULContentBuilder::CreateContainerContents(nsIContent* aElement,
                                             nsIXULTemplateResult* aResult,
                                             PRBool aForceCreation,
                                             PRBool aNotify,
                                             PRBool aNotifyAtEnd)
{
    if (!aForceCreation && !IsOpen(aElement))
        return NS_OK;

    
    if (aResult != mRootResult) {
        if (mFlags & eDontRecurse)
            return NS_OK;

        PRBool mayProcessChildren;
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

    PRInt32 newIndexInContainer = -1;
    nsIContent* container = nsnull;

    PRInt32 querySetCount = mQuerySets.Length();

    for (PRInt32 r = 0; r < querySetCount; r++) {
        nsTemplateQuerySet* queryset = mQuerySets[r];

        nsIAtom* tag = queryset->GetTag();
        if (tag && tag != aElement->Tag())
            continue;

        CreateContainerContentsForQuerySet(aElement, aResult, aNotify, queryset,
                                           &container, &newIndexInContainer);
    }

    if (aNotifyAtEnd && container) {
        MOZ_AUTO_DOC_UPDATE(container->GetCurrentDoc(), UPDATE_CONTENT_MODEL,
                            PR_TRUE);
        nsNodeUtils::ContentAppended(container, newIndexInContainer);
    }

    NS_IF_RELEASE(container);

    return NS_OK;
}

nsresult
nsXULContentBuilder::CreateContainerContentsForQuerySet(nsIContent* aElement,
                                                        nsIXULTemplateResult* aResult,
                                                        PRBool aNotify,
                                                        nsTemplateQuerySet* aQuerySet,
                                                        nsIContent** aContainer,
                                                        PRInt32* aNewIndexInContainer)
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

        if (!resultid)
            continue;

        nsTemplateMatch *newmatch =
            nsTemplateMatch::Create(mPool, aQuerySet->Priority(),
                                    nextresult, aElement);
        if (!newmatch)
            return NS_ERROR_OUT_OF_MEMORY;

        
        
        

        PRBool generateContent = PR_TRUE;

        nsTemplateMatch* prevmatch = nsnull;
        nsTemplateMatch* existingmatch = nsnull;
        nsTemplateMatch* removematch = nsnull;
        if (mMatchMap.Get(resultid, &existingmatch)){
            
            while (existingmatch) {
                
                
                
                PRInt32 priority = existingmatch->QuerySetPriority();
                if (priority > aQuerySet->Priority())
                    break;

                
                if (existingmatch->GetContainer() == aElement) {
                    
                    
                    if (priority == aQuerySet->Priority()) {
                        removematch = existingmatch;
                        break;
                    }

                    if (existingmatch->IsActive())
                        generateContent = PR_FALSE;
                }

                prevmatch = existingmatch;
                existingmatch = existingmatch->mNext;
            }
        }

        if (removematch) {
            
            rv = ReplaceMatch(removematch->mResult, nsnull, nsnull, aElement);
            if (NS_FAILED(rv))
                return rv;
        }

        if (generateContent) {
            
            

            PRInt16 ruleindex;
            nsTemplateRule* matchedrule = nsnull;
            rv = DetermineMatchedRule(aElement, nextresult, aQuerySet,
                                      &matchedrule, &ruleindex);
            if (NS_FAILED(rv)) {
                nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                return rv;
            }

            if (matchedrule) {
                rv = newmatch->RuleMatched(aQuerySet, matchedrule,
                                           ruleindex, nextresult);
                if (NS_FAILED(rv)) {
                    nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                    return rv;
                }

                
                nsCOMPtr<nsIContent> action = matchedrule->GetAction();
                BuildContentFromTemplate(action, aElement, aElement, PR_TRUE,
                                         mRefVariable == matchedrule->GetMemberVariable(),
                                         nextresult, aNotify, newmatch,
                                         aContainer, aNewIndexInContainer);
            }
        }

        if (prevmatch) {
            prevmatch->mNext = newmatch;
        }
        else if (!mMatchMap.Put(resultid, newmatch)) {
            nsTemplateMatch::Destroy(mPool, newmatch, PR_TRUE);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        if (removematch) {
            newmatch->mNext = removematch->mNext;
            nsTemplateMatch::Destroy(mPool, removematch, PR_TRUE);
        }
        else {
            newmatch->mNext = existingmatch;
        }
    }

    return rv;
}

nsresult
nsXULContentBuilder::EnsureElementHasGenericChild(nsIContent* parent,
                                                  PRInt32 nameSpaceID,
                                                  nsIAtom* tag,
                                                  PRBool aNotify,
                                                  nsIContent** result)
{
    nsresult rv;

    rv = nsXULContentUtils::FindChildByTag(parent, nameSpaceID, tag, result);
    if (NS_FAILED(rv))
        return rv;

    if (rv == NS_RDF_NO_VALUE) {
        
        nsCOMPtr<nsIContent> element;

        rv = CreateElement(nameSpaceID, tag, getter_AddRefs(element));
        if (NS_FAILED(rv))
            return rv;

        
        rv = parent->AppendChildTo(element, aNotify);
        if (NS_FAILED(rv))
            return rv;

        *result = element;
        NS_ADDREF(*result);
        return NS_ELEMENT_GOT_CREATED;
    }
    else {
        return NS_ELEMENT_WAS_THERE;
    }
}

PRBool
nsXULContentBuilder::IsOpen(nsIContent* aElement)
{
    
    if (!aElement->IsXUL())
        return PR_TRUE;

    
    nsIAtom *tag = aElement->Tag();
    if (tag == nsGkAtoms::menu ||
        tag == nsGkAtoms::menubutton ||
        tag == nsGkAtoms::toolbarbutton ||
        tag == nsGkAtoms::button ||
        tag == nsGkAtoms::treeitem)
        return aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::open,
                                     nsGkAtoms::_true, eCaseMatters);
    return PR_TRUE;
}

nsresult
nsXULContentBuilder::RemoveGeneratedContent(nsIContent* aElement)
{
    
    
    nsAutoTArray<nsIContent*, 8> ungenerated;
    if (ungenerated.AppendElement(aElement) == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 count;
    while (0 != (count = ungenerated.Length())) {
        
        PRUint32 last = count - 1;
        nsIContent* element = ungenerated[last];
        ungenerated.RemoveElementAt(last);

        PRUint32 i = element->GetChildCount();

        while (i-- > 0) {
            nsCOMPtr<nsIContent> child = element->GetChildAt(i);

            
            
            
            
            
            if (element->NodeInfo()->Equals(nsGkAtoms::_template,
                                            kNameSpaceID_XUL) ||
                !element->IsNodeOfType(nsINode::eELEMENT))
                continue;

            
            
            nsCOMPtr<nsIContent> tmpl;
            mTemplateMap.GetTemplateFor(child, getter_AddRefs(tmpl));

            if (! tmpl) {
                
                
                if (ungenerated.AppendElement(child) == nsnull)
                    return NS_ERROR_OUT_OF_MEMORY;
                continue;
            }

            
            element->RemoveChildAt(i, PR_TRUE);

            
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
    
    
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mRoot->GetDocument());
    if (! xuldoc)
        return NS_OK;

    nsAutoString id;
    aResult->GetId(id);

    return xuldoc->GetElementsForID(id, aElements);
}

nsresult
nsXULContentBuilder::CreateElement(PRInt32 aNameSpaceID,
                                   nsIAtom* aTag,
                                   nsIContent** aResult)
{
    nsCOMPtr<nsIDocument> doc = mRoot->GetDocument();
    NS_ASSERTION(doc != nsnull, "not initialized");
    if (! doc)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;
    nsCOMPtr<nsIContent> result;

    nsCOMPtr<nsINodeInfo> nodeInfo;
    nodeInfo = doc->NodeInfoManager()->GetNodeInfo(aTag, nsnull, aNameSpaceID);

    rv = NS_NewElement(getter_AddRefs(result), aNameSpaceID, nodeInfo,
                       PR_FALSE);
    if (NS_FAILED(rv))
        return rv;

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}

nsresult
nsXULContentBuilder::SetContainerAttrs(nsIContent *aElement,
                                       nsIXULTemplateResult* aResult,
                                       PRBool aIgnoreNonContainers,
                                       PRBool aNotify)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    PRBool iscontainer;
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
        PRBool isempty;
        aResult->GetIsEmpty(&isempty);

        const nsAString& newempty =
            (iscontainer && isempty) ? true_ : false_;

        aElement->SetAttr(kNameSpaceID_None, nsGkAtoms::empty,
                          newempty, aNotify);
    }

    return NS_OK;
}







NS_IMETHODIMP
nsXULContentBuilder::CreateContents(nsIContent* aElement, PRBool aForceCreation)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    
    
    if (!aForceCreation && !IsOpen(aElement))
        return NS_OK;

    return CreateTemplateAndContainerContents(aElement, aForceCreation);
}

NS_IMETHODIMP
nsXULContentBuilder::HasGeneratedContent(nsIRDFResource* aResource,
                                         nsIAtom* aTag,
                                         PRBool* aGenerated)
{
    *aGenerated = PR_FALSE;
    NS_ENSURE_TRUE(mRoot, NS_ERROR_NOT_INITIALIZED);
    NS_ENSURE_STATE(mRootResult);

    nsCOMPtr<nsIRDFResource> rootresource;
    nsresult rv = mRootResult->GetResource(getter_AddRefs(rootresource));
    if (NS_FAILED(rv))
        return rv;

    
    if (aResource == rootresource) {
        if (!aTag || mRoot->Tag() == aTag)
            *aGenerated = PR_TRUE;
    }
    else {
        const char* uri;
        aResource->GetValueConst(&uri);

        NS_ConvertUTF8toUTF16 refID(uri);

        
        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mRoot->GetDocument());
        if (! xuldoc)
            return NS_OK;

        nsCOMArray<nsIContent> elements;
        xuldoc->GetElementsForID(refID, elements);

        PRUint32 cnt = elements.Count();

        for (PRInt32 i = PRInt32(cnt) - 1; i >= 0; --i) {
            nsCOMPtr<nsIContent> content = elements.SafeObjectAt(i);

            do {
                nsTemplateMatch* match;
                if (content == mRoot || mContentSupportMap.Get(content, &match)) {
                    
                    if (!aTag || content->Tag() == aTag) {
                        *aGenerated = PR_TRUE;
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
        nsTemplateMatch *match = nsnull;
        if (mContentSupportMap.Get(content, &match))
            *aResult = match->mResult;
        else
            *aResult = nsnull;
    }

    NS_IF_ADDREF(*aResult);
    return NS_OK;
}






void
nsXULContentBuilder::AttributeChanged(nsIDocument* aDocument,
                                      nsIContent*  aContent,
                                      PRInt32      aNameSpaceID,
                                      nsIAtom*     aAttribute,
                                      PRInt32      aModType,
                                      PRUint32     aStateMask)
{
    
    
    
    if ((aContent->GetNameSpaceID() == kNameSpaceID_XUL) &&
        (aAttribute == nsGkAtoms::open)) {
        
        if (aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::open,
                                  nsGkAtoms::_true, eCaseMatters))
            OpenContainer(aContent);
        else
            CloseContainer(aContent);
    }

    if ((aNameSpaceID == kNameSpaceID_XUL) &&
        ((aAttribute == nsGkAtoms::sort) ||
         (aAttribute == nsGkAtoms::sortDirection) ||
         (aAttribute == nsGkAtoms::sortResource) ||
         (aAttribute == nsGkAtoms::sortResource2)))
        mSortState.initialized = PR_FALSE;

    
    nsXULTemplateBuilder::AttributeChanged(aDocument, aContent, aNameSpaceID,
                                           aAttribute, aModType, aStateMask);
}

void
nsXULContentBuilder::NodeWillBeDestroyed(const nsINode* aNode)
{
    
    mContentSupportMap.Clear();

    nsXULTemplateBuilder::NodeWillBeDestroyed(aNode);
}







PRBool
nsXULContentBuilder::GetInsertionLocations(nsIXULTemplateResult* aResult,
                                           nsCOMArray<nsIContent>** aLocations)
{
    *aLocations = nsnull;

    nsAutoString ref;
    nsresult rv = aResult->GetBindingFor(mRefVariable, ref);
    if (NS_FAILED(rv))
        return PR_FALSE;

    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mRoot->GetDocument());
    if (! xuldoc)
        return PR_FALSE;

    *aLocations = new nsCOMArray<nsIContent>;
    NS_ENSURE_TRUE(*aLocations, PR_FALSE);

    xuldoc->GetElementsForID(ref, **aLocations);
    PRUint32 count = (*aLocations)->Count();

    PRBool found = PR_FALSE;

    for (PRUint32 t = 0; t < count; t++) {
        nsCOMPtr<nsIContent> content = (*aLocations)->SafeObjectAt(t);

        nsTemplateMatch* refmatch;
        if (content == mRoot || mContentSupportMap.Get(content, &refmatch)) {
            
            
            
            
            
            nsXULElement *xulcontent = nsXULElement::FromContent(content);
            if (!xulcontent || xulcontent->GetTemplateGenerated()) {
                found = PR_TRUE;
                continue;
            }
        }

        
        (*aLocations)->ReplaceObjectAt(nsnull, t);
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
                SetContainerAttrs(content, refResult, PR_FALSE, PR_TRUE);
        }
    }

    if (aOldResult) {
        nsCOMArray<nsIContent> elements;
        rv = GetElementsForResult(aOldResult, elements);
        if (NS_FAILED(rv))
            return rv;

        PRUint32 count = elements.Count();

        for (PRInt32 e = PRInt32(count) - 1; e >= 0; --e) {
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
        return BuildContentFromTemplate(action, content, content, PR_TRUE,
                                        mRefVariable == aNewMatchRule->GetMemberVariable(),
                                        aNewMatch->mResult, PR_TRUE, aNewMatch,
                                        nsnull, nsnull);
    }

    return NS_OK;
}


nsresult
nsXULContentBuilder::SynchronizeResult(nsIXULTemplateResult* aResult)
{
    nsCOMArray<nsIContent> elements;
    GetElementsForResult(aResult, elements);

    PRUint32 cnt = elements.Count();

    for (PRInt32 i = PRInt32(cnt) - 1; i >= 0; --i) {
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

        PRBool rightBuilder = PR_FALSE;

        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(aElement->GetDocument());
        if (! xuldoc)
            return NS_OK;

        
        nsIContent* content = aElement;
        do {
            nsCOMPtr<nsIXULTemplateBuilder> builder;
            xuldoc->GetTemplateBuilderFor(content, getter_AddRefs(builder));
            if (builder) {
                if (builder == this)
                    rightBuilder = PR_TRUE;
                break;
            }

            content = content->GetParent();
        } while (content);

        if (! rightBuilder)
            return NS_OK;
    }

    CreateTemplateAndContainerContents(aElement, PR_FALSE);

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

    
    nsCOMPtr<nsIDocument> doc = mRoot->GetDocument();
    if (!doc)
        return NS_OK;

    if (mQueriesCompiled)
        Uninit(PR_FALSE);

    nsresult rv = CompileQueries();
    if (NS_FAILED(rv))
        return rv;

    if (mQuerySets.Length() == 0)
        return NS_OK;

    nsXULElement *xulcontent = nsXULElement::FromContent(mRoot);
    if (xulcontent)
        xulcontent->ClearTemplateGenerated();

    
    
    CreateTemplateAndContainerContents(mRoot, PR_FALSE);

    return NS_OK;
}



nsresult
nsXULContentBuilder::CompareResultToNode(nsIXULTemplateResult* aResult,
                                         nsIContent* aContent,
                                         PRInt32* aSortOrder)
{
    NS_ASSERTION(aSortOrder, "CompareResultToNode: null out param aSortOrder");
  
    *aSortOrder = 0;

    nsTemplateMatch *match = nsnull;
    if (!mContentSupportMap.Get(aContent, &match)) {
        *aSortOrder = mSortState.sortStaticsLast ? -1 : 1;
        return NS_OK;
    }

    if (!mQueryProcessor)
        return NS_OK;

    if (mSortState.direction == nsSortState_natural) {
        
        nsresult rv = mQueryProcessor->CompareResults(aResult, match->mResult,
                                                      nsnull, aSortOrder);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        
        
        PRInt32 length = mSortState.sortKeys.Count();
        for (PRInt32 t = 0; t < length; t++) {
            nsresult rv = mQueryProcessor->CompareResults(aResult, match->mResult,
                                                          mSortState.sortKeys[t], aSortOrder);
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
                                      PRBool aNotify)
{
    nsresult rv;

    if (!mSortState.initialized) {
        nsAutoString sort, sortDirection;
        mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::sort, sort);
        mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::sortDirection, sortDirection);
        rv = XULSortServiceImpl::InitializeSortState(mRoot, aContainer,
                                                     sort, sortDirection, &mSortState);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    
    mSortState.isContainerRDFSeq = PR_FALSE;
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

    PRBool childAdded = PR_FALSE;
    PRUint32 numChildren = aContainer->GetChildCount();

    if (mSortState.direction != nsSortState_natural ||
        (mSortState.direction == nsSortState_natural && mSortState.isContainerRDFSeq))
    {
        
        PRInt32 realNumChildren = numChildren;
        nsIContent *child = nsnull;

        
        PRInt32 staticCount = 0;

        nsAutoString staticValue;
        aContainer->GetAttr(kNameSpaceID_None, nsGkAtoms::staticHint, staticValue);
        if (!staticValue.IsEmpty())
        {
            
            PRInt32 strErr = 0;
            staticCount = staticValue.ToInteger(&strErr);
            if (strErr)
                staticCount = 0;
        } else {
            
            for (PRUint32 childLoop = 0; childLoop < numChildren; ++childLoop) {
                child = aContainer->GetChildAt(childLoop);
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
            aContainer->SetAttr(kNameSpaceID_None, nsGkAtoms::staticHint, valueStr, PR_FALSE);
        }

        if (staticCount <= 0) {
            numChildren += staticCount;
            staticCount = 0;
        } else if (staticCount > (PRInt32)numChildren) {
            staticCount = numChildren;
            numChildren -= staticCount;
        }

        
        if (numChildren > 0) {
            nsIContent *temp;
            PRInt32 direction;

            
            

            if (mSortState.lastWasFirst) {
                child = aContainer->GetChildAt(staticCount);
                temp = child;
                rv = CompareResultToNode(aResult, temp, &direction);
                if (direction < 0) {
                    aContainer->InsertChildAt(aNode, staticCount, aNotify);
                    childAdded = PR_TRUE;
                } else
                    mSortState.lastWasFirst = PR_FALSE;
            } else if (mSortState.lastWasLast) {
                child = aContainer->GetChildAt(realNumChildren - 1);
                temp = child;
                rv = CompareResultToNode(aResult, temp, &direction);
                if (direction > 0) {
                    aContainer->InsertChildAt(aNode, realNumChildren, aNotify);
                    childAdded = PR_TRUE;
                } else
                    mSortState.lastWasLast = PR_FALSE;
            }

            PRInt32 left = staticCount + 1, right = realNumChildren, x;
            while (!childAdded && right >= left) {
                x = (left + right) / 2;
                child = aContainer->GetChildAt(x - 1);
                temp = child;

                rv = CompareResultToNode(aResult, temp, &direction);
                if ((x == left && direction < 0) ||
                    (x == right && direction >= 0) ||
                    left == right)
                {
                    PRInt32 thePos = (direction > 0 ? x : x - 1);
                    aContainer->InsertChildAt(aNode, thePos, aNotify);
                    childAdded = PR_TRUE;

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
