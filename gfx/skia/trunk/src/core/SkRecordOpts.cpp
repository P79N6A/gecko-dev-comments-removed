






#include "SkRecordOpts.h"

#include "SkRecordPattern.h"
#include "SkRecords.h"
#include "SkTDArray.h"

using namespace SkRecords;

void SkRecordOptimize(SkRecord* record) {
    
    SkRecordNoopCulls(record);
    SkRecordNoopSaveRestores(record);
    
    

    SkRecordAnnotateCullingPairs(record);
    SkRecordReduceDrawPosTextStrength(record);  
    SkRecordBoundDrawPosTextH(record);
}









template <typename Pass>
static bool apply(Pass* pass, SkRecord* record) {
    typename Pass::Pattern pattern;
    bool changed = false;
    unsigned begin, end = 0;

    while (pattern.search(record, &begin, &end)) {
        changed |= pass->onMatch(record, &pattern, begin, end);
    }
    return changed;
}

struct CullNooper {
    typedef Pattern3<Is<PushCull>, Star<Is<NoOp> >, Is<PopCull> > Pattern;

    bool onMatch(SkRecord* record, Pattern* pattern, unsigned begin, unsigned end) {
        record->replace<NoOp>(begin);  
        record->replace<NoOp>(end-1);  
        return true;
    }
};

void SkRecordNoopCulls(SkRecord* record) {
    CullNooper pass;
    while (apply(&pass, record));
}


struct SaveOnlyDrawsRestoreNooper {
    typedef Pattern3<Is<Save>,
                     Star<Or<Is<NoOp>, IsDraw> >,
                     Is<Restore> >
        Pattern;

    bool onMatch(SkRecord* record, Pattern* pattern, unsigned begin, unsigned end) {
        record->replace<NoOp>(begin);  
        record->replace<NoOp>(end-1);  
        return true;
    }
};

struct SaveNoDrawsRestoreNooper {
    
    typedef Pattern3<Is<Save>,
                     Star<Not<Or3<Is<Save>,
                                  Is<Restore>,
                                  IsDraw> > >,
                     Is<Restore> >
        Pattern;

    bool onMatch(SkRecord* record, Pattern* pattern, unsigned begin, unsigned end) {
        
        for (unsigned i = begin; i < end; i++) {
            record->replace<NoOp>(i);
        }
        return true;
    }
};
void SkRecordNoopSaveRestores(SkRecord* record) {
    SaveOnlyDrawsRestoreNooper onlyDraws;
    SaveNoDrawsRestoreNooper noDraws;

    
    while (apply(&onlyDraws, record) || apply(&noDraws, record));
}



struct SaveLayerDrawRestoreNooper {
    typedef Pattern3<Is<SaveLayer>, IsDraw, Is<Restore> > Pattern;

    bool onMatch(SkRecord* record, Pattern* pattern, unsigned begin, unsigned end) {
        SaveLayer* saveLayer = pattern->first<SaveLayer>();
        if (saveLayer->bounds != NULL) {
            
            return false;
        }

        SkPaint* layerPaint = saveLayer->paint;
        if (NULL == layerPaint) {
            
            return KillSaveLayerAndRestore(record, begin);
        }

        SkPaint* drawPaint = pattern->second<SkPaint>();
        if (drawPaint == NULL) {
            
            
            return false;
        }

        const uint32_t layerColor = layerPaint->getColor();
        const uint32_t  drawColor =  drawPaint->getColor();
        if (!IsOnlyAlpha(layerColor)  || !IsOpaque(drawColor) ||
            HasAnyEffect(*layerPaint) || HasAnyEffect(*drawPaint)) {
            
            
            return false;
        }

        drawPaint->setColor(SkColorSetA(drawColor, SkColorGetA(layerColor)));
        return KillSaveLayerAndRestore(record, begin);
    }

    static bool KillSaveLayerAndRestore(SkRecord* record, unsigned saveLayerIndex) {
        record->replace<NoOp>(saveLayerIndex);    
        record->replace<NoOp>(saveLayerIndex+2);  
        return true;
    }

    static bool HasAnyEffect(const SkPaint& paint) {
        return paint.getPathEffect()  ||
               paint.getShader()      ||
               paint.getXfermode()    ||
               paint.getMaskFilter()  ||
               paint.getColorFilter() ||
               paint.getRasterizer()  ||
               paint.getLooper()      ||
               paint.getImageFilter();
    }

