

































#ifndef android_npapi_H
#define android_npapi_H

#include <stdint.h>
#include <jni.h>
#include "npapi.h"
#include "GLDefs.h"




enum ANPBitmapFormats {
    kUnknown_ANPBitmapFormat    = 0,
    kRGBA_8888_ANPBitmapFormat  = 1,
    kRGB_565_ANPBitmapFormat    = 2
};
typedef int32_t ANPBitmapFormat;

struct ANPPixelPacking {
    uint8_t AShift;
    uint8_t ABits;
    uint8_t RShift;
    uint8_t RBits;
    uint8_t GShift;
    uint8_t GBits;
    uint8_t BShift;
    uint8_t BBits;
};

struct ANPBitmap {
    void*           baseAddr;
    ANPBitmapFormat format;
    int32_t         width;
    int32_t         height;
    int32_t         rowBytes;
};

struct ANPRectF {
    float   left;
    float   top;
    float   right;
    float   bottom;
};

struct ANPRectI {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};

struct ANPCanvas;
struct ANPMatrix;
struct ANPPaint;
struct ANPPath;
struct ANPTypeface;

enum ANPMatrixFlags {
    kIdentity_ANPMatrixFlag     = 0,
    kTranslate_ANPMatrixFlag    = 0x01,
    kScale_ANPMatrixFlag        = 0x02,
    kAffine_ANPMatrixFlag       = 0x04,
    kPerspective_ANPMatrixFlag  = 0x08,
};
typedef uint32_t ANPMatrixFlag;










#define kLogInterfaceV0_ANPGetValue         ((NPNVariable)1000)
#define kAudioTrackInterfaceV0_ANPGetValue  ((NPNVariable)1001)
#define kCanvasInterfaceV0_ANPGetValue      ((NPNVariable)1002)
#define kMatrixInterfaceV0_ANPGetValue      ((NPNVariable)1003)
#define kPaintInterfaceV0_ANPGetValue       ((NPNVariable)1004)
#define kPathInterfaceV0_ANPGetValue        ((NPNVariable)1005)
#define kTypefaceInterfaceV0_ANPGetValue    ((NPNVariable)1006)
#define kWindowInterfaceV0_ANPGetValue      ((NPNVariable)1007)
#define kBitmapInterfaceV0_ANPGetValue      ((NPNVariable)1008)
#define kSurfaceInterfaceV0_ANPGetValue     ((NPNVariable)1009)
#define kSystemInterfaceV0_ANPGetValue      ((NPNVariable)1010)
#define kEventInterfaceV0_ANPGetValue       ((NPNVariable)1011)

#define kAudioTrackInterfaceV1_ANPGetValue  ((NPNVariable)1012)
#define kOpenGLInterfaceV0_ANPGetValue      ((NPNVariable)1013)
#define kWindowInterfaceV1_ANPGetValue      ((NPNVariable)1014)
#define kVideoInterfaceV0_ANPGetValue       ((NPNVariable)1015)
#define kSystemInterfaceV1_ANPGetValue      ((NPNVariable)1016)
#define kSystemInterfaceV2_ANPGetValue      ((NPNVariable)1017)
#define kWindowInterfaceV2_ANPGetValue      ((NPNVariable)1018)
#define kNativeWindowInterfaceV0_ANPGetValue ((NPNVariable)1019)
#define kVideoInterfaceV1_ANPGetValue       ((NPNVariable)1020)





#define kSupportedDrawingModel_ANPGetValue  ((NPNVariable)2000)












#define kJavaContext_ANPGetValue            ((NPNVariable)2001)









#define kRequestDrawingModel_ANPSetValue    ((NPPVariable)1000)






enum ANPDrawingModels {
    


    kBitmap_ANPDrawingModel  = 1 << 0,
    



















    kSurface_ANPDrawingModel = 1 << 1,
    kOpenGL_ANPDrawingModel  = 1 << 2,
};
typedef int32_t ANPDrawingModel;







#define kAcceptEvents_ANPSetValue           ((NPPVariable)1001)





