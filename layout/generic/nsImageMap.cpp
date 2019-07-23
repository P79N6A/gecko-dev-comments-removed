







































#include "nsImageMap.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIRenderingContext.h"
#include "nsPresContext.h"
#include "nsIURL.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsTextFragment.h"
#include "nsIContent.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsIDOMEventTarget.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsFrameManager.h"
#include "nsCoord.h"
#include "nsIImageMap.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIStringBundle.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"

static NS_DEFINE_CID(kCStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

class Area {
public:
  Area(nsIContent* aArea);
  virtual ~Area();

  virtual void ParseCoords(const nsAString& aSpec);

  virtual PRBool IsInside(nscoord x, nscoord y) const = 0;
  virtual void Draw(nsIFrame* aFrame, nsIRenderingContext& aRC) = 0;
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect) = 0;

  void HasFocus(PRBool aHasFocus);

  void GetHREF(nsAString& aHref) const;
  void GetArea(nsIContent** aArea) const;

  nsCOMPtr<nsIContent> mArea;
  nscoord* mCoords;
  PRInt32 mNumCoords;
  PRPackedBool mHasFocus;
};

Area::Area(nsIContent* aArea)
  : mArea(aArea)
{
  MOZ_COUNT_CTOR(Area);
  mCoords = nsnull;
  mNumCoords = 0;
  mHasFocus = PR_FALSE;
}

Area::~Area()
{
  MOZ_COUNT_DTOR(Area);
  delete [] mCoords;
}

void 
Area::GetHREF(nsAString& aHref) const
{
  aHref.Truncate();
  if (mArea) {
    mArea->GetAttr(kNameSpaceID_None, nsGkAtoms::href, aHref);
  }
}
 
void 
Area::GetArea(nsIContent** aArea) const
{
  *aArea = mArea;
  NS_IF_ADDREF(*aArea);
}

#include <stdlib.h>

inline PRBool
is_space(char c)
{
  return (c == ' ' ||
          c == '\f' ||
          c == '\n' ||
          c == '\r' ||
          c == '\t' ||
          c == '\v');
}

static void logMessage(nsIContent*      aContent,
                       const nsAString& aCoordsSpec,
                       PRInt32          aFlags,
                       const char* aMessageName) {
  nsIURI* documentURI = nsnull;
  nsIDocument* doc = aContent->GetOwnerDoc();
  if (doc) {
    documentURI = doc->GetDocumentURI();
  }
  nsContentUtils::ReportToConsole(
     nsContentUtils::eLAYOUT_PROPERTIES,
     aMessageName,
     nsnull,  
     0, 
     documentURI,
     PromiseFlatString(NS_LITERAL_STRING("coords=\"") +
                       aCoordsSpec +
                       NS_LITERAL_STRING("\"")), 
     0, 
     0, 
     aFlags,
     "ImageMap");
}

void Area::ParseCoords(const nsAString& aSpec)
{
  char* cp = ToNewCString(aSpec);
  if (cp) {
    char *tptr;
    char *n_str;
    PRInt32 i, cnt;
    PRInt32 *value_list;

    


    mNumCoords = 0;
    mCoords = nsnull;
    if (*cp == '\0')
    {
      return;
    }

    


    n_str = cp;
    while (is_space(*n_str))
    {
      n_str++;
    }
    if (*n_str == '\0')
    {
      return;
    }

    



    cnt = 0;
    while (*n_str != '\0')
    {
      PRBool has_comma;

      


      tptr = n_str;
      while (!is_space(*tptr) && *tptr != ',' && *tptr != '\0')
      {
        tptr++;
      }
      n_str = tptr;

      


      if (*n_str == '\0')
      {
        break;
      }

      



      has_comma = PR_FALSE;
      while (is_space(*tptr) || *tptr == ',')
      {
        if (*tptr == ',')
        {
          if (has_comma == PR_FALSE)
          {
            has_comma = PR_TRUE;
          }
          else
          {
            break;
          }
        }
        tptr++;
      }
      


      if ((*tptr == '\0')&&(has_comma == PR_FALSE))
      {
        break;
      }
      



      else if (has_comma == PR_FALSE)
      {
        *n_str = ',';
      }

      


      cnt++;

      n_str = tptr;
    }
    


    cnt++;
 
    


    value_list = new nscoord[cnt];
    if (!value_list)
    {
      return;
    }

    


    tptr = cp;
    for (i=0; i<cnt; i++)
    {
      char *ptr;

      ptr = strchr(tptr, ',');
      if (ptr)
      {
        *ptr = '\0';
      }
      



      while (is_space(*tptr))
      {
        tptr++;
      }
      if (*tptr == '\0')
      {
        value_list[i] = 0;
      }
      else
      {
        value_list[i] = (nscoord) ::atoi(tptr);
      }
      if (ptr)
      {
        *ptr = ',';
        tptr = ptr + 1;
      }
    }

    mNumCoords = cnt;
    mCoords = value_list;
  
    NS_Free(cp);
  }
}

