








#include "SkPDFShader.h"

#include "SkCanvas.h"
#include "SkData.h"
#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFTypes.h"
#include "SkPDFUtils.h"
#include "SkScalar.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkThread.h"
#include "SkTypes.h"

static void transformBBox(const SkMatrix& matrix, SkRect* bbox) {
    SkMatrix inverse;
    inverse.reset();
    matrix.invert(&inverse);
    inverse.mapRect(bbox);
}

static void unitToPointsMatrix(const SkPoint pts[2], SkMatrix* matrix) {
    SkVector    vec = pts[1] - pts[0];
    SkScalar    mag = vec.length();
    SkScalar    inv = mag ? SkScalarInvert(mag) : 0;

    vec.scale(inv);
    matrix->setSinCos(vec.fY, vec.fX);
    matrix->preTranslate(pts[0].fX, pts[0].fY);
    matrix->preScale(mag, mag);
}









static void interpolateColorCode(SkScalar range, SkScalar* curColor,
                                 SkScalar* prevColor, int components,
                                 SkString* result) {
    
    SkAutoSTMalloc<4, SkScalar> multiplierAlloc(components);
    SkScalar *multiplier = multiplierAlloc.get();
    for (int i = 0; i < components; i++) {
        multiplier[i] = SkScalarDiv(curColor[i] - prevColor[i], range);
    }

    
    
    
    SkAutoSTMalloc<4, bool> dupInputAlloc(components);
    bool *dupInput = dupInputAlloc.get();
    dupInput[components - 1] = false;
    for (int i = components - 2; i >= 0; i--) {
        dupInput[i] = dupInput[i + 1] || multiplier[i + 1] != 0;
    }

    if (!dupInput[0] && multiplier[0] == 0) {
        result->append("pop ");
    }

    for (int i = 0; i < components; i++) {
        
        if (dupInput[i]) {
            result->append("dup ");
        }

        if (multiplier[i] == 0) {
            result->appendScalar(prevColor[i]);
            result->append(" ");
        } else {
            if (multiplier[i] != 1) {
                result->appendScalar(multiplier[i]);
                result->append(" mul ");
            }
            if (prevColor[i] != 0) {
                result->appendScalar(prevColor[i]);
                result->append(" add ");
            }
        }

        if (dupInput[i]) {
            result->append("exch\n");
        }
    }
}






















static void gradientFunctionCode(const SkShader::GradientInfo& info,
                                 SkString* result) {
    




    static const int kColorComponents = 3;
    typedef SkScalar ColorTuple[kColorComponents];
    SkAutoSTMalloc<4, ColorTuple> colorDataAlloc(info.fColorCount);
    ColorTuple *colorData = colorDataAlloc.get();
    const SkScalar scale = SkScalarInvert(SkIntToScalar(255));
    for (int i = 0; i < info.fColorCount; i++) {
        colorData[i][0] = SkScalarMul(SkColorGetR(info.fColors[i]), scale);
        colorData[i][1] = SkScalarMul(SkColorGetG(info.fColors[i]), scale);
        colorData[i][2] = SkScalarMul(SkColorGetB(info.fColors[i]), scale);
    }

    
    result->append("dup 0 le {pop ");
    result->appendScalar(colorData[0][0]);
    result->append(" ");
    result->appendScalar(colorData[0][1]);
    result->append(" ");
    result->appendScalar(colorData[0][2]);
    result->append(" }\n");

    
    for (int i = 1 ; i < info.fColorCount; i++) {
        result->append("{dup ");
        result->appendScalar(info.fColorOffsets[i]);
        result->append(" le {");
        if (info.fColorOffsets[i - 1] != 0) {
            result->appendScalar(info.fColorOffsets[i - 1]);
            result->append(" sub\n");
        }

        interpolateColorCode(info.fColorOffsets[i] - info.fColorOffsets[i - 1],
                             colorData[i], colorData[i - 1], kColorComponents,
                             result);
        result->append("}\n");
    }

    
    result->append("{pop ");
    result->appendScalar(colorData[info.fColorCount - 1][0]);
    result->append(" ");
    result->appendScalar(colorData[info.fColorCount - 1][1]);
    result->append(" ");
    result->appendScalar(colorData[info.fColorCount - 1][2]);

    for (int i = 0 ; i < info.fColorCount; i++) {
        result->append("} ifelse\n");
    }
}


