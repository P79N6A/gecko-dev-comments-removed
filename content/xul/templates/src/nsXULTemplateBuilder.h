









































#ifndef nsXULTemplateBuilder_h__
#define nsXULTemplateBuilder_h__

#include "nsStubDocumentObserver.h"
#include "nsIScriptSecurityManager.h"
#include "nsIContent.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"
#include "nsIRDFService.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIDOMEventListener.h"

#include "nsFixedSizeAllocator.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsDataHashtable.h"
#include "nsTemplateRule.h"
#include "nsTemplateMatch.h"
#include "nsIXULTemplateQueryProcessor.h"
#include "nsCycleCollectionParticipant.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gXULTemplateLog;
#endif

class nsIXULDocument;
class nsIRDFCompositeDataSource;





class nsXULTemplateBuilder : public nsIXULTemplateBuilder,
                             public nsIDOMEventListener,
                             public nsStubDocumentObserver
{
public:
    nsXULTemplateBuilder();
    virtual ~nsXULTemplateBuilder();

    nsresult InitGlobals();

    



    virtual void Uninit(PRBool aIsFinal);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULTemplateBuilder,
                                             nsIXULTemplateBuilder)

    
    NS_DECL_NSIXULTEMPLATEBUILDER

    NS_DECL_NSIDOMEVENTLISTENER

    
    virtual void AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType);
    virtual void ContentRemoved(nsIDocument* aDocument,
                                nsIContent* aContainer,
                                nsIContent* aChild,
                                PRInt32 aIndexInContainer);
    virtual void NodeWillBeDestroyed(const nsINode* aNode);

    









    nsresult
    UpdateResult(nsIXULTemplateResult* aOldResult,
                 nsIXULTemplateResult* aNewResult,
                 nsIDOMNode* aQueryNode);

    









    nsresult
    UpdateResultInContainer(nsIXULTemplateResult* aOldResult,
                            nsIXULTemplateResult* aNewResult,
                            nsTemplateQuerySet* aQuerySet,
                            nsIRDFResource* aOldId,
                            nsIRDFResource* aNewId,
                            nsIContent* aInsertionPoint);

    nsresult
    ComputeContainmentProperties();

    static PRBool
    IsTemplateElement(nsIContent* aContent);

    virtual nsresult
    RebuildAll() = 0; 

    


    nsresult
    GetTemplateRoot(nsIContent** aResult);

    


    nsresult
    CompileQueries();

    















    nsresult
    CompileTemplate(nsIContent* aTemplate,
                    nsTemplateQuerySet* aQuerySet,
                    PRBool aIsQuerySet,
                    PRInt32* aPriority,
                    PRBool* aCanUseTemplate);

    








    nsresult 
    CompileExtendedQuery(nsIContent* aRuleElement,
                         nsIContent* aActionElement,
                         nsIAtom* aMemberVariable,
                         nsTemplateQuerySet* aQuerySet);

    


    void DetermineRDFQueryRef(nsIContent* aQueryElement, nsIAtom** tag);

    



    nsresult
    DetermineMemberVariable(nsIContent* aActionElement, nsIAtom** aMemberVariable);

    








    nsresult 
    CompileSimpleQuery(nsIContent* aRuleElement,
                       nsTemplateQuerySet* aQuerySet,
                       PRBool* aCanUseTemplate);

    





    nsresult
    CompileConditions(nsTemplateRule* aRule, nsIContent* aConditions);

    









    nsresult
    CompileWhereCondition(nsTemplateRule* aRule,
                          nsIContent* aCondition,
                          nsTemplateCondition** aCurrentCondition);

    


    nsresult
    CompileBindings(nsTemplateRule* aRule, nsIContent* aBindings);

    


    nsresult
    CompileBinding(nsTemplateRule* aRule, nsIContent* aBinding);

    


    nsresult
    AddSimpleRuleBindings(nsTemplateRule* aRule, nsIContent* aElement);

    static void
    AddBindingsFor(nsXULTemplateBuilder* aSelf,
                   const nsAString& aVariable,
                   void* aClosure);

    








    nsresult
    LoadDataSources(nsIDocument* aDoc, PRBool* shouldDelayBuilding);

    






    nsresult
    LoadDataSourceUrls(nsIDocument* aDocument,
                       const nsAString& aDataSources,
                       PRBool aIsRDFQuery,
                       PRBool* aShouldDelayBuilding);

    nsresult
    InitHTMLTemplateRoot();

    











    nsresult
    DetermineMatchedRule(nsIContent* aContainer,
                         nsIXULTemplateResult* aResult,
                         nsTemplateQuerySet* aQuerySet,
                         nsTemplateRule** aMatchedRule,
                         PRInt16 *aRuleIndex);

    
    
    
    void
    ParseAttribute(const nsAString& aAttributeValue,
                   void (*aVariableCallback)(nsXULTemplateBuilder* aThis, const nsAString&, void*),
                   void (*aTextCallback)(nsXULTemplateBuilder* aThis, const nsAString&, void*),
                   void* aClosure);

    nsresult
    SubstituteText(nsIXULTemplateResult* aMatch,
                   const nsAString& aAttributeValue,
                   nsAString& aResult);

    static void
    SubstituteTextAppendText(nsXULTemplateBuilder* aThis, const nsAString& aText, void* aClosure);

    static void
    SubstituteTextReplaceVariable(nsXULTemplateBuilder* aThis, const nsAString& aVariable, void* aClosure);    

    nsresult 
    IsSystemPrincipal(nsIPrincipal *principal, PRBool *result);

    



    nsresult GetResultResource(nsIXULTemplateResult* aResult,
                               nsIRDFResource** aResource);

