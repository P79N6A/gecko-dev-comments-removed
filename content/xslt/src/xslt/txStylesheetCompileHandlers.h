





































#ifndef TRANSFRMX_TXSTYLESHEETCOMPILEHANDLERS_H
#define TRANSFRMX_TXSTYLESHEETCOMPILEHANDLERS_H

#include "txError.h"
#include "txNamespaceMap.h"
#include "txExpandedNameMap.h"

struct txStylesheetAttr;
class txStylesheetCompilerState;

typedef nsresult (*HandleStartFn) (PRInt32 aNamespaceID,
                                   nsIAtom* aLocalName,
                                   nsIAtom* aPrefix,
                                   txStylesheetAttr* aAttributes,
                                   PRInt32 aAttrCount,
                                   txStylesheetCompilerState& aState);
typedef nsresult (*HandleEndFn)   (txStylesheetCompilerState& aState);
typedef nsresult (*HandleTextFn)  (const nsAString& aStr,
                                   txStylesheetCompilerState& aState);

struct txElementHandler {
    PRInt32 mNamespaceID;
    char* mLocalName;
    HandleStartFn mStartFunction;
    HandleEndFn mEndFunction;
};

class txHandlerTable
{
public:
    txHandlerTable(const HandleTextFn aTextHandler,
                   const txElementHandler* aLREHandler,
                   const txElementHandler* aOtherHandler);
    nsresult init(const txElementHandler* aHandlers, PRUint32 aCount);
    const txElementHandler* find(PRInt32 aNamespaceID, nsIAtom* aLocalName);
    
    const HandleTextFn mTextHandler;
    const txElementHandler* const mLREHandler;

    static MBool init();
    static void shutdown();

private:
    const txElementHandler* const mOtherHandler;
    txExpandedNameMap<const txElementHandler> mHandlers;
};

extern txHandlerTable* gTxRootHandler;
extern txHandlerTable* gTxEmbedHandler;

#endif 
