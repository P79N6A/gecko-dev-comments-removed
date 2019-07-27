




#ifndef __nsPrintingPromptServiceProxy_h
#define __nsPrintingPromptServiceProxy_h

#include "nsIPrintingPromptService.h"
#include "mozilla/embedding/PPrintingChild.h"

class nsPrintingPromptServiceProxy: public nsIPrintingPromptService,
                                    public mozilla::embedding::PPrintingChild
{
    virtual ~nsPrintingPromptServiceProxy();

public:
    nsPrintingPromptServiceProxy();

    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPRINTINGPROMPTSERVICE

    virtual PPrintProgressDialogChild*
    AllocPPrintProgressDialogChild() MOZ_OVERRIDE;

    virtual bool
    DeallocPPrintProgressDialogChild(PPrintProgressDialogChild* aActor) MOZ_OVERRIDE;
};

#endif

