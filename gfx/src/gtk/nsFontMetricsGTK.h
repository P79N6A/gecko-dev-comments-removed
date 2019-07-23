




































#ifndef nsFontMetricsGTK_h__
#define nsFontMetricsGTK_h__

#include "nsDeviceContextGTK.h"
#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsRenderingContextGTK.h"
#include "nsICharRepresentable.h"
#include "nsCompressedCharMap.h"
#include "nsIFontMetricsGTK.h"
#ifdef MOZ_ENABLE_FREETYPE2
#include "nsIFontCatalogService.h"
#endif

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#undef FONT_HAS_GLYPH
#define FONT_HAS_GLYPH(map, char) IS_REPRESENTABLE(map, char)
#define WEIGHT_INDEX(weight) (((weight) / 100) - 1)

typedef struct nsFontCharSetInfo nsFontCharSetInfo;

typedef gint (*nsFontCharSetConverter)(nsFontCharSetInfo* aSelf,
  XFontStruct* aFont, const PRUnichar* aSrcBuf, PRInt32 aSrcLen,
  char* aDestBuf, PRInt32 aDestLen);

struct nsFontCharSet;
struct nsFontFamily;
struct nsFontNode;
struct nsFontStretch;
struct nsFontWeight;
class nsXFont;

class nsFontGTKUserDefined;
class nsFontMetricsGTK;
class nsFreeTypeFace;
class nsFontGTK;

struct nsFontStretch
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void SortSizes(void);

  nsFontGTK**        mSizes;
  PRUint16           mSizesAlloc;
  PRUint16           mSizesCount;

  char*              mScalable;
  PRBool             mOutlineScaled;
  nsVoidArray        mScaledFonts;
#ifdef MOZ_ENABLE_FREETYPE2
  nsITrueTypeFontCatalogEntry*   mFreeTypeFaceID;
#endif
};

struct nsFontStyle
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void FillWeightHoles(void);

  nsFontWeight* mWeights[9];
};

struct nsFontWeight
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void FillStretchHoles(void);

  nsFontStretch* mStretches[9];
};

struct nsFontNode
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void FillStyleHoles(void);

  nsCAutoString      mName;
  nsFontCharSetInfo* mCharSetInfo;
  nsFontStyle*       mStyles[3];
  PRUint8            mHolesFilled;
  PRUint8            mDummy;
};

class nsFontNodeArray : public nsAutoVoidArray
{
public:
  nsFontNodeArray() {};

  nsFontNode* GetElement(PRInt32 aIndex)
  {
    return (nsFontNode*) ElementAt(aIndex);
  };
};








typedef struct nsFontLangGroup {
  const char *mFontLangGroupName;
  nsIAtom*    mFontLangGroupAtom;
} nsFontLangGroup;

struct nsFontCharSetMap
{
  const char*        mName;
  nsFontLangGroup*   mFontLangGroup;
  nsFontCharSetInfo* mInfo;
};

class nsFontGTK
{
public:
  nsFontGTK();
  nsFontGTK(nsFontGTK*);
  virtual ~nsFontGTK();
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void LoadFont(void);
  PRBool IsEmptyFont(XFontStruct*);

  inline int SupportsChar(PRUint32 aChar)
    { return mCCMap && CCMAP_HAS_CHAR_EXT(mCCMap, aChar); };

  virtual GdkFont* GetGDKFont(void);
  virtual nsXFont* GetXFont(void);
  virtual PRBool   GetXFontIs10646(void);
  virtual PRBool   IsFreeTypeFont(void);
  virtual gint GetWidth(const PRUnichar* aString, PRUint32 aLength) = 0;
  virtual gint DrawString(nsRenderingContextGTK* aContext,
                          nsDrawingSurfaceGTK* aSurface, nscoord aX,
                          nscoord aY, const PRUnichar* aString,
                          PRUint32 aLength) = 0;
#ifdef MOZ_MATHML
  
  
  
  virtual nsresult
  GetBoundingMetrics(const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics) = 0;
#endif

  PRUint16*              mCCMap;
  nsFontCharSetInfo*     mCharSetInfo;
  char*                  mName;
  nsFontGTKUserDefined*  mUserDefinedFont;
  PRUint16               mSize;
  PRUint16               mAABaseSize;
  PRInt16                mBaselineAdjust;

  
  
  PRInt16                mMaxAscent;
  PRInt16                mMaxDescent;

protected:
  GdkFont*               mFont;
  GdkFont*               mFontHolder;
  nsXFont*               mXFont;
  PRBool                 mAlreadyCalledLoadFont;
};

struct nsFontSwitchGTK {
  
  
  nsFontGTK* mFontGTK;
};

