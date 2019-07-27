






#ifndef gfxFailure_h_
#define gfxFailure_h_

#include "nsString.h"
#include "nsIGfxInfo.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
    namespace gfx {
        inline
        void LogFailure(const nsCString &failure) {
            nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
            gfxInfo->LogFailure(failure);
        }
    } 
} 

#endif 
