
#ifndef ICC_H
#define ICC_H
































































































#define icMagicNumber                   0x61637370L     /* 'acsp' */
#define icVersionNumber                 0x02100000L     /* 2.1.0, BCD */


#define icPrtrDefaultScreensFalse       0x00000000L     /* Bit pos 0 */
#define icPrtrDefaultScreensTrue        0x00000001L     /* Bit pos 0 */
#define icLinesPerInch                  0x00000002L     /* Bit pos 1 */
#define icLinesPerCm                    0x00000000L     /* Bit pos 1 */






#define icReflective                    0x00000000L     /* Bit pos 0 */
#define icTransparency                  0x00000001L     /* Bit pos 0 */
#define icGlossy                        0x00000000L     /* Bit pos 1 */
#define icMatte                         0x00000002L     /* Bit pos 1 */





#define icEmbeddedProfileFalse          0x00000000L     /* Bit pos 0 */
#define icEmbeddedProfileTrue           0x00000001L     /* Bit pos 0 */
#define icUseAnywhere                   0x00000000L     /* Bit pos 1 */
#define icUseWithEmbeddedDataOnly       0x00000002L     /* Bit pos 1 */


#define icAsciiData                     0x00000000L 
#define icBinaryData                    0x00000001L




#define icAny                           1











#ifdef PACKAGE_NAME






typedef @UINT8_T@	icUInt8Number;
typedef @UINT16_T@	icUInt16Number;
typedef @UINT32_T@	icUInt32Number;
typedef @UINT32_T@	icUInt64Number[2];

typedef @INT8_T@	icInt8Number;
typedef @INT16_T@	icInt16Number;
typedef @INT32_T@	icInt32Number;
typedef @INT32_T@	icInt64Number[2];

#else





#if defined (__digital__) && defined (__unix__)



#include <inttypes.h>

typedef uint8_t   icUInt8Number;
typedef uint16_t  icUInt16Number;
typedef uint32_t  icUInt32Number;
typedef uint32_t  icUInt64Number[2];

typedef int8_t     icInt8Number;
typedef int16_t    icInt16Number;
typedef int32_t    icInt32Number;
typedef int32_t    icInt64Number[2];

#else
#ifdef __sgi
#include "sgidefs.h"







typedef unsigned char   icUInt8Number;
typedef unsigned short  icUInt16Number;
typedef __uint32_t      icUInt32Number;
typedef __uint32_t      icUInt64Number[2];


typedef char            icInt8Number;
typedef short           icInt16Number;
typedef __int32_t       icInt32Number;
typedef __int32_t       icInt64Number[2];


#else   
#if defined(__GNUC__) || defined(__unix__) || defined(__unix)

#include <sys/types.h>

#if defined(__sun) || defined(__hpux) || defined (__MINGW) || defined(__MINGW32__)

#if defined (__MINGW) || defined(__MINGW32__)
#include <stdint.h>
#endif


typedef uint8_t   icUInt8Number;
typedef uint16_t  icUInt16Number;
typedef uint32_t  icUInt32Number;
typedef uint32_t  icUInt64Number[2];

#else


typedef u_int8_t   icUInt8Number;
typedef u_int16_t  icUInt16Number;
typedef u_int32_t  icUInt32Number;
typedef u_int32_t  icUInt64Number[2];

#endif



typedef int8_t     icInt8Number;
typedef int16_t    icInt16Number;
typedef int32_t    icInt32Number;
typedef int32_t    icInt64Number[2];


#else 






typedef unsigned char   icUInt8Number;
typedef unsigned short  icUInt16Number;
typedef unsigned long   icUInt32Number;
typedef unsigned long   icUInt64Number[2];


typedef char            icInt8Number;
typedef short           icInt16Number;
typedef long            icInt32Number;
typedef long            icInt64Number[2];


#endif  
#endif
#endif
#endif



typedef icInt32Number    icSignature;
typedef icInt32Number    icS15Fixed16Number;
typedef icUInt32Number   icU16Fixed16Number;




