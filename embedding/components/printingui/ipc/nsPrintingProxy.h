




#ifndef __nsPrintingProxy_h
#define __nsPrintingProxy_h

#include "nsIPrintingPromptService.h"
#include "mozilla/embedding/PPrintingChild.h"

class nsPrintingProxy: public nsIPrintingPromptService,
                       public mozilla::embedding::PPrintingChild
{
    virtual ~nsPrintingProxy();

public:
    nsPrintingProxy();

    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPRINTINGPROMPTSERVICE

    virtual PPrintProgressDialogChild*
    AllocPPrintProgressDialogChild() MOZ_OVERRIDE;

    virtual bool
    DeallocPPrintProgressDialogChild(PPrintProgressDialogChild* aActor) MOZ_OVERRIDE;
};

#endif

