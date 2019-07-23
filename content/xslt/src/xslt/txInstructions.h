





































#ifndef TRANSFRMX_TXINSTRUCTIONS_H
#define TRANSFRMX_TXINSTRUCTIONS_H

#include "nsCOMPtr.h"
#include "txCore.h"
#include "nsString.h"
#include "txXMLUtils.h"
#include "txNamespaceMap.h"
#include "nsAutoPtr.h"
#include "txXSLTNumber.h"

class nsIAtom;
class txExecutionState;

class txInstruction : public TxObject
{
public:
    txInstruction() : mNext(0)
    {
    }

    virtual ~txInstruction()
    {
        delete mNext;
    }

    virtual nsresult execute(txExecutionState& aEs) = 0;

    txInstruction* mNext;
};

#define TX_DECL_TXINSTRUCTION  \
    virtual nsresult execute(txExecutionState& aEs);


class txApplyDefaultElementTemplate : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txApplyImportsEnd : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txApplyImportsStart : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txApplyTemplates : public txInstruction
{
public:
    txApplyTemplates(const txExpandedName& aMode);

    TX_DECL_TXINSTRUCTION
    
    txExpandedName mMode;
};

class txAttribute : public txInstruction
{
public:
    txAttribute(nsAutoPtr<Expr> aName, nsAutoPtr<Expr> aNamespace,
                txNamespaceMap* aMappings);

    TX_DECL_TXINSTRUCTION

    nsAutoPtr<Expr> mName;
    nsAutoPtr<Expr> mNamespace;
    nsRefPtr<txNamespaceMap> mMappings;
};

class txCallTemplate : public txInstruction
{
public:
    txCallTemplate(const txExpandedName& aName);

    TX_DECL_TXINSTRUCTION

    txExpandedName mName;
};

class txCheckParam : public txInstruction
{
public:
    txCheckParam(const txExpandedName& aName);

    TX_DECL_TXINSTRUCTION

    txExpandedName mName;
    txInstruction* mBailTarget;
};

class txConditionalGoto : public txInstruction
{
public:
    txConditionalGoto(nsAutoPtr<Expr> aCondition, txInstruction* aTarget);

    TX_DECL_TXINSTRUCTION
    
    nsAutoPtr<Expr> mCondition;
    txInstruction* mTarget;
};

class txComment : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txCopyBase : public txInstruction
{
protected:
    nsresult copyNode(const txXPathNode& aNode, txExecutionState& aEs);
};

class txCopy : public txCopyBase
{
public:
    txCopy();

    TX_DECL_TXINSTRUCTION
    
    txInstruction* mBailTarget;
};

class txCopyOf : public txCopyBase
{
public:
    txCopyOf(nsAutoPtr<Expr> aSelect);

    TX_DECL_TXINSTRUCTION
    
    nsAutoPtr<Expr> mSelect;
};

class txEndElement : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txErrorInstruction : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txGoTo : public txInstruction
{
public:
    txGoTo(txInstruction* aTarget);

    TX_DECL_TXINSTRUCTION
    
    txInstruction* mTarget;
};

class txInsertAttrSet : public txInstruction
{
public:
    txInsertAttrSet(const txExpandedName& aName);

    TX_DECL_TXINSTRUCTION

    txExpandedName mName;
};

class txLoopNodeSet : public txInstruction
{
public:
    txLoopNodeSet(txInstruction* aTarget);

    TX_DECL_TXINSTRUCTION
    
    txInstruction* mTarget;
};

class txLREAttribute : public txInstruction
{
public:
    txLREAttribute(PRInt32 aNamespaceID, nsIAtom* aLocalName,
                   nsIAtom* aPrefix, nsAutoPtr<Expr> aValue);

    TX_DECL_TXINSTRUCTION

    PRInt32 mNamespaceID;
    nsCOMPtr<nsIAtom> mLocalName;
    nsCOMPtr<nsIAtom> mLowercaseLocalName;
    nsCOMPtr<nsIAtom> mPrefix;
    nsAutoPtr<Expr> mValue;
};

class txMessage : public txInstruction
{
public:
    txMessage(PRBool aTerminate);

    TX_DECL_TXINSTRUCTION

    PRBool mTerminate;
};

class txNumber : public txInstruction
{
public:
    txNumber(txXSLTNumber::LevelType aLevel, nsAutoPtr<txPattern> aCount,
             nsAutoPtr<txPattern> aFrom, nsAutoPtr<Expr> aValue,
             nsAutoPtr<Expr> aFormat, nsAutoPtr<Expr> aGroupingSeparator,
             nsAutoPtr<Expr> aGroupingSize);