typedef enum {
    icSigAToB0Tag                       = 0x41324230L,   
    icSigAToB1Tag                       = 0x41324231L,  
    icSigAToB2Tag                       = 0x41324232L,   
    icSigBlueColorantTag                = 0x6258595AL,  
    icSigBlueTRCTag                     = 0x62545243L,  
    icSigBToA0Tag                       = 0x42324130L,  
    icSigBToA1Tag                       = 0x42324131L,  
    icSigBToA2Tag                       = 0x42324132L,  
    icSigCalibrationDateTimeTag         = 0x63616C74L,  
    icSigCharTargetTag                  = 0x74617267L,   
    icSigCopyrightTag                   = 0x63707274L,  
    icSigCrdInfoTag                     = 0x63726469L,  
    icSigDeviceMfgDescTag               = 0x646D6E64L,  
    icSigDeviceModelDescTag             = 0x646D6464L,  
    icSigGamutTag                       = 0x67616D74L,  
    icSigGrayTRCTag                     = 0x6b545243L,  
    icSigGreenColorantTag               = 0x6758595AL,  
    icSigGreenTRCTag                    = 0x67545243L,  
    icSigLuminanceTag                   = 0x6C756d69L,  
    icSigMeasurementTag                 = 0x6D656173L,  
    icSigMediaBlackPointTag             = 0x626B7074L,  
    icSigMediaWhitePointTag             = 0x77747074L,  
    icSigNamedColorTag                  = 0x6E636f6CL,  

    icSigNamedColor2Tag                 = 0x6E636C32L,  
    icSigPreview0Tag                    = 0x70726530L,  
    icSigPreview1Tag                    = 0x70726531L,  
    icSigPreview2Tag                    = 0x70726532L,  
    icSigProfileDescriptionTag          = 0x64657363L,  
    icSigProfileSequenceDescTag         = 0x70736571L,  
    icSigPs2CRD0Tag                     = 0x70736430L,  
    icSigPs2CRD1Tag                     = 0x70736431L,  
    icSigPs2CRD2Tag                     = 0x70736432L,  
    icSigPs2CRD3Tag                     = 0x70736433L,  
    icSigPs2CSATag                      = 0x70733273L,  
    icSigPs2RenderingIntentTag          = 0x70733269L,  
    icSigRedColorantTag                 = 0x7258595AL,  
    icSigRedTRCTag                      = 0x72545243L,  
    icSigScreeningDescTag               = 0x73637264L,  
    icSigScreeningTag                   = 0x7363726EL,  
    icSigTechnologyTag                  = 0x74656368L,  
    icSigUcrBgTag                       = 0x62666420L,  
    icSigViewingCondDescTag             = 0x76756564L,  
    icSigViewingConditionsTag           = 0x76696577L,  
    icMaxEnumTag                        = 0xFFFFFFFFL 
} icTagSignature;


typedef enum {
    icSigDigitalCamera                  = 0x6463616DL,  
    icSigFilmScanner                    = 0x6673636EL,  
    icSigReflectiveScanner              = 0x7273636EL,  
    icSigInkJetPrinter                  = 0x696A6574L,   
    icSigThermalWaxPrinter              = 0x74776178L,  
    icSigElectrophotographicPrinter     = 0x6570686FL,  
    icSigElectrostaticPrinter           = 0x65737461L,  
    icSigDyeSublimationPrinter          = 0x64737562L,  
    icSigPhotographicPaperPrinter       = 0x7270686FL,  
    icSigFilmWriter                     = 0x6670726EL,  
    icSigVideoMonitor                   = 0x7669646DL,  
    icSigVideoCamera                    = 0x76696463L,  
    icSigProjectionTelevision           = 0x706A7476L,  
    icSigCRTDisplay                     = 0x43525420L,  
    icSigPMDisplay                      = 0x504D4420L,  
    icSigAMDisplay                      = 0x414D4420L,  
    icSigPhotoCD                        = 0x4B504344L,  
    icSigPhotoImageSetter               = 0x696D6773L,  
    icSigGravure                        = 0x67726176L,  
    icSigOffsetLithography              = 0x6F666673L,  
    icSigSilkscreen                     = 0x73696C6BL,  
    icSigFlexography                    = 0x666C6578L,  
    icMaxEnumTechnology                 = 0xFFFFFFFFL   
} icTechnologySignature;


