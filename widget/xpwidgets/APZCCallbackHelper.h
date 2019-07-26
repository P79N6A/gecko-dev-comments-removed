




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

    
    static already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils(nsIDocument* doc);

    

    static already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils(nsIContent* content);
};

}
}

#endif 
