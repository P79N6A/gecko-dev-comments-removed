






#include "SkNullCanvas.h"

#include "SkCanvas.h"
#include "SkNWayCanvas.h"


SkCanvas* SkCreateNullCanvas() {
    
    
    return SkNEW_ARGS(SkNWayCanvas, (0,0));
}