typedef enum {
    icSigCurveType                      = 0x63757276L,  
    icSigDataType                       = 0x64617461L,  
    icSigDateTimeType                   = 0x6474696DL,  
    icSigLut16Type                      = 0x6d667432L,  
    icSigLut8Type                       = 0x6d667431L,  
    icSigMeasurementType                = 0x6D656173L,  
    icSigNamedColorType                 = 0x6E636f6CL,  

    icSigProfileSequenceDescType        = 0x70736571L,  
    icSigS15Fixed16ArrayType            = 0x73663332L,  
    icSigScreeningType                  = 0x7363726EL,  
    icSigSignatureType                  = 0x73696720L,  
    icSigTextType                       = 0x74657874L,  
    icSigTextDescriptionType            = 0x64657363L,  
    icSigU16Fixed16ArrayType            = 0x75663332L,  
    icSigUcrBgType                      = 0x62666420L,  
    icSigUInt16ArrayType                = 0x75693136L,  
    icSigUInt32ArrayType                = 0x75693332L,  
    icSigUInt64ArrayType                = 0x75693634L,  
    icSigUInt8ArrayType                 = 0x75693038L,  
    icSigViewingConditionsType          = 0x76696577L,  
    icSigXYZType                        = 0x58595A20L,  
    icSigXYZArrayType                   = 0x58595A20L,  
    icSigNamedColor2Type                = 0x6E636C32L,  
    icSigCrdInfoType                    = 0x63726469L,  
    icMaxEnumType                       = 0xFFFFFFFFL   
} icTagTypeSignature;





 
typedef enum {
    icSigXYZData                        = 0x58595A20L,  
    icSigLabData                        = 0x4C616220L,  
    icSigLuvData                        = 0x4C757620L,  
    icSigYCbCrData                      = 0x59436272L,  
    icSigYxyData                        = 0x59787920L,  
    icSigRgbData                        = 0x52474220L,  
    icSigGrayData                       = 0x47524159L,  
    icSigHsvData                        = 0x48535620L,  
    icSigHlsData                        = 0x484C5320L,  
    icSigCmykData                       = 0x434D594BL,  
    icSigCmyData                        = 0x434D5920L,  
    icSig2colorData                     = 0x32434C52L,  
    icSig3colorData                     = 0x33434C52L,  
    icSig4colorData                     = 0x34434C52L,  
    icSig5colorData                     = 0x35434C52L,  
    icSig6colorData                     = 0x36434C52L,  
    icSig7colorData                     = 0x37434C52L,  
    icSig8colorData                     = 0x38434C52L,  
    icSig9colorData                     = 0x39434C52L,  
    icSig10colorData                    = 0x41434C52L,  
    icSig11colorData                    = 0x42434C52L,  
    icSig12colorData                    = 0x43434C52L,  
    icSig13colorData                    = 0x44434C52L,  
    icSig14colorData                    = 0x45434C52L,  
    icSig15colorData                    = 0x46434C52L,  
    icMaxEnumData                       = 0xFFFFFFFFL   
} icColorSpaceSignature;


typedef enum {
    icSigInputClass                     = 0x73636E72L,  
    icSigDisplayClass                   = 0x6D6E7472L,  
    icSigOutputClass                    = 0x70727472L,  
    icSigLinkClass                      = 0x6C696E6BL,  
    icSigAbstractClass                  = 0x61627374L,  
    icSigColorSpaceClass                = 0x73706163L,  
    icSigNamedColorClass                = 0x6e6d636cL,  
    icMaxEnumClass                      = 0xFFFFFFFFL  
} icProfileClassSignature;


typedef enum {
    icSigMacintosh                      = 0x4150504CL,  
    icSigMicrosoft                      = 0x4D534654L,  
    icSigSolaris                        = 0x53554E57L,  
    icSigSGI                            = 0x53474920L,  
    icSigTaligent                       = 0x54474E54L,  
    icMaxEnumPlatform                   = 0xFFFFFFFFL  
} icPlatformSignature;







typedef enum {
    icFlare0                            = 0x00000000L,  
    icFlare100                          = 0x00000001L,  
    icMaxFlare                          = 0xFFFFFFFFL   
} icMeasurementFlare;


typedef enum {
    icGeometryUnknown                   = 0x00000000L,  
    icGeometry045or450                  = 0x00000001L,  
    icGeometry0dord0                    = 0x00000002L,  
    icMaxGeometry                       = 0xFFFFFFFFL   
} icMeasurementGeometry;


typedef enum {
    icPerceptual                        = 0,
    icRelativeColorimetric              = 1,
    icSaturation                        = 2,
    icAbsoluteColorimetric              = 3,
    icMaxEnumIntent                     = 0xFFFFFFFFL   
} icRenderingIntent;


typedef enum {
    icSpotShapeUnknown                  = 0,
    icSpotShapePrinterDefault           = 1,
    icSpotShapeRound                    = 2,
    icSpotShapeDiamond                  = 3,
    icSpotShapeEllipse                  = 4,
    icSpotShapeLine                     = 5,
    icSpotShapeSquare                   = 6,
    icSpotShapeCross                    = 7,
    icMaxEnumSpot                       = 0xFFFFFFFFL   
} icSpotShape;


typedef enum {
    icStdObsUnknown                     = 0x00000000L,  
    icStdObs1931TwoDegrees              = 0x00000001L,  
    icStdObs1964TenDegrees              = 0x00000002L,  
    icMaxStdObs                         = 0xFFFFFFFFL   
} icStandardObserver;


