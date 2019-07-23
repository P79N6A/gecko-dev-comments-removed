










































#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIImage.h"
#include "nsIFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsIViewManager.h"
#include "nsIPresShell.h"
#include "nsFrameManager.h"
#include "nsStyleContext.h"
#include "nsGkAtoms.h"
#include "nsIDrawingSurface.h"
#include "nsTransform2D.h"
#include "nsIDeviceContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIScrollableFrame.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsCSSRendering.h"
#include "nsCSSColorUtils.h"
#include "nsITheme.h"
#include "nsThemeConstants.h"
#include "nsIServiceManager.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMHTMLDocument.h"
#include "nsLayoutUtils.h"
#include "nsINameSpaceManager.h"

#include "gfxContext.h"

#define BORDER_FULL    0        //entire side
#define BORDER_INSIDE  1        //inside half
#define BORDER_OUTSIDE 2        //outside half


#define DOT_LENGTH  1           //square
#define DASH_LENGTH 3           //3 times longer than dot


#define SIDE_BIT_TOP (1 << NS_SIDE_TOP)
#define SIDE_BIT_RIGHT (1 << NS_SIDE_RIGHT)
#define SIDE_BIT_BOTTOM (1 << NS_SIDE_BOTTOM)
#define SIDE_BIT_LEFT (1 << NS_SIDE_LEFT)
#define SIDE_BITS_ALL (SIDE_BIT_TOP|SIDE_BIT_RIGHT|SIDE_BIT_BOTTOM|SIDE_BIT_LEFT)



#define MAXPATHSIZE 12
#define MAXPOLYPATHSIZE 1000

enum ePathTypes{
  eOutside =0,
  eInside,
  eCalc,
  eCalcRev
};





struct InlineBackgroundData
{
  InlineBackgroundData()
      : mFrame(nsnull)
  {
  }

  ~InlineBackgroundData()
  {
  }

  void Reset()
  {
    mBoundingBox.SetRect(0,0,0,0);
    mContinuationPoint = mUnbrokenWidth = 0;
    mFrame = nsnull;    
  }

  nsRect GetContinuousRect(nsIFrame* aFrame)
  {
    SetFrame(aFrame);

    
    
    
    return nsRect(-mContinuationPoint, 0, mUnbrokenWidth, mFrame->GetSize().height);
  }

  nsRect GetBoundingRect(nsIFrame* aFrame)
  {
    SetFrame(aFrame);

    
    
    
    
    
    nsRect boundingBox(mBoundingBox);
    nsPoint point = mFrame->GetPosition();
    boundingBox.MoveBy(-point.x, -point.y);

    return boundingBox;
  }

protected:
  nsIFrame*     mFrame;
  nscoord       mContinuationPoint;
  nscoord       mUnbrokenWidth;
  nsRect        mBoundingBox;

  void SetFrame(nsIFrame* aFrame)
  {
    NS_PRECONDITION(aFrame, "Need a frame");

    nsIFrame *prevInFlow = aFrame->GetPrevInFlow();

    if (!prevInFlow || mFrame != prevInFlow) {
      
      Reset();
      Init(aFrame);
      return;
    }

    
    
    mContinuationPoint += mFrame->GetSize().width;

    mFrame = aFrame;
  }

  void Init(nsIFrame* aFrame)
  {    
    
    
    nsIFrame* inlineFrame = aFrame->GetPrevInFlow();

    while (inlineFrame) {
      nsRect rect = inlineFrame->GetRect();
      mContinuationPoint += rect.width;
      mUnbrokenWidth += rect.width;
      mBoundingBox.UnionRect(mBoundingBox, rect);
      inlineFrame = inlineFrame->GetPrevInFlow();
    }

    
    
    inlineFrame = aFrame;
    while (inlineFrame) {
      nsRect rect = inlineFrame->GetRect();
      mUnbrokenWidth += rect.width;
      mBoundingBox.UnionRect(mBoundingBox, rect);
      inlineFrame = inlineFrame->GetNextInFlow();
    }

    mFrame = aFrame;
  }
};

static InlineBackgroundData* gInlineBGData = nsnull;

static void GetPath(nsFloatPoint aPoints[],nsPoint aPolyPath[],PRInt32 *aCurIndex,ePathTypes  aPathType,PRInt32 &aC1Index,float aFrac=0);


static void FillOrInvertRect(nsIRenderingContext& aRC,nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight, PRBool aInvert);
static void FillOrInvertRect(nsIRenderingContext& aRC,const nsRect& aRect, PRBool aInvert);


