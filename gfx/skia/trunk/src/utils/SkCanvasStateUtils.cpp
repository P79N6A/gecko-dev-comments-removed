






#include "SkCanvasStateUtils.h"

#include "SkBitmapDevice.h"
#include "SkCanvas.h"
#include "SkCanvasStack.h"
#include "SkErrorInternals.h"
#include "SkWriter32.h"

#define CANVAS_STATE_VERSION 1








enum RasterConfigs {
  kUnknown_RasterConfig   = 0,
  kRGB_565_RasterConfig   = 1,
  kARGB_8888_RasterConfig = 2
};
typedef int32_t RasterConfig;

enum CanvasBackends {
    kUnknown_CanvasBackend = 0,
    kRaster_CanvasBackend  = 1,
    kGPU_CanvasBackend     = 2,
    kPDF_CanvasBackend     = 3
};
typedef int32_t CanvasBackend;

struct ClipRect {
    int32_t left, top, right, bottom;
};

struct SkMCState {
    float matrix[9];
    
    int32_t clipRectCount;
    ClipRect* clipRects;
};


struct SkCanvasLayerState {
    CanvasBackend type;
    int32_t x, y;
    int32_t width;
    int32_t height;

    SkMCState mcState;

    union {
        struct {
            RasterConfig config; 
            size_t rowBytes;     
            void* pixels;        
        } raster;
        struct {
            int32_t textureID;
        } gpu;
    };
};

class SkCanvasState {
public:
    SkCanvasState(SkCanvas* canvas) {
        SkASSERT(canvas);
        version = CANVAS_STATE_VERSION;
        width = canvas->getDeviceSize().width();
        height = canvas->getDeviceSize().height();
        layerCount = 0;
        layers = NULL;
        originalCanvas = SkRef(canvas);

        mcState.clipRectCount = 0;
        mcState.clipRects = NULL;
    }

    ~SkCanvasState() {
        
        for (int i = 0; i < layerCount; ++i) {
            sk_free(layers[i].mcState.clipRects);
        }

        sk_free(mcState.clipRects);
        sk_free(layers);

        
        
        originalCanvas->unref();
    }

    





    int32_t version;

    int32_t width;
    int32_t height;

    SkMCState mcState;

    int32_t layerCount;
    SkCanvasLayerState* layers;

private:
    SkCanvas* originalCanvas;
};



class ClipValidator : public SkCanvas::ClipVisitor {
public:
    ClipValidator() : fFailed(false) {}
    bool failed() { return fFailed; }

    
    virtual void clipRect(const SkRect& rect, SkRegion::Op op, bool antialias) SK_OVERRIDE {
        fFailed |= antialias;
    }

    virtual void clipRRect(const SkRRect& rrect, SkRegion::Op op, bool antialias) SK_OVERRIDE {
        fFailed |= antialias;
    }

    virtual void clipPath(const SkPath&, SkRegion::Op, bool antialias) SK_OVERRIDE {
        fFailed |= antialias;
    }

private:
    bool fFailed;
};

static void setup_MC_state(SkMCState* state, const SkMatrix& matrix, const SkRegion& clip) {
    
    state->clipRectCount = 0;

    
    for (int i = 0; i < 9; i++) {
        state->matrix[i] = matrix.get(i);
    }

    







    SkSWriter32<4*sizeof(ClipRect)> clipWriter;

    if (!clip.isEmpty()) {
        
        SkRegion::Iterator clip_iterator(clip);
        for (; !clip_iterator.done(); clip_iterator.next()) {
            
            
            clipWriter.writeIRect(clip_iterator.rect());
            state->clipRectCount++;
        }
    }

    
    state->clipRects = (ClipRect*) sk_malloc_throw(clipWriter.bytesWritten());
    clipWriter.flatten(state->clipRects);
}



