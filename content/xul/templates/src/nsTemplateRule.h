





































#ifndef nsTemplateRule_h__
#define nsTemplateRule_h__

#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFResource.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsVoidArray.h"
#include "nsString.h"
#include "nsIXULTemplateRuleFilter.h"
#include "nsCycleCollectionParticipant.h"

class nsIXULTemplateQueryProcessor;
class nsTemplateQuerySet;

class nsTemplateCondition
{
public:
    
    
    
    
    
    
    
    enum ConditionRelation {
        eUnknown,
        eEquals,
        eLess,
        eGreater,
        eBefore,
        eAfter,
        eStartswith,
        eEndswith,
        eContains
    };

    nsTemplateCondition(nsIAtom* aSourceVariable,
                        const nsAString& aRelation,
                        nsIAtom* aTargetVariable,
                        PRBool mIgnoreCase,
                        PRBool mNegate);

    nsTemplateCondition(nsIAtom* aSourceVariable,
                        const nsAString& aRelation,
                        const nsAString& aTargets,
                        PRBool mIgnoreCase,
                        PRBool mNegate,
                        PRBool aIsMultiple);

    nsTemplateCondition(const nsAString& aSource,
                        const nsAString& aRelation,
                        nsIAtom* aTargetVariable,
                        PRBool mIgnoreCase,
                        PRBool mNegate);

    ~nsTemplateCondition() { MOZ_COUNT_DTOR(nsTemplateCondition); }

    nsTemplateCondition* GetNext() { return mNext; }
    void SetNext(nsTemplateCondition* aNext) { mNext = aNext; }

    void SetRelation(const nsAString& aRelation);

    PRBool
    CheckMatch(nsIXULTemplateResult* aResult);

    PRBool
    CheckMatchStrings(const nsAString& aLeftString,
                      const nsAString& aRightString);
protected:

    nsCOMPtr<nsIAtom>   mSourceVariable;
    nsString            mSource;
    ConditionRelation   mRelation;
    nsCOMPtr<nsIAtom>   mTargetVariable;
    nsStringArray       mTargetList;
    PRPackedBool        mIgnoreCase;
    PRPackedBool        mNegate;

   nsTemplateCondition* mNext;
};

















class nsTemplateRule
{
public:
    nsTemplateRule(nsIContent* aRuleNode,
                   nsIContent* aAction,
                   nsTemplateQuerySet* aQuerySet);

    ~nsTemplateRule();

    







    nsresult GetAction(nsIContent** aAction) const;

    




    nsresult GetRuleNode(nsIDOMNode** aResult) const;

    void SetVars(nsIAtom* aRefVariable, nsIAtom* aMemberVariable)
    {
        mRefVariable = aRefVariable;
        mMemberVariable = aMemberVariable;
    }

    void SetRuleFilter(nsIXULTemplateRuleFilter* aRuleFilter)
    {
        mRuleFilter = aRuleFilter;
    }

    nsIAtom* GetTag() { return mTag; }
    void SetTag(nsIAtom* aTag) { mTag = aTag; }

    nsIAtom* GetMemberVariable() { return mMemberVariable; }

    



    void SetCondition(nsTemplateCondition* aConditions);

    





    PRBool
    CheckMatch(nsIXULTemplateResult* aResult) const;

    


    PRBool
    HasBinding(nsIAtom* aSourceVariable,
               nsAString& aExpr,
               nsIAtom* aTargetVariable) const;

    












    nsresult AddBinding(nsIAtom* aSourceVariable,
                        nsAString& aExpr,
                        nsIAtom* aTargetVariable);

    



    nsresult
    AddBindingsToQueryProcessor(nsIXULTemplateQueryProcessor* aProcessor);

    void Traverse(nsCycleCollectionTraversalCallback &cb) const
    {
        cb.NoteXPCOMChild(mRuleNode);
        cb.NoteXPCOMChild(mAction);
    }

protected:

    struct Binding {
        nsCOMPtr<nsIAtom>        mSourceVariable;
        nsCOMPtr<nsIAtom>        mTargetVariable;
        nsString                 mExpr;
        Binding*                 mNext;
        Binding*                 mParent;
    };

    
    nsTemplateQuerySet* mQuerySet;

    
    nsCOMPtr<nsIDOMNode> mRuleNode;

    
    
    nsCOMPtr<nsIContent> mAction;

    
    nsCOMPtr<nsIXULTemplateRuleFilter> mRuleFilter;

    
    
    nsCOMPtr<nsIAtom> mTag;

    
    Binding* mBindings;

    nsCOMPtr<nsIAtom> mRefVariable;
    nsCOMPtr<nsIAtom> mMemberVariable;

    nsTemplateCondition* mConditions; 
};










class nsTemplateQuerySet
{
protected:
    nsVoidArray mRules; 

    
    
    
    PRInt32 mPriority;

public:

    
    nsCOMPtr<nsIContent> mQueryNode;

    
    
    nsCOMPtr<nsISupports> mCompiledQuery;

    
    
    nsCOMPtr<nsIAtom> mTag;

    nsTemplateQuerySet(PRInt32 aPriority)
        : mPriority(aPriority)
    {
        MOZ_COUNT_CTOR(nsTemplateQuerySet);
    }

    ~nsTemplateQuerySet()
    {
        MOZ_COUNT_DTOR(nsTemplateQuerySet);
        Clear();
    }

    PRInt32 Priority() const
    {
        return mPriority;
    }

    nsIAtom* GetTag() { return mTag; }
    void SetTag(nsIAtom* aTag) { mTag = aTag; }

    nsresult AddRule(nsTemplateRule *aChild)
    {
        
        
        if (mRules.Count() == PR_INT16_MAX)
            return NS_ERROR_FAILURE;

        if (!mRules.AppendElement(aChild))
            return NS_ERROR_OUT_OF_MEMORY;
        return NS_OK;
    }

    PRInt16 RuleCount() const
    {
        return mRules.Count();
    }

    nsTemplateRule* GetRuleAt(PRInt16 aIndex)
    {
        return static_cast<nsTemplateRule*>(mRules[aIndex]);
    }

    void Clear()
    {
        for (PRInt32 r = mRules.Count() - 1; r >= 0; r--) {
            nsTemplateRule* rule = static_cast<nsTemplateRule*>(mRules[r]);
            delete rule;
        }
        mRules.Clear();
    }
};

#endif
