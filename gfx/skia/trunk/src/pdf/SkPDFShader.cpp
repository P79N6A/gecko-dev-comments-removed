








#include "SkPDFShader.h"

#include "SkData.h"
#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFFormXObject.h"
#include "SkPDFGraphicState.h"
#include "SkPDFResourceDict.h"
#include "SkPDFUtils.h"
#include "SkScalar.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkThread.h"
#include "SkTSet.h"
#include "SkTypes.h"

static bool inverseTransformBBox(const SkMatrix& matrix, SkRect* bbox) {
    SkMatrix inverse;
    if (!matrix.invert(&inverse)) {
        return false;
    }
    inverse.mapRect(bbox);
    return true;
}

static void unitToPointsMatrix(const SkPoint pts[2], SkMatrix* matrix) {
    SkVector    vec = pts[1] - pts[0];
    SkScalar    mag = vec.length();
    SkScalar    inv = mag ? SkScalarInvert(mag) : 0;

    vec.scale(inv);
    matrix->setSinCos(vec.fY, vec.fX);
    matrix->preScale(mag, mag);
    matrix->postTranslate(pts[0].fX, pts[0].fY);
}










static void interpolateColorCode(SkScalar range, SkScalar* curColor,
                                 SkScalar* prevColor, SkString* result) {
    SkASSERT(range != SkIntToScalar(0));
    static const int kColorComponents = 3;

    
    SkScalar multiplier[kColorComponents];
    for (int i = 0; i < kColorComponents; i++) {
        multiplier[i] = SkScalarDiv(curColor[i] - prevColor[i], range);
    }

    
    
    
    bool dupInput[kColorComponents];
    dupInput[kColorComponents - 1] = false;
    for (int i = kColorComponents - 2; i >= 0; i--) {
        dupInput[i] = dupInput[i + 1] || multiplier[i + 1] != 0;
    }

    if (!dupInput[0] && multiplier[0] == 0) {
        result->append("pop ");
    }

    for (int i = 0; i < kColorComponents; i++) {
        
        
        if (dupInput[i] && multiplier[i] != 0) {
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

    
    int gradients = 0;
    for (int i = 1 ; i < info.fColorCount; i++) {
        if (info.fColorOffsets[i] == info.fColorOffsets[i - 1]) {
            continue;
        }
        gradients++;

        result->append("{dup ");
        result->appendScalar(info.fColorOffsets[i]);
        result->append(" le {");
        if (info.fColorOffsets[i - 1] != 0) {
            result->appendScalar(info.fColorOffsets[i - 1]);
            result->append(" sub\n");
        }

        interpolateColorCode(info.fColorOffsets[i] - info.fColorOffsets[i - 1],
                             colorData[i], colorData[i - 1], result);
        result->append("}\n");
    }

    
    result->append("{pop ");
    result->appendScalar(colorData[info.fColorCount - 1][0]);
    result->append(" ");
    result->appendScalar(colorData[info.fColorCount - 1][1]);
    result->append(" ");
    result->appendScalar(colorData[info.fColorCount - 1][2]);

    for (int i = 0 ; i < gradients + 1; i++) {
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










static SkString apply_perspective_to_coordinates(
        const SkMatrix& inversePerspectiveMatrix) {
    SkString code;
    if (!inversePerspectiveMatrix.hasPerspective()) {
        return code;
    }

    
    
    
    

    const SkScalar p0 = inversePerspectiveMatrix[SkMatrix::kMPersp0];
    const SkScalar p1 = inversePerspectiveMatrix[SkMatrix::kMPersp1];
    const SkScalar p2 = inversePerspectiveMatrix[SkMatrix::kMPersp2];

    
    

    
    code.append(" dup ");               
    code.appendScalar(p1);              
    code.append(" mul "                 
                " 2 index ");           
    code.appendScalar(p0);              
    code.append(" mul ");               
    code.appendScalar(p2);              
    code.append(" add "                 
                "add "                  
                "3 1 roll "             
                "2 index "              
                "div "                  
                "3 1 roll "             
                "exch "                 
                "div "                  
                "exch\n");              
    return code;
}

static SkString linearCode(const SkShader::GradientInfo& info,
                           const SkMatrix& perspectiveRemover) {
    SkString function("{");

    function.append(apply_perspective_to_coordinates(perspectiveRemover));

    function.append("pop\n");  
    tileModeCode(info.fTileMode, &function);
    gradientFunctionCode(info, &function);
    function.append("}");
    return function;
}

static SkString radialCode(const SkShader::GradientInfo& info,
                           const SkMatrix& perspectiveRemover) {
    SkString function("{");

    function.append(apply_perspective_to_coordinates(perspectiveRemover));

    
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





static SkString twoPointRadialCode(const SkShader::GradientInfo& info,
                                   const SkMatrix& perspectiveRemover) {
    SkScalar dx = info.fPoint[0].fX - info.fPoint[1].fX;
    SkScalar dy = info.fPoint[0].fY - info.fPoint[1].fY;
    SkScalar sr = info.fRadius[0];
    SkScalar a = SkScalarMul(dx, dx) + SkScalarMul(dy, dy) - SK_Scalar1;
    bool posRoot = info.fRadius[1] > info.fRadius[0];

    
    
    SkString function("{");

    function.append(apply_perspective_to_coordinates(perspectiveRemover));

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




static SkString twoPointConicalCode(const SkShader::GradientInfo& info,
                                    const SkMatrix& perspectiveRemover) {
    SkScalar dx = info.fPoint[1].fX - info.fPoint[0].fX;
    SkScalar dy = info.fPoint[1].fY - info.fPoint[0].fY;
    SkScalar r0 = info.fRadius[0];
    SkScalar dr = info.fRadius[1] - info.fRadius[0];
    SkScalar a = SkScalarMul(dx, dx) + SkScalarMul(dy, dy) -
                 SkScalarMul(dr, dr);

    
    

    
    
    SkString function("{");

    function.append(apply_perspective_to_coordinates(perspectiveRemover));

    function.append("2 copy ");

    
    function.appendScalar(dy);
    function.append(" mul exch ");
    function.appendScalar(dx);
    function.append(" mul add ");
    function.appendScalar(SkScalarMul(r0, dr));
    function.append(" add -2 mul dup dup mul\n");

    
    function.append("4 2 roll dup mul exch dup mul add ");
    function.appendScalar(SkScalarMul(r0, r0));
    function.append(" sub dup 4 1 roll\n");

    

    
    if (a == 0) {

        
        function.append("pop pop div neg dup ");

        
        function.appendScalar(dr);
        function.append(" mul ");
        function.appendScalar(r0);
        function.append(" add\n");

        
        function.append("0 lt {pop false} {true} ifelse\n");

    } else {

        
        

        
        function.appendScalar(SkScalarMul(SkIntToScalar(4), a));
        function.append(" mul sub dup\n");

        
        function.append("0 ge {\n");

        
        
        function.append("sqrt exch dup 0 lt {exch -1 mul} if");
        function.append(" add -0.5 mul dup\n");

        
        function.appendScalar(a);
        function.append(" div\n");

        
        function.append("3 1 roll div\n");

        
        function.append("2 copy gt {exch} if\n");

        
        function.append("dup ");
        function.appendScalar(dr);
        function.append(" mul ");
        function.appendScalar(r0);
        function.append(" add\n");

        
        function.append(" 0 gt {exch pop true}\n");

        
        function.append("{pop dup\n");
        function.appendScalar(dr);
        function.append(" mul ");
        function.appendScalar(r0);
        function.append(" add\n");

        
        function.append("0 le {pop false} {true} ifelse\n");
        function.append("} ifelse\n");

        
        function.append("} {pop pop pop false} ifelse\n");
    }

    
    function.append("{");
    tileModeCode(info.fTileMode, &function);
    gradientFunctionCode(info, &function);

    
    function.append("} {0 0 0} ifelse }");

    return function;
}

static SkString sweepCode(const SkShader::GradientInfo& info,
                          const SkMatrix& perspectiveRemover) {
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

    State(const SkShader& shader, const SkMatrix& canvasTransform,
          const SkIRect& bbox);

    bool operator==(const State& b) const;

    SkPDFShader::State* CreateAlphaToLuminosityState() const;
    SkPDFShader::State* CreateOpaqueState() const;

    bool GradientHasAlpha() const;

private:
    State(const State& other);
    State operator=(const State& rhs);
    void AllocateGradientInfoStorage();
};

class SkPDFFunctionShader : public SkPDFDict, public SkPDFShader {
    SK_DECLARE_INST_COUNT(SkPDFFunctionShader)
public:
    explicit SkPDFFunctionShader(SkPDFShader::State* state);
    virtual ~SkPDFFunctionShader() {
        if (isValid()) {
            RemoveShader(this);
        }
        fResources.unrefAll();
    }

    virtual bool isValid() { return fResources.count() > 0; }

    void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                      SkTSet<SkPDFObject*>* newResourceObjects) {
        GetResourcesHelper(&fResources,
                           knownResourceObjects,
                           newResourceObjects);
    }

private:
    static SkPDFObject* RangeObject();

    SkTDArray<SkPDFObject*> fResources;
    SkAutoTDelete<const SkPDFShader::State> fState;

    SkPDFStream* makePSFunction(const SkString& psCode, SkPDFArray* domain);
    typedef SkPDFDict INHERITED;
};






class SkPDFAlphaFunctionShader : public SkPDFStream, public SkPDFShader {
public:
    explicit SkPDFAlphaFunctionShader(SkPDFShader::State* state);
    virtual ~SkPDFAlphaFunctionShader() {
        if (isValid()) {
            RemoveShader(this);
        }
    }

    virtual bool isValid() {
        return fColorShader.get() != NULL;
    }

private:
    SkAutoTDelete<const SkPDFShader::State> fState;

    SkPDFGraphicState* CreateSMaskGraphicState();

    void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                      SkTSet<SkPDFObject*>* newResourceObjects) {
        fResourceDict->getReferencedResources(knownResourceObjects,
                                              newResourceObjects,
                                              true);
    }

    SkAutoTUnref<SkPDFObject> fColorShader;
    SkAutoTUnref<SkPDFResourceDict> fResourceDict;
};

class SkPDFImageShader : public SkPDFStream, public SkPDFShader {
public:
    explicit SkPDFImageShader(SkPDFShader::State* state);
    virtual ~SkPDFImageShader() {
        if (isValid()) {
            RemoveShader(this);
        }
        fResources.unrefAll();
    }

    virtual bool isValid() { return size() > 0; }

    void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                      SkTSet<SkPDFObject*>* newResourceObjects) {
        GetResourcesHelper(&fResources.toArray(),
                           knownResourceObjects,
                           newResourceObjects);
    }

private:
    SkTSet<SkPDFObject*> fResources;
    SkAutoTDelete<const SkPDFShader::State> fState;
};

SkPDFShader::SkPDFShader() {}


SkPDFObject* SkPDFShader::GetPDFShaderByState(State* inState) {
    SkPDFObject* result;

    SkAutoTDelete<State> shaderState(inState);
    if (shaderState.get()->fType == SkShader::kNone_GradientType &&
            shaderState.get()->fImage.isNull()) {
        
        
        
        
        return NULL;
    }

    ShaderCanonicalEntry entry(NULL, shaderState.get());
    int index = CanonicalShaders().find(entry);
    if (index >= 0) {
        result = CanonicalShaders()[index].fPDFShader;
        result->ref();
        return result;
    }

    bool valid = false;
    
    if (shaderState.get()->fType == SkShader::kNone_GradientType) {
        SkPDFImageShader* imageShader =
            new SkPDFImageShader(shaderState.detach());
        valid = imageShader->isValid();
        result = imageShader;
    } else {
        if (shaderState.get()->GradientHasAlpha()) {
            SkPDFAlphaFunctionShader* gradientShader =
                SkNEW_ARGS(SkPDFAlphaFunctionShader, (shaderState.detach()));
            valid = gradientShader->isValid();
            result = gradientShader;
        } else {
            SkPDFFunctionShader* functionShader =
                SkNEW_ARGS(SkPDFFunctionShader, (shaderState.detach()));
            valid = functionShader->isValid();
            result = functionShader;
        }
    }
    if (!valid) {
        delete result;
        return NULL;
    }
    entry.fPDFShader = result;
    CanonicalShaders().push(entry);
    return result;  
}


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
    SkAutoMutexAcquire lock(CanonicalShadersMutex());
    return GetPDFShaderByState(
            SkNEW_ARGS(State, (shader, matrix, surfaceBBox)));
}


SkTDArray<SkPDFShader::ShaderCanonicalEntry>& SkPDFShader::CanonicalShaders() {
    SkPDFShader::CanonicalShadersMutex().assertHeld();
    static SkTDArray<ShaderCanonicalEntry> gCanonicalShaders;
    return gCanonicalShaders;
}

SK_DECLARE_STATIC_MUTEX(gCanonicalShadersMutex);

SkBaseMutex& SkPDFShader::CanonicalShadersMutex() {
    return gCanonicalShadersMutex;
}


SkPDFObject* SkPDFFunctionShader::RangeObject() {
    SkPDFShader::CanonicalShadersMutex().assertHeld();
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

static SkPDFResourceDict* get_gradient_resource_dict(
        SkPDFObject* functionShader,
        SkPDFObject* gState) {
    SkPDFResourceDict* dict = new SkPDFResourceDict();

    if (functionShader != NULL) {
        dict->insertResourceAsReference(
                SkPDFResourceDict::kPattern_ResourceType, 0, functionShader);
    }
    if (gState != NULL) {
        dict->insertResourceAsReference(
                SkPDFResourceDict::kExtGState_ResourceType, 0, gState);
    }

    return dict;
}

static void populate_tiling_pattern_dict(SkPDFDict* pattern,
                                      SkRect& bbox, SkPDFDict* resources,
                                      const SkMatrix& matrix) {
    const int kTiling_PatternType = 1;
    const int kColoredTilingPattern_PaintType = 1;
    const int kConstantSpacing_TilingType = 1;

    pattern->insertName("Type", "Pattern");
    pattern->insertInt("PatternType", kTiling_PatternType);
    pattern->insertInt("PaintType", kColoredTilingPattern_PaintType);
    pattern->insertInt("TilingType", kConstantSpacing_TilingType);
    pattern->insert("BBox", SkPDFUtils::RectToArray(bbox))->unref();
    pattern->insertScalar("XStep", bbox.width());
    pattern->insertScalar("YStep", bbox.height());
    pattern->insert("Resources", resources);
    if (!matrix.isIdentity()) {
        pattern->insert("Matrix", SkPDFUtils::MatrixToArray(matrix))->unref();
    }
}






static SkStream* create_pattern_fill_content(int gsIndex, SkRect& bounds) {
    SkDynamicMemoryWStream content;
    if (gsIndex >= 0) {
        SkPDFUtils::ApplyGraphicState(gsIndex, &content);
    }
    SkPDFUtils::ApplyPattern(0, &content);
    SkPDFUtils::AppendRectangle(bounds, &content);
    SkPDFUtils::PaintPath(SkPaint::kFill_Style, SkPath::kEvenOdd_FillType,
                          &content);

    return content.detachAsStream();
}





SkPDFGraphicState* SkPDFAlphaFunctionShader::CreateSMaskGraphicState() {
    SkRect bbox;
    bbox.set(fState.get()->fBBox);

    SkAutoTUnref<SkPDFObject> luminosityShader(
            SkPDFShader::GetPDFShaderByState(
                 fState->CreateAlphaToLuminosityState()));

    SkAutoTUnref<SkStream> alphaStream(create_pattern_fill_content(-1, bbox));

    SkAutoTUnref<SkPDFResourceDict>
        resources(get_gradient_resource_dict(luminosityShader, NULL));

    SkAutoTUnref<SkPDFFormXObject> alphaMask(
            new SkPDFFormXObject(alphaStream.get(), bbox, resources.get()));

    return SkPDFGraphicState::GetSMaskGraphicState(
            alphaMask.get(), false,
            SkPDFGraphicState::kLuminosity_SMaskMode);
}

SkPDFAlphaFunctionShader::SkPDFAlphaFunctionShader(SkPDFShader::State* state)
        : fState(state) {
    SkRect bbox;
    bbox.set(fState.get()->fBBox);

    fColorShader.reset(
            SkPDFShader::GetPDFShaderByState(state->CreateOpaqueState()));

    
    
    SkAutoTUnref<SkPDFGraphicState> alphaGs(CreateSMaskGraphicState());
    fResourceDict.reset(
            get_gradient_resource_dict(fColorShader.get(), alphaGs.get()));

    SkAutoTUnref<SkStream> colorStream(
            create_pattern_fill_content(0, bbox));
    setData(colorStream.get());

    populate_tiling_pattern_dict(this, bbox, fResourceDict.get(),
                                 SkMatrix::I());
}



static bool split_perspective(const SkMatrix in, SkMatrix* affine,
                              SkMatrix* perspectiveInverse) {
    const SkScalar p2 = in[SkMatrix::kMPersp2];

    if (SkScalarNearlyZero(p2)) {
        return false;
    }

    const SkScalar zero = SkIntToScalar(0);
    const SkScalar one = SkIntToScalar(1);

    const SkScalar sx = in[SkMatrix::kMScaleX];
    const SkScalar kx = in[SkMatrix::kMSkewX];
    const SkScalar tx = in[SkMatrix::kMTransX];
    const SkScalar ky = in[SkMatrix::kMSkewY];
    const SkScalar sy = in[SkMatrix::kMScaleY];
    const SkScalar ty = in[SkMatrix::kMTransY];
    const SkScalar p0 = in[SkMatrix::kMPersp0];
    const SkScalar p1 = in[SkMatrix::kMPersp1];

    
    
    
    
    
    perspectiveInverse->setAll(one,          zero,       zero,
                               zero,         one,        zero,
                               -p0/p2,     -p1/p2,     1/p2);

    affine->setAll(sx - p0 * tx / p2,       kx - p1 * tx / p2,      tx / p2,
                   ky - p0 * ty / p2,       sy - p1 * ty / p2,      ty / p2,
                   zero,                    zero,                   one);

    return true;
}

SkPDFFunctionShader::SkPDFFunctionShader(SkPDFShader::State* state)
        : SkPDFDict("Pattern"),
          fState(state) {
    SkString (*codeFunction)(const SkShader::GradientInfo& info,
                             const SkMatrix& perspectiveRemover) = NULL;
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
        case SkShader::kConical_GradientType: {
            transformPoints[1] = transformPoints[0];
            transformPoints[1].fX += SK_Scalar1;
            codeFunction = &twoPointConicalCode;
            break;
        }
        case SkShader::kSweep_GradientType:
            transformPoints[1] = transformPoints[0];
            transformPoints[1].fX += SK_Scalar1;
            codeFunction = &sweepCode;
            break;
        case SkShader::kColor_GradientType:
        case SkShader::kNone_GradientType:
        default:
            return;
    }

    
    
    
    
    SkMatrix mapperMatrix;
    unitToPointsMatrix(transformPoints, &mapperMatrix);

    SkMatrix finalMatrix = fState.get()->fCanvasTransform;
    finalMatrix.preConcat(fState.get()->fShaderTransform);
    finalMatrix.preConcat(mapperMatrix);

    
    
    
    
    
    SkMatrix perspectiveInverseOnly = SkMatrix::I();
    if (finalMatrix.hasPerspective()) {
        if (!split_perspective(finalMatrix,
                               &finalMatrix, &perspectiveInverseOnly)) {
            return;
        }
    }

    SkRect bbox;
    bbox.set(fState.get()->fBBox);
    if (!inverseTransformBBox(finalMatrix, &bbox)) {
        return;
    }

    SkAutoTUnref<SkPDFArray> domain(new SkPDFArray);
    domain->reserve(4);
    domain->appendScalar(bbox.fLeft);
    domain->appendScalar(bbox.fRight);
    domain->appendScalar(bbox.fTop);
    domain->appendScalar(bbox.fBottom);

    SkString functionCode;
    
    
    
    if (fState.get()->fType == SkShader::kRadial2_GradientType) {
        SkShader::GradientInfo twoPointRadialInfo = *info;
        SkMatrix inverseMapperMatrix;
        if (!mapperMatrix.invert(&inverseMapperMatrix)) {
            return;
        }
        inverseMapperMatrix.mapPoints(twoPointRadialInfo.fPoint, 2);
        twoPointRadialInfo.fRadius[0] =
            inverseMapperMatrix.mapRadius(info->fRadius[0]);
        twoPointRadialInfo.fRadius[1] =
            inverseMapperMatrix.mapRadius(info->fRadius[1]);
        functionCode = codeFunction(twoPointRadialInfo, perspectiveInverseOnly);
    } else {
        functionCode = codeFunction(*info, perspectiveInverseOnly);
    }

    SkAutoTUnref<SkPDFDict> pdfShader(new SkPDFDict);
    pdfShader->insertInt("ShadingType", 1);
    pdfShader->insertName("ColorSpace", "DeviceRGB");
    pdfShader->insert("Domain", domain.get());

    SkPDFStream* function = makePSFunction(functionCode, domain.get());
    pdfShader->insert("Function", new SkPDFObjRef(function))->unref();
    fResources.push(function);  

    insertInt("PatternType", 2);
    insert("Matrix", SkPDFUtils::MatrixToArray(finalMatrix))->unref();
    insert("Shading", pdfShader.get());
}

SkPDFImageShader::SkPDFImageShader(SkPDFShader::State* state) : fState(state) {
    fState.get()->fImage.lockPixels();

    
    
    

    
    
    SkMatrix finalMatrix = fState.get()->fCanvasTransform;
    finalMatrix.preConcat(fState.get()->fShaderTransform);
    SkRect deviceBounds;
    deviceBounds.set(fState.get()->fBBox);
    if (!inverseTransformBBox(finalMatrix, &deviceBounds)) {
        return;
    }

    const SkBitmap* image = &fState.get()->fImage;
    SkRect bitmapBounds;
    image->getBounds(&bitmapBounds);

    
    
    
    
    SkShader::TileMode tileModes[2];
    tileModes[0] = fState.get()->fImageTileModes[0];
    tileModes[1] = fState.get()->fImageTileModes[1];
    if (tileModes[0] != SkShader::kClamp_TileMode ||
            tileModes[1] != SkShader::kClamp_TileMode) {
        deviceBounds.join(bitmapBounds);
    }

    SkMatrix unflip;
    unflip.setTranslate(0, SkScalarRoundToScalar(deviceBounds.height()));
    unflip.preScale(SK_Scalar1, -SK_Scalar1);
    SkISize size = SkISize::Make(SkScalarRoundToInt(deviceBounds.width()),
                                 SkScalarRoundToInt(deviceBounds.height()));
    
    
    SkPDFDevice pattern(size, size, unflip);
    SkCanvas canvas(&pattern);

    SkRect patternBBox;
    image->getBounds(&patternBBox);

    
    canvas.translate(-deviceBounds.left(), -deviceBounds.top());
    patternBBox.offset(-deviceBounds.left(), -deviceBounds.top());
    
    finalMatrix.preTranslate(deviceBounds.left(), deviceBounds.top());

    
    
    
    canvas.drawBitmap(*image, 0, 0);

    SkScalar width = SkIntToScalar(image->width());
    SkScalar height = SkIntToScalar(image->height());

    
    if (tileModes[0] == SkShader::kMirror_TileMode) {
        SkMatrix xMirror;
        xMirror.setScale(-1, 1);
        xMirror.postTranslate(2 * width, 0);
        canvas.drawBitmapMatrix(*image, xMirror);
        patternBBox.fRight += width;
    }
    if (tileModes[1] == SkShader::kMirror_TileMode) {
        SkMatrix yMirror;
        yMirror.setScale(SK_Scalar1, -SK_Scalar1);
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
        rect = SkRect::MakeLTRB(deviceBounds.left(), deviceBounds.top(), 0, 0);
        if (!rect.isEmpty()) {
            paint.setColor(image->getColor(0, 0));
            canvas.drawRect(rect, paint);
        }

        rect = SkRect::MakeLTRB(width, deviceBounds.top(),
                                deviceBounds.right(), 0);
        if (!rect.isEmpty()) {
            paint.setColor(image->getColor(image->width() - 1, 0));
            canvas.drawRect(rect, paint);
        }

        rect = SkRect::MakeLTRB(width, height,
                                deviceBounds.right(), deviceBounds.bottom());
        if (!rect.isEmpty()) {
            paint.setColor(image->getColor(image->width() - 1,
                                           image->height() - 1));
            canvas.drawRect(rect, paint);
        }

        rect = SkRect::MakeLTRB(deviceBounds.left(), height,
                                0, deviceBounds.bottom());
        if (!rect.isEmpty()) {
            paint.setColor(image->getColor(0, image->height() - 1));
            canvas.drawRect(rect, paint);
        }
    }

    
    if (tileModes[0] == SkShader::kClamp_TileMode) {
        SkIRect subset = SkIRect::MakeXYWH(0, 0, 1, image->height());
        if (deviceBounds.left() < 0) {
            SkBitmap left;
            SkAssertResult(image->extractSubset(&left, subset));

            SkMatrix leftMatrix;
            leftMatrix.setScale(-deviceBounds.left(), 1);
            leftMatrix.postTranslate(deviceBounds.left(), 0);
            canvas.drawBitmapMatrix(left, leftMatrix);

            if (tileModes[1] == SkShader::kMirror_TileMode) {
                leftMatrix.postScale(SK_Scalar1, -SK_Scalar1);
                leftMatrix.postTranslate(0, 2 * height);
                canvas.drawBitmapMatrix(left, leftMatrix);
            }
            patternBBox.fLeft = 0;
        }

        if (deviceBounds.right() > width) {
            SkBitmap right;
            subset.offset(image->width() - 1, 0);
            SkAssertResult(image->extractSubset(&right, subset));

            SkMatrix rightMatrix;
            rightMatrix.setScale(deviceBounds.right() - width, 1);
            rightMatrix.postTranslate(width, 0);
            canvas.drawBitmapMatrix(right, rightMatrix);

            if (tileModes[1] == SkShader::kMirror_TileMode) {
                rightMatrix.postScale(SK_Scalar1, -SK_Scalar1);
                rightMatrix.postTranslate(0, 2 * height);
                canvas.drawBitmapMatrix(right, rightMatrix);
            }
            patternBBox.fRight = deviceBounds.width();
        }
    }

    if (tileModes[1] == SkShader::kClamp_TileMode) {
        SkIRect subset = SkIRect::MakeXYWH(0, 0, image->width(), 1);
        if (deviceBounds.top() < 0) {
            SkBitmap top;
            SkAssertResult(image->extractSubset(&top, subset));

            SkMatrix topMatrix;
            topMatrix.setScale(SK_Scalar1, -deviceBounds.top());
            topMatrix.postTranslate(0, deviceBounds.top());
            canvas.drawBitmapMatrix(top, topMatrix);

            if (tileModes[0] == SkShader::kMirror_TileMode) {
                topMatrix.postScale(-1, 1);
                topMatrix.postTranslate(2 * width, 0);
                canvas.drawBitmapMatrix(top, topMatrix);
            }
            patternBBox.fTop = 0;
        }

        if (deviceBounds.bottom() > height) {
            SkBitmap bottom;
            subset.offset(0, image->height() - 1);
            SkAssertResult(image->extractSubset(&bottom, subset));

            SkMatrix bottomMatrix;
            bottomMatrix.setScale(SK_Scalar1, deviceBounds.bottom() - height);
            bottomMatrix.postTranslate(0, height);
            canvas.drawBitmapMatrix(bottom, bottomMatrix);

            if (tileModes[0] == SkShader::kMirror_TileMode) {
                bottomMatrix.postScale(-1, 1);
                bottomMatrix.postTranslate(2 * width, 0);
                canvas.drawBitmapMatrix(bottom, bottomMatrix);
            }
            patternBBox.fBottom = deviceBounds.height();
        }
    }

    
    SkAutoTUnref<SkStream> content(pattern.content());
    setData(content.get());
    SkPDFResourceDict* resourceDict = pattern.getResourceDict();
    resourceDict->getReferencedResources(fResources, &fResources, false);

    populate_tiling_pattern_dict(this, patternBBox,
                                 pattern.getResourceDict(), finalMatrix);

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
            case SkShader::kConical_GradientType:
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
    fShaderTransform = shader.getLocalMatrix();
    fImageTileModes[0] = fImageTileModes[1] = SkShader::kClamp_TileMode;

    fType = shader.asAGradient(&fInfo);

    if (fType == SkShader::kNone_GradientType) {
        SkShader::BitmapType bitmapType;
        SkMatrix matrix;
        bitmapType = shader.asABitmap(&fImage, &matrix, fImageTileModes);
        if (bitmapType != SkShader::kDefault_BitmapType) {
            fImage.reset();
            return;
        }
        SkASSERT(matrix.isIdentity());
        fPixelGeneration = fImage.getGenerationID();
    } else {
        AllocateGradientInfoStorage();
        shader.asAGradient(&fInfo);
    }
}

SkPDFShader::State::State(const SkPDFShader::State& other)
  : fType(other.fType),
    fCanvasTransform(other.fCanvasTransform),
    fShaderTransform(other.fShaderTransform),
    fBBox(other.fBBox)
{
    
    
    SkASSERT(fType != SkShader::kNone_GradientType);

    if (fType != SkShader::kNone_GradientType) {
        fInfo = other.fInfo;

        AllocateGradientInfoStorage();
        for (int i = 0; i < fInfo.fColorCount; i++) {
            fInfo.fColors[i] = other.fInfo.fColors[i];
            fInfo.fColorOffsets[i] = other.fInfo.fColorOffsets[i];
        }
    }
}





SkPDFShader::State* SkPDFShader::State::CreateAlphaToLuminosityState() const {
    SkASSERT(fType != SkShader::kNone_GradientType);

    SkPDFShader::State* newState = new SkPDFShader::State(*this);

    for (int i = 0; i < fInfo.fColorCount; i++) {
        SkAlpha alpha = SkColorGetA(fInfo.fColors[i]);
        newState->fInfo.fColors[i] = SkColorSetARGB(255, alpha, alpha, alpha);
    }

    return newState;
}





SkPDFShader::State* SkPDFShader::State::CreateOpaqueState() const {
    SkASSERT(fType != SkShader::kNone_GradientType);

    SkPDFShader::State* newState = new SkPDFShader::State(*this);
    for (int i = 0; i < fInfo.fColorCount; i++) {
        newState->fInfo.fColors[i] = SkColorSetA(fInfo.fColors[i],
                                                 SK_AlphaOPAQUE);
    }

    return newState;
}




bool SkPDFShader::State::GradientHasAlpha() const {
    if (fType == SkShader::kNone_GradientType) {
        return false;
    }

    for (int i = 0; i < fInfo.fColorCount; i++) {
        SkAlpha alpha = SkColorGetA(fInfo.fColors[i]);
        if (alpha != SK_AlphaOPAQUE) {
            return true;
        }
    }
    return false;
}

void SkPDFShader::State::AllocateGradientInfoStorage() {
    fColorData.set(sk_malloc_throw(
               fInfo.fColorCount * (sizeof(SkColor) + sizeof(SkScalar))));
    fInfo.fColors = reinterpret_cast<SkColor*>(fColorData.get());
    fInfo.fColorOffsets =
            reinterpret_cast<SkScalar*>(fInfo.fColors + fInfo.fColorCount);
}
