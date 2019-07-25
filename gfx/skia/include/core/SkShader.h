








#ifndef SkShader_DEFINED
#define SkShader_DEFINED

#include "SkBitmap.h"
#include "SkFlattenable.h"
#include "SkMask.h"
#include "SkMatrix.h"
#include "SkPaint.h"

class SkPath;








class SK_API SkShader : public SkFlattenable {
public:
            SkShader();
    virtual ~SkShader();

    




    bool getLocalMatrix(SkMatrix* localM) const;

    



    void setLocalMatrix(const SkMatrix& localM);

    


    void resetLocalMatrix();

    enum TileMode {
        kClamp_TileMode,    
        kRepeat_TileMode,   
        kMirror_TileMode,   

        kTileModeCount
    };

    

    enum Flags {
        
        kOpaqueAlpha_Flag  = 0x01,

        
        kHasSpan16_Flag = 0x02,

        




        kIntrinsicly16_Flag = 0x04,

        




        kConstInY32_Flag = 0x08,

        




        kConstInY16_Flag = 0x10
    };

    






    virtual uint32_t getFlags() { return 0; }

    



    virtual uint8_t getSpan16Alpha() const { return fPaintAlpha; }

    




    virtual bool setContext(const SkBitmap& device, const SkPaint& paint,
                            const SkMatrix& matrix);

    




    virtual void shadeSpan(int x, int y, SkPMColor[], int count) = 0;

    



    virtual void shadeSpan16(int x, int y, uint16_t[], int count);

    




    virtual void shadeSpanAlpha(int x, int y, uint8_t alpha[], int count);

    



    bool canCallShadeSpan16() {
        return SkShader::CanCallShadeSpan16(this->getFlags());
    }

    


    static bool CanCallShadeSpan16(uint32_t flags) {
        return (flags & kHasSpan16_Flag) != 0;
    }

    




    virtual void beginSession();
    virtual void endSession();

    




    enum BitmapType {
        kNone_BitmapType,   
        kDefault_BitmapType,
                            
        kRadial_BitmapType, 
                            
                            
                            
        kSweep_BitmapType,  
                            
                            
                            
                            
        kTwoPointRadial_BitmapType,
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            

       kLast_BitmapType = kTwoPointRadial_BitmapType
    };
    














    virtual BitmapType asABitmap(SkBitmap* outTexture, SkMatrix* outMatrix,
                         TileMode xy[2], SkScalar* twoPointRadialParams) const;

    




























    enum GradientType {
        kNone_GradientType,
        kColor_GradientType,
        kLinear_GradientType,
        kRadial_GradientType,
        kRadial2_GradientType,
        kSweep_GradientType,
        kLast_GradientType = kSweep_GradientType
    };

    struct GradientInfo {
        int         fColorCount;    
                                    
                                    
                                    
        SkColor*    fColors;        
        SkScalar*   fColorOffsets;  
        SkPoint     fPoint[2];      
        SkScalar    fRadius[2];     
        TileMode    fTileMode;      
    };

    virtual GradientType asAGradient(GradientInfo* info) const;

    
    

    





    static SkShader* CreateBitmapShader(const SkBitmap& src,
                                        TileMode tmx, TileMode tmy);

    virtual void flatten(SkFlattenableWriteBuffer& ) SK_OVERRIDE;
protected:
    enum MatrixClass {
        kLinear_MatrixClass,            
        kFixedStepInX_MatrixClass,      
        kPerspective_MatrixClass        
    };
    static MatrixClass ComputeMatrixClass(const SkMatrix&);

    
    uint8_t             getPaintAlpha() const { return fPaintAlpha; }
    SkBitmap::Config    getDeviceConfig() const { return (SkBitmap::Config)fDeviceConfig; }
    const SkMatrix&     getTotalInverse() const { return fTotalInverse; }
    MatrixClass         getInverseClass() const { return (MatrixClass)fTotalInverseClass; }

    SkShader(SkFlattenableReadBuffer& );
private:
    SkMatrix*           fLocalMatrix;
    SkMatrix            fTotalInverse;
    uint8_t             fPaintAlpha;
    uint8_t             fDeviceConfig;
    uint8_t             fTotalInverseClass;
    SkDEBUGCODE(SkBool8 fInSession;)

    static SkShader* CreateBitmapShader(const SkBitmap& src,
                                        TileMode, TileMode,
                                        void* storage, size_t storageSize);
    friend class SkAutoBitmapShaderInstall;
    typedef SkFlattenable INHERITED;
};

#endif