static void tileModeCode(SkShader::TileMode mode, SkString* result) {
    if (mode == SkShader::kRepeat_TileMode) {
        result->append("dup truncate sub\n");  
        result->append("dup 0 le {1 add} if\n");  
        return;
    }

    if (mode == SkShader::kMirror_TileMode) {
        
        
        result->append("abs "                 
                       "dup "                 
                       "truncate "            
                       "dup "                 
                       "cvi "                 
                       "2 mod "               
                       "1 eq "                
                       "3 1 roll "            
                       "sub "                 
                       "exch "                
                       "{1 exch sub} if\n");  
    }
}

static SkString linearCode(const SkShader::GradientInfo& info) {
    SkString function("{pop\n");  
    tileModeCode(info.fTileMode, &function);
    gradientFunctionCode(info, &function);
    function.append("}");
    return function;
}

static SkString radialCode(const SkShader::GradientInfo& info) {
    SkString function("{");
    
    function.append("dup "      
                    "mul "      
                    "exch "     
                    "dup "      
                    "mul "      
                    "add "      
                    "sqrt\n");  

    tileModeCode(info.fTileMode, &function);
    gradientFunctionCode(info, &function);
    function.append("}");
    return function;
}





static SkString twoPointRadialCode(const SkShader::GradientInfo& info) {
    SkScalar dx = info.fPoint[0].fX - info.fPoint[1].fX;
    SkScalar dy = info.fPoint[0].fY - info.fPoint[1].fY;
    SkScalar sr = info.fRadius[0];
    SkScalar a = SkScalarMul(dx, dx) + SkScalarMul(dy, dy) - SK_Scalar1;
    bool posRoot = info.fRadius[1] > info.fRadius[0];

    
    
    SkString function("{");
    function.append("2 copy ");

    
    function.appendScalar(dy);
    function.append(" mul exch ");
    function.appendScalar(dx);
    function.append(" mul add ");
    function.appendScalar(sr);
    function.append(" sub 2 mul neg dup dup mul\n");

    
    function.append("4 2 roll dup mul exch dup mul add ");
    function.appendScalar(SkScalarMul(sr, sr));
    function.append(" sub\n");

    
    function.appendScalar(SkScalarMul(SkIntToScalar(4), a));
    function.append(" mul sub abs sqrt\n");

    
    if (posRoot) {
        function.append("sub ");
    } else {
        function.append("add ");
    }
    function.appendScalar(SkScalarMul(SkIntToScalar(2), a));
    function.append(" div\n");

    tileModeCode(info.fTileMode, &function);
    gradientFunctionCode(info, &function);
    function.append("}");
    return function;
}

static SkString sweepCode(const SkShader::GradientInfo& info) {
    SkString function("{exch atan 360 div\n");
    tileModeCode(info.fTileMode, &function);
    gradientFunctionCode(info, &function);
    function.append("}");
    return function;
}

class SkPDFShader::State {
public:
    SkShader::GradientType fType;
    SkShader::GradientInfo fInfo;
    SkAutoFree fColorData;
    SkMatrix fCanvasTransform;
    SkMatrix fShaderTransform;
    SkIRect fBBox;

    SkBitmap fImage;
    uint32_t fPixelGeneration;
    SkShader::TileMode fImageTileModes[2];

    explicit State(const SkShader& shader, const SkMatrix& canvasTransform,
                   const SkIRect& bbox);
    bool operator==(const State& b) const;
};

