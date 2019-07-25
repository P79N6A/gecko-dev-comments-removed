












































#ifndef OMX_IVCommon_h
#define OMX_IVCommon_h

#ifdef __cplusplus
extern "C" {
#endif







#include <OMX_Core.h>













































typedef enum OMX_COLOR_FORMATTYPE {
    OMX_COLOR_FormatUnused,
    OMX_COLOR_FormatMonochrome,
    OMX_COLOR_Format8bitRGB332,
    OMX_COLOR_Format12bitRGB444,
    OMX_COLOR_Format16bitARGB4444,
    OMX_COLOR_Format16bitARGB1555,
    OMX_COLOR_Format16bitRGB565,
    OMX_COLOR_Format16bitBGR565,
    OMX_COLOR_Format18bitRGB666,
    OMX_COLOR_Format18bitARGB1665,
    OMX_COLOR_Format19bitARGB1666,
    OMX_COLOR_Format24bitRGB888,
    OMX_COLOR_Format24bitBGR888,
    OMX_COLOR_Format24bitARGB1887,
    OMX_COLOR_Format25bitARGB1888,
    OMX_COLOR_Format32bitBGRA8888,
    OMX_COLOR_Format32bitARGB8888,
    OMX_COLOR_FormatYUV411Planar,
    OMX_COLOR_FormatYUV411PackedPlanar,
    OMX_COLOR_FormatYUV420Planar,
    OMX_COLOR_FormatYUV420PackedPlanar,
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV422Planar,
    OMX_COLOR_FormatYUV422PackedPlanar,
    OMX_COLOR_FormatYUV422SemiPlanar,
    OMX_COLOR_FormatYCbYCr,
    OMX_COLOR_FormatYCrYCb,
    OMX_COLOR_FormatCbYCrY,
    OMX_COLOR_FormatCrYCbY,
    OMX_COLOR_FormatYUV444Interleaved,
    OMX_COLOR_FormatRawBayer8bit,
    OMX_COLOR_FormatRawBayer10bit,
    OMX_COLOR_FormatRawBayer8bitcompressed,
    OMX_COLOR_FormatL2,
    OMX_COLOR_FormatL4,
    OMX_COLOR_FormatL8,
    OMX_COLOR_FormatL16,
    OMX_COLOR_FormatL24,
    OMX_COLOR_FormatL32,
    OMX_COLOR_FormatYUV420PackedSemiPlanar,
    OMX_COLOR_FormatYUV422PackedSemiPlanar,
    OMX_COLOR_Format18BitBGR666,
    OMX_COLOR_Format24BitARGB6666,
    OMX_COLOR_Format24BitABGR6666,
    OMX_COLOR_FormatKhronosExtensions = 0x6F000000, 
    OMX_COLOR_FormatVendorStartUnused = 0x7F000000, 
    






    OMX_COLOR_FormatAndroidOpaque = 0x7F000789,
    OMX_TI_COLOR_FormatYUV420PackedSemiPlanar = 0x7F000100,
    OMX_QCOM_COLOR_FormatYVU420SemiPlanar = 0x7FA30C00,
    OMX_COLOR_FormatMax = 0x7FFFFFFF
} OMX_COLOR_FORMATTYPE;







typedef struct OMX_CONFIG_COLORCONVERSIONTYPE {
    OMX_U32 nSize;              
    OMX_VERSIONTYPE nVersion;   
    OMX_U32 nPortIndex;         
    OMX_S32 xColorMatrix[3][3]; 
    OMX_S32 xColorOffset[4];    
}OMX_CONFIG_COLORCONVERSIONTYPE;







typedef struct OMX_CONFIG_SCALEFACTORTYPE {
    OMX_U32 nSize;            
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex;       
    OMX_S32 xWidth;           
    OMX_S32 xHeight;          
}OMX_CONFIG_SCALEFACTORTYPE;





typedef enum OMX_IMAGEFILTERTYPE {
    OMX_ImageFilterNone,
    OMX_ImageFilterNoise,
    OMX_ImageFilterEmboss,
    OMX_ImageFilterNegative,
    OMX_ImageFilterSketch,
    OMX_ImageFilterOilPaint,
    OMX_ImageFilterHatch,
    OMX_ImageFilterGpen,
    OMX_ImageFilterAntialias,
    OMX_ImageFilterDeRing,
    OMX_ImageFilterSolarize,
    OMX_ImageFilterKhronosExtensions = 0x6F000000, 
    OMX_ImageFilterVendorStartUnused = 0x7F000000, 
    OMX_ImageFilterMax = 0x7FFFFFFF
} OMX_IMAGEFILTERTYPE;











typedef struct OMX_CONFIG_IMAGEFILTERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGEFILTERTYPE eImageFilter;
} OMX_CONFIG_IMAGEFILTERTYPE;















