






#ifndef SkRecords_DEFINED
#define SkRecords_DEFINED

#include "SkCanvas.h"

namespace SkRecords {











#define SK_RECORD_TYPES(M)                                          \
    M(NoOp)                                                         \
    M(Restore)                                                      \
    M(Save)                                                         \
    M(SaveLayer)                                                    \
    M(PushCull)                                                     \
    M(PopCull)                                                      \
    M(PairedPushCull)         /*From SkRecordAnnotateCullingPairs*/ \
    M(Concat)                                                       \
    M(SetMatrix)                                                    \
    M(ClipPath)                                                     \
    M(ClipRRect)                                                    \
    M(ClipRect)                                                     \
    M(ClipRegion)                                                   \
    M(Clear)                                                        \
    M(DrawBitmap)                                                   \
    M(DrawBitmapMatrix)                                             \
    M(DrawBitmapNine)                                               \
    M(DrawBitmapRectToRect)                                         \
    M(DrawDRRect)                                                   \
    M(DrawOval)                                                     \
    M(DrawPaint)                                                    \
    M(DrawPath)                                                     \
    M(DrawPoints)                                                   \
    M(DrawPosText)                                                  \
    M(DrawPosTextH)                                                 \
    M(DrawRRect)                                                    \
    M(DrawRect)                                                     \
    M(DrawSprite)                                                   \
    M(DrawText)                                                     \
    M(DrawTextOnPath)                                               \
    M(DrawVertices)                                                 \
    M(BoundedDrawPosTextH)    /*From SkRecordBoundDrawPosTextH*/


#define ENUM(T) T##_Type,
enum Type { SK_RECORD_TYPES(ENUM) };
#undef ENUM



#define RECORD0(T)                      \
struct T {                              \
    static const Type kType = T##_Type; \
};





#define RECORD1(T, A, a)                \
struct T {                              \
    static const Type kType = T##_Type; \
    template <typename Z>               \
    T(Z a) : a(a) {}                    \
    A a;                                \
};

#define RECORD2(T, A, a, B, b)          \
struct T {                              \
    static const Type kType = T##_Type; \
    template <typename Z, typename Y>   \
    T(Z a, Y b) : a(a), b(b) {}         \
    A a; B b;                           \
};

#define RECORD3(T, A, a, B, b, C, c)              \
struct T {                                        \
    static const Type kType = T##_Type;           \
    template <typename Z, typename Y, typename X> \
    T(Z a, Y b, X c) : a(a), b(b), c(c) {}        \
    A a; B b; C c;                                \
};

#define RECORD4(T, A, a, B, b, C, c, D, d)                    \
struct T {                                                    \
    static const Type kType = T##_Type;                       \
    template <typename Z, typename Y, typename X, typename W> \
    T(Z a, Y b, X c, W d) : a(a), b(b), c(c), d(d) {}         \
    A a; B b; C c; D d;                                       \
};

#define RECORD5(T, A, a, B, b, C, c, D, d, E, e)                          \
struct T {                                                                \
    static const Type kType = T##_Type;                                   \
    template <typename Z, typename Y, typename X, typename W, typename V> \
    T(Z a, Y b, X c, W d, V e) : a(a), b(b), c(c), d(d), e(e) {}          \
    A a; B b; C c; D d; E e;                                              \
};

#define ACT_AS_PTR(ptr)                       \
    operator T*() { return ptr; }             \
    operator const T*() const { return ptr; } \
    T* operator->() { return ptr; }           \
    const T* operator->() const { return ptr; }


template <typename T>
class Optional : SkNoncopyable {
public:
    Optional(T* ptr) : fPtr(ptr) {}
    ~Optional() { if (fPtr) fPtr->~T(); }

    ACT_AS_PTR(fPtr);
private:
    T* fPtr;
};


template <typename T>
class Adopted : SkNoncopyable {
public:
    Adopted(T* ptr) : fPtr(ptr) { SkASSERT(fPtr); }
    Adopted(Adopted* source) {
        
        fPtr = source->fPtr;
        source->fPtr = NULL;
    }
    ~Adopted() { if (fPtr) fPtr->~T(); }

    ACT_AS_PTR(fPtr);
private:
    T* fPtr;
};


template <typename T>
class PODArray {
public:
    PODArray(T* ptr) : fPtr(ptr) {}
    

    ACT_AS_PTR(fPtr);
private:
    T* fPtr;
};

#undef ACT_AS_PTR



class ImmutableBitmap {
public:
    explicit ImmutableBitmap(const SkBitmap& bitmap) {
        if (bitmap.isImmutable()) {
            fBitmap = bitmap;
        } else {
            bitmap.copyTo(&fBitmap);
        }
        fBitmap.setImmutable();
    }

