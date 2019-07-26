




#include "nsTemplateRule.h"
#include "nsTemplateMatch.h"
#include "nsXULContentUtils.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsICollation.h"

nsTemplateCondition::nsTemplateCondition(nsIAtom* aSourceVariable,
                                         const nsAString& aRelation,
                                         nsIAtom* aTargetVariable,
                                         bool aIgnoreCase,
                                         bool aNegate)
    : mSourceVariable(aSourceVariable),
      mTargetVariable(aTargetVariable),
      mIgnoreCase(aIgnoreCase),
      mNegate(aNegate),
      mNext(nullptr)
{
    SetRelation(aRelation);

    MOZ_COUNT_CTOR(nsTemplateCondition);
}

nsTemplateCondition::nsTemplateCondition(nsIAtom* aSourceVariable,
                                         const nsAString& aRelation,
                                         const nsAString& aTargets,
                                         bool aIgnoreCase,
                                         bool aNegate,
                                         bool aIsMultiple)
    : mSourceVariable(aSourceVariable),
      mIgnoreCase(aIgnoreCase),
      mNegate(aNegate),
      mNext(nullptr)
{
    SetRelation(aRelation);

    if (aIsMultiple) {
        int32_t start = 0, end = 0;
        while ((end = aTargets.FindChar(',',start)) >= 0) {
            if (end > start) {
                mTargetList.AppendElement(Substring(aTargets, start, end - start));
            }
            start = end + 1;
        }
        if (start < int32_t(aTargets.Length())) {
            mTargetList.AppendElement(Substring(aTargets, start));
        }
    }
    else {
        mTargetList.AppendElement(aTargets);
    }

    MOZ_COUNT_CTOR(nsTemplateCondition);
}

nsTemplateCondition::nsTemplateCondition(const nsAString& aSource,
                                         const nsAString& aRelation,
                                         nsIAtom* aTargetVariable,
                                         bool aIgnoreCase,
                                         bool aNegate)
    : mSource(aSource),
      mTargetVariable(aTargetVariable),
      mIgnoreCase(aIgnoreCase),
      mNegate(aNegate),
      mNext(nullptr)
{
    SetRelation(aRelation);

    MOZ_COUNT_CTOR(nsTemplateCondition);
}

void
nsTemplateCondition::SetRelation(const nsAString& aRelation)
{
    if (aRelation.EqualsLiteral("equals") || aRelation.IsEmpty())
        mRelation = eEquals;
    else if (aRelation.EqualsLiteral("less"))
        mRelation = eLess;
    else if (aRelation.EqualsLiteral("greater"))
        mRelation = eGreater;
    else if (aRelation.EqualsLiteral("before"))
        mRelation = eBefore;
    else if (aRelation.EqualsLiteral("after"))
        mRelation = eAfter;
    else if (aRelation.EqualsLiteral("startswith"))
        mRelation = eStartswith;
    else if (aRelation.EqualsLiteral("endswith"))
        mRelation = eEndswith;
    else if (aRelation.EqualsLiteral("contains"))
        mRelation = eContains;
    else
        mRelation = eUnknown;
}

bool
nsTemplateCondition::CheckMatch(nsIXULTemplateResult* aResult)
{
    bool match = false;

    nsAutoString leftString;
    if (mSourceVariable)
      aResult->GetBindingFor(mSourceVariable, leftString);
    else
      leftString.Assign(mSource);

    if (mTargetVariable) {
        nsAutoString rightString;
        aResult->GetBindingFor(mTargetVariable, rightString);

        match = CheckMatchStrings(leftString, rightString);
    }
    else {
        
        
        uint32_t length = mTargetList.Length();
        for (uint32_t t = 0; t < length; t++) {
            match = CheckMatchStrings(leftString, mTargetList[t]);

            
            
            if (match != mNegate) break;
        }
    }

    return match;
}