class SkPDFFunctionShader : public SkPDFDict, public SkPDFShader {
public:
    explicit SkPDFFunctionShader(SkPDFShader::State* state);
    ~SkPDFFunctionShader() {
        if (isValid()) {
            RemoveShader(this);
        }
        fResources.unrefAll();
    }

    bool isValid() { return fResources.count() > 0; }

    void getResources(SkTDArray<SkPDFObject*>* resourceList) {
        GetResourcesHelper(&fResources, resourceList);
    }

private:
    static SkPDFObject* RangeObject();

    SkTDArray<SkPDFObject*> fResources;
    SkAutoTDelete<const SkPDFShader::State> fState;

    SkPDFStream* makePSFunction(const SkString& psCode, SkPDFArray* domain);
};

class SkPDFImageShader : public SkPDFStream, public SkPDFShader {
public:
    explicit SkPDFImageShader(SkPDFShader::State* state);
    ~SkPDFImageShader() {
        RemoveShader(this);
        fResources.unrefAll();
    }

    void getResources(SkTDArray<SkPDFObject*>* resourceList) {
        GetResourcesHelper(&fResources, resourceList);
    }

private:
    SkTDArray<SkPDFObject*> fResources;
    SkAutoTDelete<const SkPDFShader::State> fState;
};

SkPDFShader::SkPDFShader() {}


void SkPDFShader::RemoveShader(SkPDFObject* shader) {
    SkAutoMutexAcquire lock(CanonicalShadersMutex());
    ShaderCanonicalEntry entry(shader, NULL);
    int index = CanonicalShaders().find(entry);
    SkASSERT(index >= 0);
    CanonicalShaders().removeShuffle(index);
}


SkPDFObject* SkPDFShader::GetPDFShader(const SkShader& shader,
                                       const SkMatrix& matrix,
                                       const SkIRect& surfaceBBox) {
    SkPDFObject* result;
    SkAutoMutexAcquire lock(CanonicalShadersMutex());
    SkAutoTDelete<State> shaderState(new State(shader, matrix, surfaceBBox));

    ShaderCanonicalEntry entry(NULL, shaderState.get());
    int index = CanonicalShaders().find(entry);
    if (index >= 0) {
        result = CanonicalShaders()[index].fPDFShader;
        result->ref();
        return result;
    }
    
    if (shaderState.get()->fType == SkShader::kNone_GradientType) {
        result = new SkPDFImageShader(shaderState.detach());
    } else {
        SkPDFFunctionShader* functionShader =
            new SkPDFFunctionShader(shaderState.detach());
        if (!functionShader->isValid()) {
            delete functionShader;
            return NULL;
        }
        result = functionShader;
    }
    entry.fPDFShader = result;
    CanonicalShaders().push(entry);
    return result;  
}


SkTDArray<SkPDFShader::ShaderCanonicalEntry>& SkPDFShader::CanonicalShaders() {
    
    static SkTDArray<ShaderCanonicalEntry> gCanonicalShaders;
    return gCanonicalShaders;
}


SkMutex& SkPDFShader::CanonicalShadersMutex() {
    
    static SkMutex gCanonicalShadersMutex;
    return gCanonicalShadersMutex;
}


SkPDFObject* SkPDFFunctionShader::RangeObject() {
    
    static SkPDFArray* range = NULL;
    
    
    if (range == NULL) {
        range = new SkPDFArray;
        range->reserve(6);
        range->appendInt(0);
        range->appendInt(1);
        range->appendInt(0);
        range->appendInt(1);
        range->appendInt(0);
        range->appendInt(1);
    }
    return range;
}