enum ANPEventFlag {
    kKey_ANPEventFlag               = 0x01,
    kTouch_ANPEventFlag             = 0x02,
};
typedef uint32_t ANPEventFlags;









#define kJavaSurface_ANPGetValue            ((NPPVariable)2000)













struct ANPInterface {
    uint32_t    inSize;     
};

enum ANPLogTypes {
    kError_ANPLogType   = 0,    
    kWarning_ANPLogType = 1,    
    kDebug_ANPLogType   = 2     
};
typedef int32_t ANPLogType;

struct ANPLogInterfaceV0 : ANPInterface {
    


    void (*log)(ANPLogType, const char format[], ...);
};

struct ANPBitmapInterfaceV0 : ANPInterface {
    


    bool (*getPixelPacking)(ANPBitmapFormat, ANPPixelPacking* packing);
};

struct ANPMatrixInterfaceV0 : ANPInterface {
    

    ANPMatrix*  (*newMatrix)();
    

    void        (*deleteMatrix)(ANPMatrix*);

    ANPMatrixFlag (*getFlags)(const ANPMatrix*);

    void        (*copy)(ANPMatrix* dst, const ANPMatrix* src);

    





    void        (*get3x3)(const ANPMatrix*, float[9]);
    





    void        (*set3x3)(ANPMatrix*, const float[9]);

    void        (*setIdentity)(ANPMatrix*);
    void        (*preTranslate)(ANPMatrix*, float tx, float ty);
    void        (*postTranslate)(ANPMatrix*, float tx, float ty);
    void        (*preScale)(ANPMatrix*, float sx, float sy);
    void        (*postScale)(ANPMatrix*, float sx, float sy);
    void        (*preSkew)(ANPMatrix*, float kx, float ky);
    void        (*postSkew)(ANPMatrix*, float kx, float ky);
    void        (*preRotate)(ANPMatrix*, float degrees);
    void        (*postRotate)(ANPMatrix*, float degrees);
    void        (*preConcat)(ANPMatrix*, const ANPMatrix*);
    void        (*postConcat)(ANPMatrix*, const ANPMatrix*);

    


    bool        (*invert)(ANPMatrix* dst, const ANPMatrix* src);

    




    void        (*mapPoints)(ANPMatrix*, float dst[], const float src[],
                             int32_t count);
};

struct ANPPathInterfaceV0 : ANPInterface {
    
    ANPPath* (*newPath)();

    
    void (*deletePath)(ANPPath*);

    


    void (*copy)(ANPPath* dst, const ANPPath* src);

    

    bool (*equal)(const ANPPath* path0, const ANPPath* path1);

    
    void (*reset)(ANPPath*);

    
    bool (*isEmpty)(const ANPPath*);

    
    void (*getBounds)(const ANPPath*, ANPRectF* bounds);

    void (*moveTo)(ANPPath*, float x, float y);
    void (*lineTo)(ANPPath*, float x, float y);
    void (*quadTo)(ANPPath*, float x0, float y0, float x1, float y1);
    void (*cubicTo)(ANPPath*, float x0, float y0, float x1, float y1,
                    float x2, float y2);
    void (*close)(ANPPath*);

    




    void (*offset)(ANPPath* src, float dx, float dy, ANPPath* dst);

    




    void (*transform)(ANPPath* src, const ANPMatrix*, ANPPath* dst);
};








typedef uint32_t ANPColor;
#define ANPColor_ASHIFT     24
#define ANPColor_RSHIFT     16
#define ANPColor_GSHIFT     8
#define ANPColor_BSHIFT     0
#define ANP_MAKE_COLOR(a, r, g, b)  \
                   (((a) << ANPColor_ASHIFT) |  \
                    ((r) << ANPColor_RSHIFT) |  \
                    ((g) << ANPColor_GSHIFT) |  \
                    ((b) << ANPColor_BSHIFT))