typedef struct OMX_CONFIG_COLORENHANCEMENTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bColorEnhancement;
    OMX_U8 nCustomizedU;
    OMX_U8 nCustomizedV;
} OMX_CONFIG_COLORENHANCEMENTTYPE;












typedef struct OMX_CONFIG_COLORKEYTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nARGBColor;
    OMX_U32 nARGBMask;
} OMX_CONFIG_COLORKEYTYPE;















typedef enum OMX_COLORBLENDTYPE {
    OMX_ColorBlendNone,
    OMX_ColorBlendAlphaConstant,
    OMX_ColorBlendAlphaPerPixel,
    OMX_ColorBlendAlternate,
    OMX_ColorBlendAnd,
    OMX_ColorBlendOr,
    OMX_ColorBlendInvert,
    OMX_ColorBlendKhronosExtensions = 0x6F000000, 
    OMX_ColorBlendVendorStartUnused = 0x7F000000, 
    OMX_ColorBlendMax = 0x7FFFFFFF
} OMX_COLORBLENDTYPE;












typedef struct OMX_CONFIG_COLORBLENDTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nRGBAlphaConstant;
    OMX_COLORBLENDTYPE  eColorBlend;
} OMX_CONFIG_COLORBLENDTYPE;












typedef struct OMX_FRAMESIZETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_FRAMESIZETYPE;











typedef struct OMX_CONFIG_ROTATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nRotation;
} OMX_CONFIG_ROTATIONTYPE;











typedef enum OMX_MIRRORTYPE {
    OMX_MirrorNone = 0,
    OMX_MirrorVertical,
    OMX_MirrorHorizontal,
    OMX_MirrorBoth,
    OMX_MirrorKhronosExtensions = 0x6F000000, 
    OMX_MirrorVendorStartUnused = 0x7F000000, 
    OMX_MirrorMax = 0x7FFFFFFF
} OMX_MIRRORTYPE;











typedef struct OMX_CONFIG_MIRRORTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_MIRRORTYPE  eMirror;
} OMX_CONFIG_MIRRORTYPE;












typedef struct OMX_CONFIG_POINTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nX;
    OMX_S32 nY;
} OMX_CONFIG_POINTTYPE;














typedef struct OMX_CONFIG_RECTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_CONFIG_RECTTYPE;











typedef struct OMX_PARAM_DEBLOCKINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDeblocking;
} OMX_PARAM_DEBLOCKINGTYPE;











typedef struct OMX_CONFIG_FRAMESTABTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStab;
} OMX_CONFIG_FRAMESTABTYPE;









typedef enum OMX_WHITEBALCONTROLTYPE {
    OMX_WhiteBalControlOff = 0,
    OMX_WhiteBalControlAuto,
    OMX_WhiteBalControlSunLight,
    OMX_WhiteBalControlCloudy,
    OMX_WhiteBalControlShade,
    OMX_WhiteBalControlTungsten,
    OMX_WhiteBalControlFluorescent,
    OMX_WhiteBalControlIncandescent,
    OMX_WhiteBalControlFlash,
    OMX_WhiteBalControlHorizon,
    OMX_WhiteBalControlKhronosExtensions = 0x6F000000, 
    OMX_WhiteBalControlVendorStartUnused = 0x7F000000, 
    OMX_WhiteBalControlMax = 0x7FFFFFFF
} OMX_WHITEBALCONTROLTYPE;