SkPDFFunctionShader::SkPDFFunctionShader(SkPDFShader::State* state)
        : SkPDFDict("Pattern"),
          fState(state) {
    SkString (*codeFunction)(const SkShader::GradientInfo& info) = NULL;
    SkPoint transformPoints[2];

    
    
    const SkShader::GradientInfo* info = &fState.get()->fInfo;
    transformPoints[0] = info->fPoint[0];
    transformPoints[1] = info->fPoint[1];
    switch (fState.get()->fType) {
        case SkShader::kLinear_GradientType:
            codeFunction = &linearCode;
            break;
        case SkShader::kRadial_GradientType:
            transformPoints[1] = transformPoints[0];
            transformPoints[1].fX += info->fRadius[0];
            codeFunction = &radialCode;
            break;
        case SkShader::kRadial2_GradientType: {
            
            
            if (info->fRadius[0] == info->fRadius[1]) {
                return;
            }
            transformPoints[1] = transformPoints[0];
            SkScalar dr = info->fRadius[1] - info->fRadius[0];
            transformPoints[1].fX += dr;
            codeFunction = &twoPointRadialCode;
            break;
        }
        case SkShader::kSweep_GradientType:
            transformPoints[1] = transformPoints[0];
            transformPoints[1].fX += 1;
            codeFunction = &sweepCode;
            break;
        case SkShader::kColor_GradientType:
        case SkShader::kNone_GradientType:
            return;
    }

    
    
    
    
    SkMatrix mapperMatrix;
    unitToPointsMatrix(transformPoints, &mapperMatrix);
    SkMatrix finalMatrix = fState.get()->fCanvasTransform;
    finalMatrix.preConcat(mapperMatrix);
    finalMatrix.preConcat(fState.get()->fShaderTransform);
    SkRect bbox;
    bbox.set(fState.get()->fBBox);
    transformBBox(finalMatrix, &bbox);

    SkRefPtr<SkPDFArray> domain = new SkPDFArray;
    domain->unref();  
    domain->reserve(4);
    domain->appendScalar(bbox.fLeft);
    domain->appendScalar(bbox.fRight);
    domain->appendScalar(bbox.fTop);
    domain->appendScalar(bbox.fBottom);

    SkString functionCode;
    
    
    
    if (fState.get()->fType == SkShader::kRadial2_GradientType) {
        SkShader::GradientInfo twoPointRadialInfo = *info;
        SkMatrix inverseMapperMatrix;
        mapperMatrix.invert(&inverseMapperMatrix);
        inverseMapperMatrix.mapPoints(twoPointRadialInfo.fPoint, 2);
        twoPointRadialInfo.fRadius[0] =
            inverseMapperMatrix.mapRadius(info->fRadius[0]);
        twoPointRadialInfo.fRadius[1] =
            inverseMapperMatrix.mapRadius(info->fRadius[1]);
        functionCode = codeFunction(twoPointRadialInfo);
    } else {
        functionCode = codeFunction(*info);
    }

    SkRefPtr<SkPDFStream> function = makePSFunction(functionCode, domain.get());
    
    fResources.push(function.get());

    SkRefPtr<SkPDFDict> pdfShader = new SkPDFDict;
    pdfShader->unref();  
    pdfShader->insertInt("ShadingType", 1);
    pdfShader->insertName("ColorSpace", "DeviceRGB");
    pdfShader->insert("Domain", domain.get());
    pdfShader->insert("Function", new SkPDFObjRef(function.get()))->unref();

    insertInt("PatternType", 2);
    insert("Matrix", SkPDFUtils::MatrixToArray(finalMatrix))->unref();
    insert("Shading", pdfShader.get());
}