nsresult nsCSSRendering::Init()
{  
  NS_ASSERTION(!gInlineBGData, "Init called twice");
  gInlineBGData = new InlineBackgroundData();
  if (!gInlineBGData)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


void nsCSSRendering::Shutdown()
{
  delete gInlineBGData;
  gInlineBGData = nsnull;
}



void nsCSSRendering::DrawLine (nsIRenderingContext& aContext, 
                               nscoord aX1, nscoord aY1, nscoord aX2, nscoord aY2,
                               nsRect* aGap)
{
  if (nsnull == aGap) {
    aContext.DrawLine(aX1, aY1, aX2, aY2);
  } else {
    nscoord x1 = (aX1 < aX2) ? aX1 : aX2;
    nscoord x2 = (aX1 < aX2) ? aX2 : aX1;
    nsPoint gapUpperRight(aGap->x + aGap->width, aGap->y);
    nsPoint gapLowerRight(aGap->x + aGap->width, aGap->y + aGap->height);
    if ((aGap->y <= aY1) && (gapLowerRight.y >= aY2)) {
      if ((aGap->x > x1) && (aGap->x < x2)) {
        aContext.DrawLine(x1, aY1, aGap->x, aY1);
      } 
      if ((gapLowerRight.x > x1) && (gapLowerRight.x < x2)) {
        aContext.DrawLine(gapUpperRight.x, aY2, x2, aY2);
      } 
    } else {
      aContext.DrawLine(aX1, aY1, aX2, aY2);
    }
  }
}



void nsCSSRendering::FillPolygon (nsIRenderingContext& aContext, 
                                  const nsPoint aPoints[],
                                  PRInt32 aNumPoints,
                                  nsRect* aGap)
{
#ifdef DEBUG
  nsPenMode penMode;
  if (NS_SUCCEEDED(aContext.GetPenMode(penMode)) &&
      penMode == nsPenMode_kInvert) {
    NS_WARNING( "Invert mode ignored in FillPolygon" );
  }
#endif

  if (nsnull == aGap) {
    aContext.FillPolygon(aPoints, aNumPoints);
  } else if (4 == aNumPoints) {
    nsPoint gapUpperRight(aGap->x + aGap->width, aGap->y);
    nsPoint gapLowerRight(aGap->x + aGap->width, aGap->y + aGap->height);

    
    nsPoint points[4];
    for (PRInt32 pX = 0; pX < 4; pX++) {
      points[pX] = aPoints[pX];
    }
    for (PRInt32 i = 0; i < 3; i++) {
      for (PRInt32 j = i+1; j < 4; j++) { 
        if (points[j].x < points[i].x) {
          nsPoint swap = points[i];
          points[i] = points[j];
          points[j] = swap;
        }
      }
    }

    nsPoint upperLeft  = (points[0].y <= points[1].y) ? points[0] : points[1];
    nsPoint lowerLeft  = (points[0].y <= points[1].y) ? points[1] : points[0];
    nsPoint upperRight = (points[2].y <= points[3].y) ? points[2] : points[3];
    nsPoint lowerRight = (points[2].y <= points[3].y) ? points[3] : points[2];


    if ((aGap->y <= upperLeft.y) && (gapLowerRight.y >= lowerRight.y)) {
      if ((aGap->x > upperLeft.x) && (aGap->x < upperRight.x)) {
        nsPoint leftRect[4];
        leftRect[0] = upperLeft;
        leftRect[1] = nsPoint(aGap->x, upperLeft.y);
        leftRect[2] = nsPoint(aGap->x, lowerLeft.y);
        leftRect[3] = lowerLeft;
        aContext.FillPolygon(leftRect, 4);
      } 
      if ((gapUpperRight.x > upperLeft.x) && (gapUpperRight.x < upperRight.x)) {
        nsPoint rightRect[4];
        rightRect[0] = nsPoint(gapUpperRight.x, upperRight.y);
        rightRect[1] = upperRight;
        rightRect[2] = lowerRight;
        rightRect[3] = nsPoint(gapLowerRight.x, lowerRight.y);
        aContext.FillPolygon(rightRect, 4);
      } 
    } else {
      aContext.FillPolygon(aPoints, aNumPoints);
    }      
  }
}




nscolor nsCSSRendering::MakeBevelColor(PRIntn whichSide, PRUint8 style,
                                       nscolor aBackgroundColor,
                                       nscolor aBorderColor)
{

  nscolor colors[2];
  nscolor theColor;

  
  
  NS_GetSpecial3DColors(colors, aBackgroundColor, aBorderColor);
 
  if ((style == NS_STYLE_BORDER_STYLE_OUTSET) ||
      (style == NS_STYLE_BORDER_STYLE_RIDGE)) {
    
    switch (whichSide) {
    case NS_SIDE_BOTTOM: whichSide = NS_SIDE_TOP;    break;
    case NS_SIDE_RIGHT:  whichSide = NS_SIDE_LEFT;   break;
    case NS_SIDE_TOP:    whichSide = NS_SIDE_BOTTOM; break;
    case NS_SIDE_LEFT:   whichSide = NS_SIDE_RIGHT;  break;
    }
  }

  switch (whichSide) {
  case NS_SIDE_BOTTOM:
    theColor = colors[1];
    break;
  case NS_SIDE_RIGHT:
    theColor = colors[1];
    break;
  case NS_SIDE_TOP:
    theColor = colors[0];
    break;
  case NS_SIDE_LEFT:
  default:
    theColor = colors[0];
    break;
  }
  return theColor;
}


#define MAX_POLY_POINTS 4

#define ACTUAL_THICKNESS(outside, inside, frac, tpp) \
  (NSToCoordRound(((outside) - (inside)) * (frac) / (tpp)) * (tpp))






void nsCSSRendering::DrawDashedSides(PRIntn startSide,
                                     nsIRenderingContext& aContext,
                     const nsRect& aDirtyRect,
                                     const PRUint8 borderStyles[],  
                                     const nscolor borderColors[],  
                                     const nsRect& borderOutside,
                                     const nsRect& borderInside,
                                     PRIntn aSkipSides,
                     nsRect* aGap)
{
PRIntn  dashLength;
nsRect  dashRect, firstRect, currRect;
PRBool  bSolid = PR_TRUE;
float   over = 0.0f;
PRUint8 style = borderStyles[startSide];  
PRBool  skippedSide = PR_FALSE;

  for (PRIntn whichSide = startSide; whichSide < 4; whichSide++) {
    PRUint8 prevStyle = style;
    style = borderStyles[whichSide];  
    if ((1<<whichSide) & aSkipSides) {
      
      skippedSide = PR_TRUE;
      continue;
    }
    if ((style == NS_STYLE_BORDER_STYLE_DASHED) ||
        (style == NS_STYLE_BORDER_STYLE_DOTTED))
    {
      if ((style != prevStyle) || skippedSide) {
        
        over = 0.0f;
        bSolid = PR_TRUE;
      }

      
      if (style == NS_STYLE_BORDER_STYLE_DASHED) {
        dashLength = DASH_LENGTH;
      } else {
        dashLength = DOT_LENGTH;
      }

      aContext.SetColor(borderColors[whichSide]);  
      switch (whichSide) {
      case NS_SIDE_LEFT:
        
        
        dashRect.width = borderInside.x - borderOutside.x;
        dashRect.height = nscoord(dashRect.width * dashLength);
        dashRect.x = borderOutside.x;
        dashRect.y = borderInside.YMost() - dashRect.height;

        if (over > 0.0f) {
          firstRect.x = dashRect.x;
          firstRect.width = dashRect.width;
          firstRect.height = nscoord(dashRect.height * over);
          firstRect.y = dashRect.y + (dashRect.height - firstRect.height);
          over = 0.0f;
          currRect = firstRect;
        } else {
          currRect = dashRect;
        }

        while (currRect.YMost() > borderInside.y) {
          
          if (currRect.y < borderInside.y) {
            over = float(borderInside.y - dashRect.y) /
              float(dashRect.height);
            currRect.height = currRect.height - (borderInside.y - currRect.y);
            currRect.y = borderInside.y;
          }

          
          if (bSolid) {
            aContext.FillRect(currRect);
          }

          
          if (over == 0.0f) {
            bSolid = PRBool(!bSolid);
          }
          dashRect.y = dashRect.y - currRect.height;
          currRect = dashRect;
        }
        break;

      case NS_SIDE_TOP:
        
        if (bSolid) {
          aContext.FillRect(borderOutside.x, borderOutside.y,
                            borderInside.x - borderOutside.x,
                            borderInside.y - borderOutside.y);
        }

        dashRect.height = borderInside.y - borderOutside.y;
        dashRect.width = dashRect.height * dashLength;
        dashRect.x = borderInside.x;
        dashRect.y = borderOutside.y;

        if (over > 0.0f) {
          firstRect.x = dashRect.x;
          firstRect.y = dashRect.y;
          firstRect.width = nscoord(dashRect.width * over);
          firstRect.height = dashRect.height;
          over = 0.0f;
          currRect = firstRect;
        } else {
          currRect = dashRect;
        }

        while (currRect.x < borderInside.XMost()) {
          
          if (currRect.XMost() > borderInside.XMost()) {
            over = float(dashRect.XMost() - borderInside.XMost()) /
              float(dashRect.width);
            currRect.width = currRect.width -
              (currRect.XMost() - borderInside.XMost());
          }

          
          if (bSolid) {
            aContext.FillRect(currRect);
          }

          
          if (over == 0.0f) {
            bSolid = PRBool(!bSolid);
          }
          dashRect.x = dashRect.x + currRect.width;
          currRect = dashRect;
        }
        break;

      case NS_SIDE_RIGHT:
        
        if (bSolid) {
          aContext.FillRect(borderInside.XMost(), borderOutside.y,
                            borderOutside.XMost() - borderInside.XMost(),
                            borderInside.y - borderOutside.y);
        }

        dashRect.width = borderOutside.XMost() - borderInside.XMost();
        dashRect.height = nscoord(dashRect.width * dashLength);
        dashRect.x = borderInside.XMost();
        dashRect.y = borderInside.y;

        if (over > 0.0f) {
          firstRect.x = dashRect.x;
          firstRect.y = dashRect.y;
          firstRect.width = dashRect.width;
          firstRect.height = nscoord(dashRect.height * over);
          over = 0.0f;
          currRect = firstRect;
        } else {
          currRect = dashRect;
        }

        while (currRect.y < borderInside.YMost()) {
          
          if (currRect.YMost() > borderInside.YMost()) {
            over = float(dashRect.YMost() - borderInside.YMost()) /
              float(dashRect.height);
            currRect.height = currRect.height -
              (currRect.YMost() - borderInside.YMost());
          }

          
          if (bSolid) {
            aContext.FillRect(currRect);
          }

          
          if (over == 0.0f) {
            bSolid = PRBool(!bSolid);
          }
          dashRect.y = dashRect.y + currRect.height;
          currRect = dashRect;
        }
        break;

      case NS_SIDE_BOTTOM:
        
        if (bSolid) {
          aContext.FillRect(borderInside.XMost(), borderInside.YMost(),
                            borderOutside.XMost() - borderInside.XMost(),
                            borderOutside.YMost() - borderInside.YMost());
        }

        dashRect.height = borderOutside.YMost() - borderInside.YMost();
        dashRect.width = nscoord(dashRect.height * dashLength);
        dashRect.x = borderInside.XMost() - dashRect.width;
        dashRect.y = borderInside.YMost();

        if (over > 0.0f) {
          firstRect.y = dashRect.y;
          firstRect.width = nscoord(dashRect.width * over);
          firstRect.height = dashRect.height;
          firstRect.x = dashRect.x + (dashRect.width - firstRect.width);
          over = 0.0f;
          currRect = firstRect;
        } else {
          currRect = dashRect;
        }

        while (currRect.XMost() > borderInside.x) {
          
          if (currRect.x < borderInside.x) {
            over = float(borderInside.x - dashRect.x) / float(dashRect.width);
            currRect.width = currRect.width - (borderInside.x - currRect.x);
            currRect.x = borderInside.x;
          }

          
          if (bSolid) {
            aContext.FillRect(currRect);
          }

          
          if (over == 0.0f) {
            bSolid = PRBool(!bSolid);
          }
          dashRect.x = dashRect.x - currRect.width;
          currRect = dashRect;
        }
        break;
      }
    }
    skippedSide = PR_FALSE;
  }
}





void nsCSSRendering::DrawDashedSides(PRIntn startSide,
                                     nsIRenderingContext& aContext,
                                     const nsRect& aDirtyRect,
                                     const nsStyleColor* aColorStyle,
                                     const nsStyleBorder* aBorderStyle,  
                                     const nsStyleOutline* aOutlineStyle,  
                                     PRBool aDoOutline,
                                     const nsRect& borderOutside,
                                     const nsRect& borderInside,
                                     PRIntn aSkipSides,
                     nsRect* aGap)
{

PRIntn  dashLength;
nsRect  dashRect, currRect;
nscoord temp, temp1, adjust;
PRBool  bSolid = PR_TRUE;
float   over = 0.0f;
PRBool  skippedSide = PR_FALSE;

  NS_ASSERTION(aColorStyle &&
               ((aDoOutline && aOutlineStyle) || (!aDoOutline && aBorderStyle)),
               "null params not allowed");
  PRUint8 style = aDoOutline
                  ? aOutlineStyle->GetOutlineStyle()
                  : aBorderStyle->GetBorderStyle(startSide);  

  
  nscoord xwidth = aDirtyRect.XMost();
  nscoord ywidth = aDirtyRect.YMost();

  for (PRIntn whichSide = startSide; whichSide < 4; whichSide++) {
    PRUint8 prevStyle = style;
    style = aDoOutline
              ? aOutlineStyle->GetOutlineStyle()
              : aBorderStyle->GetBorderStyle(whichSide);  
    if ((1<<whichSide) & aSkipSides) {
      
      skippedSide = PR_TRUE;
      continue;
    }
    if ((style == NS_STYLE_BORDER_STYLE_DASHED) ||
        (style == NS_STYLE_BORDER_STYLE_DOTTED))
    {
      if ((style != prevStyle) || skippedSide) {
        
        over = 0.0f;
        bSolid = PR_TRUE;
      }

      if (style == NS_STYLE_BORDER_STYLE_DASHED) {
        dashLength = DASH_LENGTH;
      } else {
        dashLength = DOT_LENGTH;
      }

      
      
      nscolor sideColor(aColorStyle->mColor);

      PRBool  isInvert = PR_FALSE;
      if (aDoOutline) {
        if (!aOutlineStyle->GetOutlineInitialColor()) {
          aOutlineStyle->GetOutlineColor(sideColor);
        }
#ifdef GFX_HAS_INVERT
        else {
          isInvert = PR_TRUE;
        }
#endif
      } else {
        PRBool transparent; 
        PRBool foreground;
        aBorderStyle->GetBorderColor(whichSide, sideColor, transparent, foreground);
        if (foreground)
          sideColor = aColorStyle->mColor;
        if (transparent)
          continue; 
      }
      aContext.SetColor(sideColor);  

      switch (whichSide) {
      case NS_SIDE_RIGHT:
      case NS_SIDE_LEFT:
        bSolid = PR_FALSE;
        
        
        if(whichSide==NS_SIDE_LEFT){ 
          dashRect.width = borderInside.x - borderOutside.x;
        } else {
          dashRect.width = borderOutside.XMost() - borderInside.XMost();
        }
        if( dashRect.width >0 ) {
          dashRect.height = dashRect.width * dashLength;
          dashRect.y = borderOutside.y;

          if(whichSide == NS_SIDE_RIGHT){
            dashRect.x = borderInside.XMost();
          } else {
            dashRect.x = borderOutside.x;
          }

          temp = borderOutside.height;
          temp1 = temp/dashRect.height;

          currRect = dashRect;

          if((temp1%2)==0){
            adjust = (dashRect.height-(temp%dashRect.height))/2; 
            
            FillOrInvertRect(aContext,  dashRect.x, borderOutside.y,dashRect.width, dashRect.height-adjust,isInvert);
            FillOrInvertRect(aContext,dashRect.x,(borderOutside.YMost()-(dashRect.height-adjust)),dashRect.width, dashRect.height-adjust,isInvert);
            currRect.y += (dashRect.height-adjust);
            temp-= (dashRect.height-adjust);
          } else {
            adjust = (temp%dashRect.width)/2;                   
            
            FillOrInvertRect(aContext, dashRect.x, borderOutside.y,dashRect.width, dashRect.height+adjust,isInvert);
            FillOrInvertRect(aContext, dashRect.x,(borderOutside.YMost()-(dashRect.height+adjust)),dashRect.width, dashRect.height+adjust,isInvert);
            currRect.y += (dashRect.height+adjust);
            temp-= (dashRect.height+adjust);
          }
        
          temp += borderOutside.y;
          if( temp > ywidth)
            temp = ywidth;

          
          if( currRect.y < aDirtyRect.y){
            temp1 = NSToCoordFloor((float)((aDirtyRect.y-currRect.y)/dashRect.height));
            currRect.y += temp1*dashRect.height;
            if((temp1%2)==1){
              bSolid = PR_TRUE;
            }
          }

          while(currRect.y<temp) {
            
            if (bSolid) {
              FillOrInvertRect(aContext, currRect,isInvert);
            }

            bSolid = PRBool(!bSolid);
            currRect.y += dashRect.height;
          }
        }
        break;

      case NS_SIDE_BOTTOM:
      case NS_SIDE_TOP:
        bSolid = PR_FALSE;
        
        

        if(whichSide==NS_SIDE_TOP){ 
          dashRect.height = borderInside.y - borderOutside.y;
        } else {
          dashRect.height = borderOutside.YMost() - borderInside.YMost();
        }
        if( dashRect.height >0 ) {
          dashRect.width = dashRect.height * dashLength;
          dashRect.x = borderOutside.x;

          if(whichSide == NS_SIDE_BOTTOM){
            dashRect.y = borderInside.YMost();
          } else {
            dashRect.y = borderOutside.y;
          }

          temp = borderOutside.width;
          temp1 = temp/dashRect.width;

          currRect = dashRect;

          if((temp1%2)==0){
            adjust = (dashRect.width-(temp%dashRect.width))/2;     
            
            FillOrInvertRect(aContext, borderOutside.x,dashRect.y,dashRect.width-adjust,dashRect.height,isInvert);
            FillOrInvertRect(aContext, (borderOutside.XMost()-(dashRect.width-adjust)),dashRect.y,dashRect.width-adjust,dashRect.height,isInvert);
            currRect.x += (dashRect.width-adjust);
            temp-= (dashRect.width-adjust);
          } else {
            adjust = (temp%dashRect.width)/2;
            
            FillOrInvertRect(aContext, borderOutside.x,dashRect.y,dashRect.width+adjust,dashRect.height,isInvert);
            FillOrInvertRect(aContext, (borderOutside.XMost()-(dashRect.width+adjust)),dashRect.y,dashRect.width+adjust,dashRect.height,isInvert);
            currRect.x += (dashRect.width+adjust);
            temp-= (dashRect.width+adjust);
          }
       
          temp += borderOutside.x;
          if( temp > xwidth)
            temp = xwidth;

          
          if( currRect.x < aDirtyRect.x){
            temp1 = NSToCoordFloor((float)((aDirtyRect.x-currRect.x)/dashRect.width));
            currRect.x += temp1*dashRect.width;
            if((temp1%2)==1){
              bSolid = PR_TRUE;
            }
          }

          while(currRect.x<temp) {
            
            if (bSolid) {
              FillOrInvertRect(aContext, currRect,isInvert);
            }

            bSolid = PRBool(!bSolid);
            currRect.x += dashRect.width;
          }
        }
      break;
      }
    }
    skippedSide = PR_FALSE;
  }
}

nscolor
nsCSSRendering::TransformColor(nscolor  aMapColor,PRBool aNoBackGround)
{
PRUint16  hue,sat,value;
nscolor   newcolor;

  newcolor = aMapColor;
  if (PR_TRUE == aNoBackGround){
    
    NS_RGB2HSV(newcolor,hue,sat,value);
    
    
    
    
    
    
    if (value > sat) {
      value = sat;
      
      NS_HSV2RGB(newcolor,hue,sat,value);
    }
  }
  return newcolor;
}

static
PRBool GetBorderColor(const nsStyleColor* aColor, const nsStyleBorder& aBorder, PRUint8 aSide, nscolor& aColorVal,
                      nsBorderColors** aCompositeColors = nsnull)
{
  PRBool transparent;
  PRBool foreground;

  if (aCompositeColors) {
    aBorder.GetCompositeColors(aSide, aCompositeColors);
    if (*aCompositeColors)
      return PR_TRUE;
  }

  aBorder.GetBorderColor(aSide, aColorVal, transparent, foreground);
  if (foreground)
    aColorVal = aColor->mColor;

  return !transparent;
}





#undef DEBUG_NEW_BORDERS

#ifdef DEBUG_NEW_BORDERS
#include <stdarg.h>

static inline void S(const gfxPoint& p) {
  fprintf (stderr, "[%f,%f]", p.x, p.y);
}

static inline void S(const gfxSize& s) {
  fprintf (stderr, "[%f %f]", s.width, s.height);
}

static inline void S(const gfxRect& r) {
  fprintf (stderr, "[%f %f %f %f]", r.pos.x, r.pos.y, r.size.width, r.size.height);
}

static inline void S(const gfxFloat f) {
  fprintf (stderr, "%f", f);
}

static inline void S(const char *s) {
  fprintf (stderr, "%s", s);
}

static inline void SN(const char *s = nsnull) {
  if (s)
    fprintf (stderr, "%s", s);
  fprintf (stderr, "\n");
  fflush (stderr);
}

static inline void SF(const char *fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  vfprintf (stderr, fmt, vl);
  va_end(vl);
}

static inline void SX(gfxContext *ctx) {
  gfxPoint p = ctx->CurrentPoint();
  fprintf (stderr, "p: %f %f\n", p.x, p.y);
  return;
  ctx->MoveTo(p + gfxPoint(-2, -2)); ctx->LineTo(p + gfxPoint(2, 2));
  ctx->MoveTo(p + gfxPoint(-2, 2)); ctx->LineTo(p + gfxPoint(2, -2));
  ctx->MoveTo(p);
}


#else
static inline void S(const gfxPoint& p) {}
static inline void S(const gfxSize& s) {}
static inline void S(const gfxRect& r) {}
static inline void S(const gfxFloat f) {}
static inline void S(const char *s) {}
static inline void SN(const char *s = nsnull) {}
static inline void SF(const char *fmt, ...) {}
static inline void SX(gfxContext *ctx) {}
#endif


static const PRUint8 gBorderSideOrder[] = { NS_SIDE_TOP, NS_SIDE_RIGHT, NS_SIDE_BOTTOM, NS_SIDE_LEFT };












static PRBool
ShouldDoSeparateSides (const nsStyleBorder& aBorderStyle,
                       const nsStyleColor *aOurColor)
{
  PRUint8 firstSideStyle;
  nscolor firstSideColor;
  nscolor sideColor;
  nsBorderColors* compositeColors = nsnull;

  for (int i = 0; i < 4; i++) {
    PRUint8 side = gBorderSideOrder[i];
    PRUint8 borderRenderStyle = aBorderStyle.GetBorderStyle(side);

    
    
    if (borderRenderStyle != NS_STYLE_BORDER_STYLE_SOLID &&
        borderRenderStyle != NS_STYLE_BORDER_STYLE_DOUBLE)
      return PR_TRUE;

    if (i == 0)
      firstSideStyle = borderRenderStyle;
    else if (borderRenderStyle != firstSideStyle)
      return PR_TRUE;

    if (GetBorderColor(aOurColor, aBorderStyle, side, sideColor, &compositeColors)) {
      
      if (compositeColors)
        return PR_TRUE;

      
      if (i == 0)
        firstSideColor = sideColor;
      else if (sideColor != firstSideColor)
        return PR_TRUE;
    }
  }

  return PR_FALSE;
}

#define C_TL 0
#define C_TR 1
#define C_BR 2
#define C_BL 3

#ifndef NS_PI
#define NS_PI 3.14159265358979323846
#endif



static gfxFloat
SideLength(gfxRect& oRect,
           gfxRect& lRect,
           PRUint8 side,
           gfxFloat *radii)
{
  if (radii == nsnull) {
    if (side == NS_SIDE_TOP || side == NS_SIDE_BOTTOM)
      return oRect.size.width;

    return oRect.size.height;
  } else {
    
    
    if (side == NS_SIDE_TOP || side == NS_SIDE_BOTTOM)
      return oRect.size.width;

    return oRect.size.height;
  }
}





static void
DoAllSidesBorderPath(gfxContext *ctx,
                     gfxRect& lRect,
                     gfxFloat *radii,
                     PRIntn skipSides = 0)
{
  ctx->NewPath();

  SF("DoAllSidesBorderPath: [%f %f %f %f] radii: %p skipSides: %d\n", lRect.pos.x, lRect.pos.y, lRect.size.width, lRect.size.height, radii, skipSides);
  if (radii) {
    SF("   %f %f %f %f\n", radii[0], radii[1], radii[2], radii[3]);
  }

  
  if (radii == nsnull) {
    ctx->Rectangle(lRect);
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    

    gfxMatrix mat = ctx->CurrentMatrix();
    ctx->Translate(lRect.pos);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    PRIntn currentSide = NS_SIDE_TOP;
    switch (skipSides) {
      case SIDE_BIT_TOP:
      case SIDE_BIT_TOP | SIDE_BIT_LEFT:
      case SIDE_BIT_TOP | SIDE_BIT_LEFT | SIDE_BIT_BOTTOM:
        currentSide = NS_SIDE_RIGHT;
        break;
      case SIDE_BIT_RIGHT:
      case SIDE_BIT_RIGHT | SIDE_BIT_TOP:
      case SIDE_BIT_RIGHT | SIDE_BIT_TOP | SIDE_BIT_LEFT:
        currentSide = NS_SIDE_BOTTOM;
        break;
      case SIDE_BIT_BOTTOM:
      case SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT:
      case SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT | SIDE_BIT_TOP:
        currentSide = NS_SIDE_LEFT;
        break;
      case SIDE_BIT_LEFT:
      case SIDE_BIT_LEFT | SIDE_BIT_BOTTOM:
      case SIDE_BIT_LEFT | SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT:
        currentSide = NS_SIDE_TOP;
        break;
   }

    switch (currentSide) {
      case NS_SIDE_TOP:
        ctx->MoveTo(gfxPoint(radii[C_TL] / 2.0, 0.0));
        break;

      case NS_SIDE_RIGHT:
        ctx->MoveTo(gfxPoint(lRect.size.width, radii[C_TR] / 2.0));
        break;

      case NS_SIDE_BOTTOM:
        ctx->MoveTo(gfxPoint(lRect.size.width - radii[C_BR] / 2.0, lRect.size.height));
        break;

      case NS_SIDE_LEFT:
        ctx->MoveTo(gfxPoint(0.0, lRect.size.height - radii[C_BL] / 2.0));
        break;
    }

    SX(ctx);

    int sidesToDraw = 4;
    while (sidesToDraw-- > 0) {
      switch (currentSide) {
        case NS_SIDE_TOP:
          if (skipSides & SIDE_BIT_TOP) {
            ctx->MoveTo(gfxPoint(lRect.size.width, 0.0));
          } else if (radii[C_TR]) {
            ctx->LineTo(gfxPoint(lRect.size.width - radii[C_TR] / 2.0, 0.0));
            SX(ctx);
            
            ctx->Arc(gfxPoint(lRect.size.width - radii[C_TR] / 2.0, radii[C_TR] / 2.0),
                     radii[C_TR] / 2.0,
                     3.0 * NS_PI / 2.0, 0.0);
            SX(ctx);
          } else {
            ctx->LineTo(gfxPoint(lRect.size.width, 0.0));
            SX(ctx);
          }
          break;

        case NS_SIDE_RIGHT:
          if (skipSides & SIDE_BIT_RIGHT) {
            ctx->MoveTo(gfxPoint(lRect.size.width, lRect.size.height));
          } else if (radii[C_BR]) {
            ctx->LineTo(gfxPoint(lRect.size.width, lRect.size.height - radii[C_BR] / 2.0));
            SX(ctx);

            ctx->Arc(gfxPoint(lRect.size.width - radii[C_BR] / 2.0, lRect.size.height - radii[C_BR] / 2.0),
                     radii[C_BR] / 2.0,
                     0.0, NS_PI / 2.0);
            SX(ctx);
          } else {
            ctx->LineTo(gfxPoint(lRect.size.width, lRect.size.height));
            SX(ctx);
          }
          break;

        case NS_SIDE_BOTTOM:
          if (skipSides & SIDE_BIT_BOTTOM) {
            ctx->MoveTo(gfxPoint(0.0, lRect.size.height));
          } else if (radii[C_BL]) {
            ctx->LineTo(gfxPoint(radii[C_BL] / 2.0, lRect.size.height));
            SX(ctx);

            ctx->Arc(gfxPoint(radii[C_BL] / 2.0, lRect.size.height - radii[C_BL] / 2.0),
                     radii[C_BL] / 2.0,
                     NS_PI / 2.0,
                     NS_PI);
            SX(ctx);
          } else {
            ctx->LineTo(gfxPoint(0.0, lRect.size.height));
            SX(ctx);
          }
          break;

        case NS_SIDE_LEFT:
          if (skipSides & SIDE_BIT_LEFT) {
            ctx->MoveTo(gfxPoint(0.0, lRect.size.height));
          } else if (radii[C_TL]) {
            ctx->LineTo(gfxPoint(0.0, radii[C_TL] / 2.0));
            SX(ctx);

            ctx->Arc(gfxPoint(radii[C_TL] / 2.0, radii[C_TL] / 2.0),
                     radii[C_TL] / 2.0,
                     NS_PI,
                     3.0 * NS_PI / 2.0);
            SX(ctx);
          } else {
            ctx->LineTo(gfxPoint(0.0, 0.0));
            SX(ctx);
          }
          break;
      }

      currentSide = (currentSide + 1) % 4;
    }

    ctx->SetMatrix(mat);
  }
}


static void
DoSingleSideBorderPath(gfxContext *ctx,
                       gfxRect& oRect,
                       gfxRect& lRect,
                       gfxFloat *radii,
                       PRInt8 whichSide)
{
  
  
  
  

  
  

  
  

  ctx->NewPath();

  if (whichSide == NS_SIDE_TOP) {
    ctx->MoveTo(gfxPoint(oRect.pos.x, lRect.pos.y));
    ctx->LineTo(gfxPoint(oRect.pos.x + oRect.size.width, lRect.pos.y));
  } else if (whichSide == NS_SIDE_RIGHT) {
    ctx->MoveTo(gfxPoint(lRect.pos.x + lRect.size.width, oRect.pos.y));
    ctx->LineTo(gfxPoint(lRect.pos.x + lRect.size.width, oRect.pos.y + oRect.size.height));
  } else if (whichSide == NS_SIDE_BOTTOM) {
    ctx->MoveTo(gfxPoint(oRect.pos.x + oRect.size.width,  lRect.pos.y + lRect.size.height));
    ctx->LineTo(gfxPoint(oRect.pos.x, lRect.pos.y + lRect.size.height));
  } else if (whichSide == NS_SIDE_LEFT) {
    ctx->MoveTo(gfxPoint(lRect.pos.x, oRect.pos.y + oRect.size.height));
    ctx->LineTo(gfxPoint(lRect.pos.x, oRect.pos.y));
  }
}















typedef enum {
  
  
  SIDE_CLIP_TRAPEZOID,

  
  
  
  
  
  
  
  
  SIDE_CLIP_TRAPEZOID_FULL,

  
  
  
  SIDE_CLIP_RECTANGLE
} SideClipType;

static void
DoSideClipPath(gfxContext *ctx,
               gfxRect& iRect,
               gfxRect& oRect,
               gfxRect& lRect,
               PRUint8 whichSide,
               const nsStyleBorder& borderStyle,
               const PRInt32 *borderRadii)
{
  
  
  
  
  
  
  
  
  

  gfxPoint start[2];
  gfxPoint end[2];

  PRUint8 style = borderStyle.GetBorderStyle(whichSide);
  PRUint8 startAdjacentStyle = borderStyle.GetBorderStyle(((whichSide - 1) + 4) % 4);
  PRUint8 endAdjacentStyle = borderStyle.GetBorderStyle((whichSide + 1) % 4);

  PRBool isDashed =
    (style == NS_STYLE_BORDER_STYLE_DASHED || style == NS_STYLE_BORDER_STYLE_DOTTED);
  PRBool startIsDashed =
    (startAdjacentStyle == NS_STYLE_BORDER_STYLE_DASHED || startAdjacentStyle == NS_STYLE_BORDER_STYLE_DOTTED);
  PRBool endIsDashed =
    (endAdjacentStyle == NS_STYLE_BORDER_STYLE_DASHED || endAdjacentStyle == NS_STYLE_BORDER_STYLE_DOTTED);

  PRBool startHasRadius = PR_FALSE;
  PRBool endHasRadius = PR_FALSE;

  SideClipType startType = SIDE_CLIP_TRAPEZOID;
  SideClipType endType = SIDE_CLIP_TRAPEZOID;

  if (borderRadii) {
    startHasRadius = borderRadii[whichSide] != 0;
    endHasRadius = borderRadii[(whichSide+1) % 4] != 0;
  }

  if (startHasRadius) {
    startType = SIDE_CLIP_TRAPEZOID_FULL;
  } else if (startIsDashed && isDashed) {
    startType = SIDE_CLIP_RECTANGLE;
  }

  if (endHasRadius) {
    endType = SIDE_CLIP_TRAPEZOID_FULL;
  } else if (endIsDashed && isDashed) {
    endType = SIDE_CLIP_RECTANGLE;
  }

  if (startType == SIDE_CLIP_TRAPEZOID ||
      startType == SIDE_CLIP_TRAPEZOID_FULL)
  {
    switch (whichSide) {
      case NS_SIDE_TOP:
        start[0] = oRect.TopLeft();
        start[1] = iRect.TopLeft();
        break;

      case NS_SIDE_RIGHT:
        start[0] = oRect.TopRight();
        start[1] = iRect.TopRight();
        break;

      case NS_SIDE_BOTTOM:
        start[0] = oRect.BottomRight();
        start[1] = iRect.BottomRight();
        break;

      case NS_SIDE_LEFT:
        start[0] = oRect.BottomLeft();
        start[1] = iRect.BottomLeft();
        break;
    }

    if (startType == SIDE_CLIP_TRAPEZOID_FULL) {
      gfxFloat mx = iRect.pos.x + iRect.size.width / 2.0;
      gfxFloat my = iRect.pos.y + iRect.size.height / 2.0;

      gfxPoint ps, pc;

      ps = start[1] - start[0];
      if (ps.x == 0.0 && ps.y == 0.0) {
        
      } else if (ps.x == 0.0) {
        start[1] = start[0] + gfxSize(ps.y, ps.y);
      } else if (ps.y == 0.0) {
        start[1] = start[0] + gfxSize(ps.x, ps.x);
      } else {
        gfxFloat k = PR_MIN((mx - start[0].x) / ps.x,
                            (my - start[0].y) / ps.y);
        start[1] = start[0] + ps * k;
      }
    }
  } else if (startType == SIDE_CLIP_RECTANGLE) {
    switch (whichSide) {
      case NS_SIDE_TOP:
        start[0] = oRect.TopLeft();
        start[1] = gfxPoint(start[0].x, iRect.TopLeft().y);
        break;

      case NS_SIDE_RIGHT:
        start[0] = oRect.TopRight();
        start[1] = gfxPoint(iRect.TopRight().x, start[0].y);
        break;

      case NS_SIDE_BOTTOM:
        start[0] = oRect.BottomRight();
        start[1] = gfxPoint(start[0].x, iRect.BottomRight().y);
        break;

      case NS_SIDE_LEFT:
        start[0] = oRect.BottomLeft();
        start[1] = gfxPoint(iRect.BottomLeft().x, start[0].y);
        break;
    }
  }

  if (endType == SIDE_CLIP_TRAPEZOID ||
      endType == SIDE_CLIP_TRAPEZOID_FULL)
  {
    switch (whichSide) {
      case NS_SIDE_TOP:
        end[0] = oRect.TopRight();
        end[1] = iRect.TopRight();
        break;

      case NS_SIDE_RIGHT:
        end[0] = oRect.BottomRight();
        end[1] = iRect.BottomRight();
        break;

      case NS_SIDE_BOTTOM:
        end[0] = oRect.BottomLeft();
        end[1] = iRect.BottomLeft();
        break;

      case NS_SIDE_LEFT:
        end[0] = oRect.TopLeft();
        end[1] = iRect.TopLeft();
        break;
    }

    if (endType == SIDE_CLIP_TRAPEZOID_FULL) {
      gfxFloat mx = iRect.pos.x + iRect.size.width / 2.0;
      gfxFloat my = iRect.pos.y + iRect.size.height / 2.0;

      gfxPoint ps, pc;

      ps = end[1] - end[0];
      if (ps.x == 0.0 && ps.y == 0.0) {
        
      } else if (ps.x == 0.0) {
        end[1] = end[0] + gfxSize(ps.y, ps.y);
      } else if (ps.y == 0.0) {
        end[1] = end[0] + gfxSize(ps.x, ps.x);
      } else {
        gfxFloat k = PR_MIN((mx - end[0].x) / ps.x,
                            (my - end[0].y) / ps.y);
        end[1] = end[0] + ps * k;
      }
    }
  } else if (endType == SIDE_CLIP_RECTANGLE) {
    switch (whichSide) {
      case NS_SIDE_TOP:
        end[0] = gfxPoint(iRect.TopRight().x, oRect.TopRight().y);
        end[1] = iRect.TopRight();
        break;

      case NS_SIDE_RIGHT:
        end[0] = gfxPoint(oRect.BottomRight().x, iRect.BottomRight().y);
        end[1] = iRect.BottomRight();
        break;

      case NS_SIDE_BOTTOM:
        end[0] = gfxPoint(iRect.BottomLeft().x, oRect.BottomLeft().y);
        end[1] = iRect.BottomLeft();
        break;

      case NS_SIDE_LEFT:
        end[0] = gfxPoint(oRect.TopLeft().x, iRect.TopLeft().y);
        end[1] = iRect.TopLeft();
        break;
    }
  }

  ctx->NewPath();
  ctx->MoveTo(start[0]);
  ctx->LineTo(end[0]);
  ctx->LineTo(end[1]);
  ctx->LineTo(start[1]);
  ctx->ClosePath();
}

typedef enum {
  BorderColorStyleNone,
  BorderColorStyleSolid,
  BorderColorStyleLight,
  BorderColorStyleDark
} BorderColorStyle;

static void
MakeBorderColor(gfxRGBA& color, const gfxRGBA& backgroundColor, BorderColorStyle bpat)
{
  nscolor colors[2];

  switch (bpat) {
    case BorderColorStyleNone:
      color.r = 0.0;
      color.g = 0.0;
      color.b = 0.0;
      color.a = 0.0;
      break;

    case BorderColorStyleSolid:
      break;

    case BorderColorStyleLight:
      NS_GetSpecial3DColors(colors, backgroundColor.Packed(), color.Packed());
      color.r = NS_GET_R(colors[1]) / 255.0;
      color.g = NS_GET_G(colors[1]) / 255.0;
      color.b = NS_GET_B(colors[1]) / 255.0;
      color.a = 1.0;
      break;

    case BorderColorStyleDark:
      NS_GetSpecial3DColors(colors, backgroundColor.Packed(), color.Packed());
      color.r = NS_GET_R(colors[0]) / 255.0;
      color.g = NS_GET_G(colors[0]) / 255.0;
      color.b = NS_GET_B(colors[0]) / 255.0;
      color.a = 1.0;
      break;
  }
}

static void
ComputeColorForLine(PRUint32 lineIndex,
                    PRUint32 borderWidth,
                    BorderColorStyle* borderColorStyle,
                    PRUint32 borderColorStyleCount,
                    nsBorderColors* borderColors,
                    PRUint32 borderColorCount,
                    nscolor borderColor,
                    nscolor backgroundColor,
                    gfxRGBA& outColor)
{
  NS_ASSERTION(lineIndex < borderColorStyleCount, "Invalid lineIndex given");

  if (borderColors) {
    if (lineIndex >= borderColorCount) {
      
      

      
      lineIndex = borderColorCount - 1;
    }

    while (lineIndex--)
      borderColors = borderColors->mNext;

    if (borderColors->mTransparent)
      outColor.r = outColor.g = outColor.b = outColor.a = 0.0;
    else
      outColor = gfxRGBA(borderColors->mColor);

    return;
  }

  outColor = gfxRGBA(borderColor);

  MakeBorderColor(outColor, gfxRGBA(backgroundColor), borderColorStyle[lineIndex]);
}
















static void
DrawBorderSides(gfxContext *ctx,
                PRUint32 borderWidth,
                PRUint8 borderRenderStyle,
                nscolor borderRenderColor,
                nsBorderColors *compositeColors,
                gfxRect& iRect,
                gfxRect& oRect,
                gfxRect& lRect,
                nscolor fgColor,
                nscolor bgColor,
                PRBool doSeparateSides,
                PRUint8 side,
                PRIntn skipSides,
                nscoord twipsPerPixel,
                PRInt32 *borderRadii)
{
  PRBool dashedRendering = PR_FALSE;
  gfxFloat dash[2];
  gfxFloat radii[4];
  gfxFloat *radiiPtr = nsnull;
  gfxFloat dashWidth;

  PRUint32 borderColorStyleCount = 0;
  BorderColorStyle borderColorStyleTopLeft[3], borderColorStyleBottomRight[3];
  BorderColorStyle *borderColorStyle = nsnull;
  PRUint32 compositeColorCount = 0;

  PRBool useSpecialDotDashSeparateSides = doSeparateSides;

  if (borderRadii) {
    for (int i = 0; i < 4; i++) {
      radii[i] = gfxFloat(borderRadii[i]) / twipsPerPixel;
    }

    radiiPtr = &radii[0];
  }

  
  
  if (borderRenderStyle & NS_STYLE_BORDER_STYLE_RULES_MARKER)
    return;

  
  
  if (radiiPtr)
    useSpecialDotDashSeparateSides = PR_FALSE;

  
  
  

  if (!compositeColors) {
    
    
    
    if (borderWidth == 1) {
      if (borderRenderStyle == NS_STYLE_BORDER_STYLE_RIDGE ||
          borderRenderStyle == NS_STYLE_BORDER_STYLE_GROOVE ||
          borderRenderStyle == NS_STYLE_BORDER_STYLE_DOUBLE)
        borderRenderStyle = NS_STYLE_BORDER_STYLE_SOLID;
    }

    switch (borderRenderStyle) {
      case NS_STYLE_BORDER_STYLE_SOLID:
      case NS_STYLE_BORDER_STYLE_DASHED:
      case NS_STYLE_BORDER_STYLE_DOTTED:
        borderColorStyleTopLeft[0] = BorderColorStyleSolid;
        borderColorStyleBottomRight[0] = BorderColorStyleSolid;
        borderColorStyleCount = 1;

        
        
        
        
        if (borderRenderStyle == NS_STYLE_BORDER_STYLE_DASHED) {
          dashWidth = gfxFloat(borderWidth * DOT_LENGTH * DASH_LENGTH);
          dashedRendering = PR_TRUE;

          dash[0] = dashWidth;
          dash[1] = dashWidth;
        } else if (borderRenderStyle == NS_STYLE_BORDER_STYLE_DOTTED) {
          dashWidth = gfxFloat(borderWidth * DOT_LENGTH);
          dashedRendering = PR_TRUE;

          
          if (borderWidth > 2) {
            dash[0] = 0;
            dash[1] = dashWidth * 2;

            ctx->SetLineCap(gfxContext::LINE_CAP_ROUND);
          } else {
            dash[0] = dashWidth;
            dash[1] = dashWidth;
          }
        }

        if (dashedRendering) {
          gfxFloat dashOffset = 0.0;
          gfxFloat sideLen = SideLength(oRect, lRect, side, radiiPtr);
          gfxFloat sideOffset = 0.0;

          
          
          
          
          
#if 0
          if (sideLen > dashWidth) {
            gfxFloat rep = sideLen / (dashWidth * 2.0);
            gfxFloat rem = sideLen - dashWidth * 2.0 * floor(rep);
            gfxFloat offsetrem = sideOffset - floor(sideOffset / (dashWidth * 2.0)) * (dashWidth * 20);

            if (rem < dashWidth) {
              
              dashOffset = (dashWidth - (dashWidth + rem)) / 2.0;
            } else {
              
              dashOffset = (dashWidth + (dashWidth - rem)) / 2.0;
            }

            
            dashOffset = floor(dashOffset * borderWidth) / borderWidth;
          }
#else
          if (sideLen > dashWidth) {
            gfxFloat rep = sideLen / (dashWidth * 2.0);
            gfxFloat rem = sideLen - dashWidth * 2.0 * floor(rep);

            if (rem < dashWidth) {
              
              dashOffset = dashWidth - (rem / 2.0);
            } else {
              
              dashOffset = rem / 2.0;
            }

            dashOffset = floor(dashOffset);
          }
#endif

          SF("sideLen: %f dashWidth: %f dashOffset: %f final: %f\n", sideLen, dashWidth, dashOffset, (borderWidth & 1) ? dashOffset-0.5 : dashOffset);
          ctx->SetDash(dash, 2, dashOffset );
        }

        break;

      case NS_STYLE_BORDER_STYLE_GROOVE:
        borderColorStyleTopLeft[0] = BorderColorStyleDark;
        borderColorStyleTopLeft[1] = BorderColorStyleLight;

        borderColorStyleBottomRight[0] = BorderColorStyleLight;
        borderColorStyleBottomRight[1] = BorderColorStyleDark;

        borderColorStyleCount = 2;
        break;

      case NS_STYLE_BORDER_STYLE_RIDGE:
        borderColorStyleTopLeft[0] = BorderColorStyleLight;
        borderColorStyleTopLeft[1] = BorderColorStyleDark;

        borderColorStyleBottomRight[0] = BorderColorStyleDark;
        borderColorStyleBottomRight[1] = BorderColorStyleLight;

        borderColorStyleCount = 2;
        break;

      case NS_STYLE_BORDER_STYLE_DOUBLE:
        borderColorStyleTopLeft[0] = BorderColorStyleSolid;
        borderColorStyleTopLeft[1] = BorderColorStyleNone;
        borderColorStyleTopLeft[2] = BorderColorStyleSolid;

        borderColorStyleBottomRight[0] = BorderColorStyleSolid;
        borderColorStyleBottomRight[1] = BorderColorStyleNone;
        borderColorStyleBottomRight[2] = BorderColorStyleSolid;

        borderColorStyleCount = 3;
        break;

      case NS_STYLE_BORDER_STYLE_INSET:
        borderColorStyleTopLeft[0] = BorderColorStyleDark;
        borderColorStyleBottomRight[0] = BorderColorStyleLight;

        borderColorStyleCount = 1;
        break;

      case NS_STYLE_BORDER_STYLE_OUTSET:
        borderColorStyleTopLeft[0] = BorderColorStyleLight;
        borderColorStyleBottomRight[0] = BorderColorStyleDark;

        borderColorStyleCount = 1;
        break;

      default:
        NS_NOTREACHED("Unhandled border style!!");
        break;
    }

    if (side == NS_SIDE_BOTTOM || side == NS_SIDE_RIGHT)
      borderColorStyle = borderColorStyleBottomRight;
    else
      borderColorStyle = borderColorStyleTopLeft;
  } else {
    
    
    
    borderColorStyle = new BorderColorStyle[borderWidth];
    borderColorStyleCount = borderWidth;

    nsBorderColors *tmp = compositeColors;
    do {
      compositeColorCount++;
      tmp = tmp->mNext;
    } while (tmp);

    for (unsigned int i = 0; i < borderColorStyleCount; i++) {
      borderColorStyle[i] = BorderColorStyleSolid;
    }
  }

  SF("borderWidth: %d lRect: ", borderWidth), S(lRect), SN(), SF(" borderColorStyleCount: %d special: %d\n", borderColorStyleCount, useSpecialDotDashSeparateSides);
  if (radiiPtr) {
    SF(" radii: %f %f %f %f\n", radiiPtr[0], radiiPtr[1], radiiPtr[2], radiiPtr[3]);
  }

  
  
  
  
  

  if (compositeColorCount == 0) {
    if (borderColorStyleCount == 1) {
      gfxRGBA color;
      ComputeColorForLine(0, borderWidth,
                          borderColorStyle, borderColorStyleCount,
                          compositeColors, compositeColorCount,
                          borderRenderColor, bgColor, color);

      ctx->SetLineWidth(borderWidth);
      ctx->SetColor(color);

      SF("borderColorStyle: %d color: %f %f %f %f\n", borderColorStyle[0], color.r, color.g, color.b, color.a);

      
      
      
      if (useSpecialDotDashSeparateSides)
        DoSingleSideBorderPath(ctx, oRect, lRect, radiiPtr, side);
      else
        DoAllSidesBorderPath(ctx, lRect, radiiPtr, skipSides);

      ctx->Stroke();

#if 0
  ctx->SetOperator(gfxContext::OPERATOR_OVER);
  
  ctx->SetLineWidth(1.0);
  ctx->SetDash(nsnull, 0, 0.0);
  ctx->SetColor(gfxRGBA(1.0, 0.0, 0.0, 1.0));
  ctx->Stroke();
#endif
    } else if (borderColorStyleCount == 2) {
      

      PRInt32 outerBorderWidth, innerBorderWidth;
      outerBorderWidth = (borderWidth / 2) + (borderWidth % 2);
      innerBorderWidth = (borderWidth / 2);

      gfxRGBA color;
      gfxRect sRect;

      
      if (outerBorderWidth != 0 && borderColorStyle[1] != BorderColorStyleNone) {
        ComputeColorForLine(0, borderWidth,
                            borderColorStyle, borderColorStyleCount,
                            compositeColors, compositeColorCount,
                            borderRenderColor, bgColor, color);

        sRect = lRect;
        sRect.pos.x -= innerBorderWidth / 2.0;
        sRect.pos.y -= innerBorderWidth / 2.0;
        sRect.size.width += innerBorderWidth;
        sRect.size.height += innerBorderWidth;

        ctx->SetLineWidth(outerBorderWidth);
        ctx->SetColor(color);
        DoAllSidesBorderPath(ctx, sRect, radiiPtr, skipSides);
        ctx->Stroke();
      }

      
      if (innerBorderWidth != 0 && borderColorStyle[0] != BorderColorStyleNone) {
        ComputeColorForLine(1, borderWidth,
                            borderColorStyle, borderColorStyleCount,
                            compositeColors, compositeColorCount,
                            borderRenderColor, bgColor, color);

        sRect = lRect;
        sRect.pos.x += outerBorderWidth / 2.0;
        sRect.pos.y += outerBorderWidth / 2.0;
        sRect.size.width -= outerBorderWidth;
        sRect.size.height -= outerBorderWidth;

        ctx->SetLineWidth(innerBorderWidth);
        ctx->SetColor(color);
        DoAllSidesBorderPath(ctx, sRect, radiiPtr, skipSides);

        ctx->Stroke();
      }
    } else if (borderColorStyleCount == 3) {
      
      

      PRInt32 outerBorderWidth, middleBorderWidth, innerBorderWidth;

      if (borderWidth == 1) {
        outerBorderWidth = 1;
        middleBorderWidth = innerBorderWidth = 0;
      } else {
        PRInt32 rest = borderWidth % 3;
        outerBorderWidth = innerBorderWidth = middleBorderWidth = (borderWidth - rest) / 3;
        if (rest == 1) {
          middleBorderWidth++;
        } else if (rest == 2) {
          outerBorderWidth++;
          innerBorderWidth++;
        }
      }

      gfxRGBA color;
      gfxRect sRect;

      
      if (outerBorderWidth != 0 && borderColorStyle[2] != BorderColorStyleNone) {
        ComputeColorForLine(0, borderWidth,
                            borderColorStyle, borderColorStyleCount,
                            compositeColors, compositeColorCount,
                            borderRenderColor, bgColor, color);

        sRect = lRect;
        sRect.pos.x -= (innerBorderWidth + middleBorderWidth) / 2.0;
        sRect.pos.y -= (innerBorderWidth + middleBorderWidth) / 2.0;
        sRect.size.width += (innerBorderWidth + middleBorderWidth);
        sRect.size.height += (innerBorderWidth + middleBorderWidth);

        ctx->SetLineWidth(outerBorderWidth);
        ctx->SetColor(color);
        DoAllSidesBorderPath(ctx, sRect, radiiPtr, skipSides);
        ctx->Stroke();
      }

      
      if (middleBorderWidth != 0 && borderColorStyle[1] != BorderColorStyleNone) {
        ComputeColorForLine(1, borderWidth,
                            borderColorStyle, borderColorStyleCount,
                            compositeColors, compositeColorCount,
                            borderRenderColor, bgColor, color);

        
        
        
        sRect = lRect;

        ctx->SetLineWidth(middleBorderWidth);
        ctx->SetColor(color);
        DoAllSidesBorderPath(ctx, sRect, radiiPtr, skipSides);
        ctx->Stroke();
      }

      
      if (innerBorderWidth != 0 && borderColorStyle[0] != BorderColorStyleNone) {
        ComputeColorForLine(2, borderWidth,
                            borderColorStyle, borderColorStyleCount,
                            compositeColors, compositeColorCount,
                            borderRenderColor, bgColor, color);

        sRect = lRect;
        sRect.pos.x += (outerBorderWidth + middleBorderWidth) / 2.0;
        sRect.pos.y += (outerBorderWidth + middleBorderWidth) / 2.0;
        sRect.size.width -= (outerBorderWidth + middleBorderWidth);
        sRect.size.height -= (outerBorderWidth + middleBorderWidth);

        ctx->SetLineWidth(innerBorderWidth);
        ctx->SetColor(color);
        DoAllSidesBorderPath(ctx, sRect, radiiPtr, skipSides);
        ctx->Stroke();
      }
    } else {
      
      
      
      NS_ERROR("Non-border-colors case with borderColorStyleCount < 1 or > 3; what happened?");
    }
  } else {
    
    gfxRect sRect = oRect;

    
    
    sRect.pos.x += 0.5;
    sRect.pos.y += 0.5;
    sRect.size.width -= 1.0;
    sRect.size.height -= 1.0;

    
    
    
    if (radiiPtr) {
      for (int i = 0; i < 4; i++) {
        if (radiiPtr[i] > 0.0)
          radiiPtr[i] *= 2.0;
      }
    }
      
    for (PRUint32 i = 0; i < borderColorStyleCount; i++) {
      gfxRGBA lineColor;

      ComputeColorForLine(i, borderWidth,
                          borderColorStyle, borderColorStyleCount,
                          compositeColors, compositeColorCount,
                          borderRenderColor, bgColor, lineColor);

      ctx->SetLineWidth(1.0);
      ctx->SetColor(lineColor);
      DoAllSidesBorderPath(ctx, sRect, radiiPtr, skipSides);
      ctx->Stroke();

      sRect.pos.x += 1.0;
      sRect.pos.y += 1.0;
      sRect.size.width -= 2.0;
      sRect.size.height -= 2.0;

      if (radiiPtr) {
        for (int i = 0; i < 4; i++) {
          if (radiiPtr[i] > 0.0)
            radiiPtr[i] -= 2.0;
        }
      }
    }
  }

  if (compositeColors) {
    delete [] borderColorStyle;
  }

#if 0
  ctx->SetOperator(gfxContext::OPERATOR_OVER);
  
  
  ctx->SetLineWidth(1.0);
  ctx->SetDash(nsnull, 0, 0.0);
  ctx->SetColor(gfxRGBA(1.0, 0.0, 0.0, 1.0));
  ctx->NewPath();
  ctx->Rectangle(oRect);
  ctx->Stroke();
  ctx->NewPath();
  ctx->Rectangle(iRect);
  ctx->Stroke();
#endif
}

void
nsCSSRendering::PaintBorder(nsPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            nsIFrame* aForFrame,
                            const nsRect& aDirtyRect,
                            const nsRect& aBorderArea,
                            const nsStyleBorder& aBorderStyle,
                            nsStyleContext* aStyleContext,
                            PRIntn aSkipSides,
                            nsRect* aGap,
                            nscoord aHardBorderSize,
                            PRBool  aShouldIgnoreRounded)
{
  nsMargin            border;
  nsStyleCoord        bordStyleRadius[4];
  PRInt32             borderRadii[4];
  float               percent;
  nsCompatibility     compatMode = aPresContext->CompatibilityMode();
  PRBool              haveBorderRadius = PR_FALSE;

  SN("++ PaintBorder");

  
  
  
  const nsStyleDisplay* displayData = aStyleContext->GetStyleDisplay();
  if (displayData->mAppearance) {
    nsITheme *theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame, displayData->mAppearance))
      return; 
  }

  
  const nsStyleColor* ourColor = aStyleContext->GetStyleColor();

  
  
  const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground
    (aStyleContext, compatMode == eCompatibility_NavQuirks ? PR_TRUE : PR_FALSE);

  if (aHardBorderSize > 0) {
    border.SizeTo(aHardBorderSize, aHardBorderSize, aHardBorderSize, aHardBorderSize);
  } else {
    border = aBorderStyle.GetBorder();
  }

  if ((0 == border.left) && (0 == border.right) &&
      (0 == border.top) && (0 == border.bottom)) {
    
    return;
  }

  
  aBorderStyle.mBorderRadius.GetTop(bordStyleRadius[0]);      
  aBorderStyle.mBorderRadius.GetRight(bordStyleRadius[1]);    
  aBorderStyle.mBorderRadius.GetBottom(bordStyleRadius[2]);   
  aBorderStyle.mBorderRadius.GetLeft(bordStyleRadius[3]);     

  
  for(int i = 0; i < 4; i++) {
    borderRadii[i] = 0;

    switch (bordStyleRadius[i].GetUnit()) {
      case eStyleUnit_Percent:
        percent = bordStyleRadius[i].GetPercentValue();
        borderRadii[i] = (nscoord)(percent * aForFrame->GetSize().width);
        break;

      case eStyleUnit_Coord:
        borderRadii[i] = bordStyleRadius[i].GetCoordValue();
        break;

      default:
        break;
    }

    if (borderRadii[i])
      haveBorderRadius = PR_TRUE;
  }
  SF("Border[0]: %d %d %d %d\n", borderRadii[0], borderRadii[1], borderRadii[2], borderRadii[3]);

  
  if (border.top == 0) aSkipSides |= SIDE_BIT_TOP;
  if (border.right == 0) aSkipSides |= SIDE_BIT_RIGHT;
  if (border.bottom == 0) aSkipSides |= SIDE_BIT_BOTTOM;
  if (border.left == 0) aSkipSides |= SIDE_BIT_LEFT;

  if (aSkipSides & SIDE_BIT_TOP) {
    border.top = 0;
    borderRadii[C_TL] = 0;
    borderRadii[C_TR] = 0;
  }

  if (aSkipSides & SIDE_BIT_RIGHT) {
    border.right = 0;
    borderRadii[C_TR] = 0;
    borderRadii[C_BR] = 0;
  }

  if (aSkipSides & SIDE_BIT_BOTTOM) {
    border.bottom = 0;
    borderRadii[C_BR] = 0;
    borderRadii[C_BL] = 0;
  }

  if (aSkipSides & SIDE_BIT_LEFT) {
    border.left = 0;
    borderRadii[C_BL] = 0;
    borderRadii[C_TL] = 0;
  }

  
  nsRect outerRect(aBorderArea), innerRect(aBorderArea);
  innerRect.Deflate(border);

  SF(" innerRect: %d %d %d %d\n", innerRect.x, innerRect.y, innerRect.width, innerRect.height);
  SF(" outerRect: %d %d %d %d\n", outerRect.x, outerRect.y, outerRect.width, outerRect.height);

  
  
  
  
  if (border.left + border.right > aBorderArea.width) {
    innerRect.x = outerRect.x;
    innerRect.width = outerRect.width;
  }
  if (border.top + border.bottom > aBorderArea.height) {
    innerRect.y = outerRect.y;
    innerRect.height = outerRect.height;
  }

  
  
  
  
  
  if (innerRect.Contains(aDirtyRect)) {
    return;
  }

  SF("Border[0.5]: %d %d %d %d\n", borderRadii[0], borderRadii[1], borderRadii[2], borderRadii[3]);

  
  
  
  
  
  if (haveBorderRadius) {
    borderRadii[C_TL] = PR_MIN(borderRadii[C_TL], innerRect.width + border.left);
    borderRadii[C_TL] = PR_MIN(borderRadii[C_TL], innerRect.height + border.top);
    borderRadii[C_TL] = PR_MAX(borderRadii[C_TL], 0);

    borderRadii[C_TR] = PR_MIN(borderRadii[C_TR], innerRect.width + border.right);
    borderRadii[C_TR] = PR_MIN(borderRadii[C_TR], innerRect.height + border.top);
    borderRadii[C_TR] = PR_MAX(borderRadii[C_TR], 0);

    borderRadii[C_BR] = PR_MIN(borderRadii[C_BR], innerRect.width + border.right);
    borderRadii[C_BR] = PR_MIN(borderRadii[C_BR], innerRect.height + border.bottom);
    borderRadii[C_BR] = PR_MAX(borderRadii[C_BR], 0);

    borderRadii[C_BL] = PR_MIN(borderRadii[C_BL], innerRect.width + border.left);
    borderRadii[C_BL] = PR_MIN(borderRadii[C_BL], innerRect.height + border.bottom);
    borderRadii[C_BL] = PR_MAX(borderRadii[C_BL], 0);
  }

  SF("Border[1]: %d %d %d %d\n", borderRadii[0], borderRadii[1], borderRadii[2], borderRadii[3]);

  

  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  nsRefPtr<gfxContext> ctx = (gfxContext*)
    aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);

  PRBool doSeparateSides = PR_FALSE;

  
  
  
  if (aSkipSides ||
      border.left != border.right ||
      border.left != border.top ||
      border.left != border.bottom)
  {
    doSeparateSides = PR_TRUE;
  } else {
    doSeparateSides = ShouldDoSeparateSides (aBorderStyle, ourColor);
  }

  SF("doSeparateSides: %d skipsides: t:%d l:%d r:%d b:%d\n", doSeparateSides,
     (aSkipSides & SIDE_BIT_TOP) ? 1 : 0,
     (aSkipSides & SIDE_BIT_LEFT) ? 1 : 0,
     (aSkipSides & SIDE_BIT_RIGHT) ? 1 : 0,
     (aSkipSides & SIDE_BIT_BOTTOM) ? 1 : 0);

  
  gfxRect oRect(gfxFloat(outerRect.x) / twipsPerPixel,
                gfxFloat(outerRect.y) / twipsPerPixel,
                gfxFloat(outerRect.width) / twipsPerPixel,
                gfxFloat(outerRect.height) / twipsPerPixel);

  
  gfxRect iRect(gfxFloat(innerRect.x) / twipsPerPixel,
                gfxFloat(innerRect.y) / twipsPerPixel,
                gfxFloat(innerRect.width) / twipsPerPixel,
                gfxFloat(innerRect.height) / twipsPerPixel);


  
  
  
  oRect.Round();
  iRect.Round();

  
  
  gfxRect lRect(oRect.pos.x + border.left / (2.0 * twipsPerPixel),
                oRect.pos.y + border.top / (2.0 * twipsPerPixel),
                oRect.size.width - (border.left + border.right) / (2.0 * twipsPerPixel),
                oRect.size.height - (border.top + border.bottom) / (2.0 * twipsPerPixel));

  S(" oRect: "), S(oRect), SN();
  S(" iRect: "), S(iRect), SN();
  S(" lRect: "), S(lRect), SN();

  
  
  oRect.Condition();
  if (oRect.IsEmpty())
    return;

  iRect.Condition();
  lRect.Condition();

  ctx->Save();

