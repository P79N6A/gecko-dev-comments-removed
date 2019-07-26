






#include "SkCanvasStack.h"

SkCanvasStack::SkCanvasStack(int width, int height)
        : INHERITED(width, height) {}

SkCanvasStack::~SkCanvasStack() {
    this->removeAll();
}

void SkCanvasStack::pushCanvas(SkCanvas* canvas, const SkIPoint& origin) {
    if (canvas) {
        
        const SkIRect canvasBounds = SkIRect::MakeSize(canvas->getDeviceSize());

        
        this->INHERITED::addCanvas(canvas);

        
        CanvasData* data = &fCanvasData.push_back();
        data->origin = origin;
        data->requiredClip.setRect(canvasBounds);

        
        
        
        for (int i = fList.count() - 1; i > 0; --i) {
            SkIRect localBounds = canvasBounds;
            localBounds.offset(origin - fCanvasData[i-1].origin);

            fCanvasData[i-1].requiredClip.op(localBounds, SkRegion::kDifference_Op);
            fList[i-1]->clipRegion(fCanvasData[i-1].requiredClip);
        }
    }
    SkASSERT(fList.count() == fCanvasData.count());
}

void SkCanvasStack::removeAll() {
    fCanvasData.reset();
    this->INHERITED::removeAll();
}






void SkCanvasStack::clipToZOrderedBounds() {
    SkASSERT(fList.count() == fCanvasData.count());
    for (int i = 0; i < fList.count(); ++i) {
        fList[i]->clipRegion(fCanvasData[i].requiredClip, SkRegion::kIntersect_Op);
    }
}








void SkCanvasStack::didSetMatrix(const SkMatrix& matrix) {
    SkASSERT(fList.count() == fCanvasData.count());
    for (int i = 0; i < fList.count(); ++i) {

        SkMatrix tempMatrix = matrix;
        tempMatrix.postTranslate(SkIntToScalar(-fCanvasData[i].origin.x()),
                                 SkIntToScalar(-fCanvasData[i].origin.y()));
        fList[i]->setMatrix(tempMatrix);
    }
    this->SkCanvas::didSetMatrix(matrix);
}

void SkCanvasStack::onClipRect(const SkRect& r, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->INHERITED::onClipRect(r, op, edgeStyle);
    this->clipToZOrderedBounds();
}

void SkCanvasStack::onClipRRect(const SkRRect& rr, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->INHERITED::onClipRRect(rr, op, edgeStyle);
    this->clipToZOrderedBounds();
}

void SkCanvasStack::onClipPath(const SkPath& p, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->INHERITED::onClipPath(p, op, edgeStyle);
    this->clipToZOrderedBounds();
}

void SkCanvasStack::onClipRegion(const SkRegion& deviceRgn, SkRegion::Op op) {
    SkASSERT(fList.count() == fCanvasData.count());
    for (int i = 0; i < fList.count(); ++i) {
        SkRegion tempRegion;
        deviceRgn.translate(-fCanvasData[i].origin.x(),
                            -fCanvasData[i].origin.y(), &tempRegion);
        tempRegion.op(fCanvasData[i].requiredClip, SkRegion::kIntersect_Op);
        fList[i]->clipRegion(tempRegion, op);
    }
    this->SkCanvas::onClipRegion(deviceRgn, op);
}
