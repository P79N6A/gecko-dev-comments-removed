





































#include "txToplevelItems.h"
#include "txStylesheet.h"
#include "txInstructions.h"
#include "txXSLTPatterns.h"

TX_IMPL_GETTYPE(txAttributeSetItem, txToplevelItem::attributeSet)
TX_IMPL_GETTYPE(txImportItem, txToplevelItem::import)
TX_IMPL_GETTYPE(txOutputItem, txToplevelItem::output)
TX_IMPL_GETTYPE(txDummyItem, txToplevelItem::dummy)

TX_IMPL_GETTYPE(txStripSpaceItem, txToplevelItem::stripSpace)

txStripSpaceItem::~txStripSpaceItem()
{
    PRInt32 i, count = mStripSpaceTests.Length();
    for (i = 0; i < count; ++i) {
        delete mStripSpaceTests[i];
    }
}

nsresult
txStripSpaceItem::addStripSpaceTest(txStripSpaceTest* aStripSpaceTest)
{
    if (!mStripSpaceTests.AppendElement(aStripSpaceTest)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

TX_IMPL_GETTYPE(txTemplateItem, txToplevelItem::templ)

txTemplateItem::txTemplateItem(nsAutoPtr<txPattern> aMatch,
                               const txExpandedName& aName,
                               const txExpandedName& aMode, double aPrio)
    : mMatch(aMatch), mName(aName), mMode(aMode), mPrio(aPrio)
{
}

TX_IMPL_GETTYPE(txVariableItem, txToplevelItem::variable)

txVariableItem::txVariableItem(const txExpandedName& aName,
                               nsAutoPtr<Expr> aValue,
                               PRBool aIsParam)
    : mName(aName), mValue(aValue), mIsParam(aIsParam)
{
}
