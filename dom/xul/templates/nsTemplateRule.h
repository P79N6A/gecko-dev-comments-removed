




#ifndef nsTemplateRule_h__
#define nsTemplateRule_h__

#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFResource.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsTArray.h"
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
                        bool mIgnoreCase,
                        bool mNegate);

    nsTemplateCondition(nsIAtom* aSourceVariable,
                        const nsAString& aRelation,
                        const nsAString& aTargets,
                        bool mIgnoreCase,
                        bool mNegate,
                        bool aIsMultiple);

    nsTemplateCondition(const nsAString& aSource,
                        const nsAString& aRelation,
                        nsIAtom* aTargetVariable,
                        bool mIgnoreCase,
                        bool mNegate);

    ~nsTemplateCondition() { MOZ_COUNT_DTOR(nsTemplateCondition); }

    nsTemplateCondition* GetNext() { return mNext; }
    void SetNext(nsTemplateCondition* aNext) { mNext = aNext; }

    void SetRelation(const nsAString& aRelation);

    bool
    CheckMatch(nsIXULTemplateResult* aResult);

    bool
    CheckMatchStrings(const nsAString& aLeftString,
                      const nsAString& aRightString);
protected:

    nsCOMPtr<nsIAtom>   mSourceVariable;
    nsString            mSource;
    ConditionRelation   mRelation;
    nsCOMPtr<nsIAtom>   mTargetVariable;
    nsTArray<nsString>  mTargetList;
    bool                mIgnoreCase;
    bool                mNegate;

   nsTemplateCondition* mNext;
};

















class nsTemplateRule
{
public:
    nsTemplateRule(nsIContent* aRuleNode,
                   nsIContent* aAction,
                   nsTemplateQuerySet* aQuerySet);
    




    nsTemplateRule(const nsTemplateRule& aOtherRule);

    ~nsTemplateRule();

    




    nsIContent* GetAction() const { return mAction; }

    




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

    





    bool
    CheckMatch(nsIXULTemplateResult* aResult) const;

    


    bool
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
    nsTArray<nsTemplateRule> mRules;

    
    
    
    int32_t mPriority;

public:

    
    nsCOMPtr<nsIContent> mQueryNode;

    
    
    nsCOMPtr<nsISupports> mCompiledQuery;

    
    
    nsCOMPtr<nsIAtom> mTag;

    explicit nsTemplateQuerySet(int32_t aPriority)
        : mPriority(aPriority)
    {
        MOZ_COUNT_CTOR(nsTemplateQuerySet);
    }

    ~nsTemplateQuerySet()
    {
        MOZ_COUNT_DTOR(nsTemplateQuerySet);
    }

    int32_t Priority() const
    {
        return mPriority;
    }

    nsIAtom* GetTag() { return mTag; }
    void SetTag(nsIAtom* aTag) { mTag = aTag; }

    nsTemplateRule* NewRule(nsIContent* aRuleNode,
                            nsIContent* aAction,
                            nsTemplateQuerySet* aQuerySet)
    {
        
        
        if (mRules.Length() == INT16_MAX)
            return nullptr;

        return mRules.AppendElement(nsTemplateRule(aRuleNode, aAction,
                                    aQuerySet));
    }
    
    void RemoveRule(nsTemplateRule *aRule)
    {
        mRules.RemoveElementAt(aRule - mRules.Elements());
    }

    int16_t RuleCount() const
    {
        return mRules.Length();
    }

    nsTemplateRule* GetRuleAt(int16_t aIndex)
    {
        if (uint32_t(aIndex) < mRules.Length()) {
            return &mRules[aIndex];
        }
        return nullptr;
    }

    void Clear()
    {
        mRules.Clear();
    }
};

#endif
