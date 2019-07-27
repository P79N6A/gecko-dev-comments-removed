







#include "SkBBoxHierarchyRecord.h"
#include "SkPictureStateTree.h"

SkBBoxHierarchyRecord::SkBBoxHierarchyRecord(const SkISize& size,
                                             uint32_t recordFlags,
                                             SkBBoxHierarchy* h)
    : INHERITED(size, recordFlags) {
    fStateTree = SkNEW(SkPictureStateTree);
    fBoundingHierarchy = h;
    fBoundingHierarchy->ref();
    fBoundingHierarchy->setClient(this);
}

void SkBBoxHierarchyRecord::handleBBox(const SkRect& bounds) {
    SkIRect r;
    bounds.roundOut(&r);
    SkPictureStateTree::Draw* draw = fStateTree->appendDraw(this->writeStream().bytesWritten());
    fBoundingHierarchy->insert(draw, r, true);
}

void SkBBoxHierarchyRecord::willSave() {
    fStateTree->appendSave();
    this->INHERITED::willSave();
}

SkCanvas::SaveLayerStrategy SkBBoxHierarchyRecord::willSaveLayer(const SkRect* bounds,
                                                                 const SkPaint* paint,
                                                                 SaveFlags flags) {
    
    
    bool paintAffectsTransparentBlack = NULL != paint &&
        ((NULL != paint->getImageFilter()) ||
         (NULL != paint->getColorFilter()));
    SkRect drawBounds;
    if (paintAffectsTransparentBlack) {
        if (bounds) {
            drawBounds = *bounds;
            this->getTotalMatrix().mapRect(&drawBounds);
        } else {
            SkIRect deviceBounds;
            this->getClipDeviceBounds(&deviceBounds);
            drawBounds.set(deviceBounds);
        }
    }
    fStateTree->appendSaveLayer(this->writeStream().bytesWritten());
    SkCanvas::SaveLayerStrategy strategy = this->INHERITED::willSaveLayer(bounds, paint, flags);
    if (paintAffectsTransparentBlack) {
        this->handleBBox(drawBounds);
        this->addNoOp();
    }
    return strategy;
}

void SkBBoxHierarchyRecord::willRestore() {
    fStateTree->appendRestore();
    this->INHERITED::willRestore();
}

void SkBBoxHierarchyRecord::didConcat(const SkMatrix& matrix) {
    fStateTree->appendTransform(getTotalMatrix());
    INHERITED::didConcat(matrix);
}

void SkBBoxHierarchyRecord::didSetMatrix(const SkMatrix& matrix) {
    fStateTree->appendTransform(getTotalMatrix());
    INHERITED::didSetMatrix(matrix);
}

void SkBBoxHierarchyRecord::onClipRect(const SkRect& rect,
                                       SkRegion::Op op,
                                       ClipEdgeStyle edgeStyle) {
    fStateTree->appendClip(this->writeStream().bytesWritten());
    this->INHERITED::onClipRect(rect, op, edgeStyle);
}

void SkBBoxHierarchyRecord::onClipRegion(const SkRegion& region,
                                         SkRegion::Op op) {
    fStateTree->appendClip(this->writeStream().bytesWritten());
    this->INHERITED::onClipRegion(region, op);
}

void SkBBoxHierarchyRecord::onClipPath(const SkPath& path,
                                       SkRegion::Op op,
                                       ClipEdgeStyle edgeStyle) {
    fStateTree->appendClip(this->writeStream().bytesWritten());
    this->INHERITED::onClipPath(path, op, edgeStyle);
}

void SkBBoxHierarchyRecord::onClipRRect(const SkRRect& rrect,
                                        SkRegion::Op op,
                                        ClipEdgeStyle edgeStyle) {
    fStateTree->appendClip(this->writeStream().bytesWritten());
    this->INHERITED::onClipRRect(rrect, op, edgeStyle);
}

bool SkBBoxHierarchyRecord::shouldRewind(void* data) {
    
    
    
    
    
    SkPictureStateTree::Draw* draw = static_cast<SkPictureStateTree::Draw*>(data);
    return draw->fOffset >= writeStream().bytesWritten();
}