SkPDFImageShader::SkPDFImageShader(SkPDFShader::State* state) : fState(state) {
    fState.get()->fImage.lockPixels();

    SkMatrix finalMatrix = fState.get()->fCanvasTransform;
    finalMatrix.preConcat(fState.get()->fShaderTransform);
    SkRect surfaceBBox;
    surfaceBBox.set(fState.get()->fBBox);
    transformBBox(finalMatrix, &surfaceBBox);

    SkMatrix unflip;
    unflip.setTranslate(0, SkScalarRound(surfaceBBox.height()));
    unflip.preScale(1, -1);
    SkISize size = SkISize::Make(SkScalarRound(surfaceBBox.width()),
                                 SkScalarRound(surfaceBBox.height()));
    SkPDFDevice pattern(size, size, unflip);
    SkCanvas canvas(&pattern);
    canvas.translate(-surfaceBBox.fLeft, -surfaceBBox.fTop);
    finalMatrix.preTranslate(surfaceBBox.fLeft, surfaceBBox.fTop);

    const SkBitmap* image = &fState.get()->fImage;
    int width = image->width();
    int height = image->height();
    SkShader::TileMode tileModes[2];
    tileModes[0] = fState.get()->fImageTileModes[0];
    tileModes[1] = fState.get()->fImageTileModes[1];

    canvas.drawBitmap(*image, 0, 0);
    SkRect patternBBox = SkRect::MakeXYWH(-surfaceBBox.fLeft, -surfaceBBox.fTop,
                                          width, height);

    
    if (tileModes[0] == SkShader::kMirror_TileMode) {
        SkMatrix xMirror;
        xMirror.setScale(-1, 1);
        xMirror.postTranslate(2 * width, 0);
        canvas.drawBitmapMatrix(*image, xMirror);
        patternBBox.fRight += width;
    }
    if (tileModes[1] == SkShader::kMirror_TileMode) {
        SkMatrix yMirror;
        yMirror.setScale(1, -1);
        yMirror.postTranslate(0, 2 * height);
        canvas.drawBitmapMatrix(*image, yMirror);
        patternBBox.fBottom += height;
    }
    if (tileModes[0] == SkShader::kMirror_TileMode &&
            tileModes[1] == SkShader::kMirror_TileMode) {
        SkMatrix mirror;
        mirror.setScale(-1, -1);
        mirror.postTranslate(2 * width, 2 * height);
        canvas.drawBitmapMatrix(*image, mirror);
    }

    
    

    
    
    if (tileModes[0] == SkShader::kClamp_TileMode &&
            tileModes[1] == SkShader::kClamp_TileMode) {
        SkPaint paint;
        SkRect rect;
        rect = SkRect::MakeLTRB(surfaceBBox.fLeft, surfaceBBox.fTop, 0, 0);
        if (!rect.isEmpty()) {
            paint.setColor(image->getColor(0, 0));
            canvas.drawRect(rect, paint);
        }

        rect = SkRect::MakeLTRB(width, surfaceBBox.fTop, surfaceBBox.fRight, 0);
        if (!rect.isEmpty()) {
            paint.setColor(image->getColor(width - 1, 0));
            canvas.drawRect(rect, paint);
        }

        rect = SkRect::MakeLTRB(width, height, surfaceBBox.fRight,
                                surfaceBBox.fBottom);
        if (!rect.isEmpty()) {
            paint.setColor(image->getColor(width - 1, height - 1));
            canvas.drawRect(rect, paint);
        }

        rect = SkRect::MakeLTRB(surfaceBBox.fLeft, height, 0,
                                surfaceBBox.fBottom);
        if (!rect.isEmpty()) {
            paint.setColor(image->getColor(0, height - 1));
            canvas.drawRect(rect, paint);
        }
    }

    
    if (tileModes[0] == SkShader::kClamp_TileMode) {
        SkIRect subset = SkIRect::MakeXYWH(0, 0, 1, height);
        if (surfaceBBox.fLeft < 0) {
            SkBitmap left;
            SkAssertResult(image->extractSubset(&left, subset));

            SkMatrix leftMatrix;
            leftMatrix.setScale(-surfaceBBox.fLeft, 1);
            leftMatrix.postTranslate(surfaceBBox.fLeft, 0);
            canvas.drawBitmapMatrix(left, leftMatrix);

            if (tileModes[1] == SkShader::kMirror_TileMode) {
                leftMatrix.postScale(1, -1);
                leftMatrix.postTranslate(0, 2 * height);
                canvas.drawBitmapMatrix(left, leftMatrix);
            }
            patternBBox.fLeft = 0;
        }

        if (surfaceBBox.fRight > width) {
            SkBitmap right;
            subset.offset(width - 1, 0);
            SkAssertResult(image->extractSubset(&right, subset));

            SkMatrix rightMatrix;
            rightMatrix.setScale(surfaceBBox.fRight - width, 1);
            rightMatrix.postTranslate(width, 0);
            canvas.drawBitmapMatrix(right, rightMatrix);

            if (tileModes[1] == SkShader::kMirror_TileMode) {
                rightMatrix.postScale(1, -1);
                rightMatrix.postTranslate(0, 2 * height);
                canvas.drawBitmapMatrix(right, rightMatrix);
            }
            patternBBox.fRight = surfaceBBox.width();
        }
    }

    if (tileModes[1] == SkShader::kClamp_TileMode) {
        SkIRect subset = SkIRect::MakeXYWH(0, 0, width, 1);
        if (surfaceBBox.fTop < 0) {
            SkBitmap top;
            SkAssertResult(image->extractSubset(&top, subset));

            SkMatrix topMatrix;
            topMatrix.setScale(1, -surfaceBBox.fTop);
            topMatrix.postTranslate(0, surfaceBBox.fTop);
            canvas.drawBitmapMatrix(top, topMatrix);

            if (tileModes[0] == SkShader::kMirror_TileMode) {
                topMatrix.postScale(-1, 1);
                topMatrix.postTranslate(2 * width, 0);
                canvas.drawBitmapMatrix(top, topMatrix);
            }
            patternBBox.fTop = 0;
        }

        if (surfaceBBox.fBottom > height) {
            SkBitmap bottom;
            subset.offset(0, height - 1);
            SkAssertResult(image->extractSubset(&bottom, subset));

            SkMatrix bottomMatrix;
            bottomMatrix.setScale(1, surfaceBBox.fBottom - height);
            bottomMatrix.postTranslate(0, height);
            canvas.drawBitmapMatrix(bottom, bottomMatrix);

            if (tileModes[0] == SkShader::kMirror_TileMode) {
                bottomMatrix.postScale(-1, 1);
                bottomMatrix.postTranslate(2 * width, 0);
                canvas.drawBitmapMatrix(bottom, bottomMatrix);
            }
            patternBBox.fBottom = surfaceBBox.height();
        }
    }

    SkRefPtr<SkPDFArray> patternBBoxArray = new SkPDFArray;
    patternBBoxArray->unref();  
    patternBBoxArray->reserve(4);
    patternBBoxArray->appendScalar(patternBBox.fLeft);
    patternBBoxArray->appendScalar(patternBBox.fTop);
    patternBBoxArray->appendScalar(patternBBox.fRight);
    patternBBoxArray->appendScalar(patternBBox.fBottom);

    
    SkRefPtr<SkStream> content = pattern.content();
    content->unref();  
    pattern.getResources(&fResources);

    setData(content.get());
    insertName("Type", "Pattern");
    insertInt("PatternType", 1);
    insertInt("PaintType", 1);
    insertInt("TilingType", 1);
    insert("BBox", patternBBoxArray.get());
    insertScalar("XStep", patternBBox.width());
    insertScalar("YStep", patternBBox.height());
    insert("Resources", pattern.getResourceDict());
    insert("Matrix", SkPDFUtils::MatrixToArray(finalMatrix))->unref();

    fState.get()->fImage.unlockPixels();
}

