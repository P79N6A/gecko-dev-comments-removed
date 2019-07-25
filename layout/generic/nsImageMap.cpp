







































#include "nsImageMap.h"

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsRenderingContext.h"
#include "nsPresContext.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsTextFragment.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsIDOMEventTarget.h"
#include "nsIPresShell.h"
#include "nsImageFrame.h"
#include "nsCoord.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIStringBundle.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"

namespace dom = mozilla::dom;

static NS_DEFINE_CID(kCStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

class Area {
public:
  Area(nsIContent* aArea);
  virtual ~Area();

  virtual void ParseCoords(const nsAString& aSpec);

  virtual bool IsInside(nscoord x, nscoord y) const = 0;
  virtual void Draw(nsIFrame* aFrame, nsRenderingContext& aRC) = 0;
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect) = 0;

  void HasFocus(bool aHasFocus);

  nsCOMPtr<nsIContent> mArea;
  nscoord* mCoords;
  PRInt32 mNumCoords;
  bool mHasFocus;
};

Area::Area(nsIContent* aArea)
  : mArea(aArea)
{
  MOZ_COUNT_CTOR(Area);
  NS_PRECONDITION(mArea, "How did that happen?");
  mCoords = nsnull;
  mNumCoords = 0;
  mHasFocus = false;
}

Area::~Area()
{
  MOZ_COUNT_DTOR(Area);
  delete [] mCoords;
}

#include <stdlib.h>

inline bool
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
  nsIDocument* doc = aContent->OwnerDoc();

  nsContentUtils::ReportToConsole(
     nsContentUtils::eLAYOUT_PROPERTIES,
     aMessageName,
     nsnull,  
     0, 
     nsnull,
     PromiseFlatString(NS_LITERAL_STRING("coords=\"") +
                       aCoordsSpec +
                       NS_LITERAL_STRING("\"")), 
     0, 
     0, 
     aFlags,
     "ImageMap", doc);
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
      nsMemory::Free(cp);
      return;
    }

    


    n_str = cp;
    while (is_space(*n_str))
    {
      n_str++;
    }
    if (*n_str == '\0')
    {
      nsMemory::Free(cp);
      return;
    }

    



    cnt = 0;
    while (*n_str != '\0')
    {
      bool has_comma;

      


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

      



      has_comma = false;
      while (is_space(*tptr) || *tptr == ',')
      {
        if (*tptr == ',')
        {
          if (!has_comma)
          {
            has_comma = true;
          }
          else
          {
            break;
          }
        }
        tptr++;
      }
      


      if ((*tptr == '\0') && !has_comma)
      {
        break;
      }
      



      else if (!has_comma)
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
      nsMemory::Free(cp);
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

    nsMemory::Free(cp);
  }
}

void Area::HasFocus(bool aHasFocus)
{
  mHasFocus = aHasFocus;
}



class DefaultArea : public Area {
public:
  DefaultArea(nsIContent* aArea);

  virtual bool IsInside(nscoord x, nscoord y) const;
  virtual void Draw(nsIFrame* aFrame, nsRenderingContext& aRC);
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect);
};

DefaultArea::DefaultArea(nsIContent* aArea)
  : Area(aArea)
{
}

bool DefaultArea::IsInside(nscoord x, nscoord y) const
{
  return true;
}

void DefaultArea::Draw(nsIFrame* aFrame, nsRenderingContext& aRC)
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
  virtual bool IsInside(nscoord x, nscoord y) const;
  virtual void Draw(nsIFrame* aFrame, nsRenderingContext& aRC);
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect);
};

RectArea::RectArea(nsIContent* aArea)
  : Area(aArea)
{
}

