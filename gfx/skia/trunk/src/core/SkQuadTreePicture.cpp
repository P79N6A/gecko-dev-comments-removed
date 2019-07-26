






#include "SkQuadTreePicture.h"

#include "SkQuadTree.h"

SkBBoxHierarchy* SkQuadTreePicture::createBBoxHierarchy() const {
    return SkNEW_ARGS(SkQuadTree, (fBounds));
}