#if 0
  
  ctx->Save();
  ctx->Rectangle(iRect);
  ctx->Clip();
  ctx->NewPath();

  ctx->Rectangle(oRect);
  ctx->SetColor(gfxRGBA(1.0, 0.0, 0.0, 0.5));
  ctx->Fill();
  ctx->Restore();
#endif

  
  
  
  
  gfxMatrix mat = ctx->CurrentMatrix();
  if (!mat.HasNonTranslation()) {
    mat.x0 = floor(mat.x0 + 0.5);
    mat.y0 = floor(mat.y0 + 0.5);
    ctx->SetMatrix(mat);
  }

  
  
  if (doSeparateSides) {
    
    ctx->NewPath();
    ctx->Rectangle(oRect);

    if (aGap) {
      gfxRect gapRect(gfxFloat(aGap->x) / twipsPerPixel,
                      gfxFloat(aGap->y) / twipsPerPixel,
                      gfxFloat(aGap->width) / twipsPerPixel,
                      gfxFloat(aGap->height) / twipsPerPixel);
      ctx->Rectangle(gapRect);
      ctx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
      ctx->Clip();
      ctx->SetFillRule(gfxContext::FILL_RULE_WINDING);
    } else {
      ctx->Clip();
    }

    
    
    ctx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
    ctx->SetOperator(gfxContext::OPERATOR_ADD);
  } else if (aGap) {
    gfxRect gapRect(gfxFloat(aGap->x) / twipsPerPixel,
                    gfxFloat(aGap->y) / twipsPerPixel,
                    gfxFloat(aGap->width) / twipsPerPixel,
                    gfxFloat(aGap->height) / twipsPerPixel);
    ctx->Rectangle(gapRect);
    ctx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    ctx->Clip();
    ctx->SetFillRule(gfxContext::FILL_RULE_WINDING);
  }

  
  
  
  
  int numSides = doSeparateSides ? 4 : 1;
  for (int i = 0; i < numSides; i++) {
    PRUint8 side = gBorderSideOrder[i];

    if (doSeparateSides) {
      
      if (aSkipSides & (1 << side))
        continue;

      ctx->Save();

      PRUint8 style = aBorderStyle.GetBorderStyle(side);

      
      
      
      
      
      
      
      
      
      
      
      
      DoSideClipPath(ctx, iRect, oRect, lRect, side, aBorderStyle, borderRadii);
      ctx->Clip();

#if 0
      switch (i) {
      case 0: ctx->SetColor(gfxRGBA(0.0,1.0,0.0,0.5)); break;
      case 1: ctx->SetColor(gfxRGBA(1.0,0.0,0.0,0.5)); break;
      case 2: ctx->SetColor(gfxRGBA(0.0,1.0,1.0,0.5)); break;
      case 3: ctx->SetColor(gfxRGBA(0.0,0.0,1.0,0.5)); break;
      }
      ctx->Paint();
#endif
    }

    
    
    
    

    nscolor borderRenderColor;
    PRBool transparent, foreground;
    nsBorderColors *compositeColors = nsnull;

    aBorderStyle.GetBorderColor(side, borderRenderColor, transparent, foreground);
    aBorderStyle.GetCompositeColors(side, &compositeColors);

    if (!transparent || compositeColors) {
      if (foreground)
        borderRenderColor = ourColor->mColor;

      PRUint32 borderWidth = border.side(side) / twipsPerPixel;
      NS_ASSERTION(borderWidth * twipsPerPixel == border.side(side), "Border size from style system was not an integer number of pixels!");
      DrawBorderSides(ctx,
                      borderWidth,
                      aBorderStyle.GetBorderStyle(side),
                      borderRenderColor,
                      compositeColors,
                      iRect, oRect, lRect,
                      ourColor->mColor, bgColor->mBackgroundColor,
                      doSeparateSides, side, aSkipSides,
                      twipsPerPixel,
                      haveBorderRadius ? borderRadii : nsnull);
      SN("----------------");
    }

    if (doSeparateSides)
      ctx->Restore();
  }

  if (doSeparateSides) {
    ctx->PopGroupToSource();
    ctx->Paint();
  }

  ctx->Restore();

  SN();
}

