









#ifndef GrSamplerState_DEFINED
#define GrSamplerState_DEFINED

#include "GrTypes.h"
#include "GrMatrix.h"

#define MAX_KERNEL_WIDTH 25

class GrSamplerState {
public:
    enum Filter {
        


        kNearest_Filter,
        


        kBilinear_Filter,
        






        k4x4Downsample_Filter,
        


        kConvolution_Filter
    };

    





















    enum SampleMode {
        kNormal_SampleMode,     
        kRadial_SampleMode,     
        kRadial2_SampleMode,    
        kSweep_SampleMode,      
    };

    



    enum WrapMode {
        kClamp_WrapMode,
        kRepeat_WrapMode,
        kMirror_WrapMode
    };

    



    GrSamplerState()
    : fRadial2CenterX1()
    , fRadial2Radius0()
    , fRadial2PosRoot() {
        this->setClampNoFilter();
    }

    explicit GrSamplerState(Filter filter)
    : fRadial2CenterX1()
    , fRadial2Radius0()
    , fRadial2PosRoot() {
        fWrapX = kClamp_WrapMode;
        fWrapY = kClamp_WrapMode;
        fSampleMode = kNormal_SampleMode;
        fFilter = filter;
        fMatrix.setIdentity();
        fTextureDomain.setEmpty();
    }

    GrSamplerState(WrapMode wx, WrapMode wy, Filter filter)
    : fRadial2CenterX1()
    , fRadial2Radius0()
    , fRadial2PosRoot() {
        fWrapX = wx;
        fWrapY = wy;
        fSampleMode = kNormal_SampleMode;
        fFilter = filter;
        fMatrix.setIdentity();
        fTextureDomain.setEmpty();
    }

    GrSamplerState(WrapMode wx, WrapMode wy, 
                   const GrMatrix& matrix, Filter filter)
    : fRadial2CenterX1()
    , fRadial2Radius0()
    , fRadial2PosRoot() {
        fWrapX = wx;
        fWrapY = wy;
        fSampleMode = kNormal_SampleMode;
        fFilter = filter;
        fMatrix = matrix;
        fTextureDomain.setEmpty();
    }

    GrSamplerState(WrapMode wx, WrapMode wy, SampleMode sample, 
                   const GrMatrix& matrix, Filter filter)
    : fRadial2CenterX1()
    , fRadial2Radius0()
    , fRadial2PosRoot() {
        fWrapX = wx;
        fWrapY = wy;
        fSampleMode = sample;
        fMatrix = matrix;
        fFilter = filter;
        fTextureDomain.setEmpty();
    }

    WrapMode getWrapX() const { return fWrapX; }
    WrapMode getWrapY() const { return fWrapY; }
    SampleMode getSampleMode() const { return fSampleMode; }
    const GrMatrix& getMatrix() const { return fMatrix; }
    const GrRect& getTextureDomain() const { return fTextureDomain; }
    bool hasTextureDomain() const {return SkIntToScalar(0) != fTextureDomain.right();}
    Filter getFilter() const { return fFilter; }
    int getKernelWidth() const { return fKernelWidth; }
    const float* getKernel() const { return fKernel; }
    const float* getImageIncrement() const { return fImageIncrement; }

    bool isGradient() const {
        return  kRadial_SampleMode == fSampleMode ||
                kRadial2_SampleMode == fSampleMode ||
                kSweep_SampleMode == fSampleMode;
    }

    void setWrapX(WrapMode mode) { fWrapX = mode; }
    void setWrapY(WrapMode mode) { fWrapY = mode; }
    void setSampleMode(SampleMode mode) { fSampleMode = mode; }
    
    




    void setMatrix(const GrMatrix& matrix) { fMatrix = matrix; }
    
    




    void setTextureDomain(const GrRect& textureDomain) { fTextureDomain = textureDomain; }

    









    void preConcatMatrix(const GrMatrix& matrix) { fMatrix.preConcat(matrix); }

    



    void setFilter(Filter filter) { fFilter = filter; }

    void setClampNoFilter() {
        fWrapX = kClamp_WrapMode;
        fWrapY = kClamp_WrapMode;
        fSampleMode = kNormal_SampleMode;
        fFilter = kNearest_Filter;
        fMatrix.setIdentity();
        fTextureDomain.setEmpty();
    }

    GrScalar getRadial2CenterX1() const { return fRadial2CenterX1; }
    GrScalar getRadial2Radius0() const { return fRadial2Radius0; }
    bool     isRadial2PosRoot() const { return fRadial2PosRoot; }
    
    
    bool radial2IsDegenerate() const { return GR_Scalar1 == fRadial2CenterX1; }

    






    void setRadial2Params(GrScalar centerX1, GrScalar radius0, bool posRoot) {
        fRadial2CenterX1 = centerX1;
        fRadial2Radius0 = radius0;
        fRadial2PosRoot = posRoot;
    }

    void setConvolutionParams(int kernelWidth, const float* kernel, float imageIncrement[2]) {
        GrAssert(kernelWidth >= 0 && kernelWidth <= MAX_KERNEL_WIDTH);
        fKernelWidth = kernelWidth;
        if (NULL != kernel) {
            memcpy(fKernel, kernel, kernelWidth * sizeof(float));
        }
        if (NULL != imageIncrement) {
            memcpy(fImageIncrement, imageIncrement, sizeof(fImageIncrement));
        } else {
            memset(fImageIncrement, 0, sizeof(fImageIncrement));
        }
    }

    static const GrSamplerState& ClampNoFilter() {
        return gClampNoFilter;
    }

private:
    WrapMode    fWrapX;
    WrapMode    fWrapY;
    SampleMode  fSampleMode;
    Filter      fFilter;
    GrMatrix    fMatrix;
    GrRect      fTextureDomain;

    
    GrScalar    fRadial2CenterX1;
    GrScalar    fRadial2Radius0;
    bool        fRadial2PosRoot;

    
    int         fKernelWidth;
    float       fKernel[MAX_KERNEL_WIDTH];
    float       fImageIncrement[2];

    static const GrSamplerState gClampNoFilter;
};

#endif