    TX_DECL_TXINSTRUCTION

    txXSLTNumber::LevelType mLevel;
    nsAutoPtr<txPattern> mCount;
    nsAutoPtr<txPattern> mFrom;
    nsAutoPtr<Expr> mValue;
    nsAutoPtr<Expr> mFormat;
    nsAutoPtr<Expr> mGroupingSeparator;
    nsAutoPtr<Expr> mGroupingSize;
};

class txPopParams : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txProcessingInstruction : public txInstruction
{
public:
    txProcessingInstruction(nsAutoPtr<Expr> aName);

    TX_DECL_TXINSTRUCTION

    nsAutoPtr<Expr> mName;
};

class txPushNewContext : public txInstruction
{
public:
    txPushNewContext(nsAutoPtr<Expr> aSelect);
    ~txPushNewContext();

    TX_DECL_TXINSTRUCTION
    
    
    nsresult addSort(nsAutoPtr<Expr> aSelectExpr, nsAutoPtr<Expr> aLangExpr,
                     nsAutoPtr<Expr> aDataTypeExpr, nsAutoPtr<Expr> aOrderExpr,
                     nsAutoPtr<Expr> aCaseOrderExpr);

    struct SortKey {
        SortKey(nsAutoPtr<Expr> aSelectExpr, nsAutoPtr<Expr> aLangExpr,
                nsAutoPtr<Expr> aDataTypeExpr, nsAutoPtr<Expr> aOrderExpr,
                nsAutoPtr<Expr> aCaseOrderExpr);

        nsAutoPtr<Expr> mSelectExpr;
        nsAutoPtr<Expr> mLangExpr;
        nsAutoPtr<Expr> mDataTypeExpr;
        nsAutoPtr<Expr> mOrderExpr;
        nsAutoPtr<Expr> mCaseOrderExpr;
    };
    
    nsVoidArray mSortKeys;
    nsAutoPtr<Expr> mSelect;
    txInstruction* mBailTarget;
};

class txPushNullTemplateRule : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txPushParams : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txPushRTFHandler : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txPushStringHandler : public txInstruction
{
public:
    txPushStringHandler(PRBool aOnlyText);

    TX_DECL_TXINSTRUCTION

    PRBool mOnlyText;
};

class txRemoveVariable : public txInstruction
{
public:
    txRemoveVariable(const txExpandedName& aName);

    TX_DECL_TXINSTRUCTION

    txExpandedName mName;
};

class txReturn : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txSetParam : public txInstruction
{
public:
    txSetParam(const txExpandedName& aName, nsAutoPtr<Expr> aValue);

    TX_DECL_TXINSTRUCTION

    txExpandedName mName;
    nsAutoPtr<Expr> mValue;
};

class txSetVariable : public txInstruction
{
public:
    txSetVariable(const txExpandedName& aName, nsAutoPtr<Expr> aValue);

    TX_DECL_TXINSTRUCTION

    txExpandedName mName;
    nsAutoPtr<Expr> mValue;
};

class txStartElement : public txInstruction
{
public:
    txStartElement(nsAutoPtr<Expr> aName, nsAutoPtr<Expr> aNamespace,
                   txNamespaceMap* aMappings);

    TX_DECL_TXINSTRUCTION

    nsAutoPtr<Expr> mName;
    nsAutoPtr<Expr> mNamespace;
    nsRefPtr<txNamespaceMap> mMappings;
};

class txStartLREElement : public txInstruction
{
public:
    txStartLREElement(PRInt32 aNamespaceID, nsIAtom* aLocalName,
                      nsIAtom* aPrefix);

    TX_DECL_TXINSTRUCTION

    PRInt32 mNamespaceID;
    nsCOMPtr<nsIAtom> mLocalName;
    nsCOMPtr<nsIAtom> mLowercaseLocalName;
    nsCOMPtr<nsIAtom> mPrefix;
};

class txText : public txInstruction
{
public:
    txText(const nsAString& aStr, PRBool aDOE);

    TX_DECL_TXINSTRUCTION

    nsString mStr;
    PRBool mDOE;
};

class txValueOf : public txInstruction
{
public:
    txValueOf(nsAutoPtr<Expr> aExpr, PRBool aDOE);

    TX_DECL_TXINSTRUCTION

    nsAutoPtr<Expr> mExpr;
    PRBool mDOE;
};

#endif
