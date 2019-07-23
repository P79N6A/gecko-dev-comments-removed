





































#ifndef nsAFMObject_h__
#define nsAFMObject_h__ 


#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"

class nsDeviceContextPS;



typedef enum
{
  kComment,

  
  kStartFontMetrics,
  kEndFontMetrics,
  kStartCompFontMetrics,
  kEndCompFontMetrics,
  kStartDescendent,
  kEndDescendent,
  kStartMasterFontMetrics,
  kEndMasterFontMetrics,

  
  kMetricsSets,
  kDescendents,
  kMasters,
  kAxes,

  
  kFontName,
  kFullName,
  kFamilyName,
  kWeight,
  kFontBBox,
  kVersion,
  kNotice,
  kEncodingScheme,
  kMappingScheme,
  kEscChar,
  kCharacterSet,
  kCharacters,
  kIsBaseFont,
  kVVector,
  kIsFixedV,
  kCapHeight,
  kXHeight,
  kAscender,
  kDescender,
  kWeightVector,
  kBlendDesignPositions,
  kBlendDesignMap,
  kBlendAxisTypes,


  
  kStartDirection,
  kEndDirection,
  kUnderlinePosition,
  kUnderlineThickness,
  kItalicAngle,
  kCharWidth,
  kIsFixedPitch,

  
  kStartCharMetrics,
  kEndCharMetrics,
  kC,
  kCH,
  kWX,
  kW0X,
  kW1X,
  kWY,
  kW0Y,
  kW1Y,
  kW,
  kW0,
  kW1,
  kVV,
  kN,
  kB,
  kL,

  
  kStartKernData,
  kEndKernData,
  kStartTrackKern,
  kEndTrackKern,
  kTrackKern,
  kStartKernPairs,
  kEndKernPairs,
  kKP,
  kKPH,
  kKPX,
  kKPY,

  
  kStartComposites,
  kEndComposites,
  kCC,
  kPCC,

  
  kStartAxis,
  kEndAxis,
  kAxisType,
  kAxisLabel,


  
  kStartMaster,
  kEndMaster

} AFMKey;




struct AFM_Single_Char_Metrics
{

  PRInt32   mCharacter_Code;      
  float     mW0x;                 
  float     mW0y;                 
  float     mW1x;                 
  float     mW1y;                 
  
  
  

  
  float     mLlx;
  float     mLly;
  float     mUrx;
  float     mUry;

  

  
};


typedef struct AFM_Single_Char_Metrics  AFMscm;



struct fontInformation
{
  double      mFontVersion;
  const char *mFontName;
  const char *mFullName;
  const char *mFamilyName;
  const char *mWeight;
  float       mFontBBox_llx;
  float       mFontBBox_lly;
  float       mFontBBox_urx;
  float       mFontBBox_ury;
  const char *mVersion;
  char       *mNotice;         
  const char *mEncodingScheme;
  PRInt32     mMappingScheme;
  PRInt32     mEscChar;
  const char *mCharacterSet;
  PRInt32     mCharacters;
  PRBool      mIsBaseFont;
  float       mVVector_0;
  float       mVVector_1;
  PRBool      mIsFixedV;
  float       mCapHeight;
  float       mXHeight;
  float       mAscender;
  float       mDescender;
  float       mUnderlinePosition;
  float       mUnderlineThickness;

  PRInt32     mNumCharacters;
  AFMscm     *mAFMCharMetrics;
};


typedef struct fontInformation AFMFontInformation;


class nsAFMObject 
{
public:

  



  nsAFMObject();

  



 virtual ~nsAFMObject();

  





  void    Init(nscoord  aFontHeight);


  





  PRBool AFM_ReadFile(const nsFont &aFontName);

  






  PRInt16 CheckBasicFonts(const nsFont &aFont,PRBool aPrimaryOnly=PR_FALSE);

  





  PRInt16 CreateSubstituteFont(const nsFont &aFont);

  





  void    SetFontSize(nscoord  aFontHeight) { mFontHeight = aFontHeight; }


  







  void    GetStringWidth(const PRUnichar *aString,nscoord& aWidth,nscoord aLength);

  






  void    GetStringWidth(const char *aString,nscoord& aWidth,nscoord aLength);

  






  void    WriteFontHeaderInformation(FILE *aOutFile);

  






  void    WriteFontCharInformation(FILE *aOutFile);
protected:

  




  void    GetKey(AFMKey *aTheKey);

  




  PRInt32 GetToken(void);

  




  PRInt32 MatchKey(char *aKey);

  




  PRInt32 GetLine(void);

  




  char*   GetAFMString (void);

  




  char*   GetAFMName (void); 

  




  void    GetAFMInt (PRInt32 *aInt) {GetToken();*aInt = atoi (mToken);}

  




  void    GetAFMNumber (double *aFloat){GetToken();*aFloat = atof (mToken);}
  void    GetAFMNumber (float  *aFloat){GetToken();*aFloat = atof (mToken);}

  




  void    GetAFMBool (PRBool *aBool);

  





  void    ReadCharMetrics (AFMFontInformation *aFontInfo,PRInt32 aNumCharacters);


public:
  AFMFontInformation  *mPSFontInfo;


protected:
  FILE                *mAFMFile;          
  char                mToken[256];        
  nscoord             mFontHeight;        
                                          
                                          

};

#define NUM_KEYS (sizeof (keynames) / sizeof (struct keyname_st) - 1)









struct AFM_SubstituteFonts
{
  const char*         mPSName;
  const char*         mFamily;
  PRUint16            mWeight;
  PRUint8             mStyle;
  const AFMFontInformation* mFontInfo;
  const AFMscm*             mCharInfo;
  PRInt32             mIndex;
};

typedef struct AFM_SubstituteFonts  DefFonts;

extern DefFonts gSubstituteFonts[];


#define NUM_AFM_FONTS 13

#endif 

