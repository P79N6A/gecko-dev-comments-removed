










































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

#ifdef MOZ_CAIRO_GFX
#include "gfxContext.h"
#endif

#define BORDER_FULL    0        //entire side
#define BORDER_INSIDE  1        //inside half
#define BORDER_OUTSIDE 2        //outside half


#define DOT_LENGTH  1           //square
#define DASH_LENGTH 3           //3 times longer than dot



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








PRIntn nsCSSRendering::MakeSide(nsPoint aPoints[],
                                nsIRenderingContext& aContext,
                                PRIntn aWhichSide,
                                const nsRect& aOutside, const nsRect& aInside,
                                PRIntn aSkipSides,
                                PRIntn aBorderPart, float aBorderFrac,
                                nscoord aTwipsPerPixel)
{
  nscoord outsideEdge, insideEdge, outsideTL, insideTL, outsideBR, insideBR;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  switch (aWhichSide) {
  case NS_SIDE_TOP:
    
    outsideEdge = aOutside.y;
    insideEdge = aInside.y;
    outsideTL = aOutside.x;
    insideTL = aInside.x;
    insideBR = aInside.XMost();
    outsideBR = aOutside.XMost();
    break;

  case NS_SIDE_BOTTOM:
    
    outsideEdge = aOutside.YMost();
    insideEdge = aInside.YMost();
    outsideTL = aOutside.x;
    insideTL = aInside.x;
    insideBR = aInside.XMost();
    outsideBR = aOutside.XMost();
    break;

  case NS_SIDE_LEFT:
    
    outsideEdge = aOutside.x;
    insideEdge = aInside.x;
    outsideTL = aOutside.y;
    insideTL = aInside.y;
    insideBR = aInside.YMost();
    outsideBR = aOutside.YMost();
    break;

  default:
    NS_ASSERTION(aWhichSide == NS_SIDE_RIGHT, "aWhichSide is not a valid side");
    
    outsideEdge = aOutside.XMost();
    insideEdge = aInside.XMost();
    outsideTL = aOutside.y;
    insideTL = aInside.y;
    insideBR = aInside.YMost();
    outsideBR = aOutside.YMost();
    break;
  }

  

  if ( (aWhichSide == NS_SIDE_TOP) || (aWhichSide == NS_SIDE_BOTTOM) ) {
    
    if ((1<<NS_SIDE_LEFT) & aSkipSides) {
      insideTL = outsideTL;
    }
    if ((1<<NS_SIDE_RIGHT) & aSkipSides) {
      insideBR = outsideBR;
    }
  } else {
    
    if ((1<<NS_SIDE_TOP) & aSkipSides) {
      insideTL = outsideTL;
    }
    if ((1<<NS_SIDE_BOTTOM) & aSkipSides) {
      insideBR = outsideBR;
    }
  }

  nscoord fullThickness;
  if (aWhichSide == NS_SIDE_TOP || aWhichSide == NS_SIDE_LEFT)
    fullThickness = insideEdge - outsideEdge;
  else
    fullThickness = outsideEdge - insideEdge;
  if (fullThickness != 0)
    fullThickness = NS_MAX(fullThickness, aTwipsPerPixel);

  nscoord thickness = fullThickness;
  if (aBorderFrac != 1.0f && fullThickness != 0) {
    thickness = aTwipsPerPixel *
      NS_MAX(NSToCoordRound(fullThickness * aBorderFrac / aTwipsPerPixel), 1);
    if ((aWhichSide == NS_SIDE_TOP) || (aWhichSide == NS_SIDE_LEFT)) {
      if (aBorderPart == BORDER_INSIDE)
        outsideEdge = insideEdge - thickness;
      else if (aBorderPart == BORDER_OUTSIDE)
        insideEdge = outsideEdge + thickness;
    } else {
      if (aBorderPart == BORDER_INSIDE)
        outsideEdge = insideEdge + thickness;
      else if (aBorderPart == BORDER_OUTSIDE)
        insideEdge = outsideEdge - thickness;
    }

    float actualFrac = (float)thickness / (float)fullThickness;
    if (aBorderPart == BORDER_INSIDE) {
      outsideTL = insideTL +
        ACTUAL_THICKNESS(outsideTL, insideTL, actualFrac, aTwipsPerPixel);
      outsideBR = insideBR +
        ACTUAL_THICKNESS(outsideBR, insideBR, actualFrac, aTwipsPerPixel);
    } else if (aBorderPart == BORDER_OUTSIDE) {
      insideTL = outsideTL -
        ACTUAL_THICKNESS(outsideTL, insideTL, actualFrac, aTwipsPerPixel);
      insideBR = outsideBR -
        ACTUAL_THICKNESS(outsideBR, insideBR, actualFrac, aTwipsPerPixel);
    }
  }

  
  aTwipsPerPixel += aTwipsPerPixel >> 2;

  
  
  
  if ( !((thickness >= aTwipsPerPixel) || (aBorderPart != BORDER_FULL)) &&
       ((aWhichSide == NS_SIDE_BOTTOM) || (aWhichSide == NS_SIDE_RIGHT))) {
    outsideEdge = insideEdge;
    }

  
  PRIntn np = 0;
  if ((aWhichSide == NS_SIDE_TOP) || (aWhichSide == NS_SIDE_BOTTOM)) {
    
    aPoints[np++].MoveTo(outsideTL,outsideEdge);
    aPoints[np++].MoveTo(outsideBR,outsideEdge);
    
    
    
    
    
    
    if ((thickness >= aTwipsPerPixel) || (aBorderPart != BORDER_FULL)) {
      aPoints[np++].MoveTo(insideBR,insideEdge);
      aPoints[np++].MoveTo(insideTL,insideEdge);
    }
  } else {
    
    
    if ((thickness >= aTwipsPerPixel) || (aBorderPart != BORDER_FULL))  {
      aPoints[np++].MoveTo(insideEdge,insideBR);
      aPoints[np++].MoveTo(insideEdge,insideTL);
    }
    aPoints[np++].MoveTo(outsideEdge,outsideTL);
    aPoints[np++].MoveTo(outsideEdge,outsideBR);
  }
  return np;
}

