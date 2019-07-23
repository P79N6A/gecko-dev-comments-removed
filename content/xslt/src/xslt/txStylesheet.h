





































#ifndef TX_TXSTYLESHEET_H
#define TX_TXSTYLESHEET_H

#include "txOutputFormat.h"
#include "txExpandedNameMap.h"
#include "txList.h"
#include "txXSLTPatterns.h"
#include "nsTPtrArray.h"

class txInstruction;
class txToplevelItem;
class txTemplateItem;
class txVariableItem;
class txStripSpaceItem;
class txAttributeSetItem;
class txDecimalFormat;
class txStripSpaceTest;
class txXSLKey;

class txStylesheet
{
public:
    class ImportFrame;
    class GlobalVariable;
    friend class txStylesheetCompilerState;
    
    friend class ImportFrame;

    txStylesheet();
    ~txStylesheet();
    nsresult init();

    nsrefcnt AddRef()
    {
        return ++mRefCnt;
    }
    nsrefcnt Release()
    {
        if (--mRefCnt == 0) {
            mRefCnt = 1; 
            delete this;
            return 0;
        }
        return mRefCnt;
    }

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
    PRBool isStripSpaceAllowed(const txXPathNode& aNode,
                               txIMatchContext* aContext);

    


    nsresult doneCompiling();

    


    nsresult addKey(const txExpandedName& aName, nsAutoPtr<txPattern> aMatch,
                    nsAutoPtr<Expr> aUse);

    


    nsresult addDecimalFormat(const txExpandedName& aName,
                              nsAutoPtr<txDecimalFormat> aFormat);

    struct MatchableTemplate {
        txInstruction* mFirstInstruction;
        nsAutoPtr<txPattern> mMatch;
        double mPriority;
    };

    


    class ImportFrame {
    public:
        ImportFrame()
            : mFirstNotImported(nsnull)
        {
        }
        ~ImportFrame();

        
        txList mToplevelItems;

        
        txOwningExpandedNameMap< nsTArray<MatchableTemplate> > mMatchableTemplates;

        
        ImportFrame* mFirstNotImported;
    };

    class GlobalVariable : public TxObject {
    public:
        GlobalVariable(nsAutoPtr<Expr> aExpr,
                       nsAutoPtr<txInstruction> aFirstInstruction,
                       PRBool aIsParam);

        nsAutoPtr<Expr> mExpr;
        nsAutoPtr<txInstruction> mFirstInstruction;
        PRBool mIsParam;
    };

private:
    nsresult addTemplate(txTemplateItem* aTemplate, ImportFrame* aImportFrame);
    nsresult addGlobalVariable(txVariableItem* aVariable);
    nsresult addFrames(txListIterator& aInsertIter);
    nsresult addStripSpace(txStripSpaceItem* aStripSpaceItem,
                           nsTPtrArray<txStripSpaceTest>& aFrameStripSpaceTests);
    nsresult addAttributeSet(txAttributeSetItem* aAttributeSetItem);

    
    nsAutoRefCnt mRefCnt;

    
    txList mImportFrames;
    
    
    txOutputFormat mOutputFormat;

    
    
    txList mTemplateInstructions;
    
    
    ImportFrame* mRootFrame;
    
    
    txExpandedNameMap<txInstruction> mNamedTemplates;
    
    
    txOwningExpandedNameMap<txDecimalFormat> mDecimalFormats;

    
    txExpandedNameMap<txInstruction> mAttributeSets;
    
    
    txOwningExpandedNameMap<GlobalVariable> mGlobalVariables;
    
    
    txOwningExpandedNameMap<txXSLKey> mKeys;
    
    
    nsTPtrArray<txStripSpaceTest> mStripSpaceTests;
    
    
    nsAutoPtr<txInstruction> mContainerTemplate;
    nsAutoPtr<txInstruction> mCharactersTemplate;
    nsAutoPtr<txInstruction> mEmptyTemplate;
};






class txStripSpaceTest {
public:
    txStripSpaceTest(nsIAtom* aPrefix, nsIAtom* aLocalName, PRInt32 aNSID,
                     MBool stripSpace)
        : mNameTest(aPrefix, aLocalName, aNSID, txXPathNodeType::ELEMENT_NODE),
          mStrips(stripSpace)
    {
    }

    MBool matches(const txXPathNode& aNode, txIMatchContext* aContext) {
        return mNameTest.matches(aNode, aContext);
    }

    MBool stripsSpace() {
        return mStrips;
    }

    double getDefaultPriority() {
        return mNameTest.getDefaultPriority();
    }

protected:
    txNameTest mNameTest;
    MBool mStrips;
};




class txIGlobalParameter
{
public:
    virtual nsresult getValue(txAExprResult** aValue) = 0;
};


#endif 
