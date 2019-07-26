






#ifndef SKOBJECTPARSER_H_
#define SKOBJECTPARSER_H_

#include "SkCanvas.h"
#include "SkString.h"






class SkObjectParser {
public:

    



    static SkString* BitmapToString(const SkBitmap& bitmap);

    



    static SkString* BoolToString(bool doAA);

    


    static SkString* CustomTextToString(const char* text);

    





    static SkString* IntToString(int x, const char* text);
    



    static SkString* IRectToString(const SkIRect& rect);

    



    static SkString* MatrixToString(const SkMatrix& matrix);

    



    static SkString* PaintToString(const SkPaint& paint);

    



    static SkString* PathToString(const SkPath& path);

    




    static SkString* PointsToString(const SkPoint pts[], size_t count);

    


    static SkString* PointModeToString(SkCanvas::PointMode mode);

    



    static SkString* RectToString(const SkRect& rect, const char* title = NULL);

    



    static SkString* RRectToString(const SkRRect& rrect, const char* title = NULL);

    



    static SkString* RegionOpToString(SkRegion::Op op);

    



    static SkString* RegionToString(const SkRegion& region);

    



    static SkString* SaveFlagsToString(SkCanvas::SaveFlags flags);

    





    static SkString* ScalarToString(SkScalar x, const char* text);

    



    static SkString* TextToString(const void* text, size_t byteLength,
                                  SkPaint::TextEncoding encoding);
};

#endif
