




#ifndef nsRDFBinding_h__
#define nsRDFBinding_h__

#include "nsAutoPtr.h"
#include "nsIAtom.h"
#include "nsIRDFResource.h"
#include "nsISupportsImpl.h"

class nsXULTemplateResultRDF;
class nsBindingValues;








class RDFBinding {

public:

    nsCOMPtr<nsIAtom>        mSubjectVariable;
    nsCOMPtr<nsIRDFResource> mPredicate;
    nsCOMPtr<nsIAtom>        mTargetVariable;

    
    
    bool                     mHasDependency;

    RDFBinding*              mNext;

private:

    friend class RDFBindingSet;

    RDFBinding(nsIAtom* aSubjectVariable,
               nsIRDFResource* aPredicate,
               nsIAtom* aTargetVariable)
      : mSubjectVariable(aSubjectVariable),
        mPredicate(aPredicate),
        mTargetVariable(aTargetVariable),
        mHasDependency(false),
        mNext(nullptr)
    {
        MOZ_COUNT_CTOR(RDFBinding);
    }

    ~RDFBinding()
    {
        MOZ_COUNT_DTOR(RDFBinding);
    }
};





class RDFBindingSet MOZ_FINAL
{
private:
    
    ~RDFBindingSet();

    
    int32_t mCount;

    
    RDFBinding* mFirst;

public:

    RDFBindingSet()
        : mCount(0),
          mFirst(nullptr)
    {
        MOZ_COUNT_CTOR(RDFBindingSet);
    }

    NS_INLINE_DECL_REFCOUNTING(RDFBindingSet)

    int32_t Count() const { return mCount; }

    


    nsresult
    AddBinding(nsIAtom* aVar, nsIAtom* aRef, nsIRDFResource* aPredicate);

    















    bool
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

    





    int32_t
    LookupTargetIndex(nsIAtom* aTargetVariable, RDFBinding** aBinding);
};












class nsBindingValues
{
protected:

    
    nsRefPtr<RDFBindingSet> mBindings;

    





    nsCOMPtr<nsIRDFNode>* mValues;

public:

    nsBindingValues()
      : mBindings(nullptr),
        mValues(nullptr)
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