void
nsCSSRendering::PaintOutline(nsPresContext* aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             nsIFrame* aForFrame,
                             const nsRect& aDirtyRect,
                             const nsRect& aBorderArea,
                             const nsStyleBorder& aBorderStyle,
                             const nsStyleOutline& aOutlineStyle,
                             nsStyleContext* aStyleContext,
                             nsRect* aGap)
{
  nsStyleCoord        bordStyleRadius[4];
  PRInt32             borderRadii[4];

  PRBool haveBorderRadius = PR_FALSE;

  
  const nsStyleColor* ourColor = aStyleContext->GetStyleColor();

  nscoord width, offset;
  float percent;

  aOutlineStyle.GetOutlineWidth(width);

  if (width == 0) {
    
    return;
  }

  const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground
    (aStyleContext, PR_FALSE);

  
  aOutlineStyle.mOutlineRadius.GetTop(bordStyleRadius[0]);      
  aOutlineStyle.mOutlineRadius.GetRight(bordStyleRadius[1]);    
  aOutlineStyle.mOutlineRadius.GetBottom(bordStyleRadius[2]);   
  aOutlineStyle.mOutlineRadius.GetLeft(bordStyleRadius[3]);     

  
  for (int i = 0; i < 4; i++) {
    borderRadii[i] = 0;

    switch (bordStyleRadius[i].GetUnit()) {
      case eStyleUnit_Percent:
        percent = bordStyleRadius[i].GetPercentValue();
        borderRadii[i] = (nscoord)(percent * aBorderArea.width);
        break;

      case eStyleUnit_Coord:
        borderRadii[i] = bordStyleRadius[i].GetCoordValue();
        break;

      default:
        break;
    }

    if (borderRadii[i])
      haveBorderRadius = PR_TRUE;
  }

  nsRect overflowArea = aForFrame->GetOverflowRect();

  
  aOutlineStyle.GetOutlineOffset(offset);
  nsRect outside(overflowArea + aBorderArea.TopLeft());
  nsRect inside(outside);
  if (width + offset >= 0) {
    
    inside.Deflate(width, width);
  } else {
    
    
    inside.Deflate(-offset, -offset);
    if (inside.width < 0 || inside.height < 0) {
      return; 
    }
    outside = inside;
    outside.Inflate(width, width);
  }

  
  
  
  
  
  if (inside.Contains(aDirtyRect)) {
    return;
  }

  
  
  
  
  
  if (haveBorderRadius) {
    borderRadii[0] = PR_MIN(borderRadii[0], (inside.width + width) / 2);
    borderRadii[0] = PR_MIN(borderRadii[0], (inside.height + width) / 2);
    borderRadii[0] = PR_MAX(borderRadii[0], 0);

    borderRadii[1] = PR_MIN(borderRadii[1], (inside.width + width) / 2);
    borderRadii[1] = PR_MIN(borderRadii[1], (inside.height + width) / 2);
    borderRadii[1] = PR_MAX(borderRadii[1], 0);

    borderRadii[2] = PR_MIN(borderRadii[2], (inside.width + width) / 2);
    borderRadii[2] = PR_MIN(borderRadii[2], (inside.height + width) / 2);
    borderRadii[2] = PR_MAX(borderRadii[2], 0);

    borderRadii[3] = PR_MIN(borderRadii[3], (inside.width + width) / 2);
    borderRadii[3] = PR_MIN(borderRadii[3], (inside.height + width) / 2);
    borderRadii[3] = PR_MAX(borderRadii[3], 0);
  }

  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  gfxRect oRect(gfxFloat(outside.x) / twipsPerPixel,
                gfxFloat(outside.y) / twipsPerPixel,
                gfxFloat(outside.width) / twipsPerPixel,
                gfxFloat(outside.height) / twipsPerPixel);

  gfxRect iRect(gfxFloat(inside.x) / twipsPerPixel,
                gfxFloat(inside.y) / twipsPerPixel,
                gfxFloat(inside.width) / twipsPerPixel,
                gfxFloat(inside.height) / twipsPerPixel);

  oRect.Round();
  iRect.Round();

  
  
  gfxRect lRect(oRect.pos.x + width / (2.0 * twipsPerPixel),
                oRect.pos.y + width / (2.0 * twipsPerPixel),
                oRect.size.width - (2*width) / (2.0 * twipsPerPixel),
                oRect.size.height - (2*width) / (2.0 * twipsPerPixel));

  
  
  oRect.Condition();
  if (oRect.IsEmpty())
    return;

  iRect.Condition();
  lRect.Condition();

  nscolor outlineColor;

  
  
  if (!aOutlineStyle.GetOutlineColor(outlineColor))
    outlineColor = ourColor->mColor;

  PRUint8 outlineStyle = aOutlineStyle.GetOutlineStyle();

  
  nsRefPtr<gfxContext> ctx = (gfxContext*)
    aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);

  ctx->Save();

  
  
  
  
  gfxMatrix mat = ctx->CurrentMatrix();
  if (!mat.HasNonTranslation()) {
    mat.x0 = floor(mat.x0 + 0.5);
    mat.y0 = floor(mat.y0 + 0.5);
    ctx->SetMatrix(mat);
  }

  PRBool doSeparateSides = PR_FALSE;
  if (outlineStyle == NS_STYLE_BORDER_STYLE_DASHED ||
      outlineStyle == NS_STYLE_BORDER_STYLE_DOTTED)
  {
    doSeparateSides = PR_TRUE;
  }

  
  
  if (doSeparateSides) {
    
    ctx->NewPath();
    ctx->Rectangle(oRect);

    if (aGap) {
      gfxRect gapRect(gfxFloat(aGap->x) / twipsPerPixel,
                      gfxFloat(aGap->y) / twipsPerPixel,
                      gfxFloat(aGap->width) / twipsPerPixel,
                      gfxFloat(aGap->height) / twipsPerPixel);
      ctx->Rectangle(gapRect);
      ctx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
      ctx->Clip();
      ctx->SetFillRule(gfxContext::FILL_RULE_WINDING);
    } else {
      ctx->Clip();
    }

    
    
    ctx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
    ctx->SetOperator(gfxContext::OPERATOR_ADD);
  } else if (aGap) {
    gfxRect gapRect(gfxFloat(aGap->x) / twipsPerPixel,
                    gfxFloat(aGap->y) / twipsPerPixel,
                    gfxFloat(aGap->width) / twipsPerPixel,
                    gfxFloat(aGap->height) / twipsPerPixel);
    ctx->Rectangle(gapRect);
    ctx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    ctx->Clip();
    ctx->SetFillRule(gfxContext::FILL_RULE_WINDING);
  }

  
  
  
  

  int numSides = doSeparateSides ? 4 : 1;
  for (int i = 0; i < numSides; i++) {
    PRUint8 side = gBorderSideOrder[i];

    
    if (doSeparateSides) {
      ctx->Save();

      PRUint8 style = outlineStyle;

      
      
      
      
      
      
      
      
      
      
      
      
      DoSideClipPath(ctx, iRect, oRect, lRect, side, aBorderStyle, borderRadii);
      ctx->Clip();
    }

    
    
    
    

    PRUint32 outlineWidth = NSToCoordRound(float(gfxFloat(width) / twipsPerPixel));
    DrawBorderSides(ctx,
                    outlineWidth,
                    outlineStyle,
                    outlineColor,
                    nsnull,
                    iRect, oRect, lRect,
                    outlineColor, bgColor->mBackgroundColor,
                    doSeparateSides, side, 0,
                    twipsPerPixel,
                    haveBorderRadius ? borderRadii : nsnull);

    if (doSeparateSides)
      ctx->Restore();
  }

  if (doSeparateSides) {
    ctx->PopGroupToSource();
    ctx->Paint();
  }

  ctx->Restore();

  SN();
}

