typedef PRBool (*PR_CALLBACK nsFontSwitchCallbackGTK)
               (const nsFontSwitchGTK *aFontSwitch,
                const PRUnichar       *aSubstring,
                PRUint32               aSubstringLength,
                void                  *aData);

class nsFontMetricsGTK : public nsIFontMetricsGTK
{
public:
  nsFontMetricsGTK();
  virtual ~nsFontMetricsGTK();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup,
                   nsIDeviceContext* aContext);
  NS_IMETHOD  Destroy();

  NS_IMETHOD  GetXHeight(nscoord& aResult);
  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize);
  NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize);

  NS_IMETHOD  GetHeight(nscoord &aHeight);
  NS_IMETHOD  GetNormalLineHeight(nscoord &aHeight);
  NS_IMETHOD  GetLeading(nscoord &aLeading);
  NS_IMETHOD  GetEmHeight(nscoord &aHeight);
  NS_IMETHOD  GetEmAscent(nscoord &aAscent);
  NS_IMETHOD  GetEmDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxHeight(nscoord &aHeight);
  NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
  NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
  NS_IMETHOD  GetAveCharWidth(nscoord &aAveCharWidth);
  virtual PRInt32 GetMaxStringLength();
  NS_IMETHOD  GetLangGroup(nsIAtom** aLangGroup);
  NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);
  
  NS_IMETHOD  GetSpaceWidth(nscoord &aSpaceWidth);
  NS_IMETHOD  ResolveForwards(const PRUnichar* aString, PRUint32 aLength,
                              nsFontSwitchCallbackGTK aFunc, void* aData);

  nsFontGTK*  FindFont(PRUint32 aChar);
  nsFontGTK*  FindUserDefinedFont(PRUint32 aChar);
  nsFontGTK*  FindStyleSheetSpecificFont(PRUint32 aChar);
  nsFontGTK*  FindStyleSheetGenericFont(PRUint32 aChar);
  nsFontGTK*  FindLangGroupPrefFont(nsIAtom* aLangGroup, PRUint32 aChar);
  nsFontGTK*  FindLangGroupFont(nsIAtom* aLangGroup, PRUint32 aChar, nsCString* aName);
  nsFontGTK*  FindAnyFont(PRUint32 aChar);
  nsFontGTK*  FindSubstituteFont(PRUint32 aChar);

  nsFontGTK*  SearchNode(nsFontNode* aNode, PRUint32 aChar);
  nsFontGTK*  TryAliases(nsCString* aName, PRUint32 aChar);
  nsFontGTK*  TryFamily(nsCString* aName, PRUint32 aChar);
  nsFontGTK*  TryNode(nsCString* aName, PRUint32 aChar);
  nsFontGTK*  TryNodes(nsACString &aFFREName, PRUint32 aChar);
  nsFontGTK*  TryLangGroup(nsIAtom* aLangGroup, nsCString* aName, PRUint32 aChar);

  nsFontGTK*  AddToLoadedFontsList(nsFontGTK* aFont);
  nsFontGTK*  FindNearestSize(nsFontStretch* aStretch, PRUint16 aSize);
  nsFontGTK*  GetAASBBaseFont(nsFontStretch* aStretch, 
                              nsFontCharSetInfo* aCharSet);
  nsFontGTK*  PickASizeAndLoad(nsFontStretch* aStretch,
                               nsFontCharSetInfo* aCharSet, 
                               PRUint32 aChar,
                               const char *aName);

  
  virtual nsresult GetWidth(const char* aString, PRUint32 aLength,
                            nscoord& aWidth,
                            nsRenderingContextGTK *aContext);
  virtual nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                            nscoord& aWidth, PRInt32 *aFontID,
                            nsRenderingContextGTK *aContext);
  
  virtual nsresult GetTextDimensions(const PRUnichar* aString,
                                     PRUint32 aLength,
                                     nsTextDimensions& aDimensions, 
                                     PRInt32* aFontID,
                                     nsRenderingContextGTK *aContext);
  virtual nsresult GetTextDimensions(const char*         aString,
                                     PRInt32             aLength,
                                     PRInt32             aAvailWidth,
                                     PRInt32*            aBreaks,
                                     PRInt32             aNumBreaks,
                                     nsTextDimensions&   aDimensions,
                                     PRInt32&            aNumCharsFit,
                                     nsTextDimensions&   aLastWordDimensions,
                                     PRInt32*            aFontID,
                                     nsRenderingContextGTK *aContext);
  virtual nsresult GetTextDimensions(const PRUnichar*    aString,
                                     PRInt32             aLength,
                                     PRInt32             aAvailWidth,
                                     PRInt32*            aBreaks,
                                     PRInt32             aNumBreaks,
                                     nsTextDimensions&   aDimensions,
                                     PRInt32&            aNumCharsFit,
                                     nsTextDimensions&   aLastWordDimensions,
                                     PRInt32*            aFontID,
                                     nsRenderingContextGTK *aContext);

  virtual nsresult DrawString(const char *aString, PRUint32 aLength,
                              nscoord aX, nscoord aY,
                              const nscoord* aSpacing,
                              nsRenderingContextGTK *aContext,
                              nsDrawingSurfaceGTK *aSurface);
  virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                              nscoord aX, nscoord aY,
                              PRInt32 aFontID,
                              const nscoord* aSpacing,
                              nsRenderingContextGTK *aContext,
                              nsDrawingSurfaceGTK *aSurface);

