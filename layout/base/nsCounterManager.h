







#ifndef nsCounterManager_h_
#define nsCounterManager_h_

#include "mozilla/Attributes.h"
#include "nsGenConList.h"
#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "mozilla/Likely.h"
#include "CounterStyleManager.h"

class nsCounterList;
struct nsCounterUseNode;
struct nsCounterChangeNode;

struct nsCounterNode : public nsGenConNode {
    enum Type {
        RESET,     
        INCREMENT, 
        USE        
    };

    Type mType;

    
    int32_t mValueAfter;

    
    
    
    

    
    
    
    
    nsCounterNode *mScopeStart;

    
    
    

    
    
    
    
    
    nsCounterNode *mScopePrev;

    inline nsCounterUseNode* UseNode();
    inline nsCounterChangeNode* ChangeNode();

    
    
    
    
    
    
    nsCounterNode(int32_t aContentIndex, Type aType)
        : nsGenConNode(aContentIndex)
        , mType(aType)
        , mValueAfter(0)
        , mScopeStart(nullptr)
        , mScopePrev(nullptr)
    {
    }

    
    inline void Calc(nsCounterList* aList);
};

struct nsCounterUseNode : public nsCounterNode {
    
    
    
    nsRefPtr<nsCSSValue::Array> mCounterFunction;

    nsPresContext* mPresContext;
    nsRefPtr<mozilla::CounterStyle> mCounterStyle;

    
    bool mAllCounters;

    
    nsCounterUseNode(nsPresContext* aPresContext,
                     nsCSSValue::Array* aCounterFunction,
                     uint32_t aContentIndex, bool aAllCounters)
        : nsCounterNode(aContentIndex, USE)
        , mCounterFunction(aCounterFunction)
        , mPresContext(aPresContext)
        , mCounterStyle(nullptr)
        , mAllCounters(aAllCounters)
    {
        NS_ASSERTION(aContentIndex <= INT32_MAX, "out of range");
    }

    virtual bool InitTextFrame(nsGenConList* aList,
            nsIFrame* aPseudoFrame, nsIFrame* aTextFrame) MOZ_OVERRIDE;

    mozilla::CounterStyle* GetCounterStyle();
    void SetCounterStyleDirty()
    {
        mCounterStyle = nullptr;
    }

    
    
    void Calc(nsCounterList* aList);

    
    void GetText(nsString& aResult);
};

struct nsCounterChangeNode : public nsCounterNode {
    int32_t mChangeValue; 

    
    
    
    
    
    nsCounterChangeNode(nsIFrame* aPseudoFrame,
                        nsCounterNode::Type aChangeType,
                        int32_t aChangeValue,
                        int32_t aPropIndex)
        : nsCounterNode(
                        
                        
                        aPropIndex + (aChangeType == RESET
                                        ? (INT32_MIN) 
                                        : (INT32_MIN / 2)),
                        aChangeType)
        , mChangeValue(aChangeValue)
    {
        NS_ASSERTION(aPropIndex >= 0, "out of range");
        NS_ASSERTION(aChangeType == INCREMENT || aChangeType == RESET,
                     "bad type");
        mPseudoFrame = aPseudoFrame;
        CheckFrameAssertions();
    }

    
    
    void Calc(nsCounterList* aList);
};

inline nsCounterUseNode* nsCounterNode::UseNode()
{
    NS_ASSERTION(mType == USE, "wrong type");
    return static_cast<nsCounterUseNode*>(this);
}

inline nsCounterChangeNode* nsCounterNode::ChangeNode()
{
    NS_ASSERTION(mType == INCREMENT || mType == RESET, "wrong type");
    return static_cast<nsCounterChangeNode*>(this);
}

inline void nsCounterNode::Calc(nsCounterList* aList)
{
    if (mType == USE)
        UseNode()->Calc(aList);
    else
        ChangeNode()->Calc(aList);
}

class nsCounterList : public nsGenConList {
public:
    nsCounterList() : nsGenConList(),
                      mDirty(false)
    {}

    void Insert(nsCounterNode* aNode) {
        nsGenConList::Insert(aNode);
        
        
        if (MOZ_LIKELY(!IsDirty())) {
            SetScope(aNode);
        }
    }

    nsCounterNode* First() {
        return static_cast<nsCounterNode*>(mFirstNode);
    }

    static nsCounterNode* Next(nsCounterNode* aNode) {
        return static_cast<nsCounterNode*>(nsGenConList::Next(aNode));
    }
    static nsCounterNode* Prev(nsCounterNode* aNode) {
        return static_cast<nsCounterNode*>(nsGenConList::Prev(aNode));
    }

    static int32_t ValueBefore(nsCounterNode* aNode) {
        return aNode->mScopePrev ? aNode->mScopePrev->mValueAfter : 0;
    }

    
    void SetScope(nsCounterNode *aNode);

    
    
    void RecalcAll();

    bool IsDirty() { return mDirty; }
    void SetDirty() { mDirty = true; }

private:
    bool mDirty;
};





class nsCounterManager {
public:
    nsCounterManager();
    
    bool AddCounterResetsAndIncrements(nsIFrame *aFrame);

    
    
    nsCounterList* CounterListFor(const nsSubstring& aCounterName);

    
    void RecalcAll();

    
    void SetAllCounterStylesDirty();

    
    
    bool DestroyNodesFor(nsIFrame *aFrame);

    
    void Clear() { mNames.Clear(); }

#ifdef DEBUG
    void Dump();
#endif

    static int32_t IncrementCounter(int32_t aOldValue, int32_t aIncrement)
    {
        
        
        
        
        
        
        
        
        
        int32_t newValue = int32_t(uint32_t(aOldValue) + uint32_t(aIncrement));
        
        
        
        
        
        
        if ((aIncrement > 0) != (newValue > aOldValue)) {
          newValue = aOldValue;
        }
        return newValue;
    }

private:
    
    bool AddResetOrIncrement(nsIFrame *aFrame, int32_t aIndex,
                               const nsStyleCounterData *aCounterData,
                               nsCounterNode::Type aType);

    nsClassHashtable<nsStringHashKey, nsCounterList> mNames;
};

#endif 
