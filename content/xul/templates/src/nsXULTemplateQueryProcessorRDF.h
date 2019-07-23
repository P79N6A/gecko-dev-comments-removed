










































#ifndef nsXULTemplateQueryProcessorRDF_h__
#define nsXULTemplateQueryProcessorRDF_h__

#include "nsIContent.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"
#include "nsIRDFService.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIXULTemplateQueryProcessor.h"
#include "nsICollation.h"
#include "nsCollationCID.h"

#include "nsFixedSizeAllocator.h"
#include "nsResourceSet.h"
#include "nsRuleNetwork.h"
#include "nsRDFQuery.h"
#include "nsRDFBinding.h"
#include "nsXULTemplateResultSetRDF.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"
#include "nsIArray.h"
#include "nsString.h"
#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsCycleCollectionParticipant.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gXULTemplateLog;
#endif

class nsIRDFCompositeDataSource;
class nsXULTemplateResultRDF;




class nsXULTemplateQueryProcessorRDF : public nsIXULTemplateQueryProcessor,
                                       public nsIRDFObserver
{
public:

    nsXULTemplateQueryProcessorRDF();

    ~nsXULTemplateQueryProcessorRDF();

    nsresult InitGlobals();

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULTemplateQueryProcessorRDF,
                                             nsIXULTemplateQueryProcessor)

    
    NS_DECL_NSIXULTEMPLATEQUERYPROCESSOR
   
    
    NS_DECL_NSIRDFOBSERVER

    



    nsresult
    Propagate(nsIRDFResource* aSource,
              nsIRDFResource* aProperty,
              nsIRDFNode* aTarget);

    



    nsresult
    Retract(nsIRDFResource* aSource,
            nsIRDFResource* aProperty,
            nsIRDFNode* aTarget);

    


    nsresult
    SynchronizeAll(nsIRDFResource* aSource,
                   nsIRDFResource* aProperty,
                   nsIRDFNode* aOldTarget,
                   nsIRDFNode* aNewTarget);

    


    nsresult
    CheckContainer(nsIRDFResource* aTargetResource,
                   PRBool* aIsContainer);

    


    nsresult
    CheckEmpty(nsIRDFResource* aTargetResource,
               PRBool* aIsEmpty);

    


    nsresult
    CheckIsSeparator(nsIRDFResource* aResource, PRBool* aIsSeparator);

    




    nsresult
    ComputeContainmentProperties(nsIDOMNode* aRootNode);

    




    nsresult
    CompileExtendedQuery(nsRDFQuery* aQuery,
                         nsIContent* aConditions,
                         TestNode** aLastNode);

    




    virtual nsresult
    CompileQueryChild(nsIAtom* aTag,
                      nsRDFQuery* aQuery,
                      nsIContent* aConditions,
                      TestNode* aParentNode,
                      TestNode** aResult);

    



    nsresult ParseLiteral(const nsString& aParseType, 
                          const nsString& aValue,
                          nsIRDFNode** aResult);

    




    nsresult
    CompileTripleCondition(nsRDFQuery* aQuery,
                           nsIContent* aCondition,
                           TestNode* aParentNode,
                           TestNode** aResult);

    




    nsresult
    CompileMemberCondition(nsRDFQuery* aQuery,
                           nsIContent* aCondition,
                           TestNode* aParentNode,
                           TestNode** aResult);

    





    nsresult
    AddDefaultSimpleRules(nsRDFQuery* aQuery,
                          TestNode** aChildNode);

    





    nsresult
    CompileSimpleQuery(nsRDFQuery* aQuery,
                      nsIContent* aQueryElement,
                      TestNode** aLastNode);

    RDFBindingSet*
    GetBindingsForRule(nsIDOMNode* aRule);

    




    nsresult
    AddBindingDependency(nsXULTemplateResultRDF* aResult,
                         nsIRDFResource* aResource);

    


    nsresult
    RemoveBindingDependency(nsXULTemplateResultRDF* aResult,
                            nsIRDFResource* aResource);

    







    nsresult
    AddMemoryElements(const Instantiation& aInst,
                      nsXULTemplateResultRDF* aResult);

    



    nsresult
    RemoveMemoryElements(const Instantiation& aInst,
                         nsXULTemplateResultRDF* aResult);

    



    void RetractElement(const MemoryElement& aMemoryElement);

    


    PRInt32
    GetContainerIndexOf(nsIXULTemplateResult* aResult);

    




    nsresult
    GetSortValue(nsIXULTemplateResult* aResult,
                 nsIRDFResource* aPredicate,
                 nsIRDFResource* aSortPredicate,
                 nsISupports** aResultNode);

    nsIRDFDataSource* GetDataSource() { return mDB; }

    nsIXULTemplateBuilder* GetBuilder() { return mBuilder; }

    nsResourceSet& ContainmentProperties() { return mContainmentProperties; }

#ifdef PR_LOGGING
    nsresult
    Log(const char* aOperation,
        nsIRDFResource* aSource,
        nsIRDFResource* aProperty,
        nsIRDFNode* aTarget);

#define LOG(_op, _src, _prop, _targ) \
    Log(_op, _src, _prop, _targ)

#else
#define LOG(_op, _src, _prop, _targ)
#endif

protected:
    
    
    nsCOMPtr<nsIRDFDataSource> mDB;

    
    nsIXULTemplateBuilder* mBuilder;

    
    PRBool mQueryProcessorRDFInited;

    
    
    
    PRBool mGenerationStarted;

    
    PRInt32 mUpdateBatchNest;

    
    
    nsResourceSet mContainmentProperties;

    
    TestNode* mSimpleRuleMemberTest;

    
    nsCOMPtr<nsIAtom> mRefVariable;

    
    nsCOMPtr<nsIXULTemplateResult> mLastRef;

    






    nsClassHashtable<nsISupportsHashKey,
                     nsCOMArray<nsXULTemplateResultRDF> > mBindingDependencies;

    




    nsClassHashtable<nsUint32HashKey,
                     nsCOMArray<nsXULTemplateResultRDF> > mMemoryElementToResultMap;

    
    
    
    nsRefPtrHashtable<nsISupportsHashKey, RDFBindingSet> mRuleToBindingsMap;

    


    nsCOMArray<nsITemplateRDFQuery> mQueries;

    




    ReteNodeSet mRDFTests;

    


    ReteNodeSet mAllTests;

    
    static nsrefcnt gRefCnt;

public:
    static nsIRDFService*            gRDFService;
    static nsIRDFContainerUtils*     gRDFContainerUtils;
    static nsIRDFResource*           kNC_BookmarkSeparator;
    static nsIRDFResource*           kRDF_type;
};

#endif 
