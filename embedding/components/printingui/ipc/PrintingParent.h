





#ifndef mozilla_embedding_PrintingParent_h
#define mozilla_embedding_PrintingParent_h

#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/embedding/PPrintingParent.h"
#include "mozilla/embedding/PPrintProgressDialogParent.h"

class nsIDOMWindow;

namespace mozilla {
namespace embedding {

class PrintingParent MOZ_FINAL : public PPrintingParent
{
public:
    virtual bool
    RecvShowProgress(PBrowserParent* parent,
                     PPrintProgressDialogParent* printProgressDialog,
                     const bool& isForPrinting,
                     bool* notifyOnOpen,
                     bool* success);
    virtual bool
    RecvShowPrintDialog(PBrowserParent* parent,
                        const PrintData& initSettings,
                        PrintData* retVal,
                        bool* success);

    virtual bool
    RecvSavePrintSettings(const PrintData& aData,
                          const bool& aUsePrinterNamePrefix,
                          const uint32_t& aFlags,
                          nsresult* aResult);

    virtual PPrintProgressDialogParent*
    AllocPPrintProgressDialogParent();

    virtual bool
    DeallocPPrintProgressDialogParent(PPrintProgressDialogParent* aActor);

    virtual void
    ActorDestroy(ActorDestroyReason aWhy);

    MOZ_IMPLICIT PrintingParent();
    virtual ~PrintingParent();

private:
    nsIDOMWindow*
    DOMWindowFromBrowserParent(PBrowserParent* parent);
};
} 
} 

#endif

