





































#ifndef TRANSFRMX_TXSTYLESHEETCOMPILER_H
#define TRANSFRMX_TXSTYLESHEETCOMPILER_H

#include "txStack.h"
#include "txXSLTPatterns.h"
#include "txExpr.h"
#include "txIXPathContext.h"
#include "nsAutoPtr.h"
#include "txStylesheet.h"

extern PRBool
TX_XSLTFunctionAvailable(nsIAtom* aName, PRInt32 aNameSpaceID);

class txHandlerTable;
class txElementContext;
class txInstructionContainer;
class txInstruction;
class txNamespaceMap;
class txToplevelItem;
class txPushNewContext;
class txStylesheetCompiler;

class txElementContext : public TxObject
{
public:
    txElementContext(const nsAString& aBaseURI);
    txElementContext(const txElementContext& aOther);

    PRBool mPreserveWhitespace;
    PRBool mForwardsCompatibleParsing;
    nsString mBaseURI;
    nsRefPtr<txNamespaceMap> mMappings;
    nsVoidArray mInstructionNamespaces;
    PRInt32 mDepth;
};

class txACompileObserver
{
public:
    virtual nsrefcnt AddRef() = 0;
    virtual nsrefcnt Release() = 0;

    virtual nsresult loadURI(const nsAString& aUri,
                             const nsAString& aReferrerUri,
                             txStylesheetCompiler* aCompiler) = 0;
    virtual void onDoneCompiling(txStylesheetCompiler* aCompiler,
                                 nsresult aResult,
                                 const PRUnichar *aErrorText = nsnull,
                                 const PRUnichar *aParam = nsnull) = 0;
};

#define TX_DECL_ACOMPILEOBSERVER \
  nsrefcnt AddRef(); \
  nsrefcnt Release(); \
  nsresult loadURI(const nsAString& aUri, const nsAString& aReferrerUri, \
                   txStylesheetCompiler* aCompiler); \
  void onDoneCompiling(txStylesheetCompiler* aCompiler, nsresult aResult, \
                       const PRUnichar *aErrorText = nsnull, \
                       const PRUnichar *aParam = nsnull)

class txStylesheetCompilerState : public txIParseContext
{
public:
    txStylesheetCompilerState(txACompileObserver* aObserver);
    ~txStylesheetCompilerState();
    
    nsresult init(const nsAString& aStylesheetURI, txStylesheet* aStylesheet,
                  txListIterator* aInsertPosition);

    
    PRBool handleEmbeddedSheet()
    {
        return mEmbedStatus == eInEmbed;
    }
    void doneEmbedding()
    {
        mEmbedStatus = eHasEmbed;
    }

    
    nsresult pushHandlerTable(txHandlerTable* aTable);
    void popHandlerTable();
    nsresult pushSorter(txPushNewContext* aSorter);
    void popSorter();
    nsresult pushChooseGotoList();
    void popChooseGotoList();
    nsresult pushObject(TxObject* aObject);
    TxObject* popObject();
    nsresult pushPtr(void* aPtr);
    void* popPtr();

    
    nsresult addToplevelItem(txToplevelItem* aItem);
    nsresult openInstructionContainer(txInstructionContainer* aContainer);
    void closeInstructionContainer();
    nsresult addInstruction(nsAutoPtr<txInstruction> aInstruction);
    nsresult loadIncludedStylesheet(const nsAString& aURI);
    nsresult loadImportedStylesheet(const nsAString& aURI,
                                    txStylesheet::ImportFrame* aFrame);
    
    
    nsresult addGotoTarget(txInstruction** aTargetPointer);
    nsresult addVariable(const txExpandedName& aName);

    
    nsresult resolveNamespacePrefix(nsIAtom* aPrefix, PRInt32& aID);
    nsresult resolveFunctionCall(nsIAtom* aName, PRInt32 aID,
                                 FunctionCall** aFunction);
    PRBool caseInsensitiveNameTests();

    