#ifdef MOZ_MATHML
  virtual nsresult GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                      nsBoundingMetrics &aBoundingMetrics,
                                      nsRenderingContextGTK *aContext);
  virtual nsresult GetBoundingMetrics(const PRUnichar *aString,
                                      PRUint32 aLength,
                                      nsBoundingMetrics &aBoundingMetrics,
                                      PRInt32 *aFontID,
                                      nsRenderingContextGTK *aContext);
#endif 

  virtual GdkFont* GetCurrentGDKFont(void);

  virtual nsresult SetRightToLeftText(PRBool aIsRTL);
  virtual PRBool GetRightToLeftText();

  virtual nsresult GetClusterInfo(const PRUnichar *aText,
                                  PRUint32 aLength,
                                  PRUint8 *aClusterStarts);

  virtual PRInt32 GetPosition(const PRUnichar *aText,
                              PRUint32 aLength,
                              nsPoint aPt);

  virtual nsresult GetRangeWidth(const PRUnichar *aText,
                                 PRUint32 aLength,
                                 PRUint32 aStart,
                                 PRUint32 aEnd,
                                 PRUint32 &aWidth);

  virtual nsresult GetRangeWidth(const char *aText,
                                 PRUint32 aLength,
                                 PRUint32 aStart,
                                 PRUint32 aEnd,
                                 PRUint32 &aWidth);

  static nsresult FamilyExists(nsIDeviceContext *aDevice, const nsString& aName);
  static PRUint32 GetHints(void);

  

  nsFontGTK   **mLoadedFonts;
  PRUint16    mLoadedFontsAlloc;
  PRUint16    mLoadedFontsCount;

  nsFontGTK   *mSubstituteFont;

  nsCStringArray mFonts;
  PRInt32        mFontsIndex;
  nsAutoVoidArray   mFontIsGeneric;

  nsCAutoString     mDefaultFont;
  nsCString         *mGeneric;
  nsCOMPtr<nsIAtom> mLangGroup;
  nsCAutoString     mUserDefined;

  PRUint8 mTriedAllGenerics;
  PRUint8 mIsUserDefined;

protected:
  void RealizeFont();
  nsFontGTK* LocateFont(PRUint32 aChar, PRInt32 & aCount);

  nsIDeviceContext    *mDeviceContext;
  nsFontGTK           *mWesternFont;
  nsFontGTK           *mCurrentFont;

  nscoord             mLeading;
  nscoord             mEmHeight;
  nscoord             mEmAscent;
  nscoord             mEmDescent;
  nscoord             mMaxHeight;
  nscoord             mMaxAscent;
  nscoord             mMaxDescent;
  nscoord             mMaxAdvance;
  nscoord             mXHeight;
  nscoord             mSuperscriptOffset;
  nscoord             mSubscriptOffset;
  nscoord             mStrikeoutSize;
  nscoord             mStrikeoutOffset;
  nscoord             mUnderlineSize;
  nscoord             mUnderlineOffset;
  nscoord             mSpaceWidth;
  nscoord             mAveCharWidth;
  PRInt32             mMaxStringLength;

  PRUint16            mPixelSize;
  PRUint8             mStretchIndex;
  PRUint8             mStyleIndex;
  nsFontCharSetConverter mDocConverterType;
};

class nsFontEnumeratorGTK : public nsIFontEnumerator
{
public:
  nsFontEnumeratorGTK();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFONTENUMERATOR
};

class nsHashKey;
PRBool FreeNode(nsHashKey* aKey, void* aData, void* aClosure);
nsFontCharSetInfo *GetCharSetInfo(const char *aCharSetName);
#ifdef MOZ_ENABLE_FREETYPE2
void CharSetNameToCodeRangeBits(const char*, PRUint32*, PRUint32*);
#endif
nsFontCharSetMap *GetCharSetMap(const char *aCharSetName);




#endif