static void
ComputeBackgroundAnchorPoint(const nsStyleBackground& aColor,
                             const nsRect& aOriginBounds,
                             const nsRect& aClipBounds,
                             nscoord aTileWidth, nscoord aTileHeight,
                             nsPoint& aResult)
{
  nscoord x;
  if (NS_STYLE_BG_X_POSITION_LENGTH & aColor.mBackgroundFlags) {
    x = aColor.mBackgroundXPosition.mCoord;
  }
  else if (NS_STYLE_BG_X_POSITION_PERCENT & aColor.mBackgroundFlags) {
    PRFloat64 percent = PRFloat64(aColor.mBackgroundXPosition.mFloat);
    nscoord tilePos = nscoord(percent * PRFloat64(aTileWidth));
    nscoord boxPos = nscoord(percent * PRFloat64(aOriginBounds.width));
    x = boxPos - tilePos;
  }
  else {
    x = 0;
  }
  x += aOriginBounds.x - aClipBounds.x;
  if (NS_STYLE_BG_REPEAT_X & aColor.mBackgroundRepeat) {
    
    
    
    
    if (x < 0) {
      x = -x;
      if (x < 0) {
        
        x = 0;
      }
      x %= aTileWidth;
      x = -x;
    }
    else if (x != 0) {
      x %= aTileWidth;
      if (x > 0) {
        x = x - aTileWidth;
      }
    }

    NS_POSTCONDITION((x >= -(aTileWidth - 1)) && (x <= 0), "bad computed anchor value");
  }
  aResult.x = x;

  nscoord y;
  if (NS_STYLE_BG_Y_POSITION_LENGTH & aColor.mBackgroundFlags) {
    y = aColor.mBackgroundYPosition.mCoord;
  }
  else if (NS_STYLE_BG_Y_POSITION_PERCENT & aColor.mBackgroundFlags){
    PRFloat64 percent = PRFloat64(aColor.mBackgroundYPosition.mFloat);
    nscoord tilePos = nscoord(percent * PRFloat64(aTileHeight));
    nscoord boxPos = nscoord(percent * PRFloat64(aOriginBounds.height));
    y = boxPos - tilePos;
  }
  else {
    y = 0;
  }
  y += aOriginBounds.y - aClipBounds.y;
  if (NS_STYLE_BG_REPEAT_Y & aColor.mBackgroundRepeat) {
    
    
    
    
    if (y < 0) {
      y = -y;
      if (y < 0) {
        
        y = 0;
      }
      y %= aTileHeight;
      y = -y;
    }
    else if (y != 0) {
      y %= aTileHeight;
      if (y > 0) {
        y = y - aTileHeight;
      }
    }
    
    NS_POSTCONDITION((y >= -(aTileHeight - 1)) && (y <= 0), "bad computed anchor value");
  }
  aResult.y = y;
}



static nsIScrollableFrame*
GetRootScrollableFrame(nsPresContext* aPresContext, nsIFrame* aRootFrame)
{
  nsIScrollableFrame* scrollableFrame = nsnull;

  if (nsGkAtoms::viewportFrame == aRootFrame->GetType()) {
    nsIFrame* childFrame = aRootFrame->GetFirstChild(nsnull);

    if (childFrame) {
      if (nsGkAtoms::scrollFrame == childFrame->GetType()) {
        
        
        
        
        CallQueryInterface(childFrame, &scrollableFrame);
      }
    }
  }
#ifdef DEBUG
  else {
    NS_WARNING("aRootFrame is not a viewport frame");
  }
#endif 

  return scrollableFrame;
}

const nsStyleBackground*
nsCSSRendering::FindNonTransparentBackground(nsStyleContext* aContext,
                                             PRBool aStartAtParent )
{
  NS_ASSERTION(aContext, "Cannot find NonTransparentBackground in a null context" );
  
  const nsStyleBackground* result = nsnull;
  nsStyleContext* context = nsnull;
  if (aStartAtParent) {
    context = aContext->GetParent();
  }
  if (!context) {
    context = aContext;
  }
  
  while (context) {
    result = context->GetStyleBackground();
    if (0 == (result->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT))
      break;

    context = context->GetParent();
  }
  return result;
}









































inline nsIFrame*
IsCanvasFrame(nsIFrame *aFrame)
{
  nsIAtom* frameType = aFrame->GetType();
  if (frameType == nsGkAtoms::canvasFrame ||
      frameType == nsGkAtoms::rootFrame ||
      frameType == nsGkAtoms::pageFrame ||
      frameType == nsGkAtoms::pageContentFrame) {
    return aFrame;
  } else if (frameType == nsGkAtoms::viewportFrame) {
    nsIFrame* firstChild = aFrame->GetFirstChild(nsnull);
    if (firstChild) {
      return firstChild;
    }
  }
  
  return nsnull;
}

inline PRBool
FindCanvasBackground(nsIFrame* aForFrame,
                     const nsStyleBackground** aBackground)
{
  
  
  nsIFrame *firstChild = aForFrame->GetFirstChild(nsnull);
  if (firstChild) {
    const nsStyleBackground* result = firstChild->GetStyleBackground();
    nsIFrame* topFrame = aForFrame;

    if (firstChild->GetType() == nsGkAtoms::pageContentFrame) {
      topFrame = firstChild->GetFirstChild(nsnull);
      NS_ASSERTION(topFrame,
                   "nsPageContentFrame is missing a normal flow child");
      if (!topFrame) {
        return PR_FALSE;
      }
      NS_ASSERTION(topFrame->GetContent(),
                   "nsPageContentFrame child without content");
      result = topFrame->GetStyleBackground();
    }

    
    if (result->IsTransparent()) {
      nsIContent* content = topFrame->GetContent();
      if (content) {
        
        nsIDocument* document = content->GetOwnerDoc();
        nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(document);
        if (htmlDoc) {
          if (!document->IsCaseSensitive()) { 
            nsCOMPtr<nsIDOMHTMLElement> body;
            htmlDoc->GetBody(getter_AddRefs(body));
            nsCOMPtr<nsIContent> bodyContent = do_QueryInterface(body);
            
            
            
            
            
            
            
            
            
            if (bodyContent) {
              nsIFrame *bodyFrame = aForFrame->PresContext()->GetPresShell()->
                GetPrimaryFrameFor(bodyContent);
              if (bodyFrame)
                result = bodyFrame->GetStyleBackground();
            }
          }
        }
      }
    }

    *aBackground = result;
  } else {
    
    
    
    *aBackground = aForFrame->GetStyleBackground();
  }
  
  return PR_TRUE;
}

