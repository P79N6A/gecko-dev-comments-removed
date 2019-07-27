





#ifndef mozilla_embedding_PrintingParent_h
#define mozilla_embedding_PrintingParent_h

#include "mozilla/embedding/PPrintingParent.h"
#include "mozilla/dom/PBrowserParent.h"

namespace mozilla {
namespace embedding {
class PrintingParent : public PPrintingParent
{
public:
    virtual bool
    RecvShowProgress(PBrowserParent* parent,
                     const bool& isForPrinting);
    virtual bool
    RecvShowPrintDialog(PBrowserParent* parent,
                        const PrintData& initSettings,
                        PrintData* retVal,
                        bool* success);

    virtual void
    ActorDestroy(ActorDestroyReason aWhy);

    MOZ_IMPLICIT PrintingParent();
    virtual ~PrintingParent();
};
} 
} 

#endif

