







#ifndef SkShader_DEFINED
#define SkShader_DEFINED

#include "SkBitmap.h"
#include "SkFlattenable.h"
#include "SkMask.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "GrColor.h"

class SkPath;
class SkPicture;
class SkXfermode;
class GrContext;
class GrEffect;











class SK_API SkShader : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkShader)

    SkShader(const SkMatrix* localMatrix = NULL);
    virtual ~SkShader();

    





    const SkMatrix& getLocalMatrix() const { return fLocalMatrix; }

    





    bool hasLocalMatrix() const { return !fLocalMatrix.isIdentity(); }

    enum TileMode {
        


        kClamp_TileMode,

        
        kRepeat_TileMode,

        


        kMirror_TileMode,

#if 0
        
        kDecal_TileMode,
#endif

        kTileModeCount
    };

    

    enum Flags {
        
        kOpaqueAlpha_Flag  = 0x01,

        
        kHasSpan16_Flag = 0x02,

        




        kIntrinsicly16_Flag = 0x04,

        




        kConstInY32_Flag = 0x08,

        




        kConstInY16_Flag = 0x10
    };

    





    virtual bool isOpaque() const { return false; }

    


    struct ContextRec {
        ContextRec() : fDevice(NULL), fPaint(NULL), fMatrix(NULL), fLocalMatrix(NULL) {}
        ContextRec(const SkBitmap& device, const SkPaint& paint, const SkMatrix& matrix)
            : fDevice(&device)
            , fPaint(&paint)
            , fMatrix(&matrix)
            , fLocalMatrix(NULL) {}

        const SkBitmap* fDevice;        
        const SkPaint*  fPaint;         
        const SkMatrix* fMatrix;        
        const SkMatrix* fLocalMatrix;   
    };

    class Context : public ::SkNoncopyable {
    public:
        Context(const SkShader& shader, const ContextRec&);

        virtual ~Context();

        






        virtual uint32_t getFlags() const { return 0; }

        



        virtual uint8_t getSpan16Alpha() const { return fPaintAlpha; }

        




        virtual void shadeSpan(int x, int y, SkPMColor[], int count) = 0;

        typedef void (*ShadeProc)(void* ctx, int x, int y, SkPMColor[], int count);
        virtual ShadeProc asAShadeProc(void** ctx);

        



        virtual void shadeSpan16(int x, int y, uint16_t[], int count);

        




        virtual void shadeSpanAlpha(int x, int y, uint8_t alpha[], int count);

        



        bool canCallShadeSpan16() {
            return SkShader::CanCallShadeSpan16(this->getFlags());
        }

    protected:
        
        const SkShader& fShader;

        enum MatrixClass {
            kLinear_MatrixClass,            
            kFixedStepInX_MatrixClass,      
                                            
            kPerspective_MatrixClass        
        };
        static MatrixClass ComputeMatrixClass(const SkMatrix&);

        uint8_t         getPaintAlpha() const { return fPaintAlpha; }
        const SkMatrix& getTotalInverse() const { return fTotalInverse; }
        MatrixClass     getInverseClass() const { return (MatrixClass)fTotalInverseClass; }
        const SkMatrix& getCTM() const { return fCTM; }
    private:
        SkMatrix    fCTM;
        SkMatrix    fTotalInverse;
        uint8_t     fPaintAlpha;
        uint8_t     fTotalInverseClass;

        typedef SkNoncopyable INHERITED;
    };

    



    Context* createContext(const ContextRec&, void* storage) const;

    





    virtual size_t contextSize() const;

    


    static bool CanCallShadeSpan16(uint32_t flags) {
        return (flags & kHasSpan16_Flag) != 0;
    }

    




    enum BitmapType {
        kNone_BitmapType,   
        kDefault_BitmapType,
                            
        kRadial_BitmapType, 
                            
                            
                            
        kSweep_BitmapType,  
                            
                            
                            
                            
        kTwoPointRadial_BitmapType,
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            
        kTwoPointConical_BitmapType,
                            
                            
                            
                            
                            
                            
                            
                            
                            
        kLinear_BitmapType, 
                            

       kLast_BitmapType = kLinear_BitmapType
    };
    














    virtual BitmapType asABitmap(SkBitmap* outTexture, SkMatrix* outMatrix,
                         TileMode xy[2]) const;

    




























    enum GradientType {
        kNone_GradientType,
        kColor_GradientType,
        kLinear_GradientType,
        kRadial_GradientType,
        kRadial2_GradientType,
        kSweep_GradientType,
        kConical_GradientType,
        kLast_GradientType = kConical_GradientType
    };

    struct GradientInfo {
        int         fColorCount;    
                                    
                                    
                                    
        SkColor*    fColors;        
        SkScalar*   fColorOffsets;  
        SkPoint     fPoint[2];      
        SkScalar    fRadius[2];     
        TileMode    fTileMode;      
        uint32_t    fGradientFlags; 
    };

    virtual GradientType asAGradient(GradientInfo* info) const;

    






    struct ComposeRec {
        const SkShader*     fShaderA;
        const SkShader*     fShaderB;
        const SkXfermode*   fMode;
    };

    virtual bool asACompose(ComposeRec* rec) const { return false; }


    















    virtual bool asNewEffect(GrContext* context, const SkPaint& paint,
                             const SkMatrix* localMatrixOrNull, GrColor* paintColor,
                             GrEffect** effect) const;

#ifdef SK_BUILD_FOR_ANDROID_FRAMEWORK
    



    virtual bool asACustomShader(void** customData) const { return false; }
#endif

    
    

    


    static SkShader* CreateEmptyShader();

    













    static SkShader* CreateBitmapShader(const SkBitmap& src,
                                        TileMode tmx, TileMode tmy,
                                        const SkMatrix* localMatrix = NULL);

    









    static SkShader* CreatePictureShader(SkPicture* src, TileMode tmx, TileMode tmy,
                                         const SkMatrix* localMatrix = NULL);

    





    static SkShader* CreateLocalMatrixShader(SkShader* proxy, const SkMatrix& localMatrix);

    






    virtual SkShader* refAsALocalMatrixShader(SkMatrix* localMatrix) const;

    SK_TO_STRING_VIRT()
    SK_DEFINE_FLATTENABLE_TYPE(SkShader)

protected:
    SkShader(SkReadBuffer& );
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    bool computeTotalInverse(const ContextRec&, SkMatrix* totalInverse) const;

    



    virtual Context* onCreateContext(const ContextRec&, void* storage) const;

private:
    
    
    SkMatrix fLocalMatrix;

    
    friend class SkLocalMatrixShader;

    typedef SkFlattenable INHERITED;
};

#endif
