









#ifndef GrSamplerState_DEFINED
#define GrSamplerState_DEFINED

#include "GrCustomStage.h"
#include "GrMatrix.h"
#include "GrTypes.h"

#define MAX_KERNEL_WIDTH 25

class GrSamplerState {
public:
    enum Filter {
        


        kNearest_Filter,
        


        kBilinear_Filter,
        






        k4x4Downsample_Filter,
        


        kConvolution_Filter,
        


        kDilate_Filter,
        


        kErode_Filter,

        kDefault_Filter = kNearest_Filter
    };

    





















    enum SampleMode {
        kNormal_SampleMode,     
        kRadial_SampleMode,     
        kRadial2_SampleMode,    
        kSweep_SampleMode,      

        kDefault_SampleMode = kNormal_SampleMode
    };

    



    enum WrapMode {
        kClamp_WrapMode,
        kRepeat_WrapMode,
        kMirror_WrapMode,

        kDefault_WrapMode = kClamp_WrapMode
    };

    




    enum FilterDirection {
        kX_FilterDirection,
        kY_FilterDirection,

        kDefault_FilterDirection = kX_FilterDirection,
    };
    



    GrSamplerState()
    : fRadial2CenterX1()
    , fRadial2Radius0()
    , fRadial2PosRoot()
    , fCustomStage (NULL) {
        this->reset();
    }

    ~GrSamplerState() {
        GrSafeUnref(fCustomStage);
    }

    bool operator ==(const GrSamplerState& s) const {
        



        int bitwiseRegion = (intptr_t) &fCustomStage - (intptr_t) this;
        GrAssert(sizeof(GrSamplerState) ==
                 bitwiseRegion + sizeof(fCustomStage));
        return !memcmp(this, &s, bitwiseRegion) &&
               ((fCustomStage == s.fCustomStage) ||
                (fCustomStage && s.fCustomStage &&
                 (fCustomStage->getFactory() ==
                     s.fCustomStage->getFactory()) &&
                 fCustomStage->isEqual(s.fCustomStage)));
    }
    bool operator !=(const GrSamplerState& s) const { return !(*this == s); }

    GrSamplerState& operator =(const GrSamplerState s) {
        memcpy(this, &s, sizeof(GrSamplerState));
        return *this;
    }

    WrapMode getWrapX() const { return fWrapX; }
    WrapMode getWrapY() const { return fWrapY; }
    FilterDirection getFilterDirection() const { return fFilterDirection; }
    SampleMode getSampleMode() const { return fSampleMode; }
    const GrMatrix& getMatrix() const { return fMatrix; }
    const GrRect& getTextureDomain() const { return fTextureDomain; }
    bool hasTextureDomain() const {return SkIntToScalar(0) != fTextureDomain.right();}
    Filter getFilter() const { return fFilter; }
    int getKernelWidth() const { return fKernelWidth; }
    const float* getKernel() const { return fKernel; }
    bool swapsRAndB() const { return fSwapRAndB; }

    bool isGradient() const {
        return  kRadial_SampleMode == fSampleMode ||
                kRadial2_SampleMode == fSampleMode ||
                kSweep_SampleMode == fSampleMode;
    }

    void setWrapX(WrapMode mode) { fWrapX = mode; }
    void setWrapY(WrapMode mode) { fWrapY = mode; }
    void setSampleMode(SampleMode mode) { fSampleMode = mode; }
    void setFilterDirection(FilterDirection mode) { fFilterDirection = mode; }
    
    



    GrMatrix* matrix() { return &fMatrix; }

    




    void setTextureDomain(const GrRect& textureDomain) { fTextureDomain = textureDomain; }

    



    void setRAndBSwap(bool swap) { fSwapRAndB = swap; }

    









    void preConcatMatrix(const GrMatrix& matrix) { fMatrix.preConcat(matrix); }

    



    void setFilter(Filter filter) { fFilter = filter; }

    void reset(WrapMode wrapXAndY,
               Filter filter,
               FilterDirection direction,
               const GrMatrix& matrix) {
        fWrapX = wrapXAndY;
        fWrapY = wrapXAndY;
        fSampleMode = kDefault_SampleMode;
        fFilter = filter;
        fFilterDirection = direction;
        fMatrix = matrix;
        fTextureDomain.setEmpty();
        fSwapRAndB = false;
        GrSafeSetNull(fCustomStage);
    }
    void reset(WrapMode wrapXAndY, Filter filter, const GrMatrix& matrix) {
        this->reset(wrapXAndY, filter, kDefault_FilterDirection, matrix);
    }
    void reset(WrapMode wrapXAndY,
               Filter filter) {
        this->reset(wrapXAndY, filter, kDefault_FilterDirection, GrMatrix::I());
    }
    void reset(const GrMatrix& matrix) {
        this->reset(kDefault_WrapMode, kDefault_Filter, kDefault_FilterDirection, matrix);
    }
    void reset() {
        this->reset(kDefault_WrapMode, kDefault_Filter, kDefault_FilterDirection, GrMatrix::I());
    }

    GrScalar getRadial2CenterX1() const { return fRadial2CenterX1; }
    GrScalar getRadial2Radius0() const { return fRadial2Radius0; }
    bool     isRadial2PosRoot() const { return SkToBool(fRadial2PosRoot); }
    
    
    bool radial2IsDegenerate() const { return GR_Scalar1 == fRadial2CenterX1; }

    






    void setRadial2Params(GrScalar centerX1, GrScalar radius0, bool posRoot) {
        fRadial2CenterX1 = centerX1;
        fRadial2Radius0 = radius0;
        fRadial2PosRoot = posRoot;
    }

    void setConvolutionParams(int kernelWidth, const float* kernel) {
        GrAssert(kernelWidth >= 0 && kernelWidth <= MAX_KERNEL_WIDTH);
        fKernelWidth = kernelWidth;
        if (NULL != kernel) {
            memcpy(fKernel, kernel, kernelWidth * sizeof(float));
        }
    }

    void setMorphologyRadius(int radius) {
        GrAssert(radius >= 0 && radius <= MAX_KERNEL_WIDTH);
        fKernelWidth = radius;
    }

    void setCustomStage(GrCustomStage* stage) {
        GrSafeAssign(fCustomStage, stage);
    }
    GrCustomStage* getCustomStage() const { return fCustomStage; }

private:
    WrapMode            fWrapX : 8;
    WrapMode            fWrapY : 8;
    FilterDirection     fFilterDirection : 8;
    SampleMode          fSampleMode : 8;
    Filter              fFilter : 8;
    GrMatrix            fMatrix;
    bool                fSwapRAndB;
    GrRect              fTextureDomain;

    
    GrScalar            fRadial2CenterX1;
    GrScalar            fRadial2Radius0;
    SkBool8             fRadial2PosRoot;

    
    uint8_t             fKernelWidth;
    float               fKernel[MAX_KERNEL_WIDTH];

    GrCustomStage*      fCustomStage;
};

#endif