void Area::HasFocus(PRBool aHasFocus)
{
  mHasFocus = aHasFocus;
}



class DefaultArea : public Area {
public:
  DefaultArea(nsIContent* aArea);

  virtual PRBool IsInside(nscoord x, nscoord y) const;
  virtual void Draw(nsIFrame* aFrame, nsIRenderingContext& aRC);
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect);
};

DefaultArea::DefaultArea(nsIContent* aArea)
  : Area(aArea)
{
}

PRBool DefaultArea::IsInside(nscoord x, nscoord y) const
{
  return PR_TRUE;
}

void DefaultArea::Draw(nsIFrame* aFrame, nsIRenderingContext& aRC)
{
  if (mHasFocus) {
    nsRect r = aFrame->GetRect();
    r.MoveTo(0, 0);
    nscoord x1 = r.x;
    nscoord y1 = r.y;
    const nscoord kOnePixel = nsPresContext::CSSPixelsToAppUnits(1);
    nscoord x2 = r.XMost() - kOnePixel;
    nscoord y2 = r.YMost() - kOnePixel;
    
    aRC.DrawLine(x1, y1, x1, y2);
    aRC.DrawLine(x1, y2, x2, y2);
    aRC.DrawLine(x1, y1, x2, y1);
    aRC.DrawLine(x2, y1, x2, y2);
  }
}

void DefaultArea::GetRect(nsIFrame* aFrame, nsRect& aRect)
{
  aRect = aFrame->GetRect();
  aRect.MoveTo(0, 0);
}



class RectArea : public Area {
public:
  RectArea(nsIContent* aArea);

  virtual void ParseCoords(const nsAString& aSpec);
  virtual PRBool IsInside(nscoord x, nscoord y) const;
  virtual void Draw(nsIFrame* aFrame, nsIRenderingContext& aRC);
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect);
};

RectArea::RectArea(nsIContent* aArea)
  : Area(aArea)
{
}

void RectArea::ParseCoords(const nsAString& aSpec)
{
  Area::ParseCoords(aSpec);

  PRBool saneRect = PR_TRUE;
  PRInt32 flag = nsIScriptError::warningFlag;
  if (mNumCoords >= 4) {
    if (mCoords[0] > mCoords[2]) {
      
      nscoord x = mCoords[2];
      mCoords[2] = mCoords[0];
      mCoords[0] = x;
      saneRect = PR_FALSE;
    }
  
    if (mCoords[1] > mCoords[3]) {
      
      nscoord y = mCoords[3];
      mCoords[3] = mCoords[1];
      mCoords[1] = y;
      saneRect = PR_FALSE;
    }

    if (mNumCoords > 4) {
      
      saneRect = PR_FALSE;
    }
  } else {
    saneRect = PR_FALSE;
    flag = nsIScriptError::errorFlag;
  }

  if (!saneRect) {
    logMessage(mArea, aSpec, flag, "ImageMapRectBoundsError");
  }
}