    operator const SkBitmap& () const { return fBitmap; }

private:
    SkBitmap fBitmap;
};

RECORD0(NoOp);

RECORD0(Restore);
RECORD0(Save);
RECORD3(SaveLayer, Optional<SkRect>, bounds, Optional<SkPaint>, paint, SkCanvas::SaveFlags, flags);

RECORD1(PushCull, SkRect, rect);
RECORD0(PopCull);

RECORD1(Concat, SkMatrix, matrix);
RECORD1(SetMatrix, SkMatrix, matrix);

RECORD3(ClipPath, SkPath, path, SkRegion::Op, op, bool, doAA);
RECORD3(ClipRRect, SkRRect, rrect, SkRegion::Op, op, bool, doAA);
RECORD3(ClipRect, SkRect, rect, SkRegion::Op, op, bool, doAA);
RECORD2(ClipRegion, SkRegion, region, SkRegion::Op, op);

RECORD1(Clear, SkColor, color);

RECORD4(DrawBitmap, Optional<SkPaint>, paint,
                    ImmutableBitmap, bitmap,
                    SkScalar, left,
                    SkScalar, top);
RECORD3(DrawBitmapMatrix, Optional<SkPaint>, paint, ImmutableBitmap, bitmap, SkMatrix, matrix);
RECORD4(DrawBitmapNine, Optional<SkPaint>, paint,
                        ImmutableBitmap, bitmap,
                        SkIRect, center,
                        SkRect, dst);
RECORD5(DrawBitmapRectToRect, Optional<SkPaint>, paint,
                              ImmutableBitmap, bitmap,
                              Optional<SkRect>, src,
                              SkRect, dst,
                              SkCanvas::DrawBitmapRectFlags, flags);
RECORD3(DrawDRRect, SkPaint, paint, SkRRect, outer, SkRRect, inner);
RECORD2(DrawOval, SkPaint, paint, SkRect, oval);
RECORD1(DrawPaint, SkPaint, paint);
RECORD2(DrawPath, SkPaint, paint, SkPath, path);
RECORD4(DrawPoints, SkPaint, paint, SkCanvas::PointMode, mode, size_t, count, SkPoint*, pts);
RECORD4(DrawPosText, SkPaint, paint,
                     PODArray<char>, text,
                     size_t, byteLength,
                     PODArray<SkPoint>, pos);
RECORD5(DrawPosTextH, SkPaint, paint,
                      PODArray<char>, text,
                      size_t, byteLength,
                      PODArray<SkScalar>, xpos,
                      SkScalar, y);
RECORD2(DrawRRect, SkPaint, paint, SkRRect, rrect);
RECORD2(DrawRect, SkPaint, paint, SkRect, rect);
RECORD4(DrawSprite, Optional<SkPaint>, paint, ImmutableBitmap, bitmap, int, left, int, top);
RECORD5(DrawText, SkPaint, paint,
                  PODArray<char>, text,
                  size_t, byteLength,
                  SkScalar, x,
                  SkScalar, y);
RECORD5(DrawTextOnPath, SkPaint, paint,
                        PODArray<char>, text,
                        size_t, byteLength,
                        SkPath, path,
                        Optional<SkMatrix>, matrix);


struct DrawVertices {
    static const Type kType = DrawVertices_Type;

    DrawVertices(const SkPaint& paint,
                 SkCanvas::VertexMode vmode,
                 int vertexCount,
                 SkPoint* vertices,
                 SkPoint* texs,
                 SkColor* colors,
                 SkXfermode* xmode,
                 uint16_t* indices,
                 int indexCount)
        : paint(paint)
        , vmode(vmode)
        , vertexCount(vertexCount)
        , vertices(vertices)
        , texs(texs)
        , colors(colors)
        , xmode(SkSafeRef(xmode))
        , indices(indices)
        , indexCount(indexCount) {}

    SkPaint paint;
    SkCanvas::VertexMode vmode;
    int vertexCount;
    PODArray<SkPoint> vertices;
    PODArray<SkPoint> texs;
    PODArray<SkColor> colors;
    SkAutoTUnref<SkXfermode> xmode;
    PODArray<uint16_t> indices;
    int indexCount;
};


RECORD2(PairedPushCull, Adopted<PushCull>, base, unsigned, skip);
RECORD3(BoundedDrawPosTextH, Adopted<DrawPosTextH>, base, SkScalar, minY, SkScalar, maxY);

#undef RECORD0
#undef RECORD1
#undef RECORD2
#undef RECORD3
#undef RECORD4
#undef RECORD5

}  

#endif