enum ANPPaintFlag {
    kAntiAlias_ANPPaintFlag         = 1 << 0,
    kFilterBitmap_ANPPaintFlag      = 1 << 1,
    kDither_ANPPaintFlag            = 1 << 2,
    kUnderlineText_ANPPaintFlag     = 1 << 3,
    kStrikeThruText_ANPPaintFlag    = 1 << 4,
    kFakeBoldText_ANPPaintFlag      = 1 << 5,
};
typedef uint32_t ANPPaintFlags;

enum ANPPaintStyles {
    kFill_ANPPaintStyle             = 0,
    kStroke_ANPPaintStyle           = 1,
    kFillAndStroke_ANPPaintStyle    = 2
};
typedef int32_t ANPPaintStyle;

enum ANPPaintCaps {
    kButt_ANPPaintCap   = 0,
    kRound_ANPPaintCap  = 1,
    kSquare_ANPPaintCap = 2
};
typedef int32_t ANPPaintCap;

enum ANPPaintJoins {
    kMiter_ANPPaintJoin = 0,
    kRound_ANPPaintJoin = 1,
    kBevel_ANPPaintJoin = 2
};
typedef int32_t ANPPaintJoin;

enum ANPPaintAligns {
    kLeft_ANPPaintAlign     = 0,
    kCenter_ANPPaintAlign   = 1,
    kRight_ANPPaintAlign    = 2
};
typedef int32_t ANPPaintAlign;

enum ANPTextEncodings {
    kUTF8_ANPTextEncoding   = 0,
    kUTF16_ANPTextEncoding  = 1,
};
typedef int32_t ANPTextEncoding;

enum ANPTypefaceStyles {
    kBold_ANPTypefaceStyle      = 1 << 0,
    kItalic_ANPTypefaceStyle    = 1 << 1
};
typedef uint32_t ANPTypefaceStyle;

typedef uint32_t ANPFontTableTag;

struct ANPFontMetrics {
    
    float   fTop;
    
    float   fAscent;
    
    float   fDescent;
    
    float   fBottom;
    
    float   fLeading;
};

struct ANPTypefaceInterfaceV0 : ANPInterface {
    












    ANPTypeface* (*createFromName)(const char name[], ANPTypefaceStyle);

    









    ANPTypeface* (*createFromTypeface)(const ANPTypeface* family,
                                       ANPTypefaceStyle);

    



    int32_t (*getRefCount)(const ANPTypeface*);

    

    void (*ref)(ANPTypeface*);

    


    void (*unref)(ANPTypeface*);

    

    ANPTypefaceStyle (*getStyle)(const ANPTypeface*);

    

























    int32_t (*getFontPath)(const ANPTypeface*, char path[], int32_t length,
                           int32_t* index);

    



    const char* (*getFontDirectoryPath)();
};

struct ANPPaintInterfaceV0 : ANPInterface {
    






    ANPPaint*   (*newPaint)();
    void        (*deletePaint)(ANPPaint*);

    ANPPaintFlags (*getFlags)(const ANPPaint*);
    void        (*setFlags)(ANPPaint*, ANPPaintFlags);

    ANPColor    (*getColor)(const ANPPaint*);
    void        (*setColor)(ANPPaint*, ANPColor);

    ANPPaintStyle (*getStyle)(const ANPPaint*);
    void        (*setStyle)(ANPPaint*, ANPPaintStyle);

    float       (*getStrokeWidth)(const ANPPaint*);
    float       (*getStrokeMiter)(const ANPPaint*);
    ANPPaintCap (*getStrokeCap)(const ANPPaint*);
    ANPPaintJoin (*getStrokeJoin)(const ANPPaint*);
    void        (*setStrokeWidth)(ANPPaint*, float);
    void        (*setStrokeMiter)(ANPPaint*, float);
    void        (*setStrokeCap)(ANPPaint*, ANPPaintCap);
    void        (*setStrokeJoin)(ANPPaint*, ANPPaintJoin);

