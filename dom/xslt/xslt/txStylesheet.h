




#ifndef TX_TXSTYLESHEET_H
#define TX_TXSTYLESHEET_H

#include "txOutputFormat.h"
#include "txExpandedNameMap.h"
#include "txList.h"
#include "txXSLTPatterns.h"
#include "nsISupportsImpl.h"

class txInstruction;
class txTemplateItem;
class txVariableItem;
class txStripSpaceItem;
class txAttributeSetItem;
class txDecimalFormat;
class txStripSpaceTest;
class txXSLKey;

class txStylesheet final
{
public:
    class ImportFrame;
    class GlobalVariable;
    friend class txStylesheetCompilerState;
    
    friend class ImportFrame;

    txStylesheet();
    nsresult init();
    
    NS_INLINE_DECL_REFCOUNTING(txStylesheet)

    txInstruction* findTemplate(const txXPathNode& aNode,
                                const txExpandedName& aMode,
                                txIMatchContext* aContext,
                                ImportFrame* aImportedBy,
                                ImportFrame** aImportFrame);
    txDecimalFormat* getDecimalFormat(const txExpandedName& aName);
    txInstruction* getAttributeSet(const txExpandedName& aName);
    txInstruction* getNamedTemplate(const txExpandedName& aName);
    txOutputFormat* getOutputFormat();
    GlobalVariable* getGlobalVariable(const txExpandedName& aName);
    const txOwningExpandedNameMap<txXSLKey>& getKeyMap();
    bool isStripSpaceAllowed(const txXPathNode& aNode,
                               txIMatchContext* aContext);

    


    nsresult doneCompiling();

    


    nsresult addKey(const txExpandedName& aName, nsAutoPtr<txPattern> aMatch,
                    nsAutoPtr<Expr> aUse);

    


    nsresult addDecimalFormat(const txExpandedName& aName,
                              nsAutoPtr<txDecimalFormat>&& aFormat);

    struct MatchableTemplate {
        txInstruction* mFirstInstruction;
        nsAutoPtr<txPattern> mMatch;
        double mPriority;
    };

    


    class ImportFrame {
    public:
        ImportFrame()
            : mFirstNotImported(nullptr)
        {
        }
        ~ImportFrame();

        
        txList mToplevelItems;

        
        txOwningExpandedNameMap< nsTArray<MatchableTemplate> > mMatchableTemplates;

        
        ImportFrame* mFirstNotImported;
    };

    class GlobalVariable : public txObject {
    public:
        GlobalVariable(nsAutoPtr<Expr>&& aExpr,
                       nsAutoPtr<txInstruction>&& aFirstInstruction,
                       bool aIsParam);

        nsAutoPtr<Expr> mExpr;
        nsAutoPtr<txInstruction> mFirstInstruction;
        bool mIsParam;
    };

private:
    
    ~txStylesheet();

    nsresult addTemplate(txTemplateItem* aTemplate, ImportFrame* aImportFrame);
    nsresult addGlobalVariable(txVariableItem* aVariable);
    nsresult addFrames(txListIterator& aInsertIter);
    nsresult addStripSpace(txStripSpaceItem* aStripSpaceItem,
                           nsTArray<txStripSpaceTest*>& aFrameStripSpaceTests);
    nsresult addAttributeSet(txAttributeSetItem* aAttributeSetItem);

    
    txList mImportFrames;
    
    
    txOutputFormat mOutputFormat;

    
    
    txList mTemplateInstructions;
    
    
    ImportFrame* mRootFrame;
    
    
    txExpandedNameMap<txInstruction> mNamedTemplates;
    
    
    txOwningExpandedNameMap<txDecimalFormat> mDecimalFormats;

    
    txExpandedNameMap<txInstruction> mAttributeSets;
    
    
    txOwningExpandedNameMap<GlobalVariable> mGlobalVariables;
    
    
    txOwningExpandedNameMap<txXSLKey> mKeys;
    
    
    nsTArray<nsAutoPtr<txStripSpaceTest> > mStripSpaceTests;
    
    
    nsAutoPtr<txInstruction> mContainerTemplate;
    nsAutoPtr<txInstruction> mCharactersTemplate;
    nsAutoPtr<txInstruction> mEmptyTemplate;
};






class txStripSpaceTest {
public:
    txStripSpaceTest(nsIAtom* aPrefix, nsIAtom* aLocalName, int32_t aNSID,
                     bool stripSpace)
        : mNameTest(aPrefix, aLocalName, aNSID, txXPathNodeType::ELEMENT_NODE),
          mStrips(stripSpace)
    {
    }

    bool matches(const txXPathNode& aNode, txIMatchContext* aContext) {
        return mNameTest.matches(aNode, aContext);
    }

    bool stripsSpace() {
        return mStrips;
    }

    double getDefaultPriority() {
        return mNameTest.getDefaultPriority();
    }

protected:
    txNameTest mNameTest;
    bool mStrips;
};




class txIGlobalParameter
{
public:
    txIGlobalParameter()
    {
        MOZ_COUNT_CTOR(txIGlobalParameter);
    }
    virtual ~txIGlobalParameter()
    {
        MOZ_COUNT_DTOR(txIGlobalParameter);
    }
    virtual nsresult getValue(txAExprResult** aValue) = 0;
};


#endif 