typedef enum {
    icIlluminantUnknown                 = 0x00000000L,
    icIlluminantD50                     = 0x00000001L,
    icIlluminantD65                     = 0x00000002L,
    icIlluminantD93                     = 0x00000003L,
    icIlluminantF2                      = 0x00000004L,
    icIlluminantD55                     = 0x00000005L,
    icIlluminantA                       = 0x00000006L,
    icIlluminantEquiPowerE              = 0x00000007L,  
    icIlluminantF8                      = 0x00000008L,  
    icMaxEnumIluminant                  = 0xFFFFFFFFL   
} icIlluminant;








typedef struct {
    icInt8Number        data[icAny];    
} icInt8Array;


typedef struct {
    icUInt8Number       data[icAny];    
} icUInt8Array;


typedef struct {
    icUInt16Number      data[icAny];    
} icUInt16Array;


typedef struct {
    icInt16Number       data[icAny];    
} icInt16Array;


typedef struct {
    icUInt32Number      data[icAny];    
} icUInt32Array;


typedef struct {
    icInt32Number       data[icAny];    
} icInt32Array;


typedef struct {
    icUInt64Number      data[icAny];    
} icUInt64Array;


typedef struct {
    icInt64Number       data[icAny];    
} icInt64Array;
    

typedef struct {
    icU16Fixed16Number  data[icAny];    
} icU16Fixed16Array;


typedef struct {
    icS15Fixed16Number  data[icAny];    
} icS15Fixed16Array;


typedef struct {
    icUInt16Number      year;
    icUInt16Number      month;
    icUInt16Number      day;
    icUInt16Number      hours;
    icUInt16Number      minutes;
    icUInt16Number      seconds;
} icDateTimeNumber;


typedef struct {
    icS15Fixed16Number  X;
    icS15Fixed16Number  Y;
    icS15Fixed16Number  Z;
} icXYZNumber;


typedef struct {
    icXYZNumber         data[icAny];    
} icXYZArray;


typedef struct {
    icUInt32Number      count;          
    icUInt16Number      data[icAny];    




} icCurve;


typedef struct {
    icUInt32Number      dataFlag;       
    icInt8Number        data[icAny];    
} icData;


typedef struct {
    icUInt8Number       inputChan;      
    icUInt8Number       outputChan;     
    icUInt8Number       clutPoints;     
    icInt8Number        pad;            
    icS15Fixed16Number  e00;            
    icS15Fixed16Number  e01;                
    icS15Fixed16Number  e02;            
    icS15Fixed16Number  e10;            
    icS15Fixed16Number  e11;                
    icS15Fixed16Number  e12;             
    icS15Fixed16Number  e20;            
    icS15Fixed16Number  e21;                
    icS15Fixed16Number  e22;            
    icUInt16Number      inputEnt;       
    icUInt16Number      outputEnt;      
    icUInt16Number      data[icAny];    







} icLut16;


typedef struct {
    icUInt8Number       inputChan;      
    icUInt8Number       outputChan;     
    icUInt8Number       clutPoints;     
    icInt8Number        pad;
    icS15Fixed16Number  e00;            
    icS15Fixed16Number  e01;                
    icS15Fixed16Number  e02;            
    icS15Fixed16Number  e10;            
    icS15Fixed16Number  e11;                
    icS15Fixed16Number  e12;             
    icS15Fixed16Number  e20;            
    icS15Fixed16Number  e21;                
    icS15Fixed16Number  e22;            
    icUInt8Number       data[icAny];    







} icLut8;


typedef struct {
    icStandardObserver          stdObserver;    
    icXYZNumber                 backing;        
    icMeasurementGeometry       geometry;       
    icMeasurementFlare          flare;          
    icIlluminant                illuminant;     
} icMeasurement;






typedef struct {
    icUInt32Number      vendorFlag;     
    icUInt32Number      count;          
    icUInt32Number      nDeviceCoords;  
    icInt8Number        prefix[32];     
    icInt8Number        suffix[32];     
    icInt8Number        data[icAny];    
























} icNamedColor2;


typedef struct {
    icSignature                 deviceMfg;      
    icSignature                 deviceModel;    
    icUInt64Number              attributes;     
    icTechnologySignature       technology;     
    icInt8Number                data[icAny];    








} icDescStruct;


typedef struct {
    icUInt32Number      count;          
    icUInt8Number       data[icAny];    
} icProfileSequenceDesc;


typedef struct {
    icUInt32Number      count;          
    icInt8Number        data[icAny];    











} icTextDescription;