PRBool RectArea::IsInside(nscoord x, nscoord y) const
{
  if (mNumCoords >= 4) {       
    nscoord x1 = mCoords[0];
    nscoord y1 = mCoords[1];
    nscoord x2 = mCoords[2];
    nscoord y2 = mCoords[3];
    NS_ASSERTION(x1 <= x2 && y1 <= y2,
                 "Someone screwed up RectArea::ParseCoords");
    if ((x >= x1) && (x <= x2) && (y >= y1) && (y <= y2)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

void RectArea::Draw(nsIFrame* aFrame, nsIRenderingContext& aRC)
{
  if (mHasFocus) {
    if (mNumCoords >= 4) {
      nscoord x1 = nsPresContext::CSSPixelsToAppUnits(mCoords[0]);
      nscoord y1 = nsPresContext::CSSPixelsToAppUnits(mCoords[1]);
      nscoord x2 = nsPresContext::CSSPixelsToAppUnits(mCoords[2]);
      nscoord y2 = nsPresContext::CSSPixelsToAppUnits(mCoords[3]);
      NS_ASSERTION(x1 <= x2 && y1 <= y2,
                   "Someone screwed up RectArea::ParseCoords");
      aRC.DrawLine(x1, y1, x1, y2);
      aRC.DrawLine(x1, y2, x2, y2);
      aRC.DrawLine(x1, y1, x2, y1);
      aRC.DrawLine(x2, y1, x2, y2);
    }
  }
}

void RectArea::GetRect(nsIFrame* aFrame, nsRect& aRect)
{
  if (mNumCoords >= 4) {
    nscoord x1 = nsPresContext::CSSPixelsToAppUnits(mCoords[0]);
    nscoord y1 = nsPresContext::CSSPixelsToAppUnits(mCoords[1]);
    nscoord x2 = nsPresContext::CSSPixelsToAppUnits(mCoords[2]);
    nscoord y2 = nsPresContext::CSSPixelsToAppUnits(mCoords[3]);
    NS_ASSERTION(x1 <= x2 && y1 <= y2,
                 "Someone screwed up RectArea::ParseCoords");

    aRect.SetRect(x1, y1, x2, y2);
  }
}



class PolyArea : public Area {
public:
  PolyArea(nsIContent* aArea);

  virtual void ParseCoords(const nsAString& aSpec);
  virtual PRBool IsInside(nscoord x, nscoord y) const;
  virtual void Draw(nsIFrame* aFrame, nsIRenderingContext& aRC);
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect);
};

PolyArea::PolyArea(nsIContent* aArea)
  : Area(aArea)
{
}

void PolyArea::ParseCoords(const nsAString& aSpec)
{
  Area::ParseCoords(aSpec);

  if (mNumCoords >= 2) {
    if (mNumCoords & 1U) {
      logMessage(mArea,
                 aSpec,
                 nsIScriptError::warningFlag,
                 "ImageMapPolyOddNumberOfCoords");
    }
  } else {
    logMessage(mArea,
               aSpec,
               nsIScriptError::errorFlag,
               "ImageMapPolyWrongNumberOfCoords");
  }
}

PRBool PolyArea::IsInside(nscoord x, nscoord y) const
{
  if (mNumCoords >= 6) {
    PRInt32 intersects = 0;
    nscoord wherex = x;
    nscoord wherey = y;
    PRInt32 totalv = mNumCoords / 2;
    PRInt32 totalc = totalv * 2;
    nscoord xval = mCoords[totalc - 2];
    nscoord yval = mCoords[totalc - 1];
    PRInt32 end = totalc;
    PRInt32 pointer = 1;

    if ((yval >= wherey) != (mCoords[pointer] >= wherey))
      if ((xval >= wherex) == (mCoords[0] >= wherex))
        intersects += (xval >= wherex) ? 1 : 0;
      else
        intersects += ((xval - (yval - wherey) *
                        (mCoords[0] - xval) /
                        (mCoords[pointer] - yval)) >= wherex) ? 1 : 0;

    
    while (pointer < end)  {
      yval = mCoords[pointer];
      pointer += 2;
      if (yval >= wherey)  {
        while((pointer < end) && (mCoords[pointer] >= wherey))
          pointer+=2;
        if (pointer >= end)
          break;
        if ((mCoords[pointer-3] >= wherex) ==
            (mCoords[pointer-1] >= wherex)) {
          intersects += (mCoords[pointer-3] >= wherex) ? 1 : 0;
        } else {
          intersects +=
            ((mCoords[pointer-3] - (mCoords[pointer-2] - wherey) *
              (mCoords[pointer-1] - mCoords[pointer-3]) /
              (mCoords[pointer] - mCoords[pointer - 2])) >= wherex) ? 1:0;
        }
      }  else  {
        while((pointer < end) && (mCoords[pointer] < wherey))
          pointer+=2;
        if (pointer >= end)
          break;
        if ((mCoords[pointer-3] >= wherex) ==
            (mCoords[pointer-1] >= wherex)) {
          intersects += (mCoords[pointer-3] >= wherex) ? 1:0;
        } else {
          intersects +=
            ((mCoords[pointer-3] - (mCoords[pointer-2] - wherey) *
              (mCoords[pointer-1] - mCoords[pointer-3]) /
              (mCoords[pointer] - mCoords[pointer - 2])) >= wherex) ? 1:0;
        }
      }
    }
    if ((intersects & 1) != 0) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

void PolyArea::Draw(nsIFrame* aFrame, nsIRenderingContext& aRC)
{
  if (mHasFocus) {
    if (mNumCoords >= 6) {
      nscoord x0 = nsPresContext::CSSPixelsToAppUnits(mCoords[0]);
      nscoord y0 = nsPresContext::CSSPixelsToAppUnits(mCoords[1]);
      nscoord x1, y1;
      for (PRInt32 i = 2; i < mNumCoords; i += 2) {
        x1 = nsPresContext::CSSPixelsToAppUnits(mCoords[i]);
        y1 = nsPresContext::CSSPixelsToAppUnits(mCoords[i+1]);
        aRC.DrawLine(x0, y0, x1, y1);
        x0 = x1;
        y0 = y1;
      }
      x1 = nsPresContext::CSSPixelsToAppUnits(mCoords[0]);
      y1 = nsPresContext::CSSPixelsToAppUnits(mCoords[1]);
      aRC.DrawLine(x0, y0, x1, y1);
    }
  }
}

void PolyArea::GetRect(nsIFrame* aFrame, nsRect& aRect)
{
  if (mNumCoords >= 6) {
    nscoord x1, x2, y1, y2, xtmp, ytmp;
    x1 = x2 = nsPresContext::CSSPixelsToAppUnits(mCoords[0]);
    y1 = y2 = nsPresContext::CSSPixelsToAppUnits(mCoords[1]);
    for (PRInt32 i = 2; i < mNumCoords; i += 2) {
      xtmp = nsPresContext::CSSPixelsToAppUnits(mCoords[i]);
      ytmp = nsPresContext::CSSPixelsToAppUnits(mCoords[i+1]);
      x1 = x1 < xtmp ? x1 : xtmp;
      y1 = y1 < ytmp ? y1 : ytmp;
      x2 = x2 > xtmp ? x2 : xtmp;
      y2 = y2 > ytmp ? y2 : ytmp;
    }

    aRect.SetRect(x1, y1, x2, y2);
  }
}



class CircleArea : public Area {
public:
  CircleArea(nsIContent* aArea);

  virtual void ParseCoords(const nsAString& aSpec);
  virtual PRBool IsInside(nscoord x, nscoord y) const;
  virtual void Draw(nsIFrame* aFrame, nsIRenderingContext& aRC);
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect);
};

CircleArea::CircleArea(nsIContent* aArea)
  : Area(aArea)
{
}

void CircleArea::ParseCoords(const nsAString& aSpec)
{
  Area::ParseCoords(aSpec);

  PRBool wrongNumberOfCoords = PR_FALSE;
  PRInt32 flag = nsIScriptError::warningFlag;
  if (mNumCoords >= 3) {
    if (mCoords[2] < 0) {
      logMessage(mArea,
                 aSpec,
                 nsIScriptError::errorFlag,
                 "ImageMapCircleNegativeRadius");
    }
  
    if (mNumCoords > 3) {
      wrongNumberOfCoords = PR_TRUE;
    }
  } else {
    wrongNumberOfCoords = PR_TRUE;
    flag = nsIScriptError::errorFlag;
  }

  if (wrongNumberOfCoords) {
    logMessage(mArea,
               aSpec,
               flag,
               "ImageMapCircleWrongNumberOfCoords");
  }
}

PRBool CircleArea::IsInside(nscoord x, nscoord y) const
{
  
  if (mNumCoords >= 3) {
    nscoord x1 = mCoords[0];
    nscoord y1 = mCoords[1];
    nscoord radius = mCoords[2];
    if (radius < 0) {
      return PR_FALSE;
    }
    nscoord dx = x1 - x;
    nscoord dy = y1 - y;
    nscoord dist = (dx * dx) + (dy * dy);
    if (dist <= (radius * radius)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

void CircleArea::Draw(nsIFrame* aFrame, nsIRenderingContext& aRC)
{
  if (mHasFocus) {
    if (mNumCoords >= 3) {
      nscoord x1 = nsPresContext::CSSPixelsToAppUnits(mCoords[0]);
      nscoord y1 = nsPresContext::CSSPixelsToAppUnits(mCoords[1]);
      nscoord radius = nsPresContext::CSSPixelsToAppUnits(mCoords[2]);
      if (radius < 0) {
        return;
      }
      nscoord x = x1 - radius;
      nscoord y = y1 - radius;
      nscoord w = 2 * radius;
      aRC.DrawEllipse(x, y, w, w);
    }
  }
}

void CircleArea::GetRect(nsIFrame* aFrame, nsRect& aRect)
{
  if (mNumCoords >= 3) {
    nscoord x1 = nsPresContext::CSSPixelsToAppUnits(mCoords[0]);
    nscoord y1 = nsPresContext::CSSPixelsToAppUnits(mCoords[1]);
    nscoord radius = nsPresContext::CSSPixelsToAppUnits(mCoords[2]);
    if (radius < 0) {
      return;
    }

    aRect.SetRect(x1 - radius, y1 - radius, x1 + radius, y1 + radius);
  }
}




nsImageMap::nsImageMap() :
  mPresShell(nsnull),
  mImageFrame(nsnull),
  mContainsBlockContents(PR_FALSE)
{
}

nsImageMap::~nsImageMap()
{
  NS_ASSERTION(mAreas.Length() == 0, "Destroy was not called");
}

NS_IMPL_ISUPPORTS4(nsImageMap,
                   nsIMutationObserver,
                   nsIDOMFocusListener,
                   nsIDOMEventListener,
                   nsIImageMap)

NS_IMETHODIMP
nsImageMap::GetBoundsForAreaContent(nsIContent *aContent, 
                                   nsPresContext* aPresContext, 
                                   nsRect& aBounds)
{
  NS_ENSURE_TRUE(aContent && aPresContext, NS_ERROR_INVALID_ARG);

  
  PRUint32 i, n = mAreas.Length();
  for (i = 0; i < n; i++) {
    Area* area = mAreas.ElementAt(i);
    if (area->mArea == aContent) {
      aBounds = nsRect();
      nsIPresShell* shell = aPresContext->PresShell();
      if (shell) {
        nsIFrame* frame = shell->GetPrimaryFrameFor(aContent);
        if (frame) {
          area->GetRect(frame, aBounds);
        }
      }
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

void
nsImageMap::FreeAreas()
{
  nsFrameManager *frameManager = mPresShell->FrameManager();

  PRUint32 i, n = mAreas.Length();
  for (i = 0; i < n; i++) {
    Area* area = mAreas.ElementAt(i);
    frameManager->RemoveAsPrimaryFrame(area->mArea, mImageFrame);

    nsCOMPtr<nsIContent> areaContent;
    area->GetArea(getter_AddRefs(areaContent));
    if (areaContent) {
      areaContent->RemoveEventListenerByIID(this, NS_GET_IID(nsIDOMFocusListener));
    }
    delete area;
  }
  mAreas.Clear();
}

nsresult
nsImageMap::Init(nsIPresShell* aPresShell, nsIFrame* aImageFrame, nsIDOMHTMLMapElement* aMap)
{
  NS_PRECONDITION(nsnull != aMap, "null ptr");
  if (nsnull == aMap) {
    return NS_ERROR_NULL_POINTER;
  }
  mPresShell = aPresShell;
  mImageFrame = aImageFrame;

  mMap = do_QueryInterface(aMap);
  NS_ASSERTION(mMap, "aMap is not an nsIContent!");
  mMap->AddMutationObserver(this);

  
  return UpdateAreas();
}


nsresult
nsImageMap::SearchForAreas(nsIContent* aParent, PRBool& aFoundArea,
                           PRBool& aFoundAnchor)
{
  nsresult rv = NS_OK;
  PRUint32 i, n = aParent->GetChildCount();

  
  for (i = 0; i < n; i++) {
    nsIContent *child = aParent->GetChildAt(i);

    if (child->IsNodeOfType(nsINode::eHTML)) {
      
      
      if (!aFoundAnchor && child->Tag() == nsGkAtoms::area) {
        aFoundArea = PR_TRUE;
        rv = AddArea(child);
        NS_ENSURE_SUCCESS(rv, rv);
        
        
        
        
        
        continue;
      }
      
      
      if (!aFoundArea && child->Tag() == nsGkAtoms::a) {
        aFoundAnchor = PR_TRUE;
        rv = AddArea(child);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    
    if (child->IsNodeOfType(nsINode::eELEMENT)) {
      mContainsBlockContents = PR_TRUE;
      rv = SearchForAreas(child, aFoundArea, aFoundAnchor);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
nsImageMap::UpdateAreas()
{
  
  FreeAreas();

  PRBool foundArea = PR_FALSE;
  PRBool foundAnchor = PR_FALSE;
  mContainsBlockContents = PR_FALSE;

  return SearchForAreas(mMap, foundArea, foundAnchor);
}

nsresult
nsImageMap::AddArea(nsIContent* aArea)
{
  nsAutoString coords;
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::rect, &nsGkAtoms::rectangle,
     &nsGkAtoms::poly, &nsGkAtoms::polygon, &nsGkAtoms::circle,
     &nsGkAtoms::circ, &nsGkAtoms::_default, nsnull};

  aArea->GetAttr(kNameSpaceID_None, nsGkAtoms::coords, coords);

  Area* area;
  switch (aArea->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::shape,
                                 strings, eIgnoreCase)) {
    case nsIContent::ATTR_MISSING:
    case 0:
    case 1:
    case 2:
      area = new RectArea(aArea);
      break;
    case 3:
    case 4:
      area = new PolyArea(aArea);
      break;
    case 5:
    case 6:
      area = new CircleArea(aArea);
      break;
    case 7:
      area = new DefaultArea(aArea);
      break;
    default:
      
      return NS_OK;
  }
  if (!area)
    return NS_ERROR_OUT_OF_MEMORY;

  
  aArea->AddEventListenerByIID(this, NS_GET_IID(nsIDOMFocusListener));

  
  
  
  
  
  mPresShell->FrameManager()->SetPrimaryFrameFor(aArea, mImageFrame);
  aArea->SetMayHaveFrame(PR_TRUE);
  NS_ASSERTION(aArea->MayHaveFrame(), "SetMayHaveFrame failed?");

  area->ParseCoords(coords);
  mAreas.AppendElement(area);
  return NS_OK;
}

PRBool
nsImageMap::IsInside(nscoord aX, nscoord aY,
                     nsIContent** aContent) const
{
  NS_ASSERTION(mMap, "Not initialized");
  PRUint32 i, n = mAreas.Length();
  for (i = 0; i < n; i++) {
    Area* area = mAreas.ElementAt(i);
    if (area->IsInside(aX, aY)) {
      area->GetArea(aContent);

      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

void
nsImageMap::Draw(nsIFrame* aFrame, nsIRenderingContext& aRC)
{
  PRUint32 i, n = mAreas.Length();
  for (i = 0; i < n; i++) {
    Area* area = mAreas.ElementAt(i);
    area->Draw(aFrame, aRC);
  }
}

void
nsImageMap::MaybeUpdateAreas(nsIContent *aContent)
{
  if (aContent == mMap || mContainsBlockContents) {
    UpdateAreas();
  }
}

void
nsImageMap::AttributeChanged(nsIDocument* aDocument,
                             nsIContent*  aContent,
                             PRInt32      aNameSpaceID,
                             nsIAtom*     aAttribute,
                             PRInt32      aModType,
                             PRUint32     aStateMask)
{
  
  
  
  
  if ((aContent->NodeInfo()->Equals(nsGkAtoms::area) ||
       aContent->NodeInfo()->Equals(nsGkAtoms::a)) &&
      aContent->IsNodeOfType(nsINode::eHTML) &&
      aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::shape ||
       aAttribute == nsGkAtoms::coords)) {
    MaybeUpdateAreas(aContent->GetParent());
  }
}

void
nsImageMap::ContentAppended(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            PRInt32     aNewIndexInContainer)
{
  MaybeUpdateAreas(aContainer);
}

void
nsImageMap::ContentInserted(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 aIndexInContainer)
{
  MaybeUpdateAreas(aContainer);
}

void
nsImageMap::ContentRemoved(nsIDocument *aDocument,
                           nsIContent* aContainer,
                           nsIContent* aChild,
                           PRInt32 aIndexInContainer)
{
  MaybeUpdateAreas(aContainer);
}

nsresult
nsImageMap::Focus(nsIDOMEvent* aEvent)
{
  return ChangeFocus(aEvent, PR_TRUE);
}

nsresult
nsImageMap::Blur(nsIDOMEvent* aEvent)
{
  return ChangeFocus(aEvent, PR_FALSE);
}

nsresult
nsImageMap::ChangeFocus(nsIDOMEvent* aEvent, PRBool aFocus)
{
  
  nsCOMPtr<nsIDOMEventTarget> target;
  if (NS_SUCCEEDED(aEvent->GetTarget(getter_AddRefs(target))) && target) {
    nsCOMPtr<nsIContent> targetContent(do_QueryInterface(target));
    if (targetContent) {
      PRUint32 i, n = mAreas.Length();
      for (i = 0; i < n; i++) {
        Area* area = mAreas.ElementAt(i);
        nsCOMPtr<nsIContent> areaContent;
        area->GetArea(getter_AddRefs(areaContent));
        if (areaContent.get() == targetContent.get()) {
          
          area->HasFocus(aFocus);
          
          nsCOMPtr<nsIDocument> doc = targetContent->GetDocument();
          
          if (doc) {
            nsIPresShell *presShell = doc->GetPrimaryShell();
            if (presShell) {
              nsIFrame* imgFrame = presShell->GetPrimaryFrameFor(targetContent);
              if (imgFrame) {
                nsRect dmgRect;
                area->GetRect(imgFrame, dmgRect);
                imgFrame->Invalidate(dmgRect);
              }
            }
          }
          break;
        }
      }
    }
  }
  return NS_OK;
}

nsresult
nsImageMap::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

void
nsImageMap::Destroy(void)
{
  FreeAreas();
  mMap->RemoveMutationObserver(this);
}