inline PRBool
FindElementBackground(nsIFrame* aForFrame,
                      const nsStyleBackground** aBackground)
{
  nsIFrame *parentFrame = aForFrame->GetParent();
  
  if (parentFrame && IsCanvasFrame(parentFrame) == parentFrame) {
    
    nsIFrame *childFrame = parentFrame->GetFirstChild(nsnull);
    if (childFrame == aForFrame)
      return PR_FALSE; 
  }

  *aBackground = aForFrame->GetStyleBackground();

  
  

  if (aForFrame->GetStyleContext()->GetPseudoType())
    return PR_TRUE; 

  nsIContent* content = aForFrame->GetContent();
  if (!content || !content->IsNodeOfType(nsINode::eHTML))
    return PR_TRUE;  

  if (!parentFrame)
    return PR_TRUE; 

  if (content->Tag() != nsGkAtoms::body)
    return PR_TRUE; 

  
  nsIDocument* document = content->GetOwnerDoc();
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(document);
  if (!htmlDoc)
    return PR_TRUE;

  if (document->IsCaseSensitive()) 
    return PR_TRUE;
  
  nsCOMPtr<nsIDOMHTMLElement> body;
  htmlDoc->GetBody(getter_AddRefs(body));
  nsCOMPtr<nsIContent> bodyContent = do_QueryInterface(body);
  if (bodyContent != content)
    return PR_TRUE; 

  const nsStyleBackground* htmlBG = parentFrame->GetStyleBackground();
  return !htmlBG->IsTransparent();
}

PRBool
nsCSSRendering::FindBackground(nsPresContext* aPresContext,
                               nsIFrame* aForFrame,
                               const nsStyleBackground** aBackground,
                               PRBool* aIsCanvas)
{
  nsIFrame* canvasFrame = IsCanvasFrame(aForFrame);
  *aIsCanvas = canvasFrame != nsnull;
  return canvasFrame
      ? FindCanvasBackground(canvasFrame, aBackground)
      : FindElementBackground(aForFrame, aBackground);
}

void
nsCSSRendering::DidPaint()
{
  gInlineBGData->Reset();
}

void
nsCSSRendering::PaintBackground(nsPresContext* aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                nsIFrame* aForFrame,
                                const nsRect& aDirtyRect,
                                const nsRect& aBorderArea,
                                const nsStyleBorder& aBorder,
                                const nsStylePadding& aPadding,
                                PRBool aUsePrintSettings,
                                nsRect* aBGClipRect)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  PRBool isCanvas;
  const nsStyleBackground *color;

  if (!FindBackground(aPresContext, aForFrame, &color, &isCanvas)) {
    
    
    
    
    
    if (!aForFrame->GetStyleDisplay()->mAppearance) {
      return;
    }

    nsIContent* content = aForFrame->GetContent();
    if (!content || content->GetParent()) {
      return;
    }
        
    color = aForFrame->GetStyleBackground();
  }
  if (!isCanvas) {
    PaintBackgroundWithSC(aPresContext, aRenderingContext, aForFrame,
                          aDirtyRect, aBorderArea, *color, aBorder,
                          aPadding, aUsePrintSettings, aBGClipRect);
    return;
  }

  nsStyleBackground canvasColor(*color);

  nsIViewManager* vm = aPresContext->GetViewManager();

  if (canvasColor.mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT) {
    nsIView* rootView;
    vm->GetRootView(rootView);
    if (!rootView->GetParent()) {
      PRBool widgetIsTranslucent = PR_FALSE;

      if (rootView->HasWidget()) {
        rootView->GetWidget()->GetWindowTranslucency(widgetIsTranslucent);
      }
      
      if (!widgetIsTranslucent) {
        
        
        canvasColor.mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
        canvasColor.mBackgroundColor = aPresContext->DefaultBackgroundColor();
      }
    }
  }

  vm->SetDefaultBackgroundColor(canvasColor.mBackgroundColor);

  PaintBackgroundWithSC(aPresContext, aRenderingContext, aForFrame,
                        aDirtyRect, aBorderArea, canvasColor,
                        aBorder, aPadding, aUsePrintSettings, aBGClipRect);
}

inline nscoord IntDivFloor(nscoord aDividend, nscoord aDivisor)
{
  NS_PRECONDITION(aDivisor > 0,
                  "this function only works for positive divisors");
  
  
  
  
  return (aDividend < 0 ? (aDividend - aDivisor + 1) : aDividend) / aDivisor;
}

inline nscoord IntDivCeil(nscoord aDividend, nscoord aDivisor)
{
  NS_PRECONDITION(aDivisor > 0,
                  "this function only works for positive divisors");
  
  
  
  
  return (aDividend > 0 ? (aDividend + aDivisor - 1) : aDividend) / aDivisor;
}





static nscoord
FindTileStart(nscoord aDirtyStart, nscoord aTileOffset, nscoord aTileSize)
{
  
  return aTileOffset +
         IntDivFloor(aDirtyStart - aTileOffset, aTileSize) * aTileSize;
}





static nscoord
FindTileEnd(nscoord aDirtyEnd, nscoord aTileOffset, nscoord aTileSize)
{
  
  return aTileOffset +
         IntDivCeil(aDirtyEnd - aTileOffset, aTileSize) * aTileSize;
}

void
nsCSSRendering::PaintBackgroundWithSC(nsPresContext* aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      nsIFrame* aForFrame,
                                      const nsRect& aDirtyRect,
                                      const nsRect& aBorderArea,
                                      const nsStyleBackground& aColor,
                                      const nsStyleBorder& aBorder,
                                      const nsStylePadding& aPadding,
                                      PRBool aUsePrintSettings,
                                      nsRect* aBGClipRect)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  PRBool canDrawBackgroundImage = PR_TRUE;
  PRBool canDrawBackgroundColor = PR_TRUE;

  if (aUsePrintSettings) {
    canDrawBackgroundImage = aPresContext->GetBackgroundImageDraw();
    canDrawBackgroundColor = aPresContext->GetBackgroundColorDraw();
  }

  
  
  const nsStyleDisplay* displayData = aForFrame->GetStyleDisplay();
  if (displayData->mAppearance) {
    nsITheme *theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame, displayData->mAppearance)) {
      nsPoint offset = aBorderArea.TopLeft();
      nsIRenderingContext::AutoPushTranslation
          translate(&aRenderingContext, offset.x, offset.y);
      nsRect dirty;
      nsRect border = aBorderArea - offset;
      dirty.IntersectRect(aDirtyRect - offset, border);
      theme->DrawWidgetBackground(&aRenderingContext, aForFrame, 
                                  displayData->mAppearance, border, dirty);
      return;
    }
  }

  nsRect bgClipArea;
  if (aBGClipRect) {
    bgClipArea = *aBGClipRect;
  }
  else {
    
    bgClipArea = aBorderArea;
    if (aColor.mBackgroundClip != NS_STYLE_BG_CLIP_BORDER) {
      NS_ASSERTION(aColor.mBackgroundClip == NS_STYLE_BG_CLIP_PADDING,
                   "unknown background-clip value");
      nsMargin border = aForFrame->GetUsedBorder();
      aForFrame->ApplySkipSides(border);
      bgClipArea.Deflate(border);
    }
  }

  
  
  nsRect dirtyRect;
  if (!dirtyRect.IntersectRect(bgClipArea, aDirtyRect)) {
    
    return;
  }

  
  if (!aColor.mBackgroundImage || !canDrawBackgroundImage) {
    PaintBackgroundColor(aPresContext, aRenderingContext, aForFrame, bgClipArea,
                         aColor, aBorder, aPadding, canDrawBackgroundColor);
    return;
  }

  

  
  imgIRequest *req = aPresContext->LoadImage(aColor.mBackgroundImage,
                                             aForFrame);

  PRUint32 status = imgIRequest::STATUS_ERROR;
  if (req)
    req->GetImageStatus(&status);

  if (!req || !(status & imgIRequest::STATUS_FRAME_COMPLETE) || !(status & imgIRequest::STATUS_SIZE_AVAILABLE)) {
    PaintBackgroundColor(aPresContext, aRenderingContext, aForFrame, bgClipArea,
                         aColor, aBorder, aPadding, canDrawBackgroundColor);
    return;
  }

  nsCOMPtr<imgIContainer> image;
  req->GetImage(getter_AddRefs(image));

  nsSize imageSize;
  image->GetWidth(&imageSize.width);
  image->GetHeight(&imageSize.height);

  imageSize.width = nsPresContext::CSSPixelsToAppUnits(imageSize.width);
  imageSize.height = nsPresContext::CSSPixelsToAppUnits(imageSize.height);

  req = nsnull;

  nsRect bgOriginArea;

  nsIAtom* frameType = aForFrame->GetType();
  if (frameType == nsGkAtoms::inlineFrame ||
      frameType == nsGkAtoms::positionedInlineFrame) {
    switch (aColor.mBackgroundInlinePolicy) {
    case NS_STYLE_BG_INLINE_POLICY_EACH_BOX:
      bgOriginArea = aBorderArea;
      break;
    case NS_STYLE_BG_INLINE_POLICY_BOUNDING_BOX:
      bgOriginArea = gInlineBGData->GetBoundingRect(aForFrame) +
                     aBorderArea.TopLeft();
      break;
    default:
      NS_ERROR("Unknown background-inline-policy value!  "
               "Please, teach me what to do.");
    case NS_STYLE_BG_INLINE_POLICY_CONTINUOUS:
      bgOriginArea = gInlineBGData->GetContinuousRect(aForFrame) +
                     aBorderArea.TopLeft();
      break;
    }
  }
  else {
    bgOriginArea = aBorderArea;
  }

  
  
  if (aColor.mBackgroundOrigin != NS_STYLE_BG_ORIGIN_BORDER) {
    nsMargin border = aForFrame->GetUsedBorder();
    aForFrame->ApplySkipSides(border);
    bgOriginArea.Deflate(border);
    if (aColor.mBackgroundOrigin != NS_STYLE_BG_ORIGIN_PADDING) {
      nsMargin padding = aForFrame->GetUsedPadding();
      aForFrame->ApplySkipSides(padding);
      bgOriginArea.Deflate(padding);
      NS_ASSERTION(aColor.mBackgroundOrigin == NS_STYLE_BG_ORIGIN_CONTENT,
                   "unknown background-origin value");
    }
  }

  
  
  
  nscoord tileWidth = imageSize.width;
  nscoord tileHeight = imageSize.height;
  PRBool  needBackgroundColor = !(aColor.mBackgroundFlags &
                                  NS_STYLE_BG_COLOR_TRANSPARENT);
  PRIntn  repeat = aColor.mBackgroundRepeat;
  nscoord xDistance, yDistance;

  switch (repeat) {
    case NS_STYLE_BG_REPEAT_X:
      xDistance = dirtyRect.width;
      yDistance = tileHeight;
      break;
    case NS_STYLE_BG_REPEAT_Y:
      xDistance = tileWidth;
      yDistance = dirtyRect.height;
      break;
    case NS_STYLE_BG_REPEAT_XY:
      xDistance = dirtyRect.width;
      yDistance = dirtyRect.height;
      if (needBackgroundColor) {
        
        
        nsCOMPtr<gfxIImageFrame> gfxImgFrame;
        image->GetCurrentFrame(getter_AddRefs(gfxImgFrame));
        if (gfxImgFrame) {
          gfxImgFrame->GetNeedsBackground(&needBackgroundColor);

          
          nsSize iSize;
          image->GetWidth(&iSize.width);
          image->GetHeight(&iSize.height);
          nsRect iframeRect;
          gfxImgFrame->GetRect(iframeRect);
          if (iSize.width != iframeRect.width ||
              iSize.height != iframeRect.height) {
            needBackgroundColor = PR_TRUE;
          }
        }
      }
      break;
    case NS_STYLE_BG_REPEAT_OFF:
    default:
      NS_ASSERTION(repeat == NS_STYLE_BG_REPEAT_OFF, "unknown background-repeat value");
      xDistance = tileWidth;
      yDistance = tileHeight;
      break;
  }

  
  if (needBackgroundColor) {
    PaintBackgroundColor(aPresContext, aRenderingContext, aForFrame, bgClipArea,
                         aColor, aBorder, aPadding, canDrawBackgroundColor);
  }

  if ((tileWidth == 0) || (tileHeight == 0) || dirtyRect.IsEmpty()) {
    
    return;
  }

  
  
  
  

  
  nsPoint anchor;
  if (NS_STYLE_BG_ATTACHMENT_FIXED == aColor.mBackgroundAttachment) {
    
    
    nsIView* viewportView = nsnull;
    nsRect viewportArea;

    
    aPresContext->SetRenderedPositionVaryingContent();

    nsIFrame* rootFrame =
      aPresContext->PresShell()->FrameManager()->GetRootFrame();
    NS_ASSERTION(rootFrame, "no root frame");

    if (aPresContext->IsPaginated()) {
      nsIFrame* page = nsLayoutUtils::GetPageFrame(aForFrame);
      NS_ASSERTION(page, "no page");
      rootFrame = page;
    }

    viewportView = rootFrame->GetView();
    NS_ASSERTION(viewportView, "no viewport view");
    viewportArea = viewportView->GetBounds();
    viewportArea.x = 0;
    viewportArea.y = 0;

    nsIScrollableFrame* scrollableFrame =
      GetRootScrollableFrame(aPresContext, rootFrame);

    if (scrollableFrame) {
      nsMargin scrollbars = scrollableFrame->GetActualScrollbarSizes();
      viewportArea.Deflate(scrollbars);
    }

    
    ComputeBackgroundAnchorPoint(aColor, viewportArea, viewportArea, tileWidth, tileHeight, anchor);

    
    
    anchor -= aForFrame->GetOffsetTo(rootFrame);
  } else {
    if (frameType == nsGkAtoms::canvasFrame) {
      
      
      nsRect firstRootElementFrameArea;
      nsIFrame* firstRootElementFrame = aForFrame->GetFirstChild(nsnull);
      NS_ASSERTION(firstRootElementFrame, "A canvas with a background "
        "image had no child frame, which is impossible according to CSS. "
        "Make sure there isn't a background image specified on the "
        "|:viewport| pseudo-element in |html.css|.");

      
      if (firstRootElementFrame) {
        firstRootElementFrameArea = firstRootElementFrame->GetRect();

        
        const nsStyleBorder* borderStyle = firstRootElementFrame->GetStyleBorder();
        firstRootElementFrameArea.Deflate(borderStyle->GetBorder());

        
        ComputeBackgroundAnchorPoint(aColor, firstRootElementFrameArea +
            aBorderArea.TopLeft(), bgClipArea, tileWidth, tileHeight, anchor);
      } else {
        ComputeBackgroundAnchorPoint(aColor, bgOriginArea, bgClipArea, tileWidth, tileHeight, anchor);
      }
    } else {
      
      
      ComputeBackgroundAnchorPoint(aColor, bgOriginArea, bgClipArea, tileWidth, tileHeight, anchor);
    }

    
    anchor.x += bgClipArea.x - aBorderArea.x;
    anchor.y += bgClipArea.y - aBorderArea.y;
  }

#if (!defined(XP_UNIX) && !defined(XP_BEOS)) || defined(XP_MACOSX)
  
  
  aRenderingContext.PushState();
  aRenderingContext.SetClipRect(dirtyRect, nsClipCombine_kIntersect);
#endif

  

  

































































































  
  nsRect tileRect(anchor, nsSize(tileWidth, tileHeight));
  if (repeat & NS_STYLE_BG_REPEAT_X) {
    
    
    
    nscoord x0 = FindTileStart(dirtyRect.x - aBorderArea.x, anchor.x, tileWidth);
    nscoord x1 = FindTileEnd(dirtyRect.XMost() - aBorderArea.x, anchor.x, tileWidth);
    tileRect.x = x0;
    tileRect.width = x1 - x0;
  }
  if (repeat & NS_STYLE_BG_REPEAT_Y) {
    
    
    
    nscoord y0 = FindTileStart(dirtyRect.y - aBorderArea.y, anchor.y, tileHeight);
    nscoord y1 = FindTileEnd(dirtyRect.YMost() - aBorderArea.y, anchor.y, tileHeight);
    tileRect.y = y0;
    tileRect.height = y1 - y0;
  }

  
  nsRect absTileRect = tileRect + aBorderArea.TopLeft();
  
  nsRect drawRect;
  if (drawRect.IntersectRect(absTileRect, dirtyRect)) {
    
    
    NS_ASSERTION(drawRect.x >= absTileRect.x && drawRect.y >= absTileRect.y,
                 "Bogus intersection");
    NS_ASSERTION(drawRect.x < absTileRect.x + tileWidth,
                 "Bogus x coord for draw rect");
    NS_ASSERTION(drawRect.y < absTileRect.y + tileHeight,
                 "Bogus y coord for draw rect");
    
    nsRect sourceRect = drawRect - absTileRect.TopLeft();
    if (sourceRect.XMost() <= tileWidth && sourceRect.YMost() <= tileHeight) {
      
      
      nsLayoutUtils::DrawImage(&aRenderingContext, image, absTileRect, drawRect);
    } else {
      aRenderingContext.DrawTile(image, absTileRect.x, absTileRect.y, &drawRect);
    }
  }

#if (!defined(XP_UNIX) && !defined(XP_BEOS)) || defined(XP_MACOSX)
  
  aRenderingContext.PopState();
#endif

}

