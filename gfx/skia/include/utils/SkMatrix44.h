






#ifndef SkMatrix44_DEFINED
#define SkMatrix44_DEFINED

#include "SkMatrix.h"
#include "SkScalar.h"

#ifdef SK_MSCALAR_IS_DOUBLE
#ifdef SK_MSCALAR_IS_FLOAT
    #error "can't define MSCALAR both as DOUBLE and FLOAT"
#endif
    typedef double SkMScalar;

    static inline double SkFloatToMScalar(float x) {
        return static_cast<double>(x);
    }
    static inline float SkMScalarToFloat(double x) {
        return static_cast<float>(x);
    }
    static inline double SkDoubleToMScalar(double x) {
        return x;
    }
    static inline double SkMScalarToDouble(double x) {
        return x;
    }
    static const SkMScalar SK_MScalarPI = 3.141592653589793;
#elif defined SK_MSCALAR_IS_FLOAT
#ifdef SK_MSCALAR_IS_DOUBLE
    #error "can't define MSCALAR both as DOUBLE and FLOAT"
#endif
    typedef float SkMScalar;

    static inline float SkFloatToMScalar(float x) {
        return x;
    }
    static inline float SkMScalarToFloat(float x) {
        return x;
    }
    static inline float SkDoubleToMScalar(double x) {
        return static_cast<float>(x);
    }
    static inline double SkMScalarToDouble(float x) {
        return static_cast<double>(x);
    }
    static const SkMScalar SK_MScalarPI = 3.14159265f;
#endif

#define SkMScalarToScalar SkMScalarToFloat
#define SkScalarToMScalar SkFloatToMScalar

static const SkMScalar SK_MScalar1 = 1;



struct SkVector4 {
    SkScalar fData[4];

    SkVector4() {
        this->set(0, 0, 0, 1);
    }
    SkVector4(const SkVector4& src) {
        memcpy(fData, src.fData, sizeof(fData));
    }
    SkVector4(SkScalar x, SkScalar y, SkScalar z, SkScalar w = SK_Scalar1) {
        fData[0] = x;
        fData[1] = y;
        fData[2] = z;
        fData[3] = w;
    }

    SkVector4& operator=(const SkVector4& src) {
        memcpy(fData, src.fData, sizeof(fData));
        return *this;
    }

    bool operator==(const SkVector4& v) {
        return fData[0] == v.fData[0] && fData[1] == v.fData[1] &&
               fData[2] == v.fData[2] && fData[3] == v.fData[3];
    }
    bool operator!=(const SkVector4& v) {
        return !(*this == v);
    }
    bool equals(SkScalar x, SkScalar y, SkScalar z, SkScalar w = SK_Scalar1) {
        return fData[0] == x && fData[1] == y &&
               fData[2] == z && fData[3] == w;
    }

    void set(SkScalar x, SkScalar y, SkScalar z, SkScalar w = SK_Scalar1) {
        fData[0] = x;
        fData[1] = y;
        fData[2] = z;
        fData[3] = w;
    }
};

class SK_API SkMatrix44 {
public:

    enum Uninitialized_Constructor {
        kUninitialized_Constructor
    };
    enum Identity_Constructor {
        kIdentity_Constructor
    };

    SkMatrix44(Uninitialized_Constructor) { }
    SkMatrix44(Identity_Constructor) { this->setIdentity(); }

    
    SkMatrix44() { this->setIdentity(); }

    SkMatrix44(const SkMatrix44& src) {
        memcpy(fMat, src.fMat, sizeof(fMat));
        fTypeMask = src.fTypeMask;
    }

    SkMatrix44(const SkMatrix44& a, const SkMatrix44& b) {
        this->setConcat(a, b);
    }

    SkMatrix44& operator=(const SkMatrix44& src) {
        if (&src != this) {
            memcpy(fMat, src.fMat, sizeof(fMat));
            fTypeMask = src.fTypeMask;
        }
        return *this;
    }

    bool operator==(const SkMatrix44& other) const;
    bool operator!=(const SkMatrix44& other) const {
        return !(other == *this);
    }

    SkMatrix44(const SkMatrix&);
    SkMatrix44& operator=(const SkMatrix& src);
    operator SkMatrix() const;

    


    static const SkMatrix44& I();

    enum TypeMask {
        kIdentity_Mask      = 0,
        kTranslate_Mask     = 0x01,  
        kScale_Mask         = 0x02,  
        kAffine_Mask        = 0x04,  
        kPerspective_Mask   = 0x08   
    };

    






    inline TypeMask getType() const {
        if (fTypeMask & kUnknown_Mask) {
            fTypeMask = this->computeTypeMask();
        }
        SkASSERT(!(fTypeMask & kUnknown_Mask));
        return (TypeMask)fTypeMask;
    }

    


    inline bool isIdentity() const {
        return kIdentity_Mask == this->getType();
    }

    


    inline bool isTranslate() const {
        return !(this->getType() & ~kTranslate_Mask);
    }

    


    inline bool isScaleTranslate() const {
        return !(this->getType() & ~(kScale_Mask | kTranslate_Mask));
    }

    void setIdentity();
    inline void reset() { this->setIdentity();}

    





    inline SkMScalar get(int row, int col) const {
        SkASSERT((unsigned)row <= 3);
        SkASSERT((unsigned)col <= 3);
        return fMat[col][row];
    }

    





    inline void set(int row, int col, SkMScalar value) {
        SkASSERT((unsigned)row <= 3);
        SkASSERT((unsigned)col <= 3);
        fMat[col][row] = value;
        this->dirtyTypeMask();
    }