typedef struct {
    icS15Fixed16Number  frequency;      
    icS15Fixed16Number  angle;          
    icSpotShape         spotShape;      
} icScreeningData;

typedef struct {
    icUInt32Number      screeningFlag;  
    icUInt32Number      channels;       
    icScreeningData     data[icAny];    
} icScreening;


typedef struct {
    icInt8Number        data[icAny];    
} icText;


typedef struct {
    icUInt32Number      count;          
    icUInt16Number      curve[icAny];   
} icUcrBgCurve;


typedef struct {
    icInt8Number        data[icAny];            









} icUcrBg;


typedef struct {
    icXYZNumber         illuminant;     
    icXYZNumber         surround;       
    icIlluminant        stdIluminant;   
} icViewingCondition;


typedef struct {
    icUInt32Number      count;          
    icInt8Number        desc[icAny];    
} icCrdInfo;














typedef struct {
    icTagTypeSignature  sig;            
    icInt8Number        reserved[4];    
} icTagBase;


typedef struct {
    icTagBase           base;           
    icCurve             curve;          
} icCurveType;


typedef struct {
    icTagBase           base;           
    icData              data;           
} icDataType;


typedef struct {
    icTagBase           base;           
    icDateTimeNumber    date;           
} icDateTimeType;


typedef struct {
    icTagBase           base;           
    icLut16             lut;            
} icLut16Type;


typedef struct {
    icTagBase           base;           
    icLut8              lut;            
} icLut8Type;


typedef struct {
    icTagBase           base;           
    icMeasurement       measurement;    
} icMeasurementType;



typedef struct {
    icTagBase           base;           
    icNamedColor2       ncolor;         
} icNamedColor2Type;


typedef struct {
    icTagBase                   base;   
    icProfileSequenceDesc       desc;   
} icProfileSequenceDescType;


typedef struct {
    icTagBase                   base;   
    icTextDescription           desc;   
} icTextDescriptionType;


typedef struct {
    icTagBase           base;           
    icS15Fixed16Array   data;           
} icS15Fixed16ArrayType;

typedef struct {
    icTagBase           base;           
    icScreening         screen;         
} icScreeningType;


typedef struct {
    icTagBase           base;           
    icSignature         signature;      
} icSignatureType;


typedef struct {
    icTagBase           base;           
    icText              data;           
} icTextType;


typedef struct {
    icTagBase           base;           
    icU16Fixed16Array   data;           
} icU16Fixed16ArrayType;


typedef struct {
    icTagBase           base;           
    icUcrBg             data;           
} icUcrBgType;


typedef struct {
    icTagBase           base;           
    icUInt16Array       data;           
} icUInt16ArrayType;


typedef struct {
    icTagBase           base;           
    icUInt32Array       data;           
} icUInt32ArrayType;


typedef struct {
    icTagBase           base;           
    icUInt64Array       data;           
} icUInt64ArrayType;
    

typedef struct {
    icTagBase           base;           
    icUInt8Array        data;           
} icUInt8ArrayType;


typedef struct {
    icTagBase           base;           
    icViewingCondition  view;           
} icViewingConditionType;


typedef struct {
    icTagBase           base;           
    icXYZArray          data;           
} icXYZType;




typedef struct {
    icTagBase           base;           
    icCrdInfo           info;           
}icCrdInfoType;
     
      
      
      
     
    







typedef struct {
    icTagSignature      sig;            
    icUInt32Number      offset;         


    icUInt32Number      size;           
} icTag;


typedef struct {
    icUInt32Number      count;          
    icTag               tags[icAny];    
} icTagList;


typedef struct {
    icUInt32Number              size;           
    icSignature                 cmmId;          
    icUInt32Number              version;        
    icProfileClassSignature     deviceClass;    
    icColorSpaceSignature       colorSpace;     
    icColorSpaceSignature       pcs;            
    icDateTimeNumber            date;           
    icSignature                 magic;          
    icPlatformSignature         platform;       
    icUInt32Number              flags;          
    icSignature                 manufacturer;   
    icUInt32Number              model;          
    icUInt64Number              attributes;     
    icUInt32Number              renderingIntent;
    icXYZNumber                 illuminant;     
    icSignature                 creator;        
    icInt8Number                reserved[44];   
} icHeader;





typedef struct {
    icHeader            header;         
    icUInt32Number      count;          
    icInt8Number        data[icAny];    






} icProfile;           





typedef struct {
    icUInt32Number      vendorFlag;     
    icUInt32Number      count;          
    icInt8Number        data[icAny];    













} icNamedColor;


typedef struct {
    icTagBase           base;           
    icNamedColor        ncolor;         
} icNamedColorType;

#endif 
