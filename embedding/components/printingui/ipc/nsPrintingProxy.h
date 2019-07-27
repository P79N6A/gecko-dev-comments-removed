




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

    static already_AddRefed<nsPrintingProxy> GetInstance();

    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPRINTINGPROMPTSERVICE

    nsresult SavePrintSettings(nsIPrintSettings* aPS,
                               bool aUsePrinterNamePrefix,
                               uint32_t aFlags);

    virtual PPrintProgressDialogChild*
    AllocPPrintProgressDialogChild() override;

    virtual bool
    DeallocPPrintProgressDialogChild(PPrintProgressDialogChild* aActor) override;

    virtual PPrintSettingsDialogChild*
    AllocPPrintSettingsDialogChild() override;

    virtual bool
    DeallocPPrintSettingsDialogChild(PPrintSettingsDialogChild* aActor) override;
};

#endif

