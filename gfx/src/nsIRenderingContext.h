







































#ifndef nsIRenderingContext_h___
#define nsIRenderingContext_h___

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsSize.h"
#include <stdio.h>

class nsIWidget;
class nsIFontMetrics;
class nsTransform2D;
class nsString;
class nsIDeviceContext;
class nsIntRegion;
class nsIAtom;

struct nsFont;
struct nsTextDimensions;
class gfxUserFontSet;
#ifdef MOZ_MATHML
struct nsBoundingMetrics;
#endif

class gfxASurface;
class gfxContext;


class imgIContainer;



typedef enum
{
  nsClipCombine_kIntersect = 0,
  nsClipCombine_kUnion = 1,
  nsClipCombine_kSubtract = 2,
  nsClipCombine_kReplace = 3
} nsClipCombine;


typedef enum
{
  nsLineStyle_kNone   = 0,
  nsLineStyle_kSolid  = 1,
  nsLineStyle_kDashed = 2,
  nsLineStyle_kDotted = 3
} nsLineStyle;

typedef enum
{
  nsPenMode_kNone   = 0,
  nsPenMode_kInvert = 1
} nsPenMode;



#define NS_IRENDERING_CONTEXT_IID \
{ 0xefbfeb6c, 0x937e, 0x4889, \
  { 0x92, 0x46, 0x16, 0xc0, 0xe8, 0x4b, 0xfa, 0xae } }




class nsIRenderingContext : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRENDERING_CONTEXT_IID)

  
  

  





  NS_IMETHOD Init(nsIDeviceContext* aContext, nsIWidget *aWidget) = 0;

  





  NS_IMETHOD Init(nsIDeviceContext* aContext, gfxASurface* aThebesSurface) = 0;

  





  NS_IMETHOD Init(nsIDeviceContext* aContext, gfxContext* aThebesContext) = 0;

  




  virtual already_AddRefed<nsIDeviceContext> GetDeviceContext() = 0;

  


  NS_IMETHOD PushState(void) = 0;

  


  NS_IMETHOD PopState(void) = 0;

  
  NS_IMETHOD PushFilter(const nsRect& aRect, PRBool aAreaIsOpaque, float aOpacity)
  { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD PopFilter()
  { return NS_ERROR_NOT_IMPLEMENTED; }

  






  NS_IMETHOD SetClipRect(const nsRect& aRect, nsClipCombine aCombine) = 0;

  




  NS_IMETHOD SetLineStyle(nsLineStyle aLineStyle) = 0;

  






  NS_IMETHOD SetClipRegion(const nsIntRegion& aRegion, nsClipCombine aCombine) = 0;

  



  NS_IMETHOD SetColor(nscolor aColor) = 0;

  



  NS_IMETHOD GetColor(nscolor &aColor) const = 0;

  



  NS_IMETHOD SetFont(const nsFont& aFont, nsIAtom* aLanguage,
                     gfxUserFontSet *aUserFontSet) = 0;

  



  NS_IMETHOD SetFont(const nsFont& aFont,
                     gfxUserFontSet *aUserFontSet) = 0;

  




  NS_IMETHOD SetFont(nsIFontMetrics *aFontMetrics) = 0;

  



  virtual already_AddRefed<nsIFontMetrics> GetFontMetrics() = 0;

  



  NS_IMETHOD Translate(const nsPoint& aPt) = 0;

  




  NS_IMETHOD SetTranslation(const nsPoint& aPt) = 0;

  




  NS_IMETHOD Scale(float aSx, float aSy) = 0;

  struct PushedTranslation {
    float mSavedX, mSavedY;
  };

  class AutoPushTranslation {
    nsIRenderingContext* mCtx;
    PushedTranslation mPushed;
  public:
    AutoPushTranslation(nsIRenderingContext* aCtx, const nsPoint& aPt)
      : mCtx(aCtx) {
      mCtx->PushTranslation(&mPushed);
      mCtx->Translate(aPt);
    }
    ~AutoPushTranslation() {
      mCtx->PopTranslation(&mPushed);
    }
  };

  NS_IMETHOD PushTranslation(PushedTranslation* aState) = 0;

  NS_IMETHOD PopTranslation(PushedTranslation* aState) = 0;

  



  virtual nsTransform2D* GetCurrentTransform() = 0;

  




  NS_IMETHOD DrawLine(const nsPoint& aStartPt, const nsPoint& aEndPt) = 0;

  






  NS_IMETHOD DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1) = 0;

  



  NS_IMETHOD DrawRect(const nsRect& aRect) = 0;

  






  NS_IMETHOD DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;

  



  NS_IMETHOD FillRect(const nsRect& aRect) = 0;

  






  NS_IMETHOD FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;

  



  NS_IMETHOD InvertRect(const nsRect& aRect) = 0;

  






  NS_IMETHOD InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;

  




  NS_IMETHOD FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints) = 0;

  



  NS_IMETHOD DrawEllipse(const nsRect& aRect) = 0;

  






  NS_IMETHOD DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;

  



  NS_IMETHOD FillEllipse(const nsRect& aRect) = 0;

  






  NS_IMETHOD FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;

  






  NS_IMETHOD GetWidth(char aC, nscoord &aWidth) = 0;

  









  NS_IMETHOD GetWidth(PRUnichar aC, nscoord &aWidth,
                      PRInt32 *aFontID = nsnull) = 0;

  









  NS_IMETHOD GetWidth(const nsString& aString, nscoord &aWidth,
                      PRInt32 *aFontID = nsnull) = 0;

  






  NS_IMETHOD GetWidth(const char* aString, nscoord& aWidth) = 0;

  







  NS_IMETHOD GetWidth(const char* aString, PRUint32 aLength,
                      nscoord& aWidth) = 0;

  










  NS_IMETHOD GetWidth(const PRUnichar *aString, PRUint32 aLength,
                      nscoord &aWidth, PRInt32 *aFontID = nsnull) = 0;

  










  NS_IMETHOD GetTextDimensions(const char* aString, PRUint32 aLength,
                               nsTextDimensions& aDimensions) = 0;
  NS_IMETHOD GetTextDimensions(const PRUnichar* aString, PRUint32 aLength,
                               nsTextDimensions& aDimensions, PRInt32* aFontID = nsnull) = 0;