typedef struct OMX_CONFIG_WHITEBALCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_WHITEBALCONTROLTYPE eWhiteBalControl;
} OMX_CONFIG_WHITEBALCONTROLTYPE;





typedef enum OMX_EXPOSURECONTROLTYPE {
    OMX_ExposureControlOff = 0,
    OMX_ExposureControlAuto,
    OMX_ExposureControlNight,
    OMX_ExposureControlBackLight,
    OMX_ExposureControlSpotLight,
    OMX_ExposureControlSports,
    OMX_ExposureControlSnow,
    OMX_ExposureControlBeach,
    OMX_ExposureControlLargeAperture,
    OMX_ExposureControlSmallApperture,
    OMX_ExposureControlKhronosExtensions = 0x6F000000, 
    OMX_ExposureControlVendorStartUnused = 0x7F000000, 
    OMX_ExposureControlMax = 0x7FFFFFFF
} OMX_EXPOSURECONTROLTYPE;











typedef struct OMX_CONFIG_EXPOSURECONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_EXPOSURECONTROLTYPE eExposureControl;
} OMX_CONFIG_EXPOSURECONTROLTYPE;













typedef struct OMX_PARAM_SENSORMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFrameRate;
    OMX_BOOL bOneShot;
    OMX_FRAMESIZETYPE sFrameSize;
} OMX_PARAM_SENSORMODETYPE;











typedef struct OMX_CONFIG_CONTRASTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nContrast;
} OMX_CONFIG_CONTRASTTYPE;











typedef struct OMX_CONFIG_BRIGHTNESSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBrightness;
} OMX_CONFIG_BRIGHTNESSTYPE;













typedef struct OMX_CONFIG_BACKLIGHTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBacklight;
    OMX_U32 nTimeout;
} OMX_CONFIG_BACKLIGHTTYPE;











typedef struct OMX_CONFIG_GAMMATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nGamma;
} OMX_CONFIG_GAMMATYPE;












typedef struct OMX_CONFIG_SATURATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nSaturation;
} OMX_CONFIG_SATURATIONTYPE;












typedef struct OMX_CONFIG_LIGHTNESSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nLightness;
} OMX_CONFIG_LIGHTNESSTYPE;















typedef struct OMX_CONFIG_PLANEBLENDTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nDepth;
    OMX_U32 nAlpha;
} OMX_CONFIG_PLANEBLENDTYPE;















typedef struct OMX_PARAM_INTERLEAVETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_U32 nInterleavePortIndex;
} OMX_PARAM_INTERLEAVETYPE;





typedef enum OMX_TRANSITIONEFFECTTYPE {
    OMX_EffectNone,
    OMX_EffectFadeFromBlack,
    OMX_EffectFadeToBlack,
    OMX_EffectUnspecifiedThroughConstantColor,
    OMX_EffectDissolve,
    OMX_EffectWipe,
    OMX_EffectUnspecifiedMixOfTwoScenes,
    OMX_EffectKhronosExtensions = 0x6F000000, 
    OMX_EffectVendorStartUnused = 0x7F000000, 
    OMX_EffectMax = 0x7FFFFFFF
} OMX_TRANSITIONEFFECTTYPE;











typedef struct OMX_CONFIG_TRANSITIONEFFECTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TRANSITIONEFFECTTYPE eEffect;
} OMX_CONFIG_TRANSITIONEFFECTTYPE;







typedef enum OMX_DATAUNITTYPE {
    OMX_DataUnitCodedPicture,
    OMX_DataUnitVideoSegment,
    OMX_DataUnitSeveralSegments,
    OMX_DataUnitArbitraryStreamSection,
    OMX_DataUnitKhronosExtensions = 0x6F000000, 
    OMX_DataUnitVendorStartUnused = 0x7F000000, 
    OMX_DataUnitMax = 0x7FFFFFFF
} OMX_DATAUNITTYPE;