bool
nsTemplateCondition::CheckMatchStrings(const nsAString& aLeftString,
                                       const nsAString& aRightString)
{
    bool match = false;

    if (aRightString.IsEmpty()) {
        if ((mRelation == eEquals) && aLeftString.IsEmpty())
            match = true;
    }
    else {
        switch (mRelation) {
            case eEquals:
                if (mIgnoreCase)
                    match = aLeftString.Equals(aRightString,
                                               nsCaseInsensitiveStringComparator());
                else
                    match = aLeftString.Equals(aRightString);
                break;

            case eLess:
            case eGreater:
            {
                
                nsresult err;
                int32_t leftint = PromiseFlatString(aLeftString).ToInteger(&err);
                if (NS_SUCCEEDED(err)) {
                    int32_t rightint = PromiseFlatString(aRightString).ToInteger(&err);
                    if (NS_SUCCEEDED(err)) {
                        match = (mRelation == eLess) ? (leftint < rightint) :
                                                       (leftint > rightint);
                    }
                }

                break;
            }

            case eBefore:
            {
                nsICollation* collation = nsXULContentUtils::GetCollation();
                if (collation) {
                    int32_t sortOrder;
                    collation->CompareString((mIgnoreCase ?
                                              static_cast<int32_t>(nsICollation::kCollationCaseInSensitive) :
                                              static_cast<int32_t>(nsICollation::kCollationCaseSensitive)),
                                              aLeftString,
                                              aRightString,
                                              &sortOrder);
                    match = (sortOrder < 0);
                }
                else if (mIgnoreCase) {
                    match = (Compare(aLeftString, aRightString,
                                     nsCaseInsensitiveStringComparator()) < 0);
                }
                else {
                    match = (Compare(aLeftString, aRightString) < 0);
                }
                break;
            }

            case eAfter:
            {
                nsICollation* collation = nsXULContentUtils::GetCollation();
                if (collation) {
                    int32_t sortOrder;
                    collation->CompareString((mIgnoreCase ?
                                              static_cast<int32_t>(nsICollation::kCollationCaseInSensitive) :
                                              static_cast<int32_t>(nsICollation::kCollationCaseSensitive)),
                                              aLeftString,
                                              aRightString,
                                              &sortOrder);
                    match = (sortOrder > 0);
                }
                else if (mIgnoreCase) {
                    match = (Compare(aLeftString, aRightString,
                                     nsCaseInsensitiveStringComparator()) > 0);
                }
                else {
                    match = (Compare(aLeftString, aRightString) > 0);
                }
                break;
            }

            case eStartswith:
                if (mIgnoreCase)
                    match = (StringBeginsWith(aLeftString, aRightString,
                                              nsCaseInsensitiveStringComparator()));
                else
                    match = (StringBeginsWith(aLeftString, aRightString));
                break;

            case eEndswith:
                if (mIgnoreCase)
                    match = (StringEndsWith(aLeftString, aRightString,
                                            nsCaseInsensitiveStringComparator()));
                else
                    match = (StringEndsWith(aLeftString, aRightString));
                break;

            case eContains:
            {
                nsAString::const_iterator start, end;
                aLeftString.BeginReading(start);
                aLeftString.EndReading(end);
                if (mIgnoreCase)
                    match = CaseInsensitiveFindInReadable(aRightString, start, end);
                else
                    match = FindInReadable(aRightString, start, end);
                break;
            }

            default:
                break;
        }
    }

    if (mNegate) match = !match;

    return match;
}

nsTemplateRule::nsTemplateRule(nsIContent* aRuleNode,
                               nsIContent* aAction,
                               nsTemplateQuerySet* aQuerySet)
        : mQuerySet(aQuerySet),
          mAction(aAction),
          mBindings(nullptr),
          mConditions(nullptr)
{
    MOZ_COUNT_CTOR(nsTemplateRule);
    mRuleNode = do_QueryInterface(aRuleNode);
}

nsTemplateRule::nsTemplateRule(const nsTemplateRule& aOtherRule)
        : mQuerySet(aOtherRule.mQuerySet),
          mRuleNode(aOtherRule.mRuleNode),
          mAction(aOtherRule.mAction),
          mBindings(nullptr),
          mConditions(nullptr)
{
    MOZ_COUNT_CTOR(nsTemplateRule);
}

