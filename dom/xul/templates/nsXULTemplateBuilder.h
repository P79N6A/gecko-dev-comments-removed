




#ifndef nsXULTemplateBuilder_h__
#define nsXULTemplateBuilder_h__

#include "nsStubDocumentObserver.h"
#include "nsIScriptSecurityManager.h"
#include "nsIObserver.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"
#include "nsIRDFService.h"
#include "nsIXULTemplateBuilder.h"

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

class nsIContent;
class nsIObserverService;
class nsIRDFCompositeDataSource;





class nsXULTemplateBuilder : public nsIXULTemplateBuilder,
                             public nsIObserver,
                             public nsStubDocumentObserver
{
    void CleanUp(bool aIsFinal);

public:
    nsXULTemplateBuilder();

    nsresult InitGlobals();

    



    virtual void Uninit(bool aIsFinal);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULTemplateBuilder,
                                             nsIXULTemplateBuilder)

    
    NS_DECL_NSIXULTEMPLATEBUILDER

    
    NS_DECL_NSIOBSERVER

    
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

    









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

    static bool
    IsTemplateElement(nsIContent* aContent);

    virtual nsresult
    RebuildAll() = 0; 

    void RunnableRebuild() { Rebuild(); }
    void RunnableLoadAndRebuild() {
      Uninit(false);  

      nsCOMPtr<nsIDocument> doc = mRoot ? mRoot->GetComposedDoc() : nullptr;
      if (doc) {
        bool shouldDelay;
        LoadDataSources(doc, &shouldDelay);
        if (!shouldDelay) {
          Rebuild();
        }
      }
    }

    
    
    void UninitFalse() { Uninit(false); mRoot = nullptr; }
    void UninitTrue() { Uninit(true); mRoot = nullptr; }

    


    nsresult
    GetTemplateRoot(nsIContent** aResult);

    


    nsresult
    CompileQueries();

    















    nsresult
    CompileTemplate(nsIContent* aTemplate,
                    nsTemplateQuerySet* aQuerySet,
                    bool aIsQuerySet,
                    int32_t* aPriority,
                    bool* aCanUseTemplate);

    








    nsresult 
    CompileExtendedQuery(nsIContent* aRuleElement,
                         nsIContent* aActionElement,
                         nsIAtom* aMemberVariable,
                         nsTemplateQuerySet* aQuerySet);

    


    void DetermineRDFQueryRef(nsIContent* aQueryElement, nsIAtom** tag);

    



    already_AddRefed<nsIAtom> DetermineMemberVariable(nsIContent* aElement);

    








    nsresult 
    CompileSimpleQuery(nsIContent* aRuleElement,
                       nsTemplateQuerySet* aQuerySet,
                       bool* aCanUseTemplate);

    





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
    LoadDataSources(nsIDocument* aDoc, bool* shouldDelayBuilding);

    






    nsresult
    LoadDataSourceUrls(nsIDocument* aDocument,
                       const nsAString& aDataSources,
                       bool aIsRDFQuery,
                       bool* aShouldDelayBuilding);

    nsresult
    InitHTMLTemplateRoot();

    











    nsresult
    DetermineMatchedRule(nsIContent* aContainer,
                         nsIXULTemplateResult* aResult,
                         nsTemplateQuerySet* aQuerySet,
                         nsTemplateRule** aMatchedRule,
                         int16_t *aRuleIndex);

    
    
    
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
    IsSystemPrincipal(nsIPrincipal *principal, bool *result);

    



    nsresult GetResultResource(nsIXULTemplateResult* aResult,
                               nsIRDFResource** aResource);

protected:
    virtual ~nsXULTemplateBuilder();

    nsCOMPtr<nsISupports> mDataSource;
    nsCOMPtr<nsIRDFDataSource> mDB;
    nsCOMPtr<nsIRDFCompositeDataSource> mCompDB;

    


    nsCOMPtr<nsIContent> mRoot;

    


    nsCOMPtr<nsIXULTemplateResult> mRootResult;

    nsCOMArray<nsIXULBuilderListener> mListeners;

    


    nsCOMPtr<nsIXULTemplateQueryProcessor> mQueryProcessor;

    


    nsTArray<nsTemplateQuerySet *> mQuerySets;

    


    bool          mQueriesCompiled;

    


    nsCOMPtr<nsIAtom> mRefVariable;
    nsCOMPtr<nsIAtom> mMemberVariable;

    







    nsDataHashtable<nsISupportsHashKey, nsTemplateMatch*> mMatchMap;

    
    static nsrefcnt gRefCnt;
    static nsIRDFService*            gRDFService;
    static nsIRDFContainerUtils*     gRDFContainerUtils;
    static nsIScriptSecurityManager* gScriptSecurityManager;
    static nsIPrincipal*             gSystemPrincipal;
    static nsIObserverService*       gObserverService;

    enum {
        eDontTestEmpty = (1 << 0),
        eDontRecurse = (1 << 1),
        eLoggingEnabled = (1 << 2)
    };

    int32_t mFlags;

    




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

    


    bool
    IsActivated(nsIRDFResource *aResource);

    








    virtual bool
    GetInsertionLocations(nsIXULTemplateResult* aResult,
                          nsCOMArray<nsIContent>** aLocations) = 0;

    





    virtual nsresult
    ReplaceMatch(nsIXULTemplateResult* aOldResult,
                 nsTemplateMatch* aNewMatch,
                 nsTemplateRule* aNewMatchRule,
                 void *aContext) = 0;

    







    virtual nsresult
    SynchronizeResult(nsIXULTemplateResult* aResult) = 0;

    






    void
    OutputMatchToLog(nsIRDFResource* aId,
                     nsTemplateMatch* aMatch,
                     bool aIsNew);

    virtual void Traverse(nsCycleCollectionTraversalCallback &cb) const
    {
    }

    





    void StartObserving(nsIDocument* aDocument);

    



    void StopObserving();

    


    nsIDocument* mObservedDocument;
};

#endif 