void RectArea::ParseCoords(const nsAString& aSpec)
{
  Area::ParseCoords(aSpec);

  bool saneRect = true;
  PRInt32 flag = nsIScriptError::warningFlag;
  if (mNumCoords >= 4) {
    if (mCoords[0] > mCoords[2]) {
      
      nscoord x = mCoords[2];
      mCoords[2] = mCoords[0];
      mCoords[0] = x;
      saneRect = false;
    }

    if (mCoords[1] > mCoords[3]) {
      
      nscoord y = mCoords[3];
      mCoords[3] = mCoords[1];
      mCoords[1] = y;
      saneRect = false;
    }

    if (mNumCoords > 4) {
      
      saneRect = false;
    }
  } else {
    saneRect = false;
    flag = nsIScriptError::errorFlag;
  }

  if (!saneRect) {
    logMessage(mArea, aSpec, flag, "ImageMapRectBoundsError");
  }
}

bool RectArea::IsInside(nscoord x, nscoord y) const
{
  if (mNumCoords >= 4) {       
    nscoord x1 = mCoords[0];
    nscoord y1 = mCoords[1];
    nscoord x2 = mCoords[2];
    nscoord y2 = mCoords[3];
    NS_ASSERTION(x1 <= x2 && y1 <= y2,
                 "Someone screwed up RectArea::ParseCoords");
    if ((x >= x1) && (x <= x2) && (y >= y1) && (y <= y2)) {
      return true;
    }
  }
  return false;
}

void RectArea::Draw(nsIFrame* aFrame, nsRenderingContext& aRC)
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
  virtual bool IsInside(nscoord x, nscoord y) const;
  virtual void Draw(nsIFrame* aFrame, nsRenderingContext& aRC);
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

bool PolyArea::IsInside(nscoord x, nscoord y) const
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

    if ((yval >= wherey) != (mCoords[pointer] >= wherey)) {
      if ((xval >= wherex) == (mCoords[0] >= wherex)) {
        intersects += (xval >= wherex) ? 1 : 0;
      } else {
        intersects += ((xval - (yval - wherey) *
                        (mCoords[0] - xval) /
                        (mCoords[pointer] - yval)) >= wherex) ? 1 : 0;
      }
    }

    
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
      return true;
    }
  }
  return false;
}

void PolyArea::Draw(nsIFrame* aFrame, nsRenderingContext& aRC)
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
  virtual bool IsInside(nscoord x, nscoord y) const;
  virtual void Draw(nsIFrame* aFrame, nsRenderingContext& aRC);
  virtual void GetRect(nsIFrame* aFrame, nsRect& aRect);
};

CircleArea::CircleArea(nsIContent* aArea)
  : Area(aArea)
{
}

void CircleArea::ParseCoords(const nsAString& aSpec)
{
  Area::ParseCoords(aSpec);

  bool wrongNumberOfCoords = false;
  PRInt32 flag = nsIScriptError::warningFlag;
  if (mNumCoords >= 3) {
    if (mCoords[2] < 0) {
      logMessage(mArea,
                 aSpec,
                 nsIScriptError::errorFlag,
                 "ImageMapCircleNegativeRadius");
    }

    if (mNumCoords > 3) {
      wrongNumberOfCoords = true;
    }
  } else {
    wrongNumberOfCoords = true;
    flag = nsIScriptError::errorFlag;
  }

  if (wrongNumberOfCoords) {
    logMessage(mArea,
               aSpec,
               flag,
               "ImageMapCircleWrongNumberOfCoords");
  }
}

bool CircleArea::IsInside(nscoord x, nscoord y) const
{
  
  if (mNumCoords >= 3) {
    nscoord x1 = mCoords[0];
    nscoord y1 = mCoords[1];
    nscoord radius = mCoords[2];
    if (radius < 0) {
      return false;
    }
    nscoord dx = x1 - x;
    nscoord dy = y1 - y;
    nscoord dist = (dx * dx) + (dy * dy);
    if (dist <= (radius * radius)) {
      return true;
    }
  }
  return false;
}