    ANPTextEncoding (*getTextEncoding)(const ANPPaint*);
    ANPPaintAlign (*getTextAlign)(const ANPPaint*);
    float       (*getTextSize)(const ANPPaint*);
    float       (*getTextScaleX)(const ANPPaint*);
    float       (*getTextSkewX)(const ANPPaint*);
    void        (*setTextEncoding)(ANPPaint*, ANPTextEncoding);
    void        (*setTextAlign)(ANPPaint*, ANPPaintAlign);
    void        (*setTextSize)(ANPPaint*, float);
    void        (*setTextScaleX)(ANPPaint*, float);
    void        (*setTextSkewX)(ANPPaint*, float);

    


    ANPTypeface* (*getTypeface)(const ANPPaint*);

    



    void (*setTypeface)(ANPPaint*, ANPTypeface*);

    


    float (*measureText)(ANPPaint*, const void* text, uint32_t byteLength,
                         ANPRectF* bounds);

    




    int (*getTextWidths)(ANPPaint*, const void* text, uint32_t byteLength,
                         float widths[], ANPRectF bounds[]);

    



    float (*getFontMetrics)(ANPPaint*, ANPFontMetrics* metrics);
};

struct ANPCanvasInterfaceV0 : ANPInterface {
    










    ANPCanvas*  (*newCanvas)(const ANPBitmap*);
    void        (*deleteCanvas)(ANPCanvas*);

    void        (*save)(ANPCanvas*);
    void        (*restore)(ANPCanvas*);
    void        (*translate)(ANPCanvas*, float tx, float ty);
    void        (*scale)(ANPCanvas*, float sx, float sy);
    void        (*rotate)(ANPCanvas*, float degrees);
    void        (*skew)(ANPCanvas*, float kx, float ky);
    void        (*concat)(ANPCanvas*, const ANPMatrix*);
    void        (*clipRect)(ANPCanvas*, const ANPRectF*);
    void        (*clipPath)(ANPCanvas*, const ANPPath*);

    

    void        (*getTotalMatrix)(ANPCanvas*, ANPMatrix*);
    



    bool        (*getLocalClipBounds)(ANPCanvas*, ANPRectF* bounds, bool aa);
    


    bool        (*getDeviceClipBounds)(ANPCanvas*, ANPRectI* bounds);

    void        (*drawColor)(ANPCanvas*, ANPColor);
    void        (*drawPaint)(ANPCanvas*, const ANPPaint*);
    void        (*drawLine)(ANPCanvas*, float x0, float y0, float x1, float y1,
                            const ANPPaint*);
    void        (*drawRect)(ANPCanvas*, const ANPRectF*, const ANPPaint*);
    void        (*drawOval)(ANPCanvas*, const ANPRectF*, const ANPPaint*);
    void        (*drawPath)(ANPCanvas*, const ANPPath*, const ANPPaint*);
    void        (*drawText)(ANPCanvas*, const void* text, uint32_t byteLength,
                            float x, float y, const ANPPaint*);
    void       (*drawPosText)(ANPCanvas*, const void* text, uint32_t byteLength,
                               const float xy[], const ANPPaint*);
    void        (*drawBitmap)(ANPCanvas*, const ANPBitmap*, float x, float y,
                              const ANPPaint*);
    void        (*drawBitmapRect)(ANPCanvas*, const ANPBitmap*,
                                  const ANPRectI* src, const ANPRectF* dst,
                                  const ANPPaint*);
};

struct ANPWindowInterfaceV0 : ANPInterface {
    








    void (*setVisibleRects)(NPP instance, const ANPRectI rects[], int32_t count);
    


    void    (*clearVisibleRects)(NPP instance);
    




    void    (*showKeyboard)(NPP instance, bool value);
    



    void    (*requestFullScreen)(NPP instance);
    


    void    (*exitFullScreen)(NPP instance);
    

    void    (*requestCenterFitZoom)(NPP instance);
};

struct ANPWindowInterfaceV1 : ANPWindowInterfaceV0 {
    



    ANPRectI (*visibleRect)(NPP instance);
};

enum ANPScreenOrientations {
    