SkCanvasState* SkCanvasStateUtils::CaptureCanvasState(SkCanvas* canvas) {
    SkASSERT(canvas);

    
    ClipValidator validator;
    canvas->replayClips(&validator);
    if (validator.failed()) {
        SkErrorInternals::SetError(kInvalidOperation_SkError,
                "CaptureCanvasState does not support canvases with antialiased clips.\n");
        return NULL;
    }

    SkAutoTDelete<SkCanvasState> canvasState(SkNEW_ARGS(SkCanvasState, (canvas)));

    
    setup_MC_state(&canvasState->mcState, canvas->getTotalMatrix(),
                   canvas->internal_private_getTotalClip());

    






    SkSWriter32<3*sizeof(SkCanvasLayerState)> layerWriter;
    int layerCount = 0;
    for (SkCanvas::LayerIter layer(canvas, true); !layer.done(); layer.next()) {

        
        const SkBitmap& bitmap = layer.device()->accessBitmap(true);
        if (bitmap.empty() || bitmap.isNull() || !bitmap.lockPixelsAreWritable()) {
            return NULL;
        }

        SkCanvasLayerState* layerState =
                (SkCanvasLayerState*) layerWriter.reserve(sizeof(SkCanvasLayerState));
        layerState->type = kRaster_CanvasBackend;
        layerState->x = layer.x();
        layerState->y = layer.y();
        layerState->width = bitmap.width();
        layerState->height = bitmap.height();

        switch (bitmap.colorType()) {
            case kPMColor_SkColorType:
                layerState->raster.config = kARGB_8888_RasterConfig;
                break;
            case kRGB_565_SkColorType:
                layerState->raster.config = kRGB_565_RasterConfig;
                break;
            default:
                return NULL;
        }
        layerState->raster.rowBytes = bitmap.rowBytes();
        layerState->raster.pixels = bitmap.getPixels();

        setup_MC_state(&layerState->mcState, layer.matrix(), layer.clip());
        layerCount++;
    }

    
    SkASSERT(layerWriter.bytesWritten() == layerCount * sizeof(SkCanvasLayerState));
    canvasState->layerCount = layerCount;
    canvasState->layers = (SkCanvasLayerState*) sk_malloc_throw(layerWriter.bytesWritten());
    layerWriter.flatten(canvasState->layers);

    
    if (canvas->getDrawFilter()) {

    }

    return canvasState.detach();
}



static void setup_canvas_from_MC_state(const SkMCState& state, SkCanvas* canvas) {
    
    SkMatrix matrix;
    for (int i = 0; i < 9; i++) {
        matrix.set(i, state.matrix[i]);
    }

    
    SkRegion clip;
    for (int i = 0; i < state.clipRectCount; ++i) {
        clip.op(SkIRect::MakeLTRB(state.clipRects[i].left,
                                  state.clipRects[i].top,
                                  state.clipRects[i].right,
                                  state.clipRects[i].bottom),
                SkRegion::kUnion_Op);
    }

    canvas->setMatrix(matrix);
    canvas->setClipRegion(clip);
}

static SkCanvas* create_canvas_from_canvas_layer(const SkCanvasLayerState& layerState) {
    SkASSERT(kRaster_CanvasBackend == layerState.type);

    SkBitmap bitmap;
    SkColorType colorType =
        layerState.raster.config == kARGB_8888_RasterConfig ? kPMColor_SkColorType :
        layerState.raster.config == kRGB_565_RasterConfig ? kRGB_565_SkColorType :
        kUnknown_SkColorType;

    if (colorType == kUnknown_SkColorType) {
        return NULL;
    }

    bitmap.installPixels(SkImageInfo::Make(layerState.width, layerState.height,
                                           colorType, kPremul_SkAlphaType),
                         layerState.raster.pixels, layerState.raster.rowBytes,
                         NULL, NULL);

    SkASSERT(!bitmap.empty());
    SkASSERT(!bitmap.isNull());

    SkAutoTUnref<SkCanvas> canvas(SkNEW_ARGS(SkCanvas, (bitmap)));

    
    setup_canvas_from_MC_state(layerState.mcState, canvas.get());

    return canvas.detach();
}

SkCanvas* SkCanvasStateUtils::CreateFromCanvasState(const SkCanvasState* state) {
    SkASSERT(state);

    
    if (CANVAS_STATE_VERSION != state->version) {
        SkDebugf("CreateFromCanvasState version does not match the one use to create the input");
        return NULL;
    }

    if (state->layerCount < 1) {
        return NULL;
    }

    SkAutoTUnref<SkCanvasStack> canvas(SkNEW_ARGS(SkCanvasStack, (state->width, state->height)));

    
    setup_canvas_from_MC_state(state->mcState, canvas);

    
    for (int i = state->layerCount - 1; i >= 0; --i) {
        SkAutoTUnref<SkCanvas> canvasLayer(create_canvas_from_canvas_layer(state->layers[i]));
        if (!canvasLayer.get()) {
            return NULL;
        }
        canvas->pushCanvas(canvasLayer.get(), SkIPoint::Make(state->layers[i].x,
                                                             state->layers[i].y));
    }

    return canvas.detach();
}



void SkCanvasStateUtils::ReleaseCanvasState(SkCanvasState* state) {
    SkDELETE(state);
}