void nsCSSRendering::DrawSide(nsIRenderingContext& aContext,
                              PRIntn whichSide,
                              const PRUint8 borderStyle,  
                              const nscolor borderColor,
                              const nscolor aBackgroundColor,
                              const nsRect& borderOutside,
                              const nsRect& borderInside,
                              PRIntn aSkipSides,
                              nscoord twipsPerPixel,
                              nsRect* aGap)
{
  nsPoint theSide[MAX_POLY_POINTS];
  nscolor theColor = borderColor; 
  PRUint8 theStyle = borderStyle; 
  PRInt32 np;
  switch (theStyle) {
  case NS_STYLE_BORDER_STYLE_NONE:
  case NS_STYLE_BORDER_STYLE_HIDDEN:
    return;

  case NS_STYLE_BORDER_STYLE_DOTTED:    
  case NS_STYLE_BORDER_STYLE_DASHED:    
    break; 

  case NS_STYLE_BORDER_STYLE_GROOVE:
  case NS_STYLE_BORDER_STYLE_RIDGE:
    np = MakeSide (theSide, aContext, whichSide, borderOutside, borderInside, aSkipSides,
                   BORDER_INSIDE, 0.5f, twipsPerPixel);
    aContext.SetColor ( MakeBevelColor (whichSide, 
                                        ((theStyle == NS_STYLE_BORDER_STYLE_RIDGE) ?
                                         NS_STYLE_BORDER_STYLE_GROOVE :
                                         NS_STYLE_BORDER_STYLE_RIDGE), 
                                         aBackgroundColor, theColor));
    if (2 == np) {
      
      DrawLine (aContext, theSide[0].x, theSide[0].y, theSide[1].x, theSide[1].y, aGap);
    } else {
      
      FillPolygon (aContext, theSide, np, aGap);
    }
    np = MakeSide (theSide, aContext, whichSide, borderOutside, borderInside,aSkipSides,
                   BORDER_OUTSIDE, 0.5f, twipsPerPixel);
    aContext.SetColor ( MakeBevelColor (whichSide, theStyle, aBackgroundColor, 
                                        theColor));
    if (2 == np) {
      
      DrawLine (aContext, theSide[0].x, theSide[0].y, theSide[1].x, theSide[1].y, aGap);
    } else {
      
      FillPolygon (aContext, theSide, np, aGap);
    }
    break;

  case NS_STYLE_BORDER_STYLE_AUTO:
  case NS_STYLE_BORDER_STYLE_SOLID:
    np = MakeSide (theSide, aContext, whichSide, borderOutside, borderInside,aSkipSides,
                   BORDER_FULL, 1.0f, twipsPerPixel);
    aContext.SetColor (borderColor);  
    if (2 == np) {
      
      DrawLine (aContext, theSide[0].x, theSide[0].y, theSide[1].x, theSide[1].y, aGap);
    } else {
      
      FillPolygon (aContext, theSide, np, aGap);
    }
    break;

  case NS_STYLE_BORDER_STYLE_DOUBLE:
    np = MakeSide (theSide, aContext, whichSide, borderOutside, borderInside,aSkipSides,
                   BORDER_INSIDE, 0.333333f, twipsPerPixel);
    aContext.SetColor (borderColor);
    if (2 == np) {
      
      DrawLine (aContext, theSide[0].x, theSide[0].y, theSide[1].x, theSide[1].y, aGap);
    } else {
      
      FillPolygon (aContext, theSide, np, aGap);
   }
    np = MakeSide (theSide, aContext, whichSide, borderOutside, borderInside,aSkipSides,
                   BORDER_OUTSIDE, 0.333333f, twipsPerPixel);
    if (2 == np) {
      
      DrawLine (aContext, theSide[0].x, theSide[0].y, theSide[1].x, theSide[1].y, aGap);
    } else {
      
      FillPolygon (aContext, theSide, np, aGap);
    }
    break;

  case NS_STYLE_BORDER_STYLE_OUTSET:
  case NS_STYLE_BORDER_STYLE_INSET:
    np = MakeSide (theSide, aContext, whichSide, borderOutside, borderInside,aSkipSides,
                   BORDER_FULL, 1.0f, twipsPerPixel);
    aContext.SetColor ( MakeBevelColor (whichSide, theStyle, aBackgroundColor, 
                                        theColor));
    if (2 == np) {
      
      DrawLine (aContext, theSide[0].x, theSide[0].y, theSide[1].x, theSide[1].y, aGap);
    } else {
      
      FillPolygon (aContext, theSide, np, aGap);
    }
    break;
  }
}






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