    kDefault_ANPScreenOrientation        = 0,
    


    kFixedLandscape_ANPScreenOrientation = 1,
    


    kFixedPortrait_ANPScreenOrientation  = 2,
    


    kLandscape_ANPScreenOrientation      = 3,
    


    kPortrait_ANPScreenOrientation       = 4
};

typedef int32_t ANPScreenOrientation;

struct ANPWindowInterfaceV2 : ANPWindowInterfaceV1 {
    




    void (*requestFullScreenOrientation)(NPP instance, ANPScreenOrientation orientation);
};



enum ANPSampleFormats {
    kUnknown_ANPSamleFormat     = 0,
    kPCM16Bit_ANPSampleFormat   = 1,
    kPCM8Bit_ANPSampleFormat    = 2
};
typedef int32_t ANPSampleFormat;





struct ANPAudioBuffer {
    
    int32_t     channelCount;
    
    ANPSampleFormat  format;
    




    void*       bufferData;
    



    uint32_t    size;
};

enum ANPAudioEvents {
    


    kMoreData_ANPAudioEvent = 0,
    



    kUnderRun_ANPAudioEvent = 1
};
typedef int32_t ANPAudioEvent;











typedef void (*ANPAudioCallbackProc)(ANPAudioEvent event, void* user,
                                     ANPAudioBuffer* buffer);

struct ANPAudioTrack;   

struct ANPAudioTrackInterfaceV0 : ANPInterface {
    



    ANPAudioTrack*  (*newTrack)(uint32_t sampleRate,    
                                ANPSampleFormat,
                                int channelCount,       
                                ANPAudioCallbackProc,
                                void* user);
    



    void (*deleteTrack)(ANPAudioTrack*);

    void (*start)(ANPAudioTrack*);
    void (*pause)(ANPAudioTrack*);
    void (*stop)(ANPAudioTrack*);
    


    bool (*isStopped)(ANPAudioTrack*);
};

struct ANPAudioTrackInterfaceV1 : ANPAudioTrackInterfaceV0 {
    
    uint32_t (*trackLatency)(ANPAudioTrack*);
};





enum ANPEventTypes {
    kNull_ANPEventType          = 0,
    kKey_ANPEventType           = 1,
    




    kMouse_ANPEventType         = 2,
    



    kTouch_ANPEventType         = 3,
    


    kDraw_ANPEventType          = 4,
    kLifecycle_ANPEventType     = 5,

    












    kCustom_ANPEventType   = 6,
};
typedef int32_t ANPEventType;

enum ANPKeyActions {
    kDown_ANPKeyAction  = 0,
    kUp_ANPKeyAction    = 1,
};
typedef int32_t ANPKeyAction;

#include "ANPKeyCodes.h"
typedef int32_t ANPKeyCode;

enum ANPKeyModifiers {
    kAlt_ANPKeyModifier     = 1 << 0,
    kShift_ANPKeyModifier   = 1 << 1,
};

typedef uint32_t ANPKeyModifier;

enum ANPMouseActions {
    kDown_ANPMouseAction  = 0,
    kUp_ANPMouseAction    = 1,
};
typedef int32_t ANPMouseAction;

enum ANPTouchActions {
    




    kDown_ANPTouchAction        = 0,
    kUp_ANPTouchAction          = 1,
    kMove_ANPTouchAction        = 2,
    kCancel_ANPTouchAction      = 3,
    
    kLongPress_ANPTouchAction   = 4,
    kDoubleTap_ANPTouchAction   = 5,
};
typedef int32_t ANPTouchAction;

enum ANPLifecycleActions {
    


    kPause_ANPLifecycleAction           = 0,
    


    kResume_ANPLifecycleAction          = 1,
    


    kGainFocus_ANPLifecycleAction       = 2,
    


    kLoseFocus_ANPLifecycleAction       = 3,
    



    kFreeMemory_ANPLifecycleAction      = 4,
    


    kOnLoad_ANPLifecycleAction          = 5,
    



