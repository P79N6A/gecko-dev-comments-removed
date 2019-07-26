








#include "SkData.h"
#include "SkGeometry.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkPDFUtils.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkPDFTypes.h"


SkPDFArray* SkPDFUtils::MatrixToArray(const SkMatrix& matrix) {
    SkScalar values[6];
    if (!matrix.asAffine(values)) {
        SkMatrix::SetAffineIdentity(values);
    }

    SkPDFArray* result = new SkPDFArray;
    result->reserve(6);
    for (size_t i = 0; i < SK_ARRAY_COUNT(values); i++) {
        result->appendScalar(values[i]);
    }
    return result;
}


void SkPDFUtils::AppendTransform(const SkMatrix& matrix, SkWStream* content) {
    SkScalar values[6];
    if (!matrix.asAffine(values)) {
        SkMatrix::SetAffineIdentity(values);
    }
    for (size_t i = 0; i < SK_ARRAY_COUNT(values); i++) {
        SkPDFScalar::Append(values[i], content);
        content->writeText(" ");
    }
    content->writeText("cm\n");
}


void SkPDFUtils::MoveTo(SkScalar x, SkScalar y, SkWStream* content) {
    SkPDFScalar::Append(x, content);
    content->writeText(" ");
    SkPDFScalar::Append(y, content);
    content->writeText(" m\n");
}


void SkPDFUtils::AppendLine(SkScalar x, SkScalar y, SkWStream* content) {
    SkPDFScalar::Append(x, content);
    content->writeText(" ");
    SkPDFScalar::Append(y, content);
    content->writeText(" l\n");
}


void SkPDFUtils::AppendCubic(SkScalar ctl1X, SkScalar ctl1Y,
                             SkScalar ctl2X, SkScalar ctl2Y,
                             SkScalar dstX, SkScalar dstY, SkWStream* content) {
    SkString cmd("y\n");
    SkPDFScalar::Append(ctl1X, content);
    content->writeText(" ");
    SkPDFScalar::Append(ctl1Y, content);
    content->writeText(" ");
    if (ctl2X != dstX || ctl2Y != dstY) {
        cmd.set("c\n");
        SkPDFScalar::Append(ctl2X, content);
        content->writeText(" ");
        SkPDFScalar::Append(ctl2Y, content);
        content->writeText(" ");
    }
    SkPDFScalar::Append(dstX, content);
    content->writeText(" ");
    SkPDFScalar::Append(dstY, content);
    content->writeText(" ");
    content->writeText(cmd.c_str());
}


void SkPDFUtils::AppendRectangle(const SkRect& rect, SkWStream* content) {
    
    SkScalar bottom = SkMinScalar(rect.fBottom, rect.fTop);

    SkPDFScalar::Append(rect.fLeft, content);
    content->writeText(" ");
    SkPDFScalar::Append(bottom, content);
    content->writeText(" ");
    SkPDFScalar::Append(rect.width(), content);
    content->writeText(" ");
    SkPDFScalar::Append(rect.height(), content);
    content->writeText(" re\n");
}


void SkPDFUtils::EmitPath(const SkPath& path, SkPaint::Style paintStyle,
                          SkWStream* content) {
    
    
    
    
    enum SkipFillState {
        kEmpty_SkipFillState         = 0,
        kSingleLine_SkipFillState    = 1,
        kNonSingleLine_SkipFillState = 2,
    };
    SkipFillState fillState = kEmpty_SkipFillState;
    if (paintStyle != SkPaint::kFill_Style) {
        fillState = kNonSingleLine_SkipFillState;
    }
    SkPoint lastMovePt = SkPoint::Make(0,0);
    SkDynamicMemoryWStream currentSegment;
    SkPoint args[4];
    SkPath::Iter iter(path, false);
    for (SkPath::Verb verb = iter.next(args);
         verb != SkPath::kDone_Verb;
         verb = iter.next(args)) {
        
        switch (verb) {
            case SkPath::kMove_Verb:
                MoveTo(args[0].fX, args[0].fY, &currentSegment);
                lastMovePt = args[0];
                fillState = kEmpty_SkipFillState;
                break;
            case SkPath::kLine_Verb:
                AppendLine(args[1].fX, args[1].fY, &currentSegment);
                if (fillState == kEmpty_SkipFillState) {
                   if (args[0] != lastMovePt) {
                       fillState = kSingleLine_SkipFillState;
                   }
                } else if (fillState == kSingleLine_SkipFillState) {
                    fillState = kNonSingleLine_SkipFillState;
                }
                break;
            case SkPath::kQuad_Verb: {
                SkPoint cubic[4];
                SkConvertQuadToCubic(args, cubic);
                AppendCubic(cubic[1].fX, cubic[1].fY, cubic[2].fX, cubic[2].fY,
                            cubic[3].fX, cubic[3].fY, &currentSegment);
                fillState = kNonSingleLine_SkipFillState;
                break;
            }
            case SkPath::kCubic_Verb:
                AppendCubic(args[1].fX, args[1].fY, args[2].fX, args[2].fY,
                            args[3].fX, args[3].fY, &currentSegment);
                fillState = kNonSingleLine_SkipFillState;
                break;
            case SkPath::kClose_Verb:
                if (fillState != kSingleLine_SkipFillState) {
                    ClosePath(&currentSegment);
                    SkData* data = currentSegment.copyToData();
                    content->write(data->data(), data->size());
                    data->unref();
                }
                currentSegment.reset();
                break;
            default:
                SkASSERT(false);
                break;
        }
    }
    if (currentSegment.bytesWritten() > 0) {
        SkData* data = currentSegment.copyToData();
        content->write(data->data(), data->size());
        data->unref();
    }
}


void SkPDFUtils::ClosePath(SkWStream* content) {
    content->writeText("h\n");
}


void SkPDFUtils::PaintPath(SkPaint::Style style, SkPath::FillType fill,
                           SkWStream* content) {
    if (style == SkPaint::kFill_Style) {
        content->writeText("f");
    } else if (style == SkPaint::kStrokeAndFill_Style) {
        content->writeText("B");
    } else if (style == SkPaint::kStroke_Style) {
        content->writeText("S");
    }

    if (style != SkPaint::kStroke_Style) {
        NOT_IMPLEMENTED(fill == SkPath::kInverseEvenOdd_FillType, false);
        NOT_IMPLEMENTED(fill == SkPath::kInverseWinding_FillType, false);
        if (fill == SkPath::kEvenOdd_FillType) {
            content->writeText("*");
        }
    }
    content->writeText("\n");
}


void SkPDFUtils::StrokePath(SkWStream* content) {
    SkPDFUtils::PaintPath(
        SkPaint::kStroke_Style, SkPath::kWinding_FillType, content);
}


void SkPDFUtils::DrawFormXObject(int objectIndex, SkWStream* content) {
    content->writeText("/X");
    content->writeDecAsText(objectIndex);
    content->writeText(" Do\n");
}


void SkPDFUtils::ApplyGraphicState(int objectIndex, SkWStream* content) {
    content->writeText("/G");
    content->writeDecAsText(objectIndex);
    content->writeText(" gs\n");
}