SkPDFStream* SkPDFFunctionShader::makePSFunction(const SkString& psCode,
                                                 SkPDFArray* domain) {
    SkAutoDataUnref funcData(SkData::NewWithCopy(psCode.c_str(),
                                                 psCode.size()));
    SkPDFStream* result = new SkPDFStream(funcData.get());
    result->insertInt("FunctionType", 4);
    result->insert("Domain", domain);
    result->insert("Range", RangeObject());
    return result;
}

SkPDFShader::ShaderCanonicalEntry::ShaderCanonicalEntry(SkPDFObject* pdfShader,
                                                        const State* state)
    : fPDFShader(pdfShader),
      fState(state) {
}

bool SkPDFShader::ShaderCanonicalEntry::operator==(
        const ShaderCanonicalEntry& b) const {
    return fPDFShader == b.fPDFShader ||
           (fState != NULL && b.fState != NULL && *fState == *b.fState);
}

bool SkPDFShader::State::operator==(const SkPDFShader::State& b) const {
    if (fType != b.fType ||
            fCanvasTransform != b.fCanvasTransform ||
            fShaderTransform != b.fShaderTransform ||
            fBBox != b.fBBox) {
        return false;
    }

    if (fType == SkShader::kNone_GradientType) {
        if (fPixelGeneration != b.fPixelGeneration ||
                fPixelGeneration == 0 ||
                fImageTileModes[0] != b.fImageTileModes[0] ||
                fImageTileModes[1] != b.fImageTileModes[1]) {
            return false;
        }
    } else {
        if (fInfo.fColorCount != b.fInfo.fColorCount ||
                memcmp(fInfo.fColors, b.fInfo.fColors,
                       sizeof(SkColor) * fInfo.fColorCount) != 0 ||
                memcmp(fInfo.fColorOffsets, b.fInfo.fColorOffsets,
                       sizeof(SkScalar) * fInfo.fColorCount) != 0 ||
                fInfo.fPoint[0] != b.fInfo.fPoint[0] ||
                fInfo.fTileMode != b.fInfo.fTileMode) {
            return false;
        }

        switch (fType) {
            case SkShader::kLinear_GradientType:
                if (fInfo.fPoint[1] != b.fInfo.fPoint[1]) {
                    return false;
                }
                break;
            case SkShader::kRadial_GradientType:
                if (fInfo.fRadius[0] != b.fInfo.fRadius[0]) {
                    return false;
                }
                break;
            case SkShader::kRadial2_GradientType:
                if (fInfo.fPoint[1] != b.fInfo.fPoint[1] ||
                        fInfo.fRadius[0] != b.fInfo.fRadius[0] ||
                        fInfo.fRadius[1] != b.fInfo.fRadius[1]) {
                    return false;
                }
                break;
            case SkShader::kSweep_GradientType:
            case SkShader::kNone_GradientType:
            case SkShader::kColor_GradientType:
                break;
        }
    }
    return true;
}