#if defined(_WIN32) || defined(XP_OS2) || defined(MOZ_X11)
  
























  NS_IMETHOD GetTextDimensions(const char*       aString,
                               PRInt32           aLength,
                               PRInt32           aAvailWidth,
                               PRInt32*          aBreaks,
                               PRInt32           aNumBreaks,
                               nsTextDimensions& aDimensions,
                               PRInt32&          aNumCharsFit,
                               nsTextDimensions& aLastWordDimensions,
                               PRInt32*          aFontID = nsnull) = 0;

  NS_IMETHOD GetTextDimensions(const PRUnichar*  aString,
                               PRInt32           aLength,
                               PRInt32           aAvailWidth,
                               PRInt32*          aBreaks,
                               PRInt32           aNumBreaks,
                               nsTextDimensions& aDimensions,
                               PRInt32&          aNumCharsFit,
                               nsTextDimensions& aLastWordDimensions,
                               PRInt32*          aFontID = nsnull) = 0;
#endif

  







  NS_IMETHOD DrawString(const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing = nsnull) = 0;

  










  NS_IMETHOD DrawString(const PRUnichar *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull) = 0;

  









  NS_IMETHOD DrawString(const nsString& aString, nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull) = 0;

  enum GraphicDataType {
    NATIVE_CAIRO_CONTEXT = 1,
    NATIVE_GDK_DRAWABLE = 2,
    NATIVE_WINDOWS_DC = 3,
    NATIVE_MAC_THING = 4,
    NATIVE_THEBES_CONTEXT = 5,
    NATIVE_OS2_PS = 6
  };
  



  virtual void* GetNativeGraphicData(GraphicDataType aType) = 0;

#ifdef MOZ_MATHML
  






  NS_IMETHOD
  GetBoundingMetrics(const char*        aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics) = 0;
  









  NS_IMETHOD
  GetBoundingMetrics(const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics,
                     PRInt32*           aFontID = nsnull) = 0;
#endif


  



  NS_IMETHOD SetRightToLeftText(PRBool aIsRTL) = 0;

  



  virtual void SetTextRunRTL(PRBool aIsRTL) = 0;

  














  virtual PRInt32 GetPosition(const PRUnichar *aText,
                              PRUint32 aLength,
                              nsPoint aPt) = 0;

  
















  NS_IMETHOD GetRangeWidth(const PRUnichar *aText,
                           PRUint32 aLength,
                           PRUint32 aStart,
                           PRUint32 aEnd,
                           PRUint32 &aWidth) = 0;

  






  NS_IMETHOD GetRangeWidth(const char *aText,
                           PRUint32 aLength,
                           PRUint32 aStart,
                           PRUint32 aEnd,
                           PRUint32 &aWidth) = 0;

  














  NS_IMETHOD RenderEPS(const nsRect& aRect, FILE *aDataFile) = 0;

  


  virtual gfxContext *ThebesContext() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRenderingContext, NS_IRENDERING_CONTEXT_IID)






#define NS_COPYBITS_USE_SOURCE_CLIP_REGION  0x0001



#define NS_COPYBITS_XFORM_SOURCE_VALUES     0x0002



#define NS_COPYBITS_XFORM_DEST_VALUES       0x0004








#define NS_COPYBITS_TO_BACK_BUFFER          0x0008





struct nsTextDimensions {
  
  nscoord ascent;

  
  nscoord descent;

  
  nscoord width;


  nsTextDimensions()
  {
    Clear();
  }

  
  void 
  Clear() {
    ascent = descent = width = 0;
  }

  
  void 
  Combine(const nsTextDimensions& aOther) {
    if (ascent < aOther.ascent) ascent = aOther.ascent;
    if (descent < aOther.descent) descent = aOther.descent;   
    width += aOther.width;
  }
};

#ifdef MOZ_MATHML



struct nsBoundingMetrics {

  
  

  
  
  
  
  
  

  
  
  
  

  
  

  nscoord leftBearing;
       


  nscoord rightBearing;
       



  
  nscoord ascent;
       


  nscoord descent;
       




  
  

  nscoord width;
       




  nsBoundingMetrics() {
    Clear();
  }

  
  

  
  void 
  Clear() {
    leftBearing = rightBearing = 0;
    ascent = descent = width = 0;
  }

  
  void 
  operator += (const nsBoundingMetrics& bm) {
    if (ascent + descent == 0 && rightBearing - leftBearing == 0) {
      ascent = bm.ascent;
      descent = bm.descent;
      leftBearing = width + bm.leftBearing;
      rightBearing = width + bm.rightBearing;
    }
    else {
      if (ascent < bm.ascent) ascent = bm.ascent;
      if (descent < bm.descent) descent = bm.descent;   
      leftBearing = PR_MIN(leftBearing, width + bm.leftBearing);
      rightBearing = PR_MAX(rightBearing, width + bm.rightBearing);
    }
    width += bm.width;
  }
};
#endif 

#endif 