    static bool IsOpaque(SkColor color) {
        return SkColorGetA(color) == SK_AlphaOPAQUE;
    }
    static bool IsOnlyAlpha(SkColor color) {
        return SK_ColorTRANSPARENT == SkColorSetA(color, SK_AlphaTRANSPARENT);
    }
};
void SkRecordNoopSaveLayerDrawRestores(SkRecord* record) {
    SaveLayerDrawRestoreNooper pass;
    apply(&pass, record);
}



struct StrengthReducer {
    typedef Pattern1<Is<DrawPosText> > Pattern;

    bool onMatch(SkRecord* record, Pattern* pattern, unsigned begin, unsigned end) {
        SkASSERT(end == begin + 1);
        DrawPosText* draw = pattern->first<DrawPosText>();

        const unsigned points = draw->paint.countText(draw->text, draw->byteLength);
        if (points == 0) {
            return false;  
        }

        const SkScalar firstY = draw->pos[0].fY;
        for (unsigned i = 1; i < points; i++) {
            if (draw->pos[i].fY != firstY) {
                return false;  
            }
        }
        

        
        
        
        SK_COMPILE_ASSERT(sizeof(SkPoint) == 2 * sizeof(SkScalar), SquintingIsNotSafe);
        SkScalar* scalars = &draw->pos[0].fX;
        for (unsigned i = 0; i < 2*points; i += 2) {
            scalars[i/2] = scalars[i];
        }

        
        Adopted<DrawPosText> adopted(draw);
        SkNEW_PLACEMENT_ARGS(record->replace<DrawPosTextH>(begin, adopted),
                             DrawPosTextH,
                             (draw->paint, draw->text, draw->byteLength, scalars, firstY));
        return true;
    }
};
void SkRecordReduceDrawPosTextStrength(SkRecord* record) {
    StrengthReducer pass;
    apply(&pass, record);
}



struct TextBounder {
    typedef Pattern1<Is<DrawPosTextH> > Pattern;

    bool onMatch(SkRecord* record, Pattern* pattern, unsigned begin, unsigned end) {
        SkASSERT(end == begin + 1);
        DrawPosTextH* draw = pattern->first<DrawPosTextH>();

        
        
        if (draw->paint.isVerticalText() || !draw->paint.canComputeFastBounds()) {
            return false;
        }

        
        
        const SkScalar buffer = draw->paint.getTextSize() * 1.5f;
        SkDEBUGCODE(SkPaint::FontMetrics metrics;)
        SkDEBUGCODE(draw->paint.getFontMetrics(&metrics);)
        SkASSERT(-buffer <= metrics.fTop);
        SkASSERT(+buffer >= metrics.fBottom);

        
        
        SkRect bounds;
        bounds.set(0, draw->y - buffer, SK_Scalar1, draw->y + buffer);
        SkRect adjusted = draw->paint.computeFastBounds(bounds, &bounds);

        Adopted<DrawPosTextH> adopted(draw);
        SkNEW_PLACEMENT_ARGS(record->replace<BoundedDrawPosTextH>(begin, adopted),
                             BoundedDrawPosTextH,
                             (&adopted, adjusted.fTop, adjusted.fBottom));
        return true;
    }
};
void SkRecordBoundDrawPosTextH(SkRecord* record) {
    TextBounder pass;
    apply(&pass, record);
}




class CullAnnotator {
public:
    
    template <typename T> void operator()(T*) {}

    void operator()(PushCull* push) {
        Pair pair = { fIndex, push };
        fPushStack.push(pair);
    }

    void operator()(PopCull* pop) {
        Pair push = fPushStack.top();
        fPushStack.pop();

        SkASSERT(fIndex > push.index);
        unsigned skip = fIndex - push.index;

        Adopted<PushCull> adopted(push.command);
        SkNEW_PLACEMENT_ARGS(fRecord->replace<PairedPushCull>(push.index, adopted),
                             PairedPushCull, (&adopted, skip));
    }

    void apply(SkRecord* record) {
        for (fRecord = record, fIndex = 0; fIndex < record->count(); fIndex++) {
            fRecord->mutate<void>(fIndex, *this);
        }
    }

private:
    struct Pair {
        unsigned index;
        PushCull* command;
    };

    SkTDArray<Pair> fPushStack;
    SkRecord* fRecord;
    unsigned fIndex;
};
void SkRecordAnnotateCullingPairs(SkRecord* record) {
    CullAnnotator pass;
    pass.apply(record);
}