SkPDFShader::State::State(const SkShader& shader,
                          const SkMatrix& canvasTransform, const SkIRect& bbox)
        : fCanvasTransform(canvasTransform),
          fBBox(bbox),
          fPixelGeneration(0) {
    fInfo.fColorCount = 0;
    fInfo.fColors = NULL;
    fInfo.fColorOffsets = NULL;
    shader.getLocalMatrix(&fShaderTransform);
    fImageTileModes[0] = fImageTileModes[1] = SkShader::kClamp_TileMode;

    fType = shader.asAGradient(&fInfo);

    if (fType == SkShader::kNone_GradientType) {
        SkShader::BitmapType bitmapType;
        SkMatrix matrix;
        bitmapType = shader.asABitmap(&fImage, &matrix, fImageTileModes, NULL);
        if (bitmapType != SkShader::kDefault_BitmapType) {
            fImage.reset();
            return;
        }
        SkASSERT(matrix.isIdentity());
        fPixelGeneration = fImage.getGenerationID();
    } else {
        fColorData.set(sk_malloc_throw(
                    fInfo.fColorCount * (sizeof(SkColor) + sizeof(SkScalar))));
        fInfo.fColors = reinterpret_cast<SkColor*>(fColorData.get());
        fInfo.fColorOffsets = reinterpret_cast<SkScalar*>(fInfo.fColors + fInfo.fColorCount);
        shader.asAGradient(&fInfo);
    }
}