void
nsCSSRendering::PaintBackgroundColor(nsPresContext* aPresContext,
                                     nsIRenderingContext& aRenderingContext,
                                     nsIFrame* aForFrame,
                                     const nsRect& aBgClipArea,
                                     const nsStyleBackground& aColor,
                                     const nsStyleBorder& aBorder,
                                     const nsStylePadding& aPadding,
                                     PRBool aCanPaintNonWhite)
{
  
  
  
  
  if ((aColor.mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT) &&
      (aCanPaintNonWhite || aColor.IsTransparent())) {
    
    return;
  }

  nsStyleCoord bordStyleRadius[4];
  PRInt16 borderRadii[4];
  nsRect bgClipArea(aBgClipArea);

  
  aBorder.mBorderRadius.GetTop(bordStyleRadius[NS_SIDE_TOP]);       
  aBorder.mBorderRadius.GetRight(bordStyleRadius[NS_SIDE_RIGHT]);   
  aBorder.mBorderRadius.GetBottom(bordStyleRadius[NS_SIDE_BOTTOM]); 
  aBorder.mBorderRadius.GetLeft(bordStyleRadius[NS_SIDE_LEFT]);     

  PRUint8 side = 0;
  for (; side < 4; ++side) {
    borderRadii[side] = 0;
    switch (bordStyleRadius[side].GetUnit()) {
      case eStyleUnit_Percent:
        borderRadii[side] = nscoord(bordStyleRadius[side].GetPercentValue() *
                                    aForFrame->GetSize().width);
        break;
      case eStyleUnit_Coord:
        borderRadii[side] = bordStyleRadius[side].GetCoordValue();
        break;
      default:
        break;
    }
  }

  
  
  
  if (!aBorder.mBorderColors) {
    for (side = 0; side < 4; ++side) {
      if (borderRadii[side] > 0) {
        PaintRoundedBackground(aPresContext, aRenderingContext, aForFrame,
                               bgClipArea, aColor, aBorder, borderRadii,
                               aCanPaintNonWhite);
        return;
      }
    }
  }
  else if (aColor.mBackgroundClip == NS_STYLE_BG_CLIP_BORDER) {
    
    
    
    
    nsMargin border = aForFrame->GetUsedBorder();
    aForFrame->ApplySkipSides(border);
    bgClipArea.Deflate(border);
  }

  nscolor color;
  if (!aCanPaintNonWhite) {
    color = NS_RGB(255, 255, 255);
  } else {
    color = aColor.mBackgroundColor;
  }
  
  aRenderingContext.SetColor(color);
  aRenderingContext.FillRect(bgClipArea);
}





void
nsCSSRendering::PaintRoundedBackground(nsPresContext* aPresContext,
                                       nsIRenderingContext& aRenderingContext,
                                       nsIFrame* aForFrame,
                                       const nsRect& aBgClipArea,
                                       const nsStyleBackground& aColor,
                                       const nsStyleBorder& aBorder,
                                       PRInt16 aTheRadius[4],
                                       PRBool aCanPaintNonWhite)
{
  RoundedRect   outerPath;
  QBCurve       cr1,cr2,cr3,cr4;
  QBCurve       UL,UR,LL,LR;
  PRInt32       curIndex,c1Index;
  nsFloatPoint  thePath[MAXPATHSIZE];
  static nsPoint       polyPath[MAXPOLYPATHSIZE];
  PRInt16       np;

  
  nscoord appUnitsPerPixel = nsPresContext::AppUnitsPerCSSPixel();

  nscolor color = aColor.mBackgroundColor;
  if (!aCanPaintNonWhite) {
    color = NS_RGB(255, 255, 255);
  }
  aRenderingContext.SetColor(color);

  
  if (aColor.mBackgroundClip != NS_STYLE_BG_CLIP_BORDER) {
    NS_ASSERTION(aColor.mBackgroundClip == NS_STYLE_BG_CLIP_PADDING, "unknown background-clip value");

    
    
    NS_FOR_CSS_SIDES(side) {
      aTheRadius[side] -= aBorder.GetBorderWidth(side);
      aTheRadius[side] = PR_MAX(aTheRadius[side], 0);
    }
  }

  
  outerPath.Set(aBgClipArea.x, aBgClipArea.y, aBgClipArea.width,
                aBgClipArea.height, aTheRadius, appUnitsPerPixel);
  outerPath.GetRoundedBorders(UL,UR,LL,LR);

  
  
  UL.MidPointDivide(&cr1,&cr2);
  UR.MidPointDivide(&cr3,&cr4);
  np=0;
  thePath[np++].MoveTo(cr2.mAnc1.x,cr2.mAnc1.y);
  thePath[np++].MoveTo(cr2.mCon.x, cr2.mCon.y);
  thePath[np++].MoveTo(cr2.mAnc2.x, cr2.mAnc2.y);
  thePath[np++].MoveTo(cr3.mAnc1.x, cr3.mAnc1.y);
  thePath[np++].MoveTo(cr3.mCon.x, cr3.mCon.y);
  thePath[np++].MoveTo(cr3.mAnc2.x, cr3.mAnc2.y);

  polyPath[0].x = NSToCoordRound(thePath[0].x);
  polyPath[0].y = NSToCoordRound(thePath[0].y);
  curIndex = 1;
  GetPath(thePath,polyPath,&curIndex,eOutside,c1Index);

  
  LR.MidPointDivide(&cr2,&cr3);
  np=0;
  thePath[np++].MoveTo(cr4.mAnc1.x,cr4.mAnc1.y);
  thePath[np++].MoveTo(cr4.mCon.x, cr4.mCon.y);
  thePath[np++].MoveTo(cr4.mAnc2.x, cr4.mAnc2.y);
  thePath[np++].MoveTo(cr2.mAnc1.x, cr2.mAnc1.y);
  thePath[np++].MoveTo(cr2.mCon.x, cr2.mCon.y);
  thePath[np++].MoveTo(cr2.mAnc2.x, cr2.mAnc2.y);
  GetPath(thePath,polyPath,&curIndex,eOutside,c1Index);

  
  LL.MidPointDivide(&cr2,&cr4);
  np=0;
  thePath[np++].MoveTo(cr3.mAnc1.x,cr3.mAnc1.y);
  thePath[np++].MoveTo(cr3.mCon.x, cr3.mCon.y);
  thePath[np++].MoveTo(cr3.mAnc2.x, cr3.mAnc2.y);
  thePath[np++].MoveTo(cr2.mAnc1.x, cr2.mAnc1.y);
  thePath[np++].MoveTo(cr2.mCon.x, cr2.mCon.y);
  thePath[np++].MoveTo(cr2.mAnc2.x, cr2.mAnc2.y);
  GetPath(thePath,polyPath,&curIndex,eOutside,c1Index);

  
  np=0;
  thePath[np++].MoveTo(cr4.mAnc1.x,cr4.mAnc1.y);
  thePath[np++].MoveTo(cr4.mCon.x, cr4.mCon.y);
  thePath[np++].MoveTo(cr4.mAnc2.x, cr4.mAnc2.y);
  thePath[np++].MoveTo(cr1.mAnc1.x, cr1.mAnc1.y);
  thePath[np++].MoveTo(cr1.mCon.x, cr1.mCon.y);
  thePath[np++].MoveTo(cr1.mAnc2.x, cr1.mAnc2.y);
  GetPath(thePath,polyPath,&curIndex,eOutside,c1Index);

  aRenderingContext.FillPolygon(polyPath,curIndex); 
}





void 
RoundedRect::CalcInsetCurves(QBCurve &aULCurve,QBCurve &aURCurve,QBCurve &aLLCurve,QBCurve &aLRCurve,nsMargin &aBorder)
{
PRInt32   nLeft,nTop,nRight,nBottom;
PRInt32   tLeft,bLeft,tRight,bRight,lTop,rTop,lBottom,rBottom;
PRInt16   adjust=0;

  if(mDoRound)
    adjust = mRoundness[0]>>3;

  nLeft = mLeft + aBorder.left;
  tLeft = mLeft + mRoundness[0];
  bLeft = mLeft + mRoundness[3];

  if(tLeft < nLeft){
    tLeft = nLeft;
  }

  if(bLeft < nLeft){
    bLeft = nLeft;
  }

  nRight = mRight - aBorder.right;
  tRight = mRight - mRoundness[1];
  bRight = mRight - mRoundness[2];

  if(tRight > nRight){
    tRight = nRight;
  }

  if(bRight > nRight){
    bRight = nRight;
  }

  nTop = mTop + aBorder.top;
  lTop = mTop + mRoundness[0];
  rTop = mTop + mRoundness[1];

  if(lTop < nTop){
    lTop = nTop;
  }

  if(rTop < nTop){
    rTop = nTop;
  }

  nBottom = mBottom - aBorder.bottom;
  lBottom = mBottom - mRoundness[3];
  rBottom = mBottom - mRoundness[2];

  if(lBottom > nBottom){
    lBottom = nBottom;
  }

  if(rBottom > nBottom){
    rBottom = nBottom;
  }


  
  aULCurve.SetPoints( (float)nLeft,(float)lTop,
                      (float)nLeft+adjust,(float)nTop+adjust,
                      (float)tLeft,(float)nTop);
  aURCurve.SetPoints( (float)tRight,(float)nTop,
                      (float)nRight-adjust,(float)nTop+adjust,
                      (float)nRight,(float)rTop);
  aLRCurve.SetPoints( (float)nRight,(float)rBottom,
                      (float)nRight-adjust,(float)nBottom-adjust,
                      (float)bRight,(float)nBottom);
  aLLCurve.SetPoints( (float)bLeft,(float)nBottom,
                      (float)nLeft+adjust,(float)nBottom-adjust,
                      (float)nLeft,(float)lBottom);

}





void 
RoundedRect::Set(nscoord aLeft,nscoord aTop,PRInt32  aWidth,PRInt32 aHeight,PRInt16 aRadius[4],PRInt16 aNumTwipPerPix)
{
  nscoord x,y,width,height;
  int     i;

  
  
  
  x = NSToCoordRound((float)aLeft/aNumTwipPerPix)*aNumTwipPerPix;
  y = NSToCoordRound((float)aTop/aNumTwipPerPix)*aNumTwipPerPix;
  width = (NSToCoordRound((float)aLeft + aWidth)/aNumTwipPerPix)*aNumTwipPerPix - x;
  height = (NSToCoordRound((float)aTop + aHeight)/aNumTwipPerPix)*aNumTwipPerPix - y;


  for(i=0;i<4;i++) {
    if( (aRadius[i]) > (aWidth>>1) ){
      mRoundness[i] = (aWidth>>1); 
    } else {
      mRoundness[i] = aRadius[i];
    }

    if( mRoundness[i] > (aHeight>>1) )
      mRoundness[i] = aHeight>>1;
  }


  
  mDoRound = PR_FALSE;
  if(aHeight==aWidth){
    PRBool doRound = PR_TRUE;
    for(i=0;i<4;i++){
      if(mRoundness[i]<(aWidth>>1)){
        doRound = PR_FALSE;
        break;
      }
    }

    if(doRound){
      mDoRound = PR_TRUE;
      for(i=0;i<4;i++){
        mRoundness[i] = aWidth>>1;
      }
    }
  }



  
  mLeft = x;
  mTop = y;
  mRight = x+width;
  mBottom = y+height;

}





void 
RoundedRect::GetRoundedBorders(QBCurve &aULCurve,QBCurve &aURCurve,QBCurve &aLLCurve,QBCurve &aLRCurve)
{

  PRInt16 adjust=0;

  if(mDoRound)
    adjust = mRoundness[0]>>3;

  
  aULCurve.SetPoints( (float)mLeft,(float)mTop + mRoundness[0],
                      (float)mLeft+adjust,(float)mTop+adjust,
                      (float)mLeft+mRoundness[0],(float)mTop);
  aURCurve.SetPoints( (float)mRight - mRoundness[1],(float)mTop,
                      (float)mRight-adjust,(float)mTop+adjust,
                      (float)mRight,(float)mTop + mRoundness[1]);
  aLRCurve.SetPoints( (float)mRight,(float)mBottom - mRoundness[2],
                      (float)mRight-adjust,(float)mBottom-adjust,
                      (float)mRight - mRoundness[2],(float)mBottom);
  aLLCurve.SetPoints( (float)mLeft + mRoundness[3],(float)mBottom,
                      (float)mLeft+adjust,(float)mBottom-adjust,
                      (float)mLeft,(float)mBottom - mRoundness[3]);
}










static void 
GetPath(nsFloatPoint aPoints[],nsPoint aPolyPath[],PRInt32 *aCurIndex,ePathTypes  aPathType,PRInt32 &aC1Index,float aFrac)
{
  QBCurve thecurve;
  
  if (*aCurIndex >= MAXPOLYPATHSIZE)
    return;

  switch (aPathType) {
    case eOutside:
      thecurve.SetPoints(aPoints[0].x,aPoints[0].y,aPoints[1].x,aPoints[1].y,aPoints[2].x,aPoints[2].y);
      thecurve.SubDivide(nsnull,aPolyPath,aCurIndex);
      aC1Index = *aCurIndex;
      if (*aCurIndex >= MAXPOLYPATHSIZE)
        return;
      aPolyPath[*aCurIndex].x = (nscoord)aPoints[3].x;
      aPolyPath[*aCurIndex].y = (nscoord)aPoints[3].y;
      (*aCurIndex)++;
      if (*aCurIndex >= MAXPOLYPATHSIZE)
        return;
      thecurve.SetPoints(aPoints[3].x,aPoints[3].y,aPoints[4].x,aPoints[4].y,aPoints[5].x,aPoints[5].y);
      thecurve.SubDivide(nsnull,aPolyPath,aCurIndex);
      break;
    case eInside:
      thecurve.SetPoints(aPoints[6].x,aPoints[6].y,aPoints[7].x,aPoints[7].y,aPoints[8].x,aPoints[8].y);
      thecurve.SubDivide(nsnull,aPolyPath,aCurIndex);
      if (*aCurIndex >= MAXPOLYPATHSIZE)
        return;
      aPolyPath[*aCurIndex].x = (nscoord)aPoints[9].x;
      aPolyPath[*aCurIndex].y = (nscoord)aPoints[9].y;
      (*aCurIndex)++;
      if (*aCurIndex >= MAXPOLYPATHSIZE)
        return;
      thecurve.SetPoints(aPoints[9].x,aPoints[9].y,aPoints[10].x,aPoints[10].y,aPoints[11].x,aPoints[11].y);
      thecurve.SubDivide(nsnull,aPolyPath,aCurIndex);
     break;
    case eCalc:
      thecurve.SetPoints( (aPoints[0].x+aPoints[11].x)/2.0f,(aPoints[0].y+aPoints[11].y)/2.0f,
                          (aPoints[1].x+aPoints[10].x)/2.0f,(aPoints[1].y+aPoints[10].y)/2.0f,
                          (aPoints[2].x+aPoints[9].x)/2.0f,(aPoints[2].y+aPoints[9].y)/2.0f);
      thecurve.SubDivide(nsnull,aPolyPath,aCurIndex);
      if (*aCurIndex >= MAXPOLYPATHSIZE)
        return;
      aPolyPath[*aCurIndex].x = (nscoord)((aPoints[3].x+aPoints[8].x)/2.0f);
      aPolyPath[*aCurIndex].y = (nscoord)((aPoints[3].y+aPoints[8].y)/2.0f);
      (*aCurIndex)++;
      if (*aCurIndex >= MAXPOLYPATHSIZE)
        return;
      thecurve.SetPoints( (aPoints[3].x+aPoints[8].x)/2.0f,(aPoints[3].y+aPoints[8].y)/2.0f,
                          (aPoints[4].x+aPoints[7].x)/2.0f,(aPoints[4].y+aPoints[7].y)/2.0f,
                          (aPoints[5].x+aPoints[6].x)/2.0f,(aPoints[5].y+aPoints[6].y)/2.0f);
      thecurve.SubDivide(nsnull,aPolyPath,aCurIndex);
      break;
    case eCalcRev:
      thecurve.SetPoints( (aPoints[5].x+aPoints[6].x)/2.0f,(aPoints[5].y+aPoints[6].y)/2.0f,
                          (aPoints[4].x+aPoints[7].x)/2.0f,(aPoints[4].y+aPoints[7].y)/2.0f,
                          (aPoints[3].x+aPoints[8].x)/2.0f,(aPoints[3].y+aPoints[8].y)/2.0f);
      thecurve.SubDivide(nsnull,aPolyPath,aCurIndex);
      aPolyPath[*aCurIndex].x = (nscoord)((aPoints[2].x+aPoints[9].x)/2.0f);
      aPolyPath[*aCurIndex].y = (nscoord)((aPoints[2].y+aPoints[9].y)/2.0f);
      (*aCurIndex)++;
      if (*aCurIndex >= MAXPOLYPATHSIZE)
        return;
      thecurve.SetPoints( (aPoints[2].x+aPoints[9].x)/2.0f,(aPoints[2].y+aPoints[9].y)/2.0f,
                          (aPoints[1].x+aPoints[10].x)/2.0f,(aPoints[1].y+aPoints[10].y)/2.0f,
                          (aPoints[0].x+aPoints[11].x)/2.0f,(aPoints[0].y+aPoints[11].y)/2.0f);
      thecurve.SubDivide(nsnull,aPolyPath,aCurIndex);
      break;
  } 
}





