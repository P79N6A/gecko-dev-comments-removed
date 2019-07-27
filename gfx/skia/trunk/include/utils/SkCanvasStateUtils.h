






#ifndef SkCanvasStateUtils_DEFINED
#define SkCanvasStateUtils_DEFINED

#include "SkCanvas.h"

class SkCanvasState;
















namespace SkCanvasStateUtils {
    


















    SK_API SkCanvasState* CaptureCanvasState(SkCanvas* canvas);

    











    SK_API SkCanvas* CreateFromCanvasState(const SkCanvasState* state);

    







    SK_API void ReleaseCanvasState(SkCanvasState* state);
};

#endif
