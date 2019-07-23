



































#ifndef nsRDFBinding_h__
#define nsRDFBinding_h__

#include "nsAutoPtr.h"
#include "nsIAtom.h"
#include "nsIRDFResource.h"

class nsXULTemplateResultRDF;
class nsBindingValues;








class RDFBinding {

public:

    nsCOMPtr<nsIAtom>        mSubjectVariable;
    nsCOMPtr<nsIRDFResource> mPredicate;
    nsCOMPtr<nsIAtom>        mTargetVariable;

    
    
    PRBool                   mHasDependency;

    RDFBinding*              mNext;

private:

    friend class RDFBindingSet;

    RDFBinding(nsIAtom* aSubjectVariable,
               nsIRDFResource* aPredicate,
               nsIAtom* aTargetVariable)
      : mSubjectVariable(aSubjectVariable),
        mPredicate(aPredicate),
        mTargetVariable(aTargetVariable),
        mHasDependency(PR_FALSE),
        mNext(nsnull)
    {
        MOZ_COUNT_CTOR(RDFBinding);
    }

    ~RDFBinding()
    {
        MOZ_COUNT_DTOR(RDFBinding);
    }
};





class RDFBindingSet
{
protected:

    
    PRInt32 mRefCnt;

    
    PRInt32 mCount;

    
    RDFBinding* mFirst;

public:

    RDFBindingSet()
        : mRefCnt(0),
          mCount(0),
          mFirst(nsnull)
    {
        MOZ_COUNT_CTOR(RDFBindingSet);
    }

    ~RDFBindingSet();

    PRInt32 AddRef() { 
        mRefCnt++; 
        NS_LOG_ADDREF(this, mRefCnt, "RDFBindingSet", sizeof(*this));
        return mRefCnt;
    }

    PRInt32 Release()
    {
        PRInt32 refcnt = --mRefCnt;
        NS_LOG_RELEASE(this, refcnt, "RDFBindingSet");
        if (refcnt == 0) delete this;
        return refcnt;
    }

    PRInt32 Count() const { return mCount; }

    


    nsresult
    AddBinding(nsIAtom* aVar, nsIAtom* aRef, nsIRDFResource* aPredicate);

    















    PRBool
    SyncAssignments(nsIRDFResource* aSubject,
                    nsIRDFResource* aPredicate,
                    nsIRDFNode* aTarget,
                    nsIAtom* aMemberVariable,
                    nsXULTemplateResultRDF* aResult,
                    nsBindingValues& aBindingValues);

    






    void
    AddDependencies(nsIRDFResource* aSubject,
                    nsXULTemplateResultRDF* aResult);

    


    void
    RemoveDependencies(nsIRDFResource* aSubject,
                       nsXULTemplateResultRDF* aResult);

    





    PRInt32
    LookupTargetIndex(nsIAtom* aTargetVariable, RDFBinding** aBinding);
};












class nsBindingValues
{
protected:

    
    nsRefPtr<RDFBindingSet> mBindings;

    





    nsCOMPtr<nsIRDFNode>* mValues;

public:

    nsBindingValues()
      : mBindings(nsnull),
        mValues(nsnull)
    {
        MOZ_COUNT_CTOR(nsBindingValues);
    }

    ~nsBindingValues();


    



    void ClearBindingSet();

    RDFBindingSet* GetBindingSet() { return mBindings; }

    



    nsresult SetBindingSet(RDFBindingSet* aBindings);

    nsCOMPtr<nsIRDFNode>* ValuesArray() { return mValues; }

    


    void
    GetAssignmentFor(nsXULTemplateResultRDF* aResult,
                     nsIAtom* aVar,
                     nsIRDFNode** aValue);

    


    void
    RemoveDependencies(nsIRDFResource* aSubject,
                       nsXULTemplateResultRDF* aResult);
};

#endif 