void 
QBCurve::SubDivide(nsIRenderingContext *aRenderingContext,nsPoint aPointArray[],PRInt32 *aCurIndex)
{
  QBCurve   curve1,curve2;
  float     fx,fy,smag, oldfx, oldfy, oldsmag;
  
  if (aCurIndex && (*aCurIndex >= MAXPOLYPATHSIZE))
    return;
  
  oldfx = (this->mAnc1.x + this->mAnc2.x)/2.0f - this->mCon.x;
  oldfy = (this->mAnc1.y + this->mAnc2.y)/2.0f - this->mCon.y;
  oldsmag = oldfx * oldfx + oldfy * oldfy;
  
  MidPointDivide(&curve1,&curve2);

  fx = (float)fabs(curve1.mAnc2.x - this->mCon.x);
  fy = (float)fabs(curve1.mAnc2.y - this->mCon.y);

  
  smag = fx*fx + fy*fy;
 
  if (smag>1){
    if (smag + 0.2 > oldsmag)
      return; 
    
    curve1.SubDivide(aRenderingContext,aPointArray,aCurIndex);
    curve2.SubDivide(aRenderingContext,aPointArray,aCurIndex);
  }else{
    if(aPointArray ) {
      
      aPointArray[*aCurIndex].x = (nscoord)curve1.mAnc2.x;
      aPointArray[*aCurIndex].y = (nscoord)curve1.mAnc2.y;
      (*aCurIndex)++;
      if (*aCurIndex >= MAXPOLYPATHSIZE)
        return;
      aPointArray[*aCurIndex].x = (nscoord)curve2.mAnc2.x;
      aPointArray[*aCurIndex].y = (nscoord)curve2.mAnc2.y;
      (*aCurIndex)++;
    }else{
      
      nsTransform2D *aTransform;
      aRenderingContext->GetCurrentTransform(aTransform);

      
      aRenderingContext->DrawLine((nscoord)curve1.mAnc1.x,(nscoord)curve1.mAnc1.y,(nscoord)curve1.mAnc2.x,(nscoord)curve1.mAnc2.y);
      aRenderingContext->DrawLine((nscoord)curve1.mAnc2.x,(nscoord)curve1.mAnc2.y,(nscoord)curve2.mAnc2.x,(nscoord)curve2.mAnc2.y);
    }
  }
}





void 
QBCurve::MidPointDivide(QBCurve *A,QBCurve *B)
{
  float c1x,c1y,c2x,c2y;
  nsFloatPoint a1;

  c1x = (mAnc1.x+mCon.x)/2.0f;
  c1y = (mAnc1.y+mCon.y)/2.0f;
  c2x = (mAnc2.x+mCon.x)/2.0f;
  c2y = (mAnc2.y+mCon.y)/2.0f;

  a1.x = (c1x + c2x)/2.0f;
  a1.y = (c1y + c2y)/2.0f;

  
  A->mAnc1 = this->mAnc1;
  A->mCon.x = c1x;
  A->mCon.y = c1y;
  A->mAnc2 = a1;
  B->mAnc1 = a1;
  B->mCon.x = c2x;
  B->mCon.y = c2y;
  B->mAnc2 = this->mAnc2;
}

void FillOrInvertRect(nsIRenderingContext& aRC, nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight, PRBool aInvert)
{
#ifdef GFX_HAS_INVERT
  if (aInvert) {
    aRC.InvertRect(aX, aY, aWidth, aHeight);
  } else {
#endif
    aRC.FillRect(aX, aY, aWidth, aHeight);
#ifdef GFX_HAS_INVERT
  }
#endif
}

void FillOrInvertRect(nsIRenderingContext& aRC, const nsRect& aRect, PRBool aInvert)
{
#ifdef GFX_HAS_INVERT
  if (aInvert) {
    aRC.InvertRect(aRect);
  } else {
#endif
    aRC.FillRect(aRect);
#ifdef GFX_HAS_INVERT
  }
#endif
}





static nscoord
RoundIntToPixel(nscoord aValue, 
                nscoord aTwipsPerPixel,
                PRBool  aRoundDown = PR_FALSE)
{
  if (aTwipsPerPixel <= 0) 
    
    
    return aValue; 

  nscoord halfPixel = NSToCoordRound(aTwipsPerPixel / 2.0f);
  nscoord extra = aValue % aTwipsPerPixel;
  nscoord finalValue = (!aRoundDown && (extra >= halfPixel)) ? aValue + (aTwipsPerPixel - extra) : aValue - extra;
  return finalValue;
}

static nscoord
RoundFloatToPixel(float   aValue, 
                  nscoord aTwipsPerPixel,
                  PRBool  aRoundDown = PR_FALSE)
{
  return RoundIntToPixel(NSToCoordRound(aValue), aTwipsPerPixel, aRoundDown);
}

static void
SetPoly(const nsRect& aRect,
        nsPoint*      poly)
{
  poly[0].x = aRect.x;
  poly[0].y = aRect.y;
  poly[1].x = aRect.x + aRect.width;
  poly[1].y = aRect.y;
  poly[2].x = aRect.x + aRect.width;
  poly[2].y = aRect.y + aRect.height;
  poly[3].x = aRect.x;
  poly[3].y = aRect.y + aRect.height;
  poly[4].x = aRect.x;
  poly[4].y = aRect.y;
}
          
static void 
DrawSolidBorderSegment(nsIRenderingContext& aContext,
                       nsRect               aRect,
                       nscoord              aTwipsPerPixel,
                       PRUint8              aStartBevelSide = 0,
                       nscoord              aStartBevelOffset = 0,
                       PRUint8              aEndBevelSide = 0,
                       nscoord              aEndBevelOffset = 0)
{

  if ((aRect.width == aTwipsPerPixel) || (aRect.height == aTwipsPerPixel) ||
      ((0 == aStartBevelOffset) && (0 == aEndBevelOffset))) {
    
    if ((NS_SIDE_TOP == aStartBevelSide) || (NS_SIDE_BOTTOM == aStartBevelSide)) {
      if (1 == aRect.height) 
        aContext.DrawLine(aRect.x, aRect.y, aRect.x, aRect.y + aRect.height); 
      else 
        aContext.FillRect(aRect);
    }
    else {
      if (1 == aRect.width) 
        aContext.DrawLine(aRect.x, aRect.y, aRect.x + aRect.width, aRect.y); 
      else 
        aContext.FillRect(aRect);
    }
  }
  else {
    
    nsPoint poly[5];
    SetPoly(aRect, poly);
    switch(aStartBevelSide) {
    case NS_SIDE_TOP:
      poly[0].x += aStartBevelOffset;
      poly[4].x = poly[0].x;
      break;
    case NS_SIDE_BOTTOM:
      poly[3].x += aStartBevelOffset;
      break;
    case NS_SIDE_RIGHT:
      poly[1].y += aStartBevelOffset;
      break;
    case NS_SIDE_LEFT:
      poly[0].y += aStartBevelOffset;
      poly[4].y = poly[0].y;
    }

    switch(aEndBevelSide) {
    case NS_SIDE_TOP:
      poly[1].x -= aEndBevelOffset;
      break;
    case NS_SIDE_BOTTOM:
      poly[2].x -= aEndBevelOffset;
      break;
    case NS_SIDE_RIGHT:
      poly[2].y -= aEndBevelOffset;
      break;
    case NS_SIDE_LEFT:
      poly[3].y -= aEndBevelOffset;
    }

    aContext.FillPolygon(poly, 5);
  }


}

static void
GetDashInfo(nscoord  aBorderLength,
            nscoord  aDashLength,
            nscoord  aTwipsPerPixel,
            PRInt32& aNumDashSpaces,
            nscoord& aStartDashLength,
            nscoord& aEndDashLength)
{
  aNumDashSpaces = 0;
  if (aStartDashLength + aDashLength + aEndDashLength >= aBorderLength) {
    aStartDashLength = aBorderLength;
    aEndDashLength = 0;
  }
  else {
    aNumDashSpaces = aBorderLength / (2 * aDashLength); 
    nscoord extra = aBorderLength - aStartDashLength - aEndDashLength - (((2 * aNumDashSpaces) - 1) * aDashLength);
    if (extra > 0) {
      nscoord half = RoundIntToPixel(extra / 2, aTwipsPerPixel);
      aStartDashLength += half;
      aEndDashLength += (extra - half);
    }
  }
}

void 
nsCSSRendering::DrawTableBorderSegment(nsIRenderingContext&     aContext,
                                       PRUint8                  aBorderStyle,  
                                       nscolor                  aBorderColor,
                                       const nsStyleBackground* aBGColor,
                                       const nsRect&            aBorder,
                                       PRInt32                  aAppUnitsPerCSSPixel,
                                       PRUint8                  aStartBevelSide,
                                       nscoord                  aStartBevelOffset,
                                       PRUint8                  aEndBevelSide,
                                       nscoord                  aEndBevelOffset)
{
  aContext.SetColor (aBorderColor); 

  PRBool horizontal = ((NS_SIDE_TOP == aStartBevelSide) || (NS_SIDE_BOTTOM == aStartBevelSide));
  nscoord twipsPerPixel = NSIntPixelsToAppUnits(1, aAppUnitsPerCSSPixel);
  PRBool ridgeGroove = NS_STYLE_BORDER_STYLE_RIDGE;

  if ((twipsPerPixel >= aBorder.width) || (twipsPerPixel >= aBorder.height) ||
      (NS_STYLE_BORDER_STYLE_DASHED == aBorderStyle) || (NS_STYLE_BORDER_STYLE_DOTTED == aBorderStyle)) {
    
    aStartBevelOffset = 0;
    aEndBevelOffset = 0;
  }

#ifdef MOZ_CAIRO_GFX
  gfxContext *ctx = (gfxContext*) aContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
  gfxContext::AntialiasMode oldMode = ctx->CurrentAntialiasMode();
  ctx->SetAntialiasMode(gfxContext::MODE_ALIASED);
#endif

  switch (aBorderStyle) {
  case NS_STYLE_BORDER_STYLE_NONE:
  case NS_STYLE_BORDER_STYLE_HIDDEN:
    
    break;
  case NS_STYLE_BORDER_STYLE_DOTTED:
  case NS_STYLE_BORDER_STYLE_DASHED: 
    {
      nscoord dashLength = (NS_STYLE_BORDER_STYLE_DASHED == aBorderStyle) ? DASH_LENGTH : DOT_LENGTH;
      
      dashLength *= (horizontal) ? aBorder.height : aBorder.width;
      
      nscoord minDashLength = (NS_STYLE_BORDER_STYLE_DASHED == aBorderStyle) 
                              ? RoundFloatToPixel(((float)dashLength) / 2.0f, twipsPerPixel) : dashLength;
      minDashLength = PR_MAX(minDashLength, twipsPerPixel);
      nscoord numDashSpaces = 0;
      nscoord startDashLength = minDashLength;
      nscoord endDashLength   = minDashLength;
      if (horizontal) {
        GetDashInfo(aBorder.width, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, startDashLength, aBorder.height);
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        for (PRInt32 spaceX = 0; spaceX < numDashSpaces; spaceX++) {
          rect.x += rect.width + dashLength;
          rect.width = (spaceX == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        }
      }
      else {
        GetDashInfo(aBorder.height, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, aBorder.width, startDashLength);
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        for (PRInt32 spaceY = 0; spaceY < numDashSpaces; spaceY++) {
          rect.y += rect.height + dashLength;
          rect.height = (spaceY == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        }
      }
    }
    break;                                  
  case NS_STYLE_BORDER_STYLE_GROOVE:
    ridgeGroove = NS_STYLE_BORDER_STYLE_GROOVE; 
  case NS_STYLE_BORDER_STYLE_RIDGE:
    if ((horizontal && (twipsPerPixel >= aBorder.height)) ||
        (!horizontal && (twipsPerPixel >= aBorder.width))) {
      
      DrawSolidBorderSegment(aContext, aBorder, twipsPerPixel, aStartBevelSide, aStartBevelOffset,
                             aEndBevelSide, aEndBevelOffset);
    }
    else {
      nscoord startBevel = (aStartBevelOffset > 0) 
                            ? RoundFloatToPixel(0.5f * (float)aStartBevelOffset, twipsPerPixel, PR_TRUE) : 0;
      nscoord endBevel =   (aEndBevelOffset > 0) 
                            ? RoundFloatToPixel(0.5f * (float)aEndBevelOffset, twipsPerPixel, PR_TRUE) : 0;
      PRUint8 ridgeGrooveSide = (horizontal) ? NS_SIDE_TOP : NS_SIDE_LEFT;
      aContext.SetColor ( 
        MakeBevelColor(ridgeGrooveSide, ridgeGroove, aBGColor->mBackgroundColor, aBorderColor));
      nsRect rect(aBorder);
      nscoord half;
      if (horizontal) { 
        half = RoundFloatToPixel(0.5f * (float)aBorder.height, twipsPerPixel);
        rect.height = half;
        if (NS_SIDE_TOP == aStartBevelSide) {
          rect.x += startBevel;
          rect.width -= startBevel;
        }
        if (NS_SIDE_TOP == aEndBevelSide) {
          rect.width -= endBevel;
        }
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }
      else { 
        half = RoundFloatToPixel(0.5f * (float)aBorder.width, twipsPerPixel);
        rect.width = half;
        if (NS_SIDE_LEFT == aStartBevelSide) {
          rect.y += startBevel;
          rect.height -= startBevel;
        }
        if (NS_SIDE_LEFT == aEndBevelSide) {
          rect.height -= endBevel;
        }
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }

      rect = aBorder;
      ridgeGrooveSide = (NS_SIDE_TOP == ridgeGrooveSide) ? NS_SIDE_BOTTOM : NS_SIDE_RIGHT;
      aContext.SetColor ( 
        MakeBevelColor(ridgeGrooveSide, ridgeGroove, aBGColor->mBackgroundColor, aBorderColor));
      if (horizontal) {
        rect.y = rect.y + half;
        rect.height = aBorder.height - half;
        if (NS_SIDE_BOTTOM == aStartBevelSide) {
          rect.x += startBevel;
          rect.width -= startBevel;
        }
        if (NS_SIDE_BOTTOM == aEndBevelSide) {
          rect.width -= endBevel;
        }
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }
      else {
        rect.x = rect.x + half;
        rect.width = aBorder.width - half;
        if (NS_SIDE_RIGHT == aStartBevelSide) {
          rect.y += aStartBevelOffset - startBevel;
          rect.height -= startBevel;
        }
        if (NS_SIDE_RIGHT == aEndBevelSide) {
          rect.height -= endBevel;
        }
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }
    }
    break;
  case NS_STYLE_BORDER_STYLE_DOUBLE:
    if ((aBorder.width > 2) && (aBorder.height > 2)) {
      nscoord startBevel = (aStartBevelOffset > 0) 
                            ? RoundFloatToPixel(0.333333f * (float)aStartBevelOffset, twipsPerPixel) : 0;
      nscoord endBevel =   (aEndBevelOffset > 0) 
                            ? RoundFloatToPixel(0.333333f * (float)aEndBevelOffset, twipsPerPixel) : 0;
      if (horizontal) { 
        nscoord thirdHeight = RoundFloatToPixel(0.333333f * (float)aBorder.height, twipsPerPixel);

        
        nsRect topRect(aBorder.x, aBorder.y, aBorder.width, thirdHeight);
        if (NS_SIDE_TOP == aStartBevelSide) {
          topRect.x += aStartBevelOffset - startBevel;
          topRect.width -= aStartBevelOffset - startBevel;
        }
        if (NS_SIDE_TOP == aEndBevelSide) {
          topRect.width -= aEndBevelOffset - endBevel;
        }
        DrawSolidBorderSegment(aContext, topRect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);

        
        nscoord heightOffset = aBorder.height - thirdHeight; 
        nsRect bottomRect(aBorder.x, aBorder.y + heightOffset, aBorder.width, aBorder.height - heightOffset);
        if (NS_SIDE_BOTTOM == aStartBevelSide) {
          bottomRect.x += aStartBevelOffset - startBevel;
          bottomRect.width -= aStartBevelOffset - startBevel;
        }
        if (NS_SIDE_BOTTOM == aEndBevelSide) {
          bottomRect.width -= aEndBevelOffset - endBevel;
        }
        DrawSolidBorderSegment(aContext, bottomRect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }
      else { 
        nscoord thirdWidth = RoundFloatToPixel(0.333333f * (float)aBorder.width, twipsPerPixel);

        nsRect leftRect(aBorder.x, aBorder.y, thirdWidth, aBorder.height); 
        if (NS_SIDE_LEFT == aStartBevelSide) {
          leftRect.y += aStartBevelOffset - startBevel;
          leftRect.height -= aStartBevelOffset - startBevel;
        }
        if (NS_SIDE_LEFT == aEndBevelSide) {
          leftRect.height -= aEndBevelOffset - endBevel;
        }
        DrawSolidBorderSegment(aContext, leftRect, twipsPerPixel, aStartBevelSide,
                               startBevel, aEndBevelSide, endBevel);

        nscoord widthOffset = aBorder.width - thirdWidth; 
        nsRect rightRect(aBorder.x + widthOffset, aBorder.y, aBorder.width - widthOffset, aBorder.height);
        if (NS_SIDE_RIGHT == aStartBevelSide) {
          rightRect.y += aStartBevelOffset - startBevel;
          rightRect.height -= aStartBevelOffset - startBevel;
        }
        if (NS_SIDE_RIGHT == aEndBevelSide) {
          rightRect.height -= aEndBevelOffset - endBevel;
        }
        DrawSolidBorderSegment(aContext, rightRect, twipsPerPixel, aStartBevelSide,
                               startBevel, aEndBevelSide, endBevel);
      }
      break;
    }
    
  case NS_STYLE_BORDER_STYLE_SOLID:
    DrawSolidBorderSegment(aContext, aBorder, twipsPerPixel, aStartBevelSide, 
                           aStartBevelOffset, aEndBevelSide, aEndBevelOffset);
    break;
  case NS_STYLE_BORDER_STYLE_OUTSET:
  case NS_STYLE_BORDER_STYLE_INSET:
    NS_ASSERTION(PR_FALSE, "inset, outset should have been converted to groove, ridge");
    break;
  case NS_STYLE_BORDER_STYLE_AUTO:
    NS_ASSERTION(PR_FALSE, "Unexpected 'auto' table border");
    break;
  }

#ifdef MOZ_CAIRO_GFX
  ctx->SetAntialiasMode(oldMode);
#endif
}



