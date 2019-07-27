




#ifndef TRANSFRMX_TXTOPLEVELITEMS_H
#define TRANSFRMX_TXTOPLEVELITEMS_H

#include "nsError.h"
#include "txOutputFormat.h"
#include "txXMLUtils.h"
#include "txStylesheet.h"
#include "txInstructions.h"

class txPattern;
class Expr;

class txToplevelItem
{
public:
    txToplevelItem()
    {
        MOZ_COUNT_CTOR(txToplevelItem);
    }
    virtual ~txToplevelItem()
    {
        MOZ_COUNT_DTOR(txToplevelItem);
    }

    enum type {
        attributeSet,
        dummy,
        import,
        
        output,
        stripSpace, 
        templ,
        variable
    };

    virtual type getType() = 0;
};

#define TX_DECL_TOPLEVELITEM virtual type getType();
#define TX_IMPL_GETTYPE(_class, _type) \
txToplevelItem::type \
_class::getType() { return _type;}

class txInstructionContainer : public txToplevelItem
{
public:
    nsAutoPtr<txInstruction> mFirstInstruction;
};


class txAttributeSetItem : public txInstructionContainer
{
public:
    txAttributeSetItem(const txExpandedName aName) : mName(aName)
    {
    }

    TX_DECL_TOPLEVELITEM

    txExpandedName mName;
};


class txImportItem : public txToplevelItem
{
public:
    TX_DECL_TOPLEVELITEM

    nsAutoPtr<txStylesheet::ImportFrame> mFrame;
};


class txOutputItem : public txToplevelItem
{
public:
    TX_DECL_TOPLEVELITEM

    txOutputFormat mFormat;
};


class txDummyItem : public txToplevelItem
{
public:
    TX_DECL_TOPLEVELITEM
};


class txStripSpaceItem : public txToplevelItem
{
public:
    ~txStripSpaceItem();

    TX_DECL_TOPLEVELITEM

    nsresult addStripSpaceTest(txStripSpaceTest* aStripSpaceTest);

    nsTArray<txStripSpaceTest*> mStripSpaceTests;
};


class txTemplateItem : public txInstructionContainer
{
public:
    txTemplateItem(nsAutoPtr<txPattern>&& aMatch, const txExpandedName& aName,
                   const txExpandedName& aMode, double aPrio);

    TX_DECL_TOPLEVELITEM

    nsAutoPtr<txPattern> mMatch;
    txExpandedName mName;
    txExpandedName mMode;
    double mPrio;
};


class txVariableItem : public txInstructionContainer
{
public:
    txVariableItem(const txExpandedName& aName, nsAutoPtr<Expr>&& aValue,
                   bool aIsParam);
    
    TX_DECL_TOPLEVELITEM

    txExpandedName mName;
    nsAutoPtr<Expr> mValue;
    bool mIsParam;
};

#endif
