



































#include "nsXULTemplateQueryProcessorRDF.h"
#include "nsXULTemplateResultRDF.h"
#include "nsRDFBinding.h"

#ifdef DEBUG
#include "nsXULContentUtils.h"
#endif

RDFBindingSet::~RDFBindingSet()
{
    while (mFirst) {
        RDFBinding* doomed = mFirst;
        mFirst = mFirst->mNext;
        delete doomed;
    }

    MOZ_COUNT_DTOR(RDFBindingSet);
}

nsresult
RDFBindingSet::AddBinding(nsIAtom* aVar, nsIAtom* aRef, nsIRDFResource* aPredicate)
{
    RDFBinding* newbinding = new RDFBinding(aRef, aPredicate, aVar);
    if (! newbinding)
        return NS_ERROR_OUT_OF_MEMORY;

    if (mFirst) {
        RDFBinding* binding = mFirst;

        while (binding) { 
            
            if (binding->mSubjectVariable == aVar)
                newbinding->mHasDependency = PR_TRUE;

            
            
            if (binding->mTargetVariable == aVar) {
                delete newbinding;
                return NS_OK;
            }

            
            if (! binding->mNext) {
                binding->mNext = newbinding;
                break;
            }

            binding = binding->mNext;
        }
    }
    else {
        mFirst = newbinding;
    }

    mCount++;

    return NS_OK;
}

PRBool
RDFBindingSet::SyncAssignments(nsIRDFResource* aSubject,
                               nsIRDFResource* aPredicate,
                               nsIRDFNode* aTarget,
                               nsIAtom* aMemberVariable,
                               nsXULTemplateResultRDF* aResult,
                               nsBindingValues& aBindingValues)
{
    NS_ASSERTION(aBindingValues.GetBindingSet() == this,
                 "nsBindingValues not for this RDFBindingSet");
    NS_PRECONDITION(aResult, "Must have result");

    PRBool needSync = PR_FALSE;
    nsCOMPtr<nsIRDFNode>* valuesArray = aBindingValues.ValuesArray();
    if (!valuesArray)
        return PR_FALSE;

    RDFBinding* binding = mFirst;
    PRInt32 count = 0;

    
    nsCOMPtr<nsIRDFNode> subjectnode = do_QueryInterface(aSubject);

    
    
    nsCOMPtr<nsIRDFNode> value;
    while (binding) {
        if (aPredicate == binding->mPredicate) {
            
            if (binding->mSubjectVariable == aMemberVariable) {
                valuesArray[count] = aTarget;
                needSync = PR_TRUE;
            }
            else {
                aResult->GetAssignment(binding->mSubjectVariable, getter_AddRefs(value));
                if (value == subjectnode) {
                    valuesArray[count] = aTarget;
                    needSync = PR_TRUE;
                }
            }
        }

        binding = binding->mNext;
        count++;
    }

    return needSync;
}

void
RDFBindingSet::AddDependencies(nsIRDFResource* aSubject,
                               nsXULTemplateResultRDF* aResult)
{
    NS_PRECONDITION(aResult, "Must have result");

    
    

    nsXULTemplateQueryProcessorRDF* processor = aResult->GetProcessor();
    if (! processor)
        return;

    nsCOMPtr<nsIRDFNode> value;

    RDFBinding* binding = mFirst;
    while (binding) {
        aResult->GetAssignment(binding->mSubjectVariable, getter_AddRefs(value));

        nsCOMPtr<nsIRDFResource> valueres = do_QueryInterface(value);
        if (valueres)
            processor->AddBindingDependency(aResult, valueres);

        binding = binding->mNext;
    }
}

void
RDFBindingSet::RemoveDependencies(nsIRDFResource* aSubject,
                                  nsXULTemplateResultRDF* aResult)
{
    NS_PRECONDITION(aResult, "Must have result");

    
    

    nsXULTemplateQueryProcessorRDF* processor = aResult->GetProcessor();
    if (! processor)
        return;

    nsCOMPtr<nsIRDFNode> value;

    RDFBinding* binding = mFirst;
    while (binding) {
        aResult->GetAssignment(binding->mSubjectVariable, getter_AddRefs(value));

        nsCOMPtr<nsIRDFResource> valueres = do_QueryInterface(value);
        if (valueres)
            processor->RemoveBindingDependency(aResult, valueres);

        binding = binding->mNext;
    }
}

PRInt32
RDFBindingSet::LookupTargetIndex(nsIAtom* aTargetVariable, RDFBinding** aBinding)
{
    PRInt32 idx = 0;
    RDFBinding* binding = mFirst;

    while (binding) {
        if (binding->mTargetVariable == aTargetVariable) {
            *aBinding = binding;
            return idx;
        }
        idx++;
        binding = binding->mNext;
    }

    return -1;
}

nsBindingValues::~nsBindingValues()
{
    ClearBindingSet();
    MOZ_COUNT_DTOR(nsBindingValues);
}

void
nsBindingValues::ClearBindingSet()
{
    if (mBindings && mValues) {
        delete [] mValues;
        mValues = nsnull;
    }

    mBindings = nsnull;
}

nsresult
nsBindingValues::SetBindingSet(RDFBindingSet* aBindings)
{
    ClearBindingSet();

    PRInt32 count = aBindings->Count();
    if (count) {
        mValues = new nsCOMPtr<nsIRDFNode>[count];
        if (!mValues)
            return NS_ERROR_OUT_OF_MEMORY;

        mBindings = aBindings;
    }
    else {
        mValues = nsnull;
    }

    return NS_OK;
}

void
nsBindingValues::GetAssignmentFor(nsXULTemplateResultRDF* aResult,
                                  nsIAtom* aVar,
                                  nsIRDFNode** aValue)
{
    *aValue = nsnull;

    
    
    

    if (mBindings && mValues) {
        RDFBinding* binding;
        PRInt32 idx = mBindings->LookupTargetIndex(aVar, &binding);
        if (idx >= 0) {
            *aValue = mValues[idx];
            if (*aValue) {
                NS_ADDREF(*aValue);
            }
            else {
                nsXULTemplateQueryProcessorRDF* processor = aResult->GetProcessor();
                if (! processor)
                    return;

                nsIRDFDataSource* ds = processor->GetDataSource();
                if (! ds)
                    return;

                nsCOMPtr<nsIRDFNode> subjectValue;
                aResult->GetAssignment(binding->mSubjectVariable,
                                       getter_AddRefs(subjectValue));

                NS_ASSERTION(subjectValue, "Value of subject is not set");

                if (subjectValue) {
                    nsCOMPtr<nsIRDFResource> subject = do_QueryInterface(subjectValue);

                    ds->GetTarget(subject, binding->mPredicate, PR_TRUE, aValue);
                    if (*aValue)
                        mValues[idx] = *aValue;
                }
            }
        }
    }
}

void
nsBindingValues::RemoveDependencies(nsIRDFResource* aSubject,
                                    nsXULTemplateResultRDF* aResult)
{
    if (mBindings)
        mBindings->RemoveDependencies(aSubject, aResult);
}
