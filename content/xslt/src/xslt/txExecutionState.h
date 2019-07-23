





































#ifndef TRANSFRMX_TXEXECUTIONSTATE_H
#define TRANSFRMX_TXEXECUTIONSTATE_H

#include "txCore.h"
#include "txStack.h"
#include "txXMLUtils.h"
#include "nsVoidArray.h"
#include "txIXPathContext.h"
#include "txVariableMap.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "txKey.h"
#include "txStylesheet.h"
#include "txXPathTreeWalker.h"
#include "nsTArray.h"

class txAOutputHandlerFactory;
class txAXMLEventHandler;
class txInstruction;
class txIOutputHandlerFactory;

class txLoadedDocumentEntry : public nsStringHashKey
{
public:
    txLoadedDocumentEntry(KeyTypePointer aStr) : nsStringHashKey(aStr)
    {
    }
    txLoadedDocumentEntry(const txLoadedDocumentEntry& aToCopy)
        : nsStringHashKey(aToCopy)
    {
        NS_ERROR("We're horked.");
    }
    ~txLoadedDocumentEntry()
    {
        if (mDocument) {
            txXPathNodeUtils::release(mDocument);
        }
    }

    nsAutoPtr<txXPathNode> mDocument;
};

class txLoadedDocumentsHash : public nsTHashtable<txLoadedDocumentEntry>
{
public:
    ~txLoadedDocumentsHash();
    nsresult init(txXPathNode* aSourceDocument);

private:
    friend class txExecutionState;
    txXPathNode* mSourceDocument;
};


class txExecutionState : public txIMatchContext
{
public:
    txExecutionState(txStylesheet* aStylesheet, PRBool aDisableLoads);
    ~txExecutionState();
    nsresult init(const txXPathNode& aNode,
                  txOwningExpandedNameMap<txIGlobalParameter>* aGlobalParams);
    nsresult end(nsresult aResult);

    TX_DECL_MATCH_CONTEXT;

    


    struct TemplateRule {
        txStylesheet::ImportFrame* mFrame;
        PRInt32 mModeNsId;
        nsIAtom* mModeLocalName;
        txVariableMap* mParams;
    };

    
    nsresult pushEvalContext(txIEvalContext* aContext);
    txIEvalContext* popEvalContext();
    nsresult pushBool(PRBool aBool);
    PRBool popBool();
    nsresult pushResultHandler(txAXMLEventHandler* aHandler);
    txAXMLEventHandler* popResultHandler();
    nsresult pushTemplateRule(txStylesheet::ImportFrame* aFrame,
                              const txExpandedName& aMode,
                              txVariableMap* aParams);
    void popTemplateRule();
    nsresult pushParamMap(txVariableMap* aParams);
    txVariableMap* popParamMap();

    
    txIEvalContext* getEvalContext();
    const txXPathNode* retrieveDocument(const nsAString& aUri);
    nsresult getKeyNodes(const txExpandedName& aKeyName,
                         const txXPathNode& aRoot,
                         const nsAString& aKeyValue, PRBool aIndexIfNotFound,
                         txNodeSet** aResult);
    TemplateRule* getCurrentTemplateRule();
    const txXPathNode& getSourceDocument()
    {
        NS_ASSERTION(mLoadedDocuments.mSourceDocument,
                     "Need a source document!");

        return *mLoadedDocuments.mSourceDocument;
    }

    
    txInstruction* getNextInstruction();
    nsresult runTemplate(txInstruction* aInstruction);
    nsresult runTemplate(txInstruction* aInstruction,
                         txInstruction* aReturnTo);
    void gotoInstruction(txInstruction* aNext);
    void returnFromTemplate();
    nsresult bindVariable(const txExpandedName& aName,
                          txAExprResult* aValue);
    void removeVariable(const txExpandedName& aName);

    txAXMLEventHandler* mOutputHandler;
    txAXMLEventHandler* mResultHandler;
    txAOutputHandlerFactory* mOutputHandlerFactory;

    nsAutoPtr<txVariableMap> mTemplateParams;

    nsRefPtr<txStylesheet> mStylesheet;

private:
    txStack mReturnStack;
    txStack mLocalVarsStack;
    txStack mEvalContextStack;
    nsTArray<PRPackedBool> mBoolStack;
    txStack mResultHandlerStack;
    txStack mParamStack;
    txInstruction* mNextInstruction;
    txVariableMap* mLocalVariables;
    txVariableMap mGlobalVariableValues;
    nsRefPtr<txAExprResult> mGlobalVarPlaceholderValue;
    PRInt32 mRecursionDepth;

    TemplateRule* mTemplateRules;
    PRInt32 mTemplateRulesBufferSize;
    PRInt32 mTemplateRuleCount;

    txIEvalContext* mEvalContext;
    txIEvalContext* mInitialEvalContext;
    
    txOwningExpandedNameMap<txIGlobalParameter>* mGlobalParams;

    txLoadedDocumentsHash mLoadedDocuments;
    txKeyHash mKeyHash;
    nsRefPtr<txResultRecycler> mRecycler;
    PRPackedBool mDisableLoads;

    static const PRInt32 kMaxRecursionDepth;
};

#endif
