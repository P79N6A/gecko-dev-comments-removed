






#include "SkQuadTreePicture.h"

#include "SkQuadTree.h"

SkBBoxHierarchy* SkQuadTreePicture::createBBoxHierarchy() const {
    return SkQuadTree::Create(fBounds);
}
