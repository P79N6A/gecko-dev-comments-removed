








#ifndef SKDEBUGCANVAS_H_
#define SKDEBUGCANVAS_H_

#include "SkCanvas.h"
#include "SkDrawCommand.h"
#include "SkPicture.h"
#include "SkTArray.h"
#include "SkString.h"

class SkTexOverrideFilter;

class SK_API SkDebugCanvas : public SkCanvas {
public:
    SkDebugCanvas(int width, int height);
    virtual ~SkDebugCanvas();

    void toggleFilter(bool toggle) { fFilter = toggle; }

    void setMegaVizMode(bool megaVizMode) { fMegaVizMode = megaVizMode; }

    


    void setOverdrawViz(bool overdrawViz) { fOverdrawViz = overdrawViz; }

    


    void overrideTexFiltering(bool overrideTexFiltering, SkPaint::FilterLevel level);

    



    void draw(SkCanvas* canvas);

    




    void drawTo(SkCanvas* canvas, int index);

    


    const SkMatrix& getCurrentMatrix() {
        return fMatrix;
    }

    


    const SkIRect& getCurrentClip() {
        return fClip;
    }

    


    int getCommandAtPoint(int x, int y, int index);

    



    void deleteDrawCommandAt(int index);

    



    SkDrawCommand* getDrawCommandAt(int index);

    




    void setDrawCommandAt(int index, SkDrawCommand* command);

    



    SkTDArray<SkString*>* getCommandInfo(int index);

    



    bool getDrawCommandVisibilityAt(int index);

    


    SK_ATTR_DEPRECATED("please use getDrawCommandAt and getSize instead")
    const SkTDArray<SkDrawCommand*>& getDrawCommands() const;

    



    SkTDArray<SkDrawCommand*>& getDrawCommands();

    


    SkTArray<SkString>* getDrawCommandsAsStrings() const;

    


    int getSize() const {
        return fCommandVector.count();
    }

    



    void toggleCommand(int index, bool toggle);

    void setBounds(int width, int height) {
        fWidth = width;
        fHeight = height;
    }

    void setUserMatrix(SkMatrix matrix) {
        fUserMatrix = matrix;
    }





    virtual void clear(SkColor) SK_OVERRIDE;

    virtual void drawBitmap(const SkBitmap&, SkScalar left, SkScalar top,
                            const SkPaint*) SK_OVERRIDE;

    virtual void drawBitmapRectToRect(const SkBitmap&, const SkRect* src,
                                      const SkRect& dst, const SkPaint* paint,
                                      DrawBitmapRectFlags flags) SK_OVERRIDE;

    virtual void drawBitmapMatrix(const SkBitmap&, const SkMatrix&,
                                  const SkPaint*) SK_OVERRIDE;

    virtual void drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint*) SK_OVERRIDE;

    virtual void drawData(const void*, size_t) SK_OVERRIDE;

    virtual void beginCommentGroup(const char* description) SK_OVERRIDE;

    virtual void addComment(const char* kywd, const char* value) SK_OVERRIDE;

    virtual void endCommentGroup() SK_OVERRIDE;

    virtual void drawOval(const SkRect& oval, const SkPaint&) SK_OVERRIDE;

    virtual void drawPaint(const SkPaint& paint) SK_OVERRIDE;

    virtual void drawPath(const SkPath& path, const SkPaint&) SK_OVERRIDE;

    virtual void drawPicture(SkPicture& picture) SK_OVERRIDE;

    virtual void drawPoints(PointMode, size_t count, const SkPoint pts[],
                            const SkPaint&) SK_OVERRIDE;

    virtual void drawPosText(const void* text, size_t byteLength,
                             const SkPoint pos[], const SkPaint&) SK_OVERRIDE;

    virtual void drawPosTextH(const void* text, size_t byteLength,
                              const SkScalar xpos[], SkScalar constY,
                              const SkPaint&) SK_OVERRIDE;

    virtual void drawRect(const SkRect& rect, const SkPaint&) SK_OVERRIDE;

    virtual void drawRRect(const SkRRect& rrect, const SkPaint& paint) SK_OVERRIDE;

    virtual void drawSprite(const SkBitmap&, int left, int top,
                            const SkPaint*) SK_OVERRIDE;

    virtual void drawText(const void* text, size_t byteLength, SkScalar x,
                          SkScalar y, const SkPaint&) SK_OVERRIDE;

    virtual void drawTextOnPath(const void* text, size_t byteLength,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint&) SK_OVERRIDE;

    virtual void drawVertices(VertexMode, int vertexCount,
                              const SkPoint vertices[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode*,
                              const uint16_t indices[], int indexCount,
                              const SkPaint&) SK_OVERRIDE;

    static const int kVizImageHeight = 256;
    static const int kVizImageWidth = 256;

    virtual bool isClipEmpty() const SK_OVERRIDE { return false; }
    virtual bool isClipRect() const SK_OVERRIDE { return true; }
#ifdef SK_SUPPORT_LEGACY_GETCLIPTYPE
    virtual ClipType getClipType() const SK_OVERRIDE {
        return kRect_ClipType;
    }
#endif
    virtual bool getClipBounds(SkRect* bounds) const SK_OVERRIDE {
        if (NULL != bounds) {
            bounds->setXYWH(0, 0,
                            SkIntToScalar(this->imageInfo().fWidth),
                            SkIntToScalar(this->imageInfo().fHeight));
        }
        return true;
    }
    virtual bool getClipDeviceBounds(SkIRect* bounds) const SK_OVERRIDE {
        if (NULL != bounds) {
            bounds->setLargest();
        }
        return true;
    }

protected:
    virtual void willSave(SaveFlags) SK_OVERRIDE;
    virtual SaveLayerStrategy willSaveLayer(const SkRect*, const SkPaint*, SaveFlags) SK_OVERRIDE;
    virtual void willRestore() SK_OVERRIDE;

    virtual void didTranslate(SkScalar, SkScalar) SK_OVERRIDE;
    virtual void didScale(SkScalar, SkScalar) SK_OVERRIDE;
    virtual void didRotate(SkScalar) SK_OVERRIDE;
    virtual void didSkew(SkScalar, SkScalar) SK_OVERRIDE;
    virtual void didConcat(const SkMatrix&) SK_OVERRIDE;
    virtual void didSetMatrix(const SkMatrix&) SK_OVERRIDE;

    virtual void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) SK_OVERRIDE;
    virtual void onPushCull(const SkRect& cullRect) SK_OVERRIDE;
    virtual void onPopCull() SK_OVERRIDE;

    virtual void onClipRect(const SkRect&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipRRect(const SkRRect&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipPath(const SkPath&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipRegion(const SkRegion& region, SkRegion::Op) SK_OVERRIDE;

    void markActiveCommands(int index);

private:
    SkTDArray<SkDrawCommand*> fCommandVector;
    int fWidth;
    int fHeight;
    bool fFilter;
    bool fMegaVizMode;
    int fIndex;
    SkMatrix fUserMatrix;
    SkMatrix fMatrix;
    SkIRect fClip;

    bool fOverdrawViz;
    SkDrawFilter* fOverdrawFilter;

    bool fOverrideTexFiltering;
    SkTexOverrideFilter* fTexOverrideFilter;

    





    int fOutstandingSaveCount;

    



    SkTDArray<SkDrawCommand*> fActiveLayers;

    



    SkTDArray<SkDrawCommand*> fActiveCulls;

    



    void addDrawCommand(SkDrawCommand* command);

    



    void applyUserTransform(SkCanvas* canvas);

    typedef SkCanvas INHERITED;
};

#endif
