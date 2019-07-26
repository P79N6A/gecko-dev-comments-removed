




#ifndef __mozilla_widget_APZCCallbackHelper_h__
#define __mozilla_widget_APZCCallbackHelper_h__

#include "FrameMetrics.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMWindowUtils.h"

namespace mozilla {
namespace widget {







class APZCCallbackHelper
{
    typedef mozilla::layers::FrameMetrics FrameMetrics;

public:
    



    static bool HasValidPresShellId(nsIDOMWindowUtils* aUtils,
                                    const FrameMetrics& aMetrics);

    

    static void UpdateRootFrame(nsIDOMWindowUtils* aUtils,
                                const FrameMetrics& aMetrics);

    

    static void UpdateSubFrame(nsIContent* aContent,
                               const FrameMetrics& aMetrics);

    
    static already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils(const nsIDocument* aDoc);

    

    static already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils(const nsIContent* aContent);

    

    static bool GetScrollIdentifiers(const nsIContent* aContent,
                                     uint32_t* aPresShellIdOut,
                                     FrameMetrics::ViewID* aViewIdOut);
};

}
}

#endif 