    kEnterFullScreen_ANPLifecycleAction = 6,
    



    kExitFullScreen_ANPLifecycleAction  = 7,
    


    kOnScreen_ANPLifecycleAction        = 8,
    


    kOffScreen_ANPLifecycleAction       = 9,
};
typedef uint32_t ANPLifecycleAction;


struct ANPEvent {
    uint32_t        inSize;  
    ANPEventType    eventType;
    
    union {
        struct {
            ANPKeyAction    action;
            ANPKeyCode      nativeCode;
            int32_t         virtualCode;    
            ANPKeyModifier  modifiers;
            int32_t         repeatCount;    
            int32_t         unichar;        
        } key;
        struct {
            ANPMouseAction  action;
            int32_t         x;  
            int32_t         y;  
        } mouse;
        struct {
            ANPTouchAction  action;
            ANPKeyModifier  modifiers;
            int32_t         x;  
            int32_t         y;  
        } touch;
        struct {
            ANPLifecycleAction  action;
        } lifecycle;
        struct {
            ANPDrawingModel model;
            
            ANPRectI        clip;
            
            union {
                ANPBitmap   bitmap;
                struct {
                    int32_t width;
                    int32_t height;
                } surfaceSize;
            } data;
        } draw;
    } data;
};


struct ANPEventInterfaceV0 : ANPInterface {
    




    void (*postEvent)(NPP inst, const ANPEvent* event);
};

struct ANPSystemInterfaceV0 : ANPInterface {
    


    const char* (*getApplicationDataDirectory)();

    








    jclass (*loadJavaClass)(NPP instance, const char* className);
};

struct ANPSurfaceInterfaceV0 : ANPInterface {
  






  bool (*lock)(JNIEnv* env, jobject surface, ANPBitmap* bitmap, ANPRectI* dirtyRect);
  



  void (*unlock)(JNIEnv* env, jobject surface);
};




struct ANPTextureInfo {
    GLuint      textureId;
    uint32_t    width;
    uint32_t    height;
    GLenum      internalFormat;
};

typedef void* ANPEGLContext;

struct ANPOpenGLInterfaceV0 : ANPInterface {
    ANPEGLContext (*acquireContext)(NPP instance);

    ANPTextureInfo (*lockTexture)(NPP instance);

    void (*releaseTexture)(NPP instance, const ANPTextureInfo*);

    



    void (*invertPluginContent)(NPP instance, bool isContentInverted);
};

enum ANPPowerStates {
    kDefault_ANPPowerState  = 0,
    kScreenOn_ANPPowerState = 1
};
typedef int32_t ANPPowerState;

struct ANPSystemInterfaceV1 : ANPSystemInterfaceV0 {
    void (*setPowerState)(NPP instance, ANPPowerState powerState);
};

struct ANPSystemInterfaceV2 : ANPInterface {
    



    const char* (*getApplicationDataDirectory)(NPP instance);

    
    jclass (*loadJavaClass)(NPP instance, const char* className);
    void (*setPowerState)(NPP instance, ANPPowerState powerState);
};

typedef void* ANPNativeWindow;

struct ANPVideoInterfaceV0 : ANPInterface {

    











    ANPNativeWindow (*acquireNativeWindow)(NPP instance);

    





    void (*setWindowDimensions)(NPP instance, const ANPNativeWindow window, const ANPRectF* dimensions);

    

    void (*releaseNativeWindow)(NPP instance, ANPNativeWindow window);
};







typedef void (*ANPVideoFrameCallbackProc)(ANPNativeWindow* window, int64_t timestamp);

struct ANPVideoInterfaceV1 : ANPVideoInterfaceV0 {
    


    void (*setFramerateCallback)(NPP instance, const ANPNativeWindow window, ANPVideoFrameCallbackProc);
};

struct ANPNativeWindowInterfaceV0 : ANPInterface {
    







    ANPNativeWindow (*acquireNativeWindow)(NPP instance);

    



    void (*invertPluginContent)(NPP instance, bool isContentInverted);
};


#endif