    inline double getDouble(int row, int col) const {
        return SkMScalarToDouble(this->get(row, col));
    }
    inline void setDouble(int row, int col, double value) {
        this->set(row, col, SkDoubleToMScalar(value));
    }

    




    void asColMajorf(float[]) const;
    void asColMajord(double[]) const;
    void asRowMajorf(float[]) const;
    void asRowMajord(double[]) const;

    




    void setColMajorf(const float[]);
    void setColMajord(const double[]);
    void setRowMajorf(const float[]);
    void setRowMajord(const double[]);

#ifdef SK_MSCALAR_IS_FLOAT
    void setColMajor(const SkMScalar data[]) { this->setColMajorf(data); }
    void setRowMajor(const SkMScalar data[]) { this->setRowMajorf(data); }
#else
    void setColMajor(const SkMScalar data[]) { this->setColMajord(data); }
    void setRowMajor(const SkMScalar data[]) { this->setRowMajord(data); }
#endif

    void set3x3(SkMScalar m00, SkMScalar m01, SkMScalar m02,
                SkMScalar m10, SkMScalar m11, SkMScalar m12,
                SkMScalar m20, SkMScalar m21, SkMScalar m22);

    void setTranslate(SkMScalar dx, SkMScalar dy, SkMScalar dz);
    void preTranslate(SkMScalar dx, SkMScalar dy, SkMScalar dz);
    void postTranslate(SkMScalar dx, SkMScalar dy, SkMScalar dz);

    void setScale(SkMScalar sx, SkMScalar sy, SkMScalar sz);
    void preScale(SkMScalar sx, SkMScalar sy, SkMScalar sz);
    void postScale(SkMScalar sx, SkMScalar sy, SkMScalar sz);

    inline void setScale(SkMScalar scale) {
        this->setScale(scale, scale, scale);
    }
    inline void preScale(SkMScalar scale) {
        this->preScale(scale, scale, scale);
    }
    inline void postScale(SkMScalar scale) {
        this->postScale(scale, scale, scale);
    }

    void setRotateDegreesAbout(SkMScalar x, SkMScalar y, SkMScalar z,
                               SkMScalar degrees) {
        this->setRotateAbout(x, y, z, degrees * SK_MScalarPI / 180);
    }

    


    void setRotateAbout(SkMScalar x, SkMScalar y, SkMScalar z,
                        SkMScalar radians);
    


    void setRotateAboutUnit(SkMScalar x, SkMScalar y, SkMScalar z,
                            SkMScalar radians);

    void setConcat(const SkMatrix44& a, const SkMatrix44& b);
    inline void preConcat(const SkMatrix44& m) {
        this->setConcat(*this, m);
    }
    inline void postConcat(const SkMatrix44& m) {
        this->setConcat(m, *this);
    }

    friend SkMatrix44 operator*(const SkMatrix44& a, const SkMatrix44& b) {
        return SkMatrix44(a, b);
    }

    


    bool invert(SkMatrix44* inverse) const;

    
    void transpose();

    


    void mapScalars(const SkScalar src[4], SkScalar dst[4]) const;
    inline void mapScalars(SkScalar vec[4]) const {
        this->mapScalars(vec, vec);
    }

    
    void map(const SkScalar src[4], SkScalar dst[4]) const {
        this->mapScalars(src, dst);
    }
    
    void map(SkScalar vec[4]) const {
        this->mapScalars(vec, vec);
    }

#ifdef SK_MSCALAR_IS_DOUBLE
    void mapMScalars(const SkMScalar src[4], SkMScalar dst[4]) const;
#elif defined SK_MSCALAR_IS_FLOAT
    inline void mapMScalars(const SkMScalar src[4], SkMScalar dst[4]) const {
        this->mapScalars(src, dst);
    }
#endif
    inline void mapMScalars(SkMScalar vec[4]) const {
        this->mapMScalars(vec, vec);
    }

    friend SkVector4 operator*(const SkMatrix44& m, const SkVector4& src) {
        SkVector4 dst;
        m.map(src.fData, dst.fData);
        return dst;
    }

    







    void map2(const float src2[], int count, float dst4[]) const;
    void map2(const double src2[], int count, double dst4[]) const;

    void dump() const;

    double determinant() const;

private:
    SkMScalar           fMat[4][4];
    mutable unsigned    fTypeMask;

    enum {
        kUnknown_Mask = 0x80,

        kAllPublic_Masks = 0xF
    };

    SkMScalar transX() const { return fMat[3][0]; }
    SkMScalar transY() const { return fMat[3][1]; }
    SkMScalar transZ() const { return fMat[3][2]; }

    SkMScalar scaleX() const { return fMat[0][0]; }
    SkMScalar scaleY() const { return fMat[1][1]; }
    SkMScalar scaleZ() const { return fMat[2][2]; }

    SkMScalar perspX() const { return fMat[0][3]; }
    SkMScalar perspY() const { return fMat[1][3]; }
    SkMScalar perspZ() const { return fMat[2][3]; }

    int computeTypeMask() const;

    inline void dirtyTypeMask() {
        fTypeMask = kUnknown_Mask;
    }

    inline void setTypeMask(int mask) {
        SkASSERT(0 == (~(kAllPublic_Masks | kUnknown_Mask) & mask));
        fTypeMask = mask;
    }

    



    inline bool isTriviallyIdentity() const {
        return 0 == fTypeMask;
    }
};

#endif
