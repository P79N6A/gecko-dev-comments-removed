







































#ifndef nsCounterManager_h_
#define nsCounterManager_h_

#include "nsGenConList.h"
#include "nsAutoPtr.h"
#include "nsClassHashtable.h"

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

    
    PRInt32 mValueAfter;

    
    
    
    

    
    
    
    
    nsCounterNode *mScopeStart;

    
    
    

    
    
    
    
    
    nsCounterNode *mScopePrev;

    inline nsCounterUseNode* UseNode();
    inline nsCounterChangeNode* ChangeNode();

    
    
    
    
    
    
    nsCounterNode(nsIFrame* aPseudoFrame, PRInt32 aContentIndex, Type aType)
        : nsGenConNode(aPseudoFrame, aContentIndex)
        , mType(aType)
        , mValueAfter(0)
        , mScopeStart(nsnull)
        , mScopePrev(nsnull)
    {
    }

    
    inline void Calc(nsCounterList* aList);
};

struct nsCounterUseNode : public nsCounterNode {
    
    
    
    nsRefPtr<nsCSSValue::Array> mCounterStyle;

    
    PRBool mAllCounters;

    
    nsCounterUseNode(nsCSSValue::Array* aCounterStyle, nsIFrame* aPseudoFrame,
                     PRUint32 aContentIndex, PRBool aAllCounters)
        : nsCounterNode(aPseudoFrame, aContentIndex, USE)
        , mCounterStyle(aCounterStyle)
        , mAllCounters(aAllCounters)
    {
        NS_ASSERTION(aContentIndex >= 0, "out of range");
    }

    
    
    void Calc(nsCounterList* aList);

    
    void GetText(nsString& aResult);
};

struct nsCounterChangeNode : public nsCounterNode {
    PRInt32 mChangeValue; 

    
    
    
    
    
    nsCounterChangeNode(nsIFrame* aPseudoFrame,
                        nsCounterNode::Type aChangeType,
                        PRInt32 aChangeValue,
                        PRInt32 aPropIndex)
        : nsCounterNode(aPseudoFrame,
                        
                        
                        
                        aPropIndex + (aChangeType == RESET
                                        ? (PR_INT32_MIN) 
                                        : (PR_INT32_MIN / 2)),
                        aChangeType)
        , mChangeValue(aChangeValue)
    {
        NS_ASSERTION(aPropIndex >= 0, "out of range");
        NS_ASSERTION(aChangeType == INCREMENT || aChangeType == RESET,
                     "bad type");
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
                      mDirty(PR_FALSE)
    {}

    void Insert(nsCounterNode* aNode) {
        nsGenConList::Insert(aNode);
        
        
        if (NS_LIKELY(!IsDirty())) {
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

    static PRInt32 ValueBefore(nsCounterNode* aNode) {
        return aNode->mScopePrev ? aNode->mScopePrev->mValueAfter : 0;
    }

    
    void SetScope(nsCounterNode *aNode);
  
    
    
    void RecalcAll();

    PRBool IsDirty() { return mDirty; }
    void SetDirty() { mDirty = PR_TRUE; }

private:
    PRBool mDirty;
};





class nsCounterManager {
public:
    nsCounterManager();
    
    PRBool AddCounterResetsAndIncrements(nsIFrame *aFrame);

    
    
    nsCounterList* CounterListFor(const nsSubstring& aCounterName);

    
    void RecalcAll();

    
    
    PRBool DestroyNodesFor(nsIFrame *aFrame);

    
    void Clear() { mNames.Clear(); }

#ifdef DEBUG
    void Dump();
#endif

private:
    
    PRBool AddResetOrIncrement(nsIFrame *aFrame, PRInt32 aIndex,
                               const nsStyleCounterData *aCounterData,
                               nsCounterNode::Type aType);

    nsClassHashtable<nsStringHashKey, nsCounterList> mNames;
};

#endif 