typedef enum OMX_DATAUNITENCAPSULATIONTYPE {
    OMX_DataEncapsulationElementaryStream,
    OMX_DataEncapsulationGenericPayload,
    OMX_DataEncapsulationRtpPayload,
    OMX_DataEncapsulationKhronosExtensions = 0x6F000000, 
    OMX_DataEncapsulationVendorStartUnused = 0x7F000000, 
    OMX_DataEncapsulationMax = 0x7FFFFFFF
} OMX_DATAUNITENCAPSULATIONTYPE;





typedef struct OMX_PARAM_DATAUNITTYPE {
    OMX_U32 nSize;            
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex;       
    OMX_DATAUNITTYPE eUnitType;
    OMX_DATAUNITENCAPSULATIONTYPE eEncapsulationType;
} OMX_PARAM_DATAUNITTYPE;





typedef enum OMX_DITHERTYPE {
    OMX_DitherNone,
    OMX_DitherOrdered,
    OMX_DitherErrorDiffusion,
    OMX_DitherOther,
    OMX_DitherKhronosExtensions = 0x6F000000, 
    OMX_DitherVendorStartUnused = 0x7F000000, 
    OMX_DitherMax = 0x7FFFFFFF
} OMX_DITHERTYPE;





typedef struct OMX_CONFIG_DITHERTYPE {
    OMX_U32 nSize;            
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex;       
    OMX_DITHERTYPE eDither;   
} OMX_CONFIG_DITHERTYPE;

typedef struct OMX_CONFIG_CAPTUREMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;     
    OMX_BOOL bContinuous;   

    OMX_BOOL bFrameLimited; 




    OMX_U32 nFrameLimit;      

} OMX_CONFIG_CAPTUREMODETYPE;

typedef enum OMX_METERINGTYPE {

    OMX_MeteringModeAverage,     
    OMX_MeteringModeSpot,  	      
    OMX_MeteringModeMatrix,      

    OMX_MeteringKhronosExtensions = 0x6F000000, 
    OMX_MeteringVendorStartUnused = 0x7F000000, 
    OMX_EVModeMax = 0x7fffffff
} OMX_METERINGTYPE;

typedef struct OMX_CONFIG_EXPOSUREVALUETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_METERINGTYPE eMetering;
    OMX_S32 xEVCompensation;      
    OMX_U32 nApertureFNumber;     
    OMX_BOOL bAutoAperture;		
    OMX_U32 nShutterSpeedMsec;    
    OMX_BOOL bAutoShutterSpeed;	
    OMX_U32 nSensitivity;         
    OMX_BOOL bAutoSensitivity;	
} OMX_CONFIG_EXPOSUREVALUETYPE;


















typedef struct OMX_CONFIG_FOCUSREGIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bCenter;
    OMX_BOOL bLeft;
    OMX_BOOL bRight;
    OMX_BOOL bTop;
    OMX_BOOL bBottom;
    OMX_BOOL bTopLeft;
    OMX_BOOL bTopRight;
    OMX_BOOL bBottomLeft;
    OMX_BOOL bBottomRight;
} OMX_CONFIG_FOCUSREGIONTYPE;




typedef enum OMX_FOCUSSTATUSTYPE {
    OMX_FocusStatusOff = 0,
    OMX_FocusStatusRequest,
    OMX_FocusStatusReached,
    OMX_FocusStatusUnableToReach,
    OMX_FocusStatusLost,
    OMX_FocusStatusKhronosExtensions = 0x6F000000, 
    OMX_FocusStatusVendorStartUnused = 0x7F000000, 
    OMX_FocusStatusMax = 0x7FFFFFFF
} OMX_FOCUSSTATUSTYPE;



















typedef struct OMX_PARAM_FOCUSSTATUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_FOCUSSTATUSTYPE eFocusStatus;
    OMX_BOOL bCenterStatus;
    OMX_BOOL bLeftStatus;
    OMX_BOOL bRightStatus;
    OMX_BOOL bTopStatus;
    OMX_BOOL bBottomStatus;
    OMX_BOOL bTopLeftStatus;
    OMX_BOOL bTopRightStatus;
    OMX_BOOL bBottomLeftStatus;
    OMX_BOOL bBottomRightStatus;
} OMX_PARAM_FOCUSSTATUSTYPE;



#ifdef __cplusplus
}
#endif 

#endif