nsTemplateRule::~nsTemplateRule()
{
    MOZ_COUNT_DTOR(nsTemplateRule);
    
    while (mBindings) {
        Binding* doomed = mBindings;
        mBindings = mBindings->mNext;
        delete doomed;
    }

    while (mConditions) {
        nsTemplateCondition* cdel = mConditions;
        mConditions = mConditions->GetNext();
        delete cdel;
    }
}

nsresult
nsTemplateRule::GetRuleNode(nsIDOMNode** aRuleNode) const
{
    *aRuleNode = mRuleNode;
    NS_IF_ADDREF(*aRuleNode);
    return NS_OK;
}

void nsTemplateRule::SetCondition(nsTemplateCondition* aCondition)
{
    while (mConditions) {
        nsTemplateCondition* cdel = mConditions;
        mConditions = mConditions->GetNext();
        delete cdel;
    }

    mConditions = aCondition;
}

bool
nsTemplateRule::CheckMatch(nsIXULTemplateResult* aResult) const
{
    
    nsTemplateCondition* condition = mConditions;
    while (condition) {
        if (!condition->CheckMatch(aResult))
            return false;

        condition = condition->GetNext();
    }

    if (mRuleFilter) {
        
        
        bool match;
        nsresult rv = mRuleFilter->Match(aResult, mRuleNode, &match);
        return NS_FAILED(rv) || match;
    }

    return true;
}

bool
nsTemplateRule::HasBinding(nsIAtom* aSourceVariable,
                           nsAString& aExpr,
                           nsIAtom* aTargetVariable) const
{
    for (Binding* binding = mBindings; binding != nullptr; binding = binding->mNext) {
        if ((binding->mSourceVariable == aSourceVariable) &&
            (binding->mExpr.Equals(aExpr)) &&
            (binding->mTargetVariable == aTargetVariable))
            return true;
    }

    return false;
}

nsresult
nsTemplateRule::AddBinding(nsIAtom* aSourceVariable,
                           nsAString& aExpr,
                           nsIAtom* aTargetVariable)
{
    NS_PRECONDITION(aSourceVariable != 0, "no source variable!");
    if (! aSourceVariable)
        return NS_ERROR_INVALID_ARG;

    NS_PRECONDITION(aTargetVariable != 0, "no target variable!");
    if (! aTargetVariable)
        return NS_ERROR_INVALID_ARG;

    NS_ASSERTION(! HasBinding(aSourceVariable, aExpr, aTargetVariable),
                 "binding added twice");

    Binding* newbinding = new Binding;
    if (! newbinding)
        return NS_ERROR_OUT_OF_MEMORY;

    newbinding->mSourceVariable = aSourceVariable;
    newbinding->mTargetVariable = aTargetVariable;
    newbinding->mParent         = nullptr;

    newbinding->mExpr.Assign(aExpr);

    Binding* binding = mBindings;
    Binding** link = &mBindings;

    
    
    
    
    
    
    
    
    
    while (binding) {
        if (binding->mSourceVariable == newbinding->mTargetVariable) {
            binding->mParent = newbinding;
            break;
        }
        else if (binding->mTargetVariable == newbinding->mSourceVariable) {
            newbinding->mParent = binding;
        }

        link = &binding->mNext;
        binding = binding->mNext;
    }

    
    *link = newbinding;
    newbinding->mNext = binding;
    return NS_OK;
}

nsresult
nsTemplateRule::AddBindingsToQueryProcessor(nsIXULTemplateQueryProcessor* aProcessor)
{
    Binding* binding = mBindings;

    while (binding) {
        nsresult rv = aProcessor->AddBinding(mRuleNode, binding->mTargetVariable,
                                             binding->mSourceVariable, binding->mExpr);
        if (NS_FAILED(rv)) return rv;

        binding = binding->mNext;
    }

    return NS_OK;
}