void nsCSSRendering::DrawDashedSegments(nsIRenderingContext& aContext,
                                        const nsRect& aBounds,
                                        nsBorderEdges * aBorderEdges,
                                        PRIntn aSkipSides,
                        nsRect* aGap)
{
PRIntn dashLength;
nsRect dashRect, currRect;

PRBool  bSolid = PR_TRUE;
float   over = 0.0f;
PRBool  skippedSide = PR_FALSE;
PRIntn  whichSide=0;


  
  
  nsBorderEdge * segment =  (nsBorderEdge *)(aBorderEdges->mEdges[whichSide].ElementAt(0));
  PRUint8 style = segment->mStyle;  
  for ( ; whichSide < 4; whichSide++) 
  {
    if ((1<<whichSide) & aSkipSides) {
      
      skippedSide = PR_TRUE;
      continue;
    }
    nscoord x=0;  nscoord y=0;
    PRInt32 i;
    PRInt32 segmentCount = aBorderEdges->mEdges[whichSide].Count();
    nsBorderEdges * neighborBorderEdges=nsnull;
    PRIntn neighborEdgeCount=0; 
    for (i=0; i<segmentCount; i++)
    {
      bSolid=PR_TRUE;
      over = 0.0f;
      segment =  (nsBorderEdge *)(aBorderEdges->mEdges[whichSide].ElementAt(i));
      style = segment->mStyle;

      
      if (style == NS_STYLE_BORDER_STYLE_DASHED) {
        dashLength = DASH_LENGTH;
      } else {
        dashLength = DOT_LENGTH;
      }

      aContext.SetColor(segment->mColor);  
      switch (whichSide) {
      case NS_SIDE_LEFT:
      { 
        nsBorderEdge * topEdge =  (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(0));
        if (0==y)
        { 
          y = aBorderEdges->mMaxBorderWidth.top - topEdge->mWidth;
          if (PR_TRUE==aBorderEdges->mOutsideEdge)
            y += topEdge->mWidth;
        }
        
        x = aBounds.x + (aBorderEdges->mMaxBorderWidth.left - segment->mWidth);
        nscoord height = segment->mLength;
        
        nsRect borderOutside(x, y, aBounds.width, height);
        y += segment->mLength;  
        if ((style == NS_STYLE_BORDER_STYLE_DASHED) ||
            (style == NS_STYLE_BORDER_STYLE_DOTTED))
        {
          nsRect borderInside(borderOutside);
          nsMargin outsideMargin(segment->mWidth, 0, 0, 0);
          borderInside.Deflate(outsideMargin);
          nscoord totalLength = segment->mLength; 
          
          
          if (PR_TRUE==aBorderEdges->mOutsideEdge)
          {
            if (segment->mInsideNeighbor == neighborBorderEdges)
            {
              neighborEdgeCount++;
            }
            else 
            {
              neighborBorderEdges = segment->mInsideNeighbor;
              neighborEdgeCount=0;
            }
            nsBorderEdge * neighborLeft = (nsBorderEdge *)(segment->mInsideNeighbor->mEdges[NS_SIDE_LEFT].ElementAt(neighborEdgeCount));
            totalLength = neighborLeft->mLength;
          }
          dashRect.width = borderInside.x - borderOutside.x;
          dashRect.height = nscoord(dashRect.width * dashLength);
          dashRect.x = borderOutside.x;
          dashRect.y = borderOutside.y + (totalLength/2) - dashRect.height;
          if ((PR_TRUE==aBorderEdges->mOutsideEdge) && (0!=i))
            dashRect.y -= topEdge->mWidth;  
          if (0)
          {
            printf("  L: totalLength = %d, borderOutside.y = %d, midpoint %d, dashRect.y = %d\n", 
            totalLength, borderOutside.y, borderOutside.y +(totalLength/2), dashRect.y); 
          }
          currRect = dashRect;

          
          

          
          while (currRect.YMost() > borderInside.y) {
            
            if (currRect.y < borderInside.y) {
              over = float(borderInside.y - dashRect.y) /
                float(dashRect.height);
              currRect.height = currRect.height - (borderInside.y - currRect.y);
              currRect.y = borderInside.y;
            }

            
            if (0)
            {
              printf("DASHED LEFT: xywh in loop currRect = %d %d %d %d %s\n", 
                   currRect.x, currRect.y, currRect.width, currRect.height, bSolid?"TRUE":"FALSE");
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

          
          dashRect.y = borderOutside.y + (totalLength/2) + dashRect.height;
          if ((PR_TRUE==aBorderEdges->mOutsideEdge) && (0!=i))
            dashRect.y -= topEdge->mWidth;
          currRect = dashRect;
          bSolid=PR_TRUE;
          over = 0.0f;
          while (currRect.YMost() < borderInside.YMost()) {
            
            if (currRect.y < borderInside.y) {
              over = float(borderInside.y - dashRect.y) /
                float(dashRect.height);
              currRect.height = currRect.height - (borderInside.y - currRect.y);
              currRect.y = borderInside.y;
            }

            
            if (0)
            {
              printf("DASHED LEFT: xywh in loop currRect = %d %d %d %d %s\n", 
                   currRect.x, currRect.y, currRect.width, currRect.height, bSolid?"TRUE":"FALSE");
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
        }
      }
      break;

      case NS_SIDE_TOP:
      { 
        if (0==x)
        {
          nsBorderEdge * leftEdge =  (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(0));
          x = aBorderEdges->mMaxBorderWidth.left - leftEdge->mWidth;
        }
        y = aBounds.y;
        if (PR_TRUE==aBorderEdges->mOutsideEdge) 
          y += aBorderEdges->mMaxBorderWidth.top - segment->mWidth;
        nsRect borderOutside(x, y, segment->mLength, aBounds.height);
        x += segment->mLength;
        if ((style == NS_STYLE_BORDER_STYLE_DASHED) ||
            (style == NS_STYLE_BORDER_STYLE_DOTTED))
        {
          nsRect borderInside(borderOutside);
          nsBorderEdge * neighbor;
          
          
          
          if (PR_TRUE==aBorderEdges->mOutsideEdge && segment->mInsideNeighbor)
            neighbor = (nsBorderEdge *)(segment->mInsideNeighbor->mEdges[NS_SIDE_LEFT].ElementAt(0));
          else
            neighbor = (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(0));
          nsMargin outsideMargin(neighbor->mWidth, segment->mWidth, 0, segment->mWidth);
          borderInside.Deflate(outsideMargin);
          nscoord firstRectWidth = 0;
          if (PR_TRUE==aBorderEdges->mOutsideEdge && 0==i)
          {
            firstRectWidth = borderInside.x - borderOutside.x;
            aContext.FillRect(borderOutside.x, borderOutside.y,
                              firstRectWidth,
                              borderInside.y - borderOutside.y);
          }

          dashRect.height = borderInside.y - borderOutside.y;
          dashRect.width = dashRect.height * dashLength;
          dashRect.x = borderOutside.x + firstRectWidth;
          dashRect.y = borderOutside.y;
          currRect = dashRect;

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
        }
      }
      break;

      case NS_SIDE_RIGHT:
      { 
        nsBorderEdge * topEdge =  (nsBorderEdge *)
            (aBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(aBorderEdges->mEdges[NS_SIDE_TOP].Count()-1));
        if (0==y)
        {
          y = aBorderEdges->mMaxBorderWidth.top - topEdge->mWidth;
          if (PR_TRUE==aBorderEdges->mOutsideEdge)
            y += topEdge->mWidth;
        }
        nscoord width;
        if (PR_TRUE==aBorderEdges->mOutsideEdge)
        {
          width = aBounds.width - aBorderEdges->mMaxBorderWidth.right;
          width += segment->mWidth;
        }
        else
        {
          width = aBounds.width;
        }
        nscoord height = segment->mLength;
        nsRect borderOutside(aBounds.x, y, width, height);
        y += segment->mLength;
        if ((style == NS_STYLE_BORDER_STYLE_DASHED) ||
            (style == NS_STYLE_BORDER_STYLE_DOTTED))
        {
          nsRect borderInside(borderOutside);
          nsMargin outsideMargin(segment->mWidth, 0, (segment->mWidth), 0);
          borderInside.Deflate(outsideMargin);
          nscoord totalLength = segment->mLength;
          if (PR_TRUE==aBorderEdges->mOutsideEdge)
          {
            if (segment->mInsideNeighbor == neighborBorderEdges)
            {
              neighborEdgeCount++;
            }
            else 
            {
              neighborBorderEdges = segment->mInsideNeighbor;
              neighborEdgeCount=0;
            }
            nsBorderEdge * neighborRight = (nsBorderEdge *)(segment->mInsideNeighbor->mEdges[NS_SIDE_RIGHT].ElementAt(neighborEdgeCount));
            totalLength = neighborRight->mLength;
          }
          dashRect.width = borderOutside.XMost() - borderInside.XMost();
          dashRect.height = nscoord(dashRect.width * dashLength);
          dashRect.x = borderInside.XMost();
          dashRect.y = borderOutside.y + (totalLength/2) - dashRect.height;
          if ((PR_TRUE==aBorderEdges->mOutsideEdge) && (0!=i))
            dashRect.y -= topEdge->mWidth;
          currRect = dashRect;

          
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

          
          dashRect.y = borderOutside.y + (totalLength/2) + dashRect.height;
          if ((PR_TRUE==aBorderEdges->mOutsideEdge) && (0!=i))
            dashRect.y -= topEdge->mWidth;
          currRect = dashRect;
          bSolid=PR_TRUE;
          over = 0.0f;
          while (currRect.YMost() < borderInside.YMost()) {
            
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
            dashRect.y = dashRect.y + currRect.height;
            currRect = dashRect;
          }

        }
      }
      break;

      case NS_SIDE_BOTTOM:
      {  
        if (0==x)
        {
          nsBorderEdge * leftEdge =  (nsBorderEdge *)
            (aBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(aBorderEdges->mEdges[NS_SIDE_LEFT].Count()-1));
          x = aBorderEdges->mMaxBorderWidth.left - leftEdge->mWidth;
        }
        y = aBounds.y;
        if (PR_TRUE==aBorderEdges->mOutsideEdge) 
          y -= aBorderEdges->mMaxBorderWidth.bottom - segment->mWidth;
        nsRect borderOutside(x, y, segment->mLength, aBounds.height);
        x += segment->mLength;
        if ((style == NS_STYLE_BORDER_STYLE_DASHED) ||
            (style == NS_STYLE_BORDER_STYLE_DOTTED))
        {
          nsRect borderInside(borderOutside);
          nsBorderEdge * neighbor;
          if (PR_TRUE==aBorderEdges->mOutsideEdge)
            neighbor = (nsBorderEdge *)(segment->mInsideNeighbor->mEdges[NS_SIDE_LEFT].ElementAt(0));
          else
            neighbor = (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(0));
          nsMargin outsideMargin(neighbor->mWidth, segment->mWidth, 0, segment->mWidth);
          borderInside.Deflate(outsideMargin);
          nscoord firstRectWidth = 0;
          if (PR_TRUE==aBorderEdges->mOutsideEdge  &&  0==i)
          {
            firstRectWidth = borderInside.x - borderOutside.x;
            aContext.FillRect(borderOutside.x, borderInside.YMost(),
                              firstRectWidth,
                              borderOutside.YMost() - borderInside.YMost());
          }

          dashRect.height = borderOutside.YMost() - borderInside.YMost();
          dashRect.width = nscoord(dashRect.height * dashLength);
          dashRect.x = borderOutside.x + firstRectWidth;
          dashRect.y = borderInside.YMost();
          currRect = dashRect;

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


void nsCSSRendering::PaintBorder(nsPresContext* aPresContext,
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
  PRIntn              cnt;
  nsMargin            border;
  nsStyleCoord        bordStyleRadius[4];
  PRInt16             borderRadii[4],i;
  float               percent;
  nsCompatibility     compatMode = aPresContext->CompatibilityMode();
  PRBool              forceSolid;

  
  
  
  const nsStyleDisplay* displayData = aStyleContext->GetStyleDisplay();
  if (displayData->mAppearance) {
    nsITheme *theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame, displayData->mAppearance))
      return; 
  }
  
  const nsStyleColor* ourColor = aStyleContext->GetStyleColor();

  
  
  const nsStyleBackground* bgColor = 
    nsCSSRendering::FindNonTransparentBackground(aStyleContext, 
                                            compatMode == eCompatibility_NavQuirks ? PR_TRUE : PR_FALSE); 

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

  for(i=0;i<4;i++) {
    borderRadii[i] = 0;
    switch ( bordStyleRadius[i].GetUnit()) {
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
  }

  
  
  for(i=0;i<4;i++){
    if(borderRadii[i] > 0 && !aBorderStyle.mBorderColors){
      PaintRoundedBorder(aPresContext,aRenderingContext,aForFrame,aDirtyRect,aBorderArea,&aBorderStyle,nsnull,aStyleContext,aSkipSides,borderRadii,aGap,PR_FALSE);
      return;
    }
  }

  
  if (0 == border.top) aSkipSides |= (1 << NS_SIDE_TOP);
  if (0 == border.right) aSkipSides |= (1 << NS_SIDE_RIGHT);
  if (0 == border.bottom) aSkipSides |= (1 << NS_SIDE_BOTTOM);
  if (0 == border.left) aSkipSides |= (1 << NS_SIDE_LEFT);

  
  nsRect outerRect(aBorderArea);
  nsRect innerRect(outerRect);
  innerRect.Deflate(border);

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
 
  
  for (cnt = 0; cnt < 4; cnt++) {
    if ((aBorderStyle.GetBorderStyle(cnt) == NS_STYLE_BORDER_STYLE_DOTTED) || 
        (aBorderStyle.GetBorderStyle(cnt) == NS_STYLE_BORDER_STYLE_DASHED))  {
      break;
    }
  }
  if (cnt < 4) {
    DrawDashedSides(cnt, aRenderingContext,aDirtyRect, ourColor, &aBorderStyle,nsnull, PR_FALSE,
                    outerRect, innerRect, aSkipSides, aGap);
  }

  
  
  nsRect compositeInnerRect(innerRect);
  nsRect compositeOuterRect(outerRect);

  
  if (!aDirtyRect.Contains(outerRect)) {
    
    if (innerRect.y < aDirtyRect.y) {
      aSkipSides |= (1 << NS_SIDE_TOP);
      PRUint32 shortenBy =
        PR_MIN(innerRect.height, aDirtyRect.y - innerRect.y);
      innerRect.y += shortenBy;
      innerRect.height -= shortenBy;
      outerRect.y += shortenBy;
      outerRect.height -= shortenBy;
    }
    if (aDirtyRect.YMost() < innerRect.YMost()) {
      aSkipSides |= (1 << NS_SIDE_BOTTOM);
      PRUint32 shortenBy =
        PR_MIN(innerRect.height, innerRect.YMost() - aDirtyRect.YMost());
      innerRect.height -= shortenBy;
      outerRect.height -= shortenBy;
    }
    if (innerRect.x < aDirtyRect.x) {
      aSkipSides |= (1 << NS_SIDE_LEFT);
      PRUint32 shortenBy =
        PR_MIN(innerRect.width, aDirtyRect.x - innerRect.x);
      innerRect.x += shortenBy;
      innerRect.width -= shortenBy;
      outerRect.x += shortenBy;
      outerRect.width -= shortenBy;
    }
    if (aDirtyRect.XMost() < innerRect.XMost()) {
      aSkipSides |= (1 << NS_SIDE_RIGHT);
      PRUint32 shortenBy =
        PR_MIN(innerRect.width, innerRect.XMost() - aDirtyRect.XMost());
      innerRect.width -= shortenBy;
      outerRect.width -= shortenBy;
    }
  }
  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  static PRUint8 sideOrder[] = { NS_SIDE_BOTTOM, NS_SIDE_LEFT, NS_SIDE_TOP, NS_SIDE_RIGHT };
  nscolor sideColor;
  nsBorderColors* compositeColors = nsnull;

#ifdef MOZ_CAIRO_GFX
  gfxContext *ctx = (gfxContext*) aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
  gfxContext::AntialiasMode oldMode = ctx->CurrentAntialiasMode();
  ctx->SetAntialiasMode(gfxContext::MODE_ALIASED);
#endif

  for (cnt = 0; cnt < 4; cnt++) {
    PRUint8 side = sideOrder[cnt];

    
    
    if (aBorderStyle.GetBorderStyle(side) == NS_STYLE_BORDER_STYLE_DOUBLE ||
        aBorderStyle.GetBorderStyle(side) == NS_STYLE_BORDER_STYLE_GROOVE ||
        aBorderStyle.GetBorderStyle(side) == NS_STYLE_BORDER_STYLE_RIDGE) {
      nscoord widths[] = { border.top, border.right, border.bottom, border.left };
      forceSolid = (widths[side]/twipsPerPixel < 2);
    } else 
      forceSolid = PR_FALSE;

    if (0 == (aSkipSides & (1<<side))) {
      if (GetBorderColor(ourColor, aBorderStyle, side, sideColor, &compositeColors)) {
        if (compositeColors)
          DrawCompositeSide(aRenderingContext, side, compositeColors, compositeOuterRect, 
                            compositeInnerRect, borderRadii, twipsPerPixel, aGap);
        else
          DrawSide(aRenderingContext, side,
                   forceSolid ? NS_STYLE_BORDER_STYLE_SOLID : aBorderStyle.GetBorderStyle(side),
                   sideColor,
                   bgColor->mBackgroundColor,
                   outerRect,innerRect, aSkipSides,
                   twipsPerPixel, aGap);
      }
    }
  }

#ifdef MOZ_CAIRO_GFX
  ctx->SetAntialiasMode(oldMode);
#endif
}

void nsCSSRendering::DrawCompositeSide(nsIRenderingContext& aRenderingContext,
                                       PRIntn aWhichSide,
                                       nsBorderColors* aCompositeColors,
                                       const nsRect& aOuterRect,
                                       const nsRect& aInnerRect,
                                       PRInt16* aBorderRadii,
                                       nscoord twipsPerPixel,
                                       nsRect* aGap)

{
  
  
  nsRect currOuterRect(aOuterRect);

  
  
  
  
  
  
  

  
  
  nscoord shrinkage, startRadius, endRadius;
  if (aWhichSide == NS_SIDE_TOP) {
    shrinkage = aInnerRect.y - aOuterRect.y;
    startRadius = aBorderRadii[0];
    endRadius = aBorderRadii[1];
  } else if (aWhichSide == NS_SIDE_BOTTOM) {
    shrinkage = (aOuterRect.height+aOuterRect.y) - (aInnerRect.height+aInnerRect.y);
    startRadius = aBorderRadii[3];
    endRadius = aBorderRadii[2];
  } else if (aWhichSide == NS_SIDE_RIGHT) {
    shrinkage = (aOuterRect.width+aOuterRect.x) - (aInnerRect.width+aInnerRect.x);
    startRadius = aBorderRadii[1];
    endRadius = aBorderRadii[2];
  } else {
    NS_ASSERTION(aWhichSide == NS_SIDE_LEFT, "incorrect aWhichSide");
    shrinkage = aInnerRect.x - aOuterRect.x;
    startRadius = aBorderRadii[0];
    endRadius = aBorderRadii[3];
  }

  while (shrinkage > 0) {
    nscoord xshrink = 0;
    nscoord yshrink = 0;
    nscoord widthshrink = 0;
    nscoord heightshrink = 0;

    if (startRadius || endRadius) {
      if (aWhichSide == NS_SIDE_TOP || aWhichSide == NS_SIDE_BOTTOM) {
        xshrink = startRadius;
        widthshrink = startRadius + endRadius;
      }
      else if (aWhichSide == NS_SIDE_LEFT || aWhichSide == NS_SIDE_RIGHT) {
        yshrink = startRadius-1;
        heightshrink = yshrink + endRadius;
      }
    }

    
    nsRect newOuterRect(currOuterRect);
    newOuterRect.x += xshrink;
    newOuterRect.y += yshrink;
    newOuterRect.width -= widthshrink;
    newOuterRect.height -= heightshrink;

    nsRect borderInside(currOuterRect);
    
    
    
    if (aInnerRect.x > borderInside.x) { 
      borderInside.x += twipsPerPixel;
      borderInside.width -= twipsPerPixel;
    }
    if (borderInside.x+borderInside.width > aInnerRect.x+aInnerRect.width) 
      borderInside.width -= twipsPerPixel;
    
    if (aInnerRect.y > borderInside.y) { 
      borderInside.y += twipsPerPixel;
      borderInside.height -= twipsPerPixel;
    }
    if (borderInside.y+borderInside.height > aInnerRect.y+aInnerRect.height) 
      borderInside.height -= twipsPerPixel;

    if (!aCompositeColors->mTransparent) {
      nsPoint theSide[MAX_POLY_POINTS];
      PRInt32 np = MakeSide(theSide, aRenderingContext, aWhichSide, newOuterRect, borderInside, 0,
                            BORDER_FULL, 1.0f, twipsPerPixel);
      NS_ASSERTION(np == 2, "Composite border should always be single pixel!");
      aRenderingContext.SetColor(aCompositeColors->mColor);
      DrawLine(aRenderingContext, theSide[0].x, theSide[0].y, theSide[1].x, theSide[1].y, aGap);
    
      if (aWhichSide == NS_SIDE_TOP) {
        if (startRadius) {
          
          nscoord distance = (startRadius+twipsPerPixel)/2;
          nscoord remainder = distance%twipsPerPixel;
          if (remainder) 
            distance += twipsPerPixel - remainder;
          DrawLine(aRenderingContext,
                   currOuterRect.x+startRadius,
                   currOuterRect.y, 
                   currOuterRect.x+startRadius-distance,
                   currOuterRect.y+distance,
                   aGap);
        }
        if (endRadius) {
          
          nscoord distance = (endRadius+twipsPerPixel)/2;
          nscoord remainder = distance%twipsPerPixel;
          if (remainder) 
            distance += twipsPerPixel - remainder;
          DrawLine(aRenderingContext,
                   currOuterRect.x+currOuterRect.width-endRadius-twipsPerPixel,
                   currOuterRect.y, 
                   currOuterRect.x+currOuterRect.width-endRadius-twipsPerPixel+distance,
                   currOuterRect.y+distance,
                   aGap);
        }
      }
      else if (aWhichSide == NS_SIDE_BOTTOM) {
        if (startRadius) {
          
          nscoord distance = (startRadius+twipsPerPixel)/2;
          nscoord remainder = distance%twipsPerPixel;
          if (remainder) 
            distance += twipsPerPixel - remainder;
          DrawLine(aRenderingContext,
                   currOuterRect.x+startRadius, 
                   currOuterRect.y+currOuterRect.height-twipsPerPixel,
                   currOuterRect.x+startRadius-distance, 
                   currOuterRect.y+currOuterRect.height-twipsPerPixel-distance,
                   aGap);
        }
        if (endRadius) {
          
          nscoord distance = (endRadius+twipsPerPixel)/2;
          nscoord remainder = distance%twipsPerPixel;
          if (remainder) 
            distance += twipsPerPixel - remainder;
          DrawLine(aRenderingContext,
                   currOuterRect.x+currOuterRect.width-endRadius-twipsPerPixel, 
                   currOuterRect.y+currOuterRect.height-twipsPerPixel, 
                   currOuterRect.x+currOuterRect.width-endRadius-twipsPerPixel+distance, 
                   currOuterRect.y+currOuterRect.height-twipsPerPixel-distance,
                   aGap);
        }
      }
      else if (aWhichSide == NS_SIDE_LEFT) {
        if (startRadius) {
          
          nscoord distance = (startRadius-twipsPerPixel)/2;
          nscoord remainder = distance%twipsPerPixel;
          if (remainder)
            distance -= remainder;
          DrawLine(aRenderingContext,
                   currOuterRect.x+distance,
                   currOuterRect.y+startRadius-distance, 
                   currOuterRect.x,
                   currOuterRect.y+startRadius,
                   aGap);
        }
        if (endRadius) {
          
          nscoord distance = (endRadius-twipsPerPixel)/2;
          nscoord remainder = distance%twipsPerPixel;
          if (remainder)
            distance -= remainder;
          DrawLine(aRenderingContext,
                   currOuterRect.x+distance,
                   currOuterRect.y+currOuterRect.height-twipsPerPixel-endRadius+distance,
                   currOuterRect.x,
                   currOuterRect.y+currOuterRect.height-twipsPerPixel-endRadius,
                   aGap);
        }
      }
      else if (aWhichSide == NS_SIDE_RIGHT) {
       if (startRadius) {
          
          nscoord distance = (startRadius-twipsPerPixel)/2;
          nscoord remainder = distance%twipsPerPixel;
          if (remainder)
            distance -= remainder;
          DrawLine(aRenderingContext,
                   currOuterRect.x+currOuterRect.width-twipsPerPixel-distance,
                   currOuterRect.y+startRadius-distance, 
                   currOuterRect.x+currOuterRect.width-twipsPerPixel,
                   currOuterRect.y+startRadius,
                   aGap);
        }
        if (endRadius) {
          
          nscoord distance = (endRadius-twipsPerPixel)/2;
          nscoord remainder = distance%twipsPerPixel;
          if (remainder)
            distance -= remainder;
          DrawLine(aRenderingContext,
                   currOuterRect.x+currOuterRect.width-twipsPerPixel-distance, 
                   currOuterRect.y+currOuterRect.height-twipsPerPixel-endRadius+distance,
                   currOuterRect.x+currOuterRect.width-twipsPerPixel, 
                   currOuterRect.y+currOuterRect.height-twipsPerPixel-endRadius,
                   aGap);
        }
      }
    }
    
    if (aCompositeColors->mNext)
      aCompositeColors = aCompositeColors->mNext;

    currOuterRect = borderInside;
    shrinkage -= twipsPerPixel;
    
    startRadius -= twipsPerPixel;
    if (startRadius < 0) startRadius = 0;
    endRadius -= twipsPerPixel;
    if (endRadius < 0) endRadius = 0;
  }
}


void nsCSSRendering::PaintOutline(nsPresContext* aPresContext,
                                 nsIRenderingContext& aRenderingContext,
                                 nsIFrame* aForFrame,
                                 const nsRect& aDirtyRect,
                                 const nsRect& aBorderArea,
                                 const nsStyleBorder& aBorderStyle,
                                 const nsStyleOutline& aOutlineStyle,
                                 nsStyleContext* aStyleContext,
                                 PRIntn aSkipSides,
                                 nsRect* aGap)
{
nsStyleCoord        bordStyleRadius[4];
PRInt16             borderRadii[4],i;
float               percent;
const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground(aStyleContext);
nscoord width, offset;

  
  const nsStyleColor* ourColor = aStyleContext->GetStyleColor();

  aOutlineStyle.GetOutlineWidth(width);

  if (0 == width) {
    
    return;
  }

  
  aOutlineStyle.mOutlineRadius.GetTop(bordStyleRadius[0]);      
  aOutlineStyle.mOutlineRadius.GetRight(bordStyleRadius[1]);    
  aOutlineStyle.mOutlineRadius.GetBottom(bordStyleRadius[2]);   
  aOutlineStyle.mOutlineRadius.GetLeft(bordStyleRadius[3]);     

  for(i=0;i<4;i++) {
    borderRadii[i] = 0;
    switch ( bordStyleRadius[i].GetUnit()) {
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

  
  for(i=0;i<4;i++){
    if(borderRadii[i] > 0){
      PaintRoundedBorder(aPresContext, aRenderingContext, aForFrame, aDirtyRect,
                         outside, nsnull, &aOutlineStyle, aStyleContext, 
                         aSkipSides, borderRadii, aGap, PR_TRUE);
      return;
    }
  }


  PRUint8 outlineStyle = aOutlineStyle.GetOutlineStyle();
  
  if ((outlineStyle == NS_STYLE_BORDER_STYLE_DOTTED) || 
      (outlineStyle == NS_STYLE_BORDER_STYLE_DASHED))  {
    DrawDashedSides(0, aRenderingContext, aDirtyRect, ourColor, nsnull, &aOutlineStyle, PR_TRUE,
                    outside, inside, aSkipSides, aGap);
    return;
  }

  

  PRInt32 appUnitsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  
  
  nscolor outlineColor(ourColor->mColor);
  PRBool  canDraw = PR_FALSE;
#ifdef GFX_HAS_INVERT
  PRBool  modeChanged=PR_FALSE;
#endif

  
  if (aOutlineStyle.GetOutlineInitialColor()) {
    canDraw = PR_TRUE;
#ifdef GFX_HAS_INVERT
    if( NS_SUCCEEDED(aRenderingContext.SetPenMode(nsPenMode_kInvert)) ) {
      modeChanged=PR_TRUE;
    }
#endif
  } else {
    canDraw = aOutlineStyle.GetOutlineColor(outlineColor);
  }

  if (PR_TRUE == canDraw) {
    DrawSide(aRenderingContext, NS_SIDE_BOTTOM,
             outlineStyle,
             outlineColor,
             bgColor->mBackgroundColor, outside, inside, aSkipSides,
             appUnitsPerPixel, aGap);

    DrawSide(aRenderingContext, NS_SIDE_LEFT,
             outlineStyle, 
             outlineColor,
             bgColor->mBackgroundColor,outside, inside,aSkipSides,
             appUnitsPerPixel, aGap);

    DrawSide(aRenderingContext, NS_SIDE_TOP,
             outlineStyle,
             outlineColor,
             bgColor->mBackgroundColor,outside, inside,aSkipSides,
             appUnitsPerPixel, aGap);

    DrawSide(aRenderingContext, NS_SIDE_RIGHT,
             outlineStyle,
             outlineColor,
             bgColor->mBackgroundColor,outside, inside,aSkipSides,
             appUnitsPerPixel, aGap);
#ifdef GFX_HAS_INVERT
    if(modeChanged ) {
      aRenderingContext.SetPenMode(nsPenMode_kNone);
    }
#endif
  }
}











void nsCSSRendering::PaintBorderEdges(nsPresContext* aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      nsIFrame* aForFrame,
                                      const nsRect& aDirtyRect,
                                      const nsRect& aBorderArea,
                                      nsBorderEdges * aBorderEdges,
                                      nsStyleContext* aStyleContext,
                                      PRIntn aSkipSides,
                                      nsRect* aGap)
{
  const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground(aStyleContext);
  
  if (nsnull==aBorderEdges) {  
    return;
  }

  
  if (0 == aBorderEdges->mMaxBorderWidth.top) 
    aSkipSides |= (1 << NS_SIDE_TOP);
  if (0 == aBorderEdges->mMaxBorderWidth.right) 
    aSkipSides |= (1 << NS_SIDE_RIGHT);
  if (0 == aBorderEdges->mMaxBorderWidth.bottom) 
    aSkipSides |= (1 << NS_SIDE_BOTTOM);
  if (0 == aBorderEdges->mMaxBorderWidth.left) 
    aSkipSides |= (1 << NS_SIDE_LEFT);

  
  DrawDashedSegments(aRenderingContext, aBorderArea, aBorderEdges, aSkipSides, aGap);

  
  nscoord appUnitsPerPixel = nsPresContext::AppUnitsPerCSSPixel();

  if (0 == (aSkipSides & (1<<NS_SIDE_TOP))) {
    PRInt32 segmentCount = aBorderEdges->mEdges[NS_SIDE_TOP].Count();
    PRInt32 i;
    nsBorderEdge * leftEdge =  (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(0));
    nscoord x = aBorderEdges->mMaxBorderWidth.left - leftEdge->mWidth;
    for (i=0; i<segmentCount; i++)
    {
      nsBorderEdge * borderEdge =  (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(i));
      nscoord y = aBorderArea.y;
      if (PR_TRUE==aBorderEdges->mOutsideEdge) 
        y += aBorderEdges->mMaxBorderWidth.top - borderEdge->mWidth;
      nsRect inside(x, y, borderEdge->mLength, aBorderArea.height);
      x += borderEdge->mLength;
      nsRect outside(inside);
      nsMargin outsideMargin(0, borderEdge->mWidth, 0, 0);
      outside.Deflate(outsideMargin);
      DrawSide(aRenderingContext, NS_SIDE_TOP,
               borderEdge->mStyle,
               borderEdge->mColor,
               bgColor->mBackgroundColor,
               inside, outside,aSkipSides,
               appUnitsPerPixel, aGap);
    }
  }
  if (0 == (aSkipSides & (1<<NS_SIDE_LEFT))) {
    PRInt32 segmentCount = aBorderEdges->mEdges[NS_SIDE_LEFT].Count();
    PRInt32 i;
    nsBorderEdge * topEdge =  (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(0));
    nscoord y = aBorderEdges->mMaxBorderWidth.top - topEdge->mWidth;
    for (i=0; i<segmentCount; i++)
    {
      nsBorderEdge * borderEdge =  (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(i));
      nscoord x = aBorderArea.x + (aBorderEdges->mMaxBorderWidth.left - borderEdge->mWidth);
      nsRect inside(x, y, aBorderArea.width, borderEdge->mLength);
      y += borderEdge->mLength;
      nsRect outside(inside);
      nsMargin outsideMargin(borderEdge->mWidth, 0, 0, 0);
      outside.Deflate(outsideMargin);
      DrawSide(aRenderingContext, NS_SIDE_LEFT,
               borderEdge->mStyle,
               borderEdge->mColor,
               bgColor->mBackgroundColor,
               inside, outside, aSkipSides,
               appUnitsPerPixel, aGap);
    }
  }
  if (0 == (aSkipSides & (1<<NS_SIDE_BOTTOM))) {
    PRInt32 segmentCount = aBorderEdges->mEdges[NS_SIDE_BOTTOM].Count();
    PRInt32 i;
    nsBorderEdge * leftEdge =  (nsBorderEdge *)
      (aBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(aBorderEdges->mEdges[NS_SIDE_LEFT].Count()-1));
    nscoord x = aBorderEdges->mMaxBorderWidth.left - leftEdge->mWidth;
    for (i=0; i<segmentCount; i++)
    {
      nsBorderEdge * borderEdge =  (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_BOTTOM].ElementAt(i));
      nscoord y = aBorderArea.y;
      if (PR_TRUE==aBorderEdges->mOutsideEdge) 
        y -= (aBorderEdges->mMaxBorderWidth.bottom - borderEdge->mWidth);
      nsRect inside(x, y, borderEdge->mLength, aBorderArea.height);
      x += borderEdge->mLength;
      nsRect outside(inside);
      nsMargin outsideMargin(0, 0, 0, borderEdge->mWidth);
      outside.Deflate(outsideMargin);
      DrawSide(aRenderingContext, NS_SIDE_BOTTOM,
               borderEdge->mStyle,
               borderEdge->mColor,
               bgColor->mBackgroundColor,
               inside, outside,aSkipSides,
               appUnitsPerPixel, aGap);
    }
  }
  if (0 == (aSkipSides & (1<<NS_SIDE_RIGHT))) {
    PRInt32 segmentCount = aBorderEdges->mEdges[NS_SIDE_RIGHT].Count();
    PRInt32 i;
    nsBorderEdge * topEdge =  (nsBorderEdge *)
      (aBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(aBorderEdges->mEdges[NS_SIDE_TOP].Count()-1));
    nscoord y = aBorderEdges->mMaxBorderWidth.top - topEdge->mWidth;
    for (i=0; i<segmentCount; i++)
    {
      nsBorderEdge * borderEdge =  (nsBorderEdge *)(aBorderEdges->mEdges[NS_SIDE_RIGHT].ElementAt(i));
      nscoord width;
      if (PR_TRUE==aBorderEdges->mOutsideEdge)
      {
        width = aBorderArea.width - aBorderEdges->mMaxBorderWidth.right;
        width += borderEdge->mWidth;
      }
      else
      {
        width = aBorderArea.width;
      }
      nsRect inside(aBorderArea.x, y, width, borderEdge->mLength);
      y += borderEdge->mLength;
      nsRect outside(inside);
      nsMargin outsideMargin(0, 0, (borderEdge->mWidth), 0);
      outside.Deflate(outsideMargin);
      DrawSide(aRenderingContext, NS_SIDE_RIGHT,
               borderEdge->mStyle,
               borderEdge->mColor,
               bgColor->mBackgroundColor,
               inside, outside,aSkipSides,
               appUnitsPerPixel, aGap);
    }
  }
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
              nsIFrame *bodyFrame = aForFrame->GetPresContext()->GetPresShell()->
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
nsCSSRendering::PaintRoundedBorder(nsPresContext* aPresContext,
                                 nsIRenderingContext& aRenderingContext,
                                 nsIFrame* aForFrame,
                                 const nsRect& aDirtyRect,
                                 const nsRect& aBorderArea,
                                 const nsStyleBorder* aBorderStyle,
                                 const nsStyleOutline* aOutlineStyle,
                                 nsStyleContext* aStyleContext,
                                 PRIntn aSkipSides,
                                 PRInt16 aBorderRadius[4],
                                 nsRect* aGap,
                                 PRBool aIsOutline)
{
  RoundedRect   outerPath;
  QBCurve       UL,LL,UR,LR;
  QBCurve       IUL,ILL,IUR,ILR;
  QBCurve       cr1,cr2,cr3,cr4;
  QBCurve       Icr1,Icr2,Icr3,Icr4;
  nsFloatPoint  thePath[MAXPATHSIZE];
  PRInt16       np;
  nsMargin      border;

  NS_ASSERTION((aIsOutline && aOutlineStyle) || (!aIsOutline && aBorderStyle), "null params not allowed");
  if (!aIsOutline) {
    border = aBorderStyle->GetBorder();
    if ((0 == border.left) && (0 == border.right) &&
        (0 == border.top) && (0 == border.bottom)) {
      return;
    }
  } else {
    nscoord width;
    if (!aOutlineStyle->GetOutlineWidth(width)) {
      return;
    }
    border.left   = width;
    border.right  = width;
    border.top    = width;
    border.bottom = width;
  }

  
  nscoord appUnitsPerPixel = aPresContext->DevPixelsToAppUnits(1);
  nscoord quarterPixel = appUnitsPerPixel / 4;

  outerPath.Set(aBorderArea.x, aBorderArea.y, aBorderArea.width,
                aBorderArea.height, aBorderRadius, appUnitsPerPixel);
  outerPath.GetRoundedBorders(UL,UR,LL,LR);
  outerPath.CalcInsetCurves(IUL,IUR,ILL,ILR,border);

  
  UL.MidPointDivide(&cr1,&cr2);
  UR.MidPointDivide(&cr3,&cr4);
  IUL.MidPointDivide(&Icr1,&Icr2);
  IUR.MidPointDivide(&Icr3,&Icr4);
  if(0!=border.top){
    np=0;
    thePath[np++].MoveTo(cr2.mAnc1.x,cr2.mAnc1.y);
    thePath[np++].MoveTo(cr2.mCon.x, cr2.mCon.y);
    thePath[np++].MoveTo(cr2.mAnc2.x, cr2.mAnc2.y);
    thePath[np++].MoveTo(cr3.mAnc1.x, cr3.mAnc1.y);
    thePath[np++].MoveTo(cr3.mCon.x, cr3.mCon.y);
    thePath[np++].MoveTo(cr3.mAnc2.x, cr3.mAnc2.y);
 
    thePath[np++].MoveTo(Icr3.mAnc2.x,Icr3.mAnc2.y);
    thePath[np++].MoveTo(Icr3.mCon.x, Icr3.mCon.y);
    thePath[np++].MoveTo(Icr3.mAnc1.x, Icr3.mAnc1.y);
    thePath[np++].MoveTo(Icr2.mAnc2.x, Icr2.mAnc2.y);
    thePath[np++].MoveTo(Icr2.mCon.x, Icr2.mCon.y);
    thePath[np++].MoveTo(Icr2.mAnc1.x, Icr2.mAnc1.y);
    RenderSide(thePath, aRenderingContext, aBorderStyle, aOutlineStyle,
               aStyleContext, NS_SIDE_TOP, border, quarterPixel, aIsOutline);
  }
  
  LR.MidPointDivide(&cr2,&cr3);
  ILR.MidPointDivide(&Icr2,&Icr3);
  if(0!=border.right){
    np=0;
    thePath[np++].MoveTo(cr4.mAnc1.x,cr4.mAnc1.y);
    thePath[np++].MoveTo(cr4.mCon.x, cr4.mCon.y);
    thePath[np++].MoveTo(cr4.mAnc2.x,cr4.mAnc2.y);
    thePath[np++].MoveTo(cr2.mAnc1.x,cr2.mAnc1.y);
    thePath[np++].MoveTo(cr2.mCon.x, cr2.mCon.y);
    thePath[np++].MoveTo(cr2.mAnc2.x,cr2.mAnc2.y);

    thePath[np++].MoveTo(Icr2.mAnc2.x,Icr2.mAnc2.y);
    thePath[np++].MoveTo(Icr2.mCon.x, Icr2.mCon.y);
    thePath[np++].MoveTo(Icr2.mAnc1.x,Icr2.mAnc1.y);
    thePath[np++].MoveTo(Icr4.mAnc2.x,Icr4.mAnc2.y);
    thePath[np++].MoveTo(Icr4.mCon.x, Icr4.mCon.y);
    thePath[np++].MoveTo(Icr4.mAnc1.x,Icr4.mAnc1.y);
    RenderSide(thePath, aRenderingContext, aBorderStyle, aOutlineStyle,
               aStyleContext, NS_SIDE_RIGHT, border, quarterPixel, aIsOutline);
  }

  
  LL.MidPointDivide(&cr2,&cr4);
  ILL.MidPointDivide(&Icr2,&Icr4);
  if(0!=border.bottom){
    np=0;
    thePath[np++].MoveTo(cr3.mAnc1.x,cr3.mAnc1.y);
    thePath[np++].MoveTo(cr3.mCon.x, cr3.mCon.y);
    thePath[np++].MoveTo(cr3.mAnc2.x, cr3.mAnc2.y);
    thePath[np++].MoveTo(cr2.mAnc1.x, cr2.mAnc1.y);
    thePath[np++].MoveTo(cr2.mCon.x, cr2.mCon.y);
    thePath[np++].MoveTo(cr2.mAnc2.x, cr2.mAnc2.y);

    thePath[np++].MoveTo(Icr2.mAnc2.x,Icr2.mAnc2.y);
    thePath[np++].MoveTo(Icr2.mCon.x, Icr2.mCon.y);
    thePath[np++].MoveTo(Icr2.mAnc1.x, Icr2.mAnc1.y);
    thePath[np++].MoveTo(Icr3.mAnc2.x, Icr3.mAnc2.y);
    thePath[np++].MoveTo(Icr3.mCon.x, Icr3.mCon.y);
    thePath[np++].MoveTo(Icr3.mAnc1.x, Icr3.mAnc1.y);
    RenderSide(thePath, aRenderingContext, aBorderStyle, aOutlineStyle,
               aStyleContext, NS_SIDE_BOTTOM, border, quarterPixel, aIsOutline);
  }
  
  if(0==border.left)
    return;
  np=0;
  thePath[np++].MoveTo(cr4.mAnc1.x,cr4.mAnc1.y);
  thePath[np++].MoveTo(cr4.mCon.x, cr4.mCon.y);
  thePath[np++].MoveTo(cr4.mAnc2.x, cr4.mAnc2.y);
  thePath[np++].MoveTo(cr1.mAnc1.x, cr1.mAnc1.y);
  thePath[np++].MoveTo(cr1.mCon.x, cr1.mCon.y);
  thePath[np++].MoveTo(cr1.mAnc2.x, cr1.mAnc2.y);


  thePath[np++].MoveTo(Icr1.mAnc2.x,Icr1.mAnc2.y);
  thePath[np++].MoveTo(Icr1.mCon.x, Icr1.mCon.y);
  thePath[np++].MoveTo(Icr1.mAnc1.x, Icr1.mAnc1.y);
  thePath[np++].MoveTo(Icr4.mAnc2.x, Icr4.mAnc2.y);
  thePath[np++].MoveTo(Icr4.mCon.x, Icr4.mCon.y);
  thePath[np++].MoveTo(Icr4.mAnc1.x, Icr4.mAnc1.y);

  RenderSide(thePath, aRenderingContext, aBorderStyle, aOutlineStyle,
             aStyleContext, NS_SIDE_LEFT, border, quarterPixel, aIsOutline);
}






void 
nsCSSRendering::RenderSide(nsFloatPoint aPoints[],nsIRenderingContext& aRenderingContext,
                        const nsStyleBorder* aBorderStyle,const nsStyleOutline* aOutlineStyle,nsStyleContext* aStyleContext,
                        PRUint8 aSide,nsMargin  &aBorThick,nscoord aTwipsPerPixel,
                        PRBool aIsOutline)
{
  QBCurve   thecurve;
  nscolor   sideColor = NS_RGB(0,0,0);
  static nsPoint   polypath[MAXPOLYPATHSIZE];
  PRInt32   curIndex,c1Index,c2Index,junk;
  PRInt8    border_Style;
  PRInt16   thickness;

  
  const nsStyleColor* ourColor = aStyleContext->GetStyleColor();

  NS_ASSERTION((aIsOutline && aOutlineStyle) || (!aIsOutline && aBorderStyle), "null params not allowed");
  
  if (!aIsOutline) {
    if (!GetBorderColor(ourColor, *aBorderStyle, aSide, sideColor)) {
      return;
    }
  } else {
    aOutlineStyle->GetOutlineColor(sideColor);
  }
  aRenderingContext.SetColor ( sideColor );

  thickness = 0;
  switch(aSide){
    case  NS_SIDE_LEFT:
      thickness = aBorThick.left;
      break;
    case  NS_SIDE_TOP:
      thickness = aBorThick.top;
      break;
    case  NS_SIDE_RIGHT:
      thickness = aBorThick.right;
      break;
    case  NS_SIDE_BOTTOM:
      thickness = aBorThick.bottom;
      break;
  }

  
  if (thickness<=aTwipsPerPixel) {
    
    thecurve.SetPoints(aPoints[0].x,aPoints[0].y,aPoints[1].x,aPoints[1].y,aPoints[2].x,aPoints[2].y);
    thecurve.SubDivide((nsIRenderingContext*)&aRenderingContext,nsnull,nsnull);
    aRenderingContext.DrawLine((nscoord)aPoints[2].x,(nscoord)aPoints[2].y,(nscoord)aPoints[3].x,(nscoord)aPoints[3].y);
    thecurve.SetPoints(aPoints[3].x,aPoints[3].y,aPoints[4].x,aPoints[4].y,aPoints[5].x,aPoints[5].y);
    thecurve.SubDivide((nsIRenderingContext*)&aRenderingContext,nsnull,nsnull);
  } else {
    
    if (!aIsOutline) {
      border_Style = aBorderStyle->GetBorderStyle(aSide);
    } else {
      border_Style = aOutlineStyle->GetOutlineStyle();
    }
    switch (border_Style){
      case NS_STYLE_BORDER_STYLE_OUTSET:
      case NS_STYLE_BORDER_STYLE_INSET:
        {
          const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground(aStyleContext);
          aRenderingContext.SetColor(MakeBevelColor(aSide, border_Style,
                                       bgColor->mBackgroundColor, sideColor));
        }
      case NS_STYLE_BORDER_STYLE_DOTTED:
      case NS_STYLE_BORDER_STYLE_DASHED:
        
        
        
        
      case NS_STYLE_BORDER_STYLE_AUTO:
      case NS_STYLE_BORDER_STYLE_SOLID:
        polypath[0].x = NSToCoordRound(aPoints[0].x);
        polypath[0].y = NSToCoordRound(aPoints[0].y);
        curIndex = 1;
        GetPath(aPoints,polypath,&curIndex,eOutside,c1Index);
        c2Index = curIndex;
        if (curIndex >= MAXPOLYPATHSIZE)
          return;
        polypath[curIndex].x = NSToCoordRound(aPoints[6].x);
        polypath[curIndex].y = NSToCoordRound(aPoints[6].y);
        curIndex++;
        GetPath(aPoints,polypath,&curIndex,eInside,junk);
        if (curIndex >= MAXPOLYPATHSIZE)
          return;
        polypath[curIndex].x = NSToCoordRound(aPoints[0].x);
        polypath[curIndex].y = NSToCoordRound(aPoints[0].y);
        curIndex++;
        aRenderingContext.FillPolygon(polypath,curIndex);

       break;
      case NS_STYLE_BORDER_STYLE_DOUBLE:
        polypath[0].x = NSToCoordRound(aPoints[0].x);
        polypath[0].y = NSToCoordRound(aPoints[0].y);
        curIndex = 1;
        GetPath(aPoints,polypath,&curIndex,eOutside,c1Index);
        aRenderingContext.DrawPolyline(polypath,curIndex);
        polypath[0].x = NSToCoordRound(aPoints[6].x);
        polypath[0].y = NSToCoordRound(aPoints[6].y);
        curIndex = 1;
        GetPath(aPoints,polypath,&curIndex,eInside,c1Index);
        aRenderingContext.DrawPolyline(polypath,curIndex);
        break;
      case NS_STYLE_BORDER_STYLE_NONE:
      case NS_STYLE_BORDER_STYLE_HIDDEN:
        break;
      case NS_STYLE_BORDER_STYLE_RIDGE:
      case NS_STYLE_BORDER_STYLE_GROOVE:
        {
        const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground(aStyleContext);
        aRenderingContext.SetColor(MakeBevelColor(aSide, border_Style,
                                     bgColor->mBackgroundColor,sideColor));

        polypath[0].x = NSToCoordRound(aPoints[0].x);
        polypath[0].y = NSToCoordRound(aPoints[0].y);
        curIndex = 1;
        GetPath(aPoints,polypath,&curIndex,eOutside,c1Index);
        if (curIndex >= MAXPOLYPATHSIZE)
          return;
        polypath[curIndex].x = NSToCoordRound((aPoints[5].x + aPoints[6].x)/2.0f);
        polypath[curIndex].y = NSToCoordRound((aPoints[5].y + aPoints[6].y)/2.0f);
        curIndex++;
        GetPath(aPoints,polypath,&curIndex,eCalcRev,c1Index,.5);
        if (curIndex >= MAXPOLYPATHSIZE)
          return;
        polypath[curIndex].x = NSToCoordRound(aPoints[0].x);
        polypath[curIndex].y = NSToCoordRound(aPoints[0].y);
        curIndex++;
        aRenderingContext.FillPolygon(polypath,curIndex);

        aRenderingContext.SetColor ( MakeBevelColor (aSide, 
                                                ((border_Style == NS_STYLE_BORDER_STYLE_RIDGE) ?
                                                NS_STYLE_BORDER_STYLE_GROOVE :
                                                NS_STYLE_BORDER_STYLE_RIDGE), 
                                                bgColor->mBackgroundColor,sideColor));
       
        polypath[0].x = NSToCoordRound((aPoints[0].x + aPoints[11].x)/2.0f);
        polypath[0].y = NSToCoordRound((aPoints[0].y + aPoints[11].y)/2.0f);
        curIndex = 1;
        GetPath(aPoints,polypath,&curIndex,eCalc,c1Index,.5);
        if (curIndex >= MAXPOLYPATHSIZE)
          return;
        polypath[curIndex].x = NSToCoordRound(aPoints[6].x) ;
        polypath[curIndex].y = NSToCoordRound(aPoints[6].y);
        curIndex++;
        GetPath(aPoints,polypath,&curIndex,eInside,c1Index);
        if (curIndex >= MAXPOLYPATHSIZE)
          return;
        polypath[curIndex].x = NSToCoordRound(aPoints[0].x);
        polypath[curIndex].y = NSToCoordRound(aPoints[0].y);
        curIndex++;
        aRenderingContext.FillPolygon(polypath,curIndex);
        }
        break;
      default:
        break;
    }
  }
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
        DrawSolidBorderSegment(aContext, rect, PR_TRUE);
        for (PRInt32 spaceX = 0; spaceX < numDashSpaces; spaceX++) {
          rect.x += rect.width + dashLength;
          rect.width = (spaceX == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, PR_TRUE);
        }
      }
      else {
        GetDashInfo(aBorder.height, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, aBorder.width, startDashLength);
        DrawSolidBorderSegment(aContext, rect, PR_FALSE);
        for (PRInt32 spaceY = 0; spaceY < numDashSpaces; spaceY++) {
          rect.y += rect.height + dashLength;
          rect.height = (spaceY == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, PR_FALSE);
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