void CircleArea::Draw(nsIFrame* aFrame, nsRenderingContext& aRC)
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
  mImageFrame(nsnull),
  mContainsBlockContents(false)
{
}

nsImageMap::~nsImageMap()
{
  NS_ASSERTION(mAreas.Length() == 0, "Destroy was not called");
}

NS_IMPL_ISUPPORTS2(nsImageMap,
                   nsIMutationObserver,
                   nsIDOMEventListener)

nsresult
nsImageMap::GetBoundsForAreaContent(nsIContent *aContent,
                                    nsRect& aBounds)
{
  NS_ENSURE_TRUE(aContent && mImageFrame, NS_ERROR_INVALID_ARG);

  
  PRUint32 i, n = mAreas.Length();
  for (i = 0; i < n; i++) {
    Area* area = mAreas.ElementAt(i);
    if (area->mArea == aContent) {
      aBounds = nsRect();
      area->GetRect(mImageFrame, aBounds);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

void
nsImageMap::FreeAreas()
{
  PRUint32 i, n = mAreas.Length();
  for (i = 0; i < n; i++) {
    Area* area = mAreas.ElementAt(i);
    NS_ASSERTION(area->mArea->GetPrimaryFrame() == mImageFrame,
                 "Unexpected primary frame");
    area->mArea->SetPrimaryFrame(nsnull);

    area->mArea->RemoveSystemEventListener(NS_LITERAL_STRING("focus"), this,
                                           false);
    area->mArea->RemoveSystemEventListener(NS_LITERAL_STRING("blur"), this,
                                           false);
    delete area;
  }
  mAreas.Clear();
}

nsresult
nsImageMap::Init(nsImageFrame* aImageFrame, nsIContent* aMap)
{
  NS_PRECONDITION(aMap, "null ptr");
  if (!aMap) {
    return NS_ERROR_NULL_POINTER;
  }
  mImageFrame = aImageFrame;

  mMap = aMap;
  mMap->AddMutationObserver(this);

  
  return UpdateAreas();
}


nsresult
nsImageMap::SearchForAreas(nsIContent* aParent, bool& aFoundArea,
                           bool& aFoundAnchor)
{
  nsresult rv = NS_OK;
  PRUint32 i, n = aParent->GetChildCount();

  
  for (i = 0; i < n; i++) {
    nsIContent *child = aParent->GetChildAt(i);

    if (child->IsHTML()) {
      
      
      if (!aFoundAnchor && child->Tag() == nsGkAtoms::area) {
        aFoundArea = true;
        rv = AddArea(child);
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        
        
        continue;
      }
      
      
      if (!aFoundArea && child->Tag() == nsGkAtoms::a) {
        aFoundAnchor = true;
        rv = AddArea(child);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    if (child->IsElement()) {
      mContainsBlockContents = true;
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

  bool foundArea = false;
  bool foundAnchor = false;
  mContainsBlockContents = false;

  return SearchForAreas(mMap, foundArea, foundAnchor);
}

nsresult
nsImageMap::AddArea(nsIContent* aArea)
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::rect, &nsGkAtoms::rectangle,
     &nsGkAtoms::circle, &nsGkAtoms::circ,
     &nsGkAtoms::_default,
     &nsGkAtoms::poly, &nsGkAtoms::polygon,
     nsnull};

  Area* area;
  switch (aArea->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::shape,
                                 strings, eIgnoreCase)) {
  case nsIContent::ATTR_VALUE_NO_MATCH:
  case nsIContent::ATTR_MISSING:
  case 0:
  case 1:
    area = new RectArea(aArea);
    break;
  case 2:
  case 3:
    area = new CircleArea(aArea);
    break;
  case 4:
    area = new DefaultArea(aArea);
    break;
  case 5:
  case 6:
    area = new PolyArea(aArea);
    break;
  default:
    NS_NOTREACHED("FindAttrValueIn returned an unexpected value.");
    break;
  }
  if (!area)
    return NS_ERROR_OUT_OF_MEMORY;

  
  aArea->AddSystemEventListener(NS_LITERAL_STRING("focus"), this, false,
                                false);
  aArea->AddSystemEventListener(NS_LITERAL_STRING("blur"), this, false,
                                false);

  
  
  
  
  
  aArea->SetPrimaryFrame(mImageFrame);

  nsAutoString coords;
  aArea->GetAttr(kNameSpaceID_None, nsGkAtoms::coords, coords);
  area->ParseCoords(coords);
  mAreas.AppendElement(area);
  return NS_OK;
}

nsIContent*
nsImageMap::GetArea(nscoord aX, nscoord aY) const
{
  NS_ASSERTION(mMap, "Not initialized");
  PRUint32 i, n = mAreas.Length();
  for (i = 0; i < n; i++) {
    Area* area = mAreas.ElementAt(i);
    if (area->IsInside(aX, aY)) {
      return area->mArea;
    }
  }

  return nsnull;
}

void
nsImageMap::Draw(nsIFrame* aFrame, nsRenderingContext& aRC)
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
nsImageMap::AttributeChanged(nsIDocument*  aDocument,
                             dom::Element* aElement,
                             PRInt32       aNameSpaceID,
                             nsIAtom*      aAttribute,
                             PRInt32       aModType)
{
  
  
  
  
  if ((aElement->NodeInfo()->Equals(nsGkAtoms::area) ||
       aElement->NodeInfo()->Equals(nsGkAtoms::a)) &&
      aElement->IsHTML() &&
      aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::shape ||
       aAttribute == nsGkAtoms::coords)) {
    MaybeUpdateAreas(aElement->GetParent());
  } else if (aElement == mMap &&
             aNameSpaceID == kNameSpaceID_None &&
             (aAttribute == nsGkAtoms::name ||
              aAttribute == nsGkAtoms::id) &&
             mImageFrame) {
    
    mImageFrame->DisconnectMap();
  }
}

void
nsImageMap::ContentAppended(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aFirstNewContent,
                            PRInt32     )
{
  MaybeUpdateAreas(aContainer);
}

void
nsImageMap::ContentInserted(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 )
{
  MaybeUpdateAreas(aContainer);
}

void
nsImageMap::ContentRemoved(nsIDocument *aDocument,
                           nsIContent* aContainer,
                           nsIContent* aChild,
                           PRInt32 aIndexInContainer,
                           nsIContent* aPreviousSibling)
{
  MaybeUpdateAreas(aContainer);
}

void
nsImageMap::ParentChainChanged(nsIContent* aContent)
{
  NS_ASSERTION(aContent == mMap,
               "Unexpected ParentChainChanged notification!");
  if (mImageFrame) {
    mImageFrame->DisconnectMap();
  }
}

nsresult
nsImageMap::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);
  bool focus = eventType.EqualsLiteral("focus");
  NS_ABORT_IF_FALSE(focus == !eventType.EqualsLiteral("blur"),
                    "Unexpected event type");

  
  nsCOMPtr<nsIDOMEventTarget> target;
  if (NS_SUCCEEDED(aEvent->GetTarget(getter_AddRefs(target))) && target) {
    nsCOMPtr<nsIContent> targetContent(do_QueryInterface(target));
    if (targetContent) {
      PRUint32 i, n = mAreas.Length();
      for (i = 0; i < n; i++) {
        Area* area = mAreas.ElementAt(i);
        if (area->mArea == targetContent) {
          
          area->HasFocus(focus);
          
          if (mImageFrame) {
            nsRect dmgRect;
            area->GetRect(mImageFrame, dmgRect);
            mImageFrame->Invalidate(dmgRect);
          }
          break;
        }
      }
    }
  }
  return NS_OK;
}

void
nsImageMap::Destroy(void)
{
  FreeAreas();
  mImageFrame = nsnull;
  mMap->RemoveMutationObserver(this);
}