protected:
    nsCOMPtr<nsISupports> mDataSource;
    nsCOMPtr<nsIRDFDataSource> mDB;
    nsCOMPtr<nsIRDFCompositeDataSource> mCompDB;

    


    nsCOMPtr<nsIContent> mRoot;

    


    nsCOMPtr<nsIXULTemplateResult> mRootResult;

    nsCOMArray<nsIXULBuilderListener> mListeners;

    


    nsCOMPtr<nsIXULTemplateQueryProcessor> mQueryProcessor;

    


    nsTArray<nsTemplateQuerySet *> mQuerySets;

    


    PRBool        mQueriesCompiled;

    


    nsCOMPtr<nsIAtom> mRefVariable;
    nsCOMPtr<nsIAtom> mMemberVariable;

    







    nsDataHashtable<nsISupportsHashKey, nsTemplateMatch*> mMatchMap;

    


    nsFixedSizeAllocator mPool;

public:

    nsFixedSizeAllocator& GetPool() { return mPool; }

protected:
    
    static nsrefcnt gRefCnt;
    static nsIRDFService*            gRDFService;
    static nsIRDFContainerUtils*     gRDFContainerUtils;
    static nsIScriptSecurityManager* gScriptSecurityManager;
    static nsIPrincipal*             gSystemPrincipal;

    enum {
        eDontTestEmpty = (1 << 0),
        eDontRecurse = (2 << 0)
    };

    PRInt32 mFlags;

    




    class ActivationEntry {
    public:
        nsIRDFResource   *mResource;
        ActivationEntry  *mPrevious;
        ActivationEntry **mLink;

        ActivationEntry(nsIRDFResource *aResource, ActivationEntry **aLink)
            : mResource(aResource),
              mPrevious(*aLink),
              mLink(aLink) { *mLink = this; }

        ~ActivationEntry() { *mLink = mPrevious; }
    };

    



    ActivationEntry *mTop;

    


    PRBool
    IsActivated(nsIRDFResource *aResource);

    








    virtual PRBool
    GetInsertionLocations(nsIXULTemplateResult* aResult,
                          nsCOMArray<nsIContent>** aLocations) = 0;

    





    virtual nsresult
    ReplaceMatch(nsIXULTemplateResult* aOldResult,
                 nsTemplateMatch* aNewMatch,
                 nsTemplateRule* aNewMatchRule,
                 void *aContext) = 0;

    







    virtual nsresult
    SynchronizeResult(nsIXULTemplateResult* aResult) = 0;

    virtual void Traverse(nsCycleCollectionTraversalCallback &cb) const
    {
    }
};

#endif 