    PRBool fcp()
    {
        return mElementContext->mForwardsCompatibleParsing;
    }

    void SetErrorOffset(PRUint32 aOffset);

    static void shutdown();


    nsRefPtr<txStylesheet> mStylesheet;
    txHandlerTable* mHandlerTable;
    nsAutoPtr<txElementContext> mElementContext;
    txPushNewContext* mSorter;
    nsAutoPtr<txList> mChooseGotoList;
    PRPackedBool mDOE;
    PRPackedBool mSearchingForFallback;

protected:
    nsRefPtr<txACompileObserver> mObserver;
    nsVoidArray mInScopeVariables;
    nsVoidArray mChildCompilerList;
    
    nsString mTarget;
    enum 
    {
        eNoEmbed,
        eNeedEmbed,
        eInEmbed,
        eHasEmbed
    } mEmbedStatus;
    nsString mStylesheetURI;
    PRPackedBool mIsTopCompiler;
    PRPackedBool mDoneWithThisStylesheet;
    txStack mObjectStack;
    txStack mOtherStack;

private:
    txInstruction** mNextInstrPtr;
    txListIterator mToplevelIterator;
    nsVoidArray mGotoTargetPointers;
};

struct txStylesheetAttr
{
    PRInt32 mNamespaceID;
    nsCOMPtr<nsIAtom> mLocalName;
    nsCOMPtr<nsIAtom> mPrefix;
    nsString mValue;
};

class txStylesheetCompiler : private txStylesheetCompilerState,
                             public txACompileObserver
{
public:
    friend class txStylesheetCompilerState;
    friend PRBool TX_XSLTFunctionAvailable(nsIAtom* aName,
                                           PRInt32 aNameSpaceID);
    txStylesheetCompiler(const nsAString& aStylesheetURI,
                         txACompileObserver* aObserver);
    txStylesheetCompiler(const nsAString& aStylesheetURI,
                         txStylesheet* aStylesheet,
                         txListIterator* aInsertPosition,
                         txACompileObserver* aObserver);
    virtual nsrefcnt AddRef();
    virtual nsrefcnt Release();

    void setBaseURI(const nsString& aBaseURI);

    nsresult startElement(PRInt32 aNamespaceID, nsIAtom* aLocalName,
                          nsIAtom* aPrefix, txStylesheetAttr* aAttributes,
                          PRInt32 aAttrCount);
    nsresult startElement(const PRUnichar *aName,
                          const PRUnichar **aAtts,
                          PRInt32 aAttrCount, PRInt32 aIDOffset);
    nsresult endElement();
    nsresult characters(const nsAString& aStr);
    nsresult doneLoading();

    void cancel(nsresult aError, const PRUnichar *aErrorText = nsnull,
                const PRUnichar *aParam = nsnull);

    txStylesheet* getStylesheet();

    
    nsresult loadURI(const nsAString& aUri, const nsAString& aReferrerUri,
                     txStylesheetCompiler* aCompiler);
    void onDoneCompiling(txStylesheetCompiler* aCompiler, nsresult aResult,
                         const PRUnichar *aErrorText = nsnull,
                         const PRUnichar *aParam = nsnull);

private:
    nsresult startElementInternal(PRInt32 aNamespaceID, nsIAtom* aLocalName,
                                  nsIAtom* aPrefix,
                                  txStylesheetAttr* aAttributes,
                                  PRInt32 aAttrCount,
                                  PRInt32 aIDOffset = -1);

    nsresult flushCharacters();
    nsresult ensureNewElementContext();
    nsresult maybeDoneCompiling();

    nsAutoRefCnt mRefCnt;
    nsString mCharacters;
    nsresult mStatus;
};

class txInScopeVariable {
public:
    txInScopeVariable(const txExpandedName& aName) : mName(aName), mLevel(1)
    {
    }
    txExpandedName mName;
    PRInt32 mLevel;
};

#endif
