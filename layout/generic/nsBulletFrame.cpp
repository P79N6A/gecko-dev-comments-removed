






#include "nsBulletFrame.h"

#include "mozilla/MathAlgorithms.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValueInlines.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsRenderingContext.h"
#include "prprf.h"
#include "nsDisplayList.h"
#include "nsCounterManager.h"
#include "nsBidiUtils.h"
#include "CounterStyleManager.h"

#include "imgIContainer.h"
#include "imgRequestProxy.h"
#include "nsIURI.h"

#include <algorithm>

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

using namespace mozilla;

NS_DECLARE_FRAME_PROPERTY(FontSizeInflationProperty, nullptr)

NS_IMPL_FRAMEARENA_HELPERS(nsBulletFrame)

#ifdef DEBUG
NS_QUERYFRAME_HEAD(nsBulletFrame)
  NS_QUERYFRAME_ENTRY(nsBulletFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsFrame)
#endif

nsBulletFrame::~nsBulletFrame()
{
}

void
nsBulletFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  if (mImageRequest) {
    
    nsLayoutUtils::DeregisterImageRequest(PresContext(),
                                          mImageRequest,
                                          &mRequestRegistered);
    mImageRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
    mImageRequest = nullptr;
  }

  if (mListener) {
    mListener->SetFrame(nullptr);
  }

  
  nsFrame::DestroyFrom(aDestructRoot);
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsBulletFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Bullet"), aResult);
}
#endif

nsIAtom*
nsBulletFrame::GetType() const
{
  return nsGkAtoms::bulletFrame;
}

bool
nsBulletFrame::IsEmpty()
{
  return IsSelfEmpty();
}

bool
nsBulletFrame::IsSelfEmpty() 
{
  return StyleList()->GetCounterStyle()->IsNone();
}

 void
nsBulletFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsFrame::DidSetStyleContext(aOldStyleContext);

  imgRequestProxy *newRequest = StyleList()->GetListStyleImage();

  if (newRequest) {

    if (!mListener) {
      mListener = new nsBulletListener();
      mListener->SetFrame(this);
    }

    bool needNewRequest = true;

    if (mImageRequest) {
      
      nsCOMPtr<nsIURI> oldURI;
      mImageRequest->GetURI(getter_AddRefs(oldURI));
      nsCOMPtr<nsIURI> newURI;
      newRequest->GetURI(getter_AddRefs(newURI));
      if (oldURI && newURI) {
        bool same;
        newURI->Equals(oldURI, &same);
        if (same) {
          needNewRequest = false;
        }
      }
    }

    if (needNewRequest) {
      nsRefPtr<imgRequestProxy> oldRequest = mImageRequest;
      newRequest->Clone(mListener, getter_AddRefs(mImageRequest));

      
      
      
      if (oldRequest) {
        nsLayoutUtils::DeregisterImageRequest(PresContext(), oldRequest,
                                              &mRequestRegistered);
        oldRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
        oldRequest = nullptr;
      }

      
      if (mImageRequest) {
        nsLayoutUtils::RegisterImageRequestIfAnimated(PresContext(),
                                                      mImageRequest,
                                                      &mRequestRegistered);
      }
    }
  } else {
    
    if (mImageRequest) {
      nsLayoutUtils::DeregisterImageRequest(PresContext(), mImageRequest,
                                            &mRequestRegistered);

      mImageRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
      mImageRequest = nullptr;
    }
  }

#ifdef ACCESSIBILITY
  
  
  if (aOldStyleContext) {
    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (accService) {
      const nsStyleList* oldStyleList = aOldStyleContext->PeekStyleList();
      if (oldStyleList) {
        bool hadBullet = oldStyleList->GetListStyleImage() ||
          !oldStyleList->GetCounterStyle()->IsNone();

        const nsStyleList* newStyleList = StyleList();
        bool hasBullet = newStyleList->GetListStyleImage() ||
          !newStyleList->GetCounterStyle()->IsNone();

        if (hadBullet != hasBullet) {
          accService->UpdateListBullet(PresContext()->GetPresShell(), mContent,
                                       hasBullet);
        }
      }
    }
  }
#endif
}

class nsDisplayBulletGeometry : public nsDisplayItemGenericGeometry
{
public:
  nsDisplayBulletGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder)
    : nsDisplayItemGenericGeometry(aItem, aBuilder)
  {
    nsBulletFrame* f = static_cast<nsBulletFrame*>(aItem->Frame());
    mOrdinal = f->GetOrdinal();
  }

  int32_t mOrdinal;
};

class nsDisplayBullet : public nsDisplayItem {
public:
  nsDisplayBullet(nsDisplayListBuilder* aBuilder, nsBulletFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayBullet);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBullet() {
    MOZ_COUNT_DTOR(nsDisplayBullet);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = false;
    return mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
  }
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState,
                       nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE {
    aOutFrames->AppendElement(mFrame);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Bullet", TYPE_BULLET)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    bool snap;
    return GetBounds(aBuilder, &snap);
  }

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplayBulletGeometry(this, aBuilder);
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion *aInvalidRegion) MOZ_OVERRIDE
  {
    const nsDisplayBulletGeometry* geometry = static_cast<const nsDisplayBulletGeometry*>(aGeometry);
    nsBulletFrame* f = static_cast<nsBulletFrame*>(mFrame);

    if (f->GetOrdinal() != geometry->mOrdinal) {
      bool snap;
      aInvalidRegion->Or(geometry->mBounds, GetBounds(aBuilder, &snap));
      return;
    }

    nsCOMPtr<imgIContainer> image = f->GetImage();
    if (aBuilder->ShouldSyncDecodeImages() && image && !image->IsDecoded()) {
      
      
      
      bool snap;
      aInvalidRegion->Or(*aInvalidRegion, GetBounds(aBuilder, &snap));
    }

    return nsDisplayItem::ComputeInvalidationRegion(aBuilder, aGeometry, aInvalidRegion);
  }
};

void nsDisplayBullet::Paint(nsDisplayListBuilder* aBuilder,
                            nsRenderingContext* aCtx)
{
  uint32_t flags = imgIContainer::FLAG_NONE;
  if (aBuilder->ShouldSyncDecodeImages()) {
    flags |= imgIContainer::FLAG_SYNC_DECODE;
  }
  static_cast<nsBulletFrame*>(mFrame)->
    PaintBullet(*aCtx, ToReferenceFrame(), mVisibleRect, flags);
}

void
nsBulletFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsBulletFrame");
  
  aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplayBullet(aBuilder, this));
}

void
nsBulletFrame::PaintBullet(nsRenderingContext& aRenderingContext, nsPoint aPt,
                           const nsRect& aDirtyRect, uint32_t aFlags)
{
  const nsStyleList* myList = StyleList();
  CounterStyle* listStyleType = myList->GetCounterStyle();

  if (myList->GetListStyleImage() && mImageRequest) {
    uint32_t status;
    mImageRequest->GetImageStatus(&status);
    if (status & imgIRequest::STATUS_LOAD_COMPLETE &&
        !(status & imgIRequest::STATUS_ERROR)) {
      nsCOMPtr<imgIContainer> imageCon;
      mImageRequest->GetImage(getter_AddRefs(imageCon));
      if (imageCon) {
        nsRect dest(mPadding.left, mPadding.top,
                    mRect.width - (mPadding.left + mPadding.right),
                    mRect.height - (mPadding.top + mPadding.bottom));
        nsLayoutUtils::DrawSingleImage(&aRenderingContext,
             imageCon, nsLayoutUtils::GetGraphicsFilterForFrame(this),
             dest + aPt, aDirtyRect, nullptr, aFlags);
        return;
      }
    }
  }

  nsRefPtr<nsFontMetrics> fm;
  aRenderingContext.SetColor(nsLayoutUtils::GetColor(this, eCSSProperty_color));

  nsAutoString text;
  switch (listStyleType->GetStyle()) {
  case NS_STYLE_LIST_STYLE_NONE:
    break;

  case NS_STYLE_LIST_STYLE_DISC:
    aRenderingContext.FillEllipse(mPadding.left + aPt.x, mPadding.top + aPt.y,
                                  mRect.width - (mPadding.left + mPadding.right),
                                  mRect.height - (mPadding.top + mPadding.bottom));
    break;

  case NS_STYLE_LIST_STYLE_CIRCLE:
    aRenderingContext.DrawEllipse(mPadding.left + aPt.x, mPadding.top + aPt.y,
                                  mRect.width - (mPadding.left + mPadding.right),
                                  mRect.height - (mPadding.top + mPadding.bottom));
    break;

  case NS_STYLE_LIST_STYLE_SQUARE:
    {
      nsRect rect(aPt, mRect.Size());
      rect.Deflate(mPadding);

      
      
      
      
      
      
      nsPresContext *pc = PresContext();
      nsRect snapRect(rect.x, rect.y, 
                      pc->RoundAppUnitsToNearestDevPixels(rect.width),
                      pc->RoundAppUnitsToNearestDevPixels(rect.height));
      snapRect.MoveBy((rect.width - snapRect.width) / 2,
                      (rect.height - snapRect.height) / 2);
      aRenderingContext.FillRect(snapRect.x, snapRect.y,
                                 snapRect.width, snapRect.height);
    }
    break;

  default:
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                          GetFontSizeInflation());
    GetListItemText(text);
    aRenderingContext.SetFont(fm);
    nscoord ascent = fm->MaxAscent();
    aPt.MoveBy(mPadding.left, mPadding.top);
    aPt.y = NSToCoordRound(nsLayoutUtils::GetSnappedBaselineY(
            this, aRenderingContext.ThebesContext(), aPt.y, ascent));
    nsPresContext* presContext = PresContext();
    if (!presContext->BidiEnabled() && HasRTLChars(text)) {
      presContext->SetBidiEnabled();
    }
    nsLayoutUtils::DrawString(this, &aRenderingContext,
                              text.get(), text.Length(), aPt);
    break;
  }
}

int32_t
nsBulletFrame::SetListItemOrdinal(int32_t aNextOrdinal,
                                  bool* aChanged,
                                  int32_t aIncrement)
{
  MOZ_ASSERT(aIncrement == 1 || aIncrement == -1,
             "We shouldn't have weird increments here");

  
  int32_t oldOrdinal = mOrdinal;
  mOrdinal = aNextOrdinal;

  
  
  
  nsIContent* parentContent = GetParent()->GetContent();
  if (parentContent) {
    nsGenericHTMLElement *hc =
      nsGenericHTMLElement::FromContent(parentContent);
    if (hc) {
      const nsAttrValue* attr = hc->GetParsedAttr(nsGkAtoms::value);
      if (attr && attr->Type() == nsAttrValue::eInteger) {
        
        mOrdinal = attr->GetIntegerValue();
      }
    }
  }

  *aChanged = oldOrdinal != mOrdinal;

  return nsCounterManager::IncrementCounter(mOrdinal, aIncrement);
}







static bool DecimalToText(int32_t ordinal, nsString& result)
{
   char cbuf[40];
   PR_snprintf(cbuf, sizeof(cbuf), "%ld", ordinal);
   result.AppendASCII(cbuf);
   return true;
}
static bool TamilToText(int32_t ordinal,  nsString& result)
{
   if (ordinal < 1 || ordinal > 9999) {
     
     return false;
   }
   char16_t diff = 0x0BE6 - char16_t('0');
   
   
   
   size_t offset = result.Length();
   DecimalToText(ordinal, result); 
   char16_t* p = result.BeginWriting() + offset;
   for(; '\0' != *p ; p++) 
      if(*p != char16_t('0'))
         *p += diff;
   return true;
}






#define NUM_BUF_SIZE 34 

enum CJKIdeographicLang {
  CHINESE, KOREAN, JAPANESE
};
struct CJKIdeographicData {
  const char16_t *negative;
  char16_t digit[10];
  char16_t unit[3];
  char16_t unit10K[2];
  uint8_t lang;
  bool informal;
};
extern const char16_t gJapaneseNegative[] = {
  0x30de, 0x30a4, 0x30ca, 0x30b9, 0x0000
};
static const CJKIdeographicData gDataJapaneseInformal = {
  gJapaneseNegative,          
  {                           
    0x3007, 0x4e00, 0x4e8c, 0x4e09, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x5341, 0x767e, 0x5343 }, 
  { 0x4e07, 0x5104 },         
  JAPANESE,                   
  true                        
};
static const CJKIdeographicData gDataJapaneseFormal = {
  gJapaneseNegative,          
  {                           
    0x96f6, 0x58f1, 0x5f10, 0x53c2, 0x56db,
    0x4f0d, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x62fe, 0x767e, 0x9621 }, 
  { 0x842c, 0x5104 },         
  JAPANESE,                   
  false                       
};
extern const char16_t gKoreanNegative[] = {
  0xb9c8, 0xc774, 0xb108, 0xc2a4, 0x0020, 0x0000
};
static const CJKIdeographicData gDataKoreanHangulFormal = {
  gKoreanNegative,            
  {                           
    0xc601, 0xc77c, 0xc774, 0xc0bc, 0xc0ac,
    0xc624, 0xc721, 0xce60, 0xd314, 0xad6c
  },
  { 0xc2ed, 0xbc31, 0xcc9c }, 
  { 0xb9cc, 0xc5b5 },         
  KOREAN,                     
  false                       
};
static const CJKIdeographicData gDataKoreanHanjaInformal = {
  gKoreanNegative,            
  {                           
    0x96f6, 0x4e00, 0x4e8c, 0x4e09, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x5341, 0x767e, 0x5343 }, 
  { 0x842c, 0x5104 },         
  KOREAN,                     
  true                        
};
static const CJKIdeographicData gDataKoreanHanjaFormal = {
  gKoreanNegative,            
  {                           
    0x96f6, 0x58f9, 0x8cb3, 0x53c3, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x62fe, 0x767e, 0x4edf }, 
  { 0x842c, 0x5104 },         
  KOREAN,                     
  false                       
};
extern const char16_t gSimpChineseNegative[] = {
  0x8d1f, 0x0000
};
static const CJKIdeographicData gDataSimpChineseInformal = {
  gSimpChineseNegative,       
  {                           
    0x96f6, 0x4e00, 0x4e8c, 0x4e09, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x5341, 0x767e, 0x5343 }, 
  { 0x4e07, 0x4ebf },         
  CHINESE,                    
  true                        
};
static const CJKIdeographicData gDataSimpChineseFormal = {
  gSimpChineseNegative,       
  {                           
    0x96f6, 0x58f9, 0x8d30, 0x53c1, 0x8086,
    0x4f0d, 0x9646, 0x67d2, 0x634c, 0x7396
  },
  { 0x62fe, 0x4f70, 0x4edf }, 
  { 0x4e07, 0x4ebf },         
  CHINESE,                    
  false                       
};
extern const char16_t gTradChineseNegative[] = {
  0x8ca0, 0x0000
};
static const CJKIdeographicData gDataTradChineseInformal = {
  gTradChineseNegative,       
  {                           
    0x96f6, 0x4e00, 0x4e8c, 0x4e09, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x5341, 0x767e, 0x5343 }, 
  { 0x842c, 0x5104 },         
  CHINESE,                    
  true                        
};
static const CJKIdeographicData gDataTradChineseFormal = {
  gTradChineseNegative,       
  {                           
    0x96f6, 0x58f9, 0x8cb3, 0x53c3, 0x8086,
    0x4f0d, 0x9678, 0x67d2, 0x634c, 0x7396
  },
  { 0x62fe, 0x4f70, 0x4edf }, 
  { 0x842c, 0x5104 },         
  CHINESE,                    
  false                       
};

static const bool CJKIdeographicToText(int32_t aOrdinal, nsString& result,
                                       const CJKIdeographicData& data)
{
  char16_t buf[NUM_BUF_SIZE];
  int32_t idx = NUM_BUF_SIZE;
  int32_t pos = 0;
  bool isNegative = (aOrdinal < 0);
  bool needZero = (aOrdinal == 0);
  int32_t unitidx = 0, unit10Kidx = 0;
  uint32_t ordinal = mozilla::Abs(aOrdinal);
  do {
    unitidx = pos % 4;
    if (unitidx == 0) {
      unit10Kidx = pos / 4;
    }
    int32_t cur = ordinal % 10;
    if (cur == 0) {
      if (needZero) {
        needZero = false;
        buf[--idx] = data.digit[0];
      }
    } else {
      if (data.lang == CHINESE) {
        needZero = true;
      }
      if (unit10Kidx != 0) {
        if (data.lang == KOREAN && idx != NUM_BUF_SIZE) {
          buf[--idx] = ' ';
        }
        buf[--idx] = data.unit10K[unit10Kidx - 1];
      }
      if (unitidx != 0) {
        buf[--idx] = data.unit[unitidx - 1];
      }
      if (cur != 1) {
        buf[--idx] = data.digit[cur];
      } else {
        bool needOne = true;
        if (data.informal) {
          switch (data.lang) {
            case CHINESE:
              if (unitidx == 1 &&
                  (ordinal == 1 || (pos > 4 && ordinal % 1000 == 1))) {
                needOne = false;
              }
              break;
            case JAPANESE:
              if (unitidx > 0 &&
                  (unitidx != 3 || (pos == 3 && ordinal == 1))) {
                needOne = false;
              }
              break;
            case KOREAN:
              if (unitidx > 0 || (pos == 4 && (ordinal % 1000) == 1)) {
                needOne = false;
              }
              break;
          }
        }
        if (needOne) {
          buf[--idx] = data.digit[1];
        }
      }
      unit10Kidx = 0;
    }
    ordinal /= 10;
    pos++;
  } while (ordinal > 0);
  if (isNegative) {
    result.Append(data.negative);
  }
  result.Append(buf + idx, NUM_BUF_SIZE - idx);
  return true;
}

#define HEBREW_GERESH       0x05F3
static const char16_t gHebrewDigit[22] = 
{

0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8,

0x05D9, 0x05DB, 0x05DC, 0x05DE, 0x05E0, 0x05E1, 0x05E2, 0x05E4, 0x05E6,

0x05E7, 0x05E8, 0x05E9, 0x05EA
};

static bool HebrewToText(int32_t ordinal, nsString& result)
{
  if (ordinal < 1 || ordinal > 999999) {
    return false;
  }
  bool outputSep = false;
  nsAutoString allText, thousandsGroup;
  do {
    thousandsGroup.Truncate();
    int32_t n3 = ordinal % 1000;
    
    for(int32_t n1 = 400; n1 > 0; )
    {
      if( n3 >= n1)
      {
        n3 -= n1;
        thousandsGroup.Append(gHebrewDigit[(n1/100)-1+18]);
      } else {
        n1 -= 100;
      } 
    } 

    
    int32_t n2;
    if( n3 >= 10 )
    {
      
      if(( 15 == n3 ) || (16 == n3)) {
        
        
        
        n2 = 9;
        thousandsGroup.Append(gHebrewDigit[ n2 - 1]);
      } else {
        n2 = n3 - (n3 % 10);
        thousandsGroup.Append(gHebrewDigit[(n2/10)-1+9]);
      } 
      n3 -= n2;
    } 
  
    
    if ( n3 > 0)
      thousandsGroup.Append(gHebrewDigit[n3-1]);
    if (outputSep) 
      thousandsGroup.Append((char16_t)HEBREW_GERESH);
    if (allText.IsEmpty())
      allText = thousandsGroup;
    else
      allText = thousandsGroup + allText;
    ordinal /= 1000;
    outputSep = true;
  } while (ordinal >= 1);

  result.Append(allText);
  return true;
}






#define ETHIOPIC_ONE             0x1369
#define ETHIOPIC_TEN             0x1372
#define ETHIOPIC_HUNDRED         0x137B
#define ETHIOPIC_TEN_THOUSAND    0x137C

static bool EthiopicToText(int32_t ordinal, nsString& result)
{
  if (ordinal < 1) {
    return false;
  }
  nsAutoString asciiNumberString;      
  DecimalToText(ordinal, asciiNumberString);
  uint8_t asciiStringLength = asciiNumberString.Length();

  
  
  
  
  
  if (asciiStringLength & 1) {
    asciiNumberString.Insert(NS_LITERAL_STRING("0"), 0);
  } else {
    asciiStringLength--;
  }

  
  
  
  for (uint8_t indexFromLeft = 0, groupIndexFromRight = asciiStringLength >> 1;
       indexFromLeft <= asciiStringLength;
       indexFromLeft += 2, groupIndexFromRight--) {
    uint8_t tensValue  = asciiNumberString.CharAt(indexFromLeft) & 0x0F;
    uint8_t unitsValue = asciiNumberString.CharAt(indexFromLeft + 1) & 0x0F;
    uint8_t groupValue = tensValue * 10 + unitsValue;

    bool oddGroup = (groupIndexFromRight & 1);

    
    if (ordinal > 1 &&
        groupValue == 1 &&                  
        (oddGroup || indexFromLeft == 0)) { 
      unitsValue = 0;
    }

    
    if (tensValue) {
      
      result.Append((char16_t) (tensValue +  ETHIOPIC_TEN - 1));
    }
    if (unitsValue) {
      
      result.Append((char16_t) (unitsValue + ETHIOPIC_ONE - 1));
    }
    
    
    if (oddGroup) {
      if (groupValue) {
        result.Append((char16_t) ETHIOPIC_HUNDRED);
      }
    } else {
      if (groupIndexFromRight) {
        result.Append((char16_t) ETHIOPIC_TEN_THOUSAND);
      }
    }
  }
  return true;
}


 void
nsBulletFrame::AppendCounterText(int32_t aListStyleType,
                                 int32_t aOrdinal,
                                 nsString& result,
                                 bool& isRTL)
{
  bool success = true;
  int32_t fallback = NS_STYLE_LIST_STYLE_DECIMAL;
  isRTL = false;
  
  switch (aListStyleType) {
    case NS_STYLE_LIST_STYLE_NONE: 
      break;

    case NS_STYLE_LIST_STYLE_DISC: 
      
      result.Append(char16_t(0x2022));
      break;

    case NS_STYLE_LIST_STYLE_CIRCLE: 
      
      result.Append(char16_t(0x25E6));
      break;

    case NS_STYLE_LIST_STYLE_SQUARE: 
      
      result.Append(char16_t(0x25FE));
      break;

    case NS_STYLE_LIST_STYLE_DECIMAL:
    default: 
      
      success = DecimalToText(aOrdinal, result);
      NS_ASSERTION(success, "DecimalToText must never fail");
      break;

    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
      success =
        CJKIdeographicToText(aOrdinal, result, gDataTradChineseInformal);
      break;

    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
      success = CJKIdeographicToText(aOrdinal, result, gDataTradChineseFormal);
      break;

    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
      success =
        CJKIdeographicToText(aOrdinal, result, gDataSimpChineseInformal);
      break;

    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
      success = CJKIdeographicToText(aOrdinal, result, gDataSimpChineseFormal);
      break;

    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
      success = CJKIdeographicToText(aOrdinal, result, gDataJapaneseInformal);
      break;

    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
      success = CJKIdeographicToText(aOrdinal, result, gDataJapaneseFormal);
      break;

    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
      success =
        CJKIdeographicToText(aOrdinal, result, gDataKoreanHangulFormal);
      break;

    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
      success =
        CJKIdeographicToText(aOrdinal, result, gDataKoreanHanjaInformal);
      break;

    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
      success = CJKIdeographicToText(aOrdinal, result, gDataKoreanHanjaFormal);
      break;

    case NS_STYLE_LIST_STYLE_HEBREW: 
      isRTL = true;
      success = HebrewToText(aOrdinal, result);
      break;
 
    case NS_STYLE_LIST_STYLE_MOZ_TAMIL:
      success = TamilToText(aOrdinal, result);
      break;

    case NS_STYLE_LIST_STYLE_MOZ_ETHIOPIC_NUMERIC:
      success = EthiopicToText(aOrdinal, result);
      break;
  }
  if (!success) {
    AppendCounterText(fallback, aOrdinal, result, isRTL);
  }
}

 void
nsBulletFrame::GetListItemSuffix(int32_t aListStyleType,
                                 nsString& aResult)
{
  aResult.AssignLiteral(MOZ_UTF16(". "));

  switch (aListStyleType) {
    case NS_STYLE_LIST_STYLE_NONE: 
    case NS_STYLE_LIST_STYLE_DISC: 
    case NS_STYLE_LIST_STYLE_CIRCLE: 
    case NS_STYLE_LIST_STYLE_SQUARE: 
      aResult = ' ';
      break;

    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
      aResult = 0x3001;
      break;

    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
      aResult.AssignLiteral(MOZ_UTF16(", "));
      break;
  }
}

void
nsBulletFrame::GetListItemText(nsAString& aResult)
{
  CounterStyle* style = StyleList()->GetCounterStyle();
  NS_ASSERTION(style->GetStyle() != NS_STYLE_LIST_STYLE_NONE &&
               style->GetStyle() != NS_STYLE_LIST_STYLE_DISC &&
               style->GetStyle() != NS_STYLE_LIST_STYLE_CIRCLE &&
               style->GetStyle() != NS_STYLE_LIST_STYLE_SQUARE,
               "we should be using specialized code for these types");

  bool isRTL;
  nsAutoString counter, prefix, suffix;
  style->GetPrefix(prefix);
  style->GetSuffix(suffix);
  style->GetCounterText(mOrdinal, counter, isRTL);

  aResult.Truncate();
  aResult.Append(prefix);
  if (GetWritingMode().IsBidiLTR() != isRTL) {
    aResult.Append(counter);
  } else {
    
    char16_t mark = isRTL ? 0x200f : 0x200e;
    aResult.Append(mark);
    aResult.Append(counter);
    aResult.Append(mark);
  }
  aResult.Append(suffix);
}

#define MIN_BULLET_SIZE 1


void
nsBulletFrame::GetDesiredSize(nsPresContext*  aCX,
                              nsRenderingContext *aRenderingContext,
                              nsHTMLReflowMetrics& aMetrics,
                              float aFontSizeInflation)
{
  
  mPadding.SizeTo(0, 0, 0, 0);
  
  const nsStyleList* myList = StyleList();
  nscoord ascent;
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                        aFontSizeInflation);

  RemoveStateBits(BULLET_FRAME_IMAGE_LOADING);

  if (myList->GetListStyleImage() && mImageRequest) {
    uint32_t status;
    mImageRequest->GetImageStatus(&status);
    if (status & imgIRequest::STATUS_SIZE_AVAILABLE &&
        !(status & imgIRequest::STATUS_ERROR)) {
      
      aMetrics.Width() = mIntrinsicSize.width;
      aMetrics.SetBlockStartAscent(aMetrics.Height() = mIntrinsicSize.height);

      
      nscoord halfEm = fm->EmHeight() / 2;
      WritingMode wm = GetWritingMode();
      if (wm.IsVertical()) {
        mPadding.bottom += halfEm;
      } else if (wm.IsBidiLTR()) {
        mPadding.right += halfEm;
      } else {
        mPadding.left += halfEm;
      }

      AddStateBits(BULLET_FRAME_IMAGE_LOADING);

      return;
    }
  }

  
  
  
  
  
  
  mIntrinsicSize.SizeTo(0, 0);

  nscoord bulletSize;

  nsAutoString text;
  switch (myList->GetCounterStyle()->GetStyle()) {
    case NS_STYLE_LIST_STYLE_NONE:
      aMetrics.Width() = aMetrics.Height() = 0;
      aMetrics.SetBlockStartAscent(0);
      break;

    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE: {
      ascent = fm->MaxAscent();
      bulletSize = std::max(nsPresContext::CSSPixelsToAppUnits(MIN_BULLET_SIZE),
                          NSToCoordRound(0.8f * (float(ascent) / 2.0f)));
      mPadding.bottom = NSToCoordRound(float(ascent) / 8.0f);
      aMetrics.Width() = aMetrics.Height() = bulletSize;
      aMetrics.SetBlockStartAscent(bulletSize + mPadding.bottom);

      
      nscoord halfEm = fm->EmHeight() / 2;
      WritingMode wm = GetWritingMode();
      if (wm.IsVertical()) {
        mPadding.bottom += halfEm;
      } else if (wm.IsBidiLTR()) {
        mPadding.right += halfEm;
      } else {
        mPadding.left += halfEm;
      }
      break;
    }

    default:
      GetListItemText(text);
      aMetrics.Height() = fm->MaxHeight();
      aRenderingContext->SetFont(fm);
      aMetrics.Width() =
        nsLayoutUtils::GetStringWidth(this, aRenderingContext,
                                      text.get(), text.Length());
      aMetrics.SetBlockStartAscent(fm->MaxAscent());
      break;
  }
}

void
nsBulletFrame::Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsBulletFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);

  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  SetFontSizeInflation(inflation);

  
  GetDesiredSize(aPresContext, aReflowState.rendContext, aMetrics, inflation);

  
  
  const nsMargin& borderPadding = aReflowState.ComputedPhysicalBorderPadding();
  mPadding.top += NSToCoordRound(borderPadding.top * inflation);
  mPadding.right += NSToCoordRound(borderPadding.right * inflation);
  mPadding.bottom += NSToCoordRound(borderPadding.bottom * inflation);
  mPadding.left += NSToCoordRound(borderPadding.left * inflation);
  aMetrics.Width() += mPadding.left + mPadding.right;
  aMetrics.Height() += mPadding.top + mPadding.bottom;
  aMetrics.SetBlockStartAscent(aMetrics.BlockStartAscent() + mPadding.top);

  
  
  
  
  aMetrics.SetOverflowAreasToDesiredBounds();

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
}

 nscoord
nsBulletFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nsHTMLReflowMetrics metrics(GetWritingMode());
  DISPLAY_MIN_WIDTH(this, metrics.Width());
  GetDesiredSize(PresContext(), aRenderingContext, metrics, 1.0f);
  return metrics.Width();
}

 nscoord
nsBulletFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nsHTMLReflowMetrics metrics(GetWritingMode());
  DISPLAY_PREF_WIDTH(this, metrics.Width());
  GetDesiredSize(PresContext(), aRenderingContext, metrics, 1.0f);
  return metrics.Width();
}

NS_IMETHODIMP
nsBulletFrame::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    nsCOMPtr<imgIContainer> image;
    aRequest->GetImage(getter_AddRefs(image));
    return OnStartContainer(aRequest, image);
  }

  if (aType == imgINotificationObserver::FRAME_UPDATE) {
    
    
    
    InvalidateFrame();
  }

  if (aType == imgINotificationObserver::IS_ANIMATED) {
    
    
    if (aRequest == mImageRequest) {
      nsLayoutUtils::RegisterImageRequest(PresContext(), mImageRequest,
                                          &mRequestRegistered);
    }
  }

  return NS_OK;
}

nsresult nsBulletFrame::OnStartContainer(imgIRequest *aRequest,
                                         imgIContainer *aImage)
{
  if (!aImage) return NS_ERROR_INVALID_ARG;
  if (!aRequest) return NS_ERROR_INVALID_ARG;

  uint32_t status;
  aRequest->GetImageStatus(&status);
  if (status & imgIRequest::STATUS_ERROR) {
    return NS_OK;
  }
  
  nscoord w, h;
  aImage->GetWidth(&w);
  aImage->GetHeight(&h);

  nsPresContext* presContext = PresContext();

  nsSize newsize(nsPresContext::CSSPixelsToAppUnits(w),
                 nsPresContext::CSSPixelsToAppUnits(h));

  if (mIntrinsicSize != newsize) {
    mIntrinsicSize = newsize;

    
    
    nsIPresShell *shell = presContext->GetPresShell();
    if (shell) {
      shell->FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                              NS_FRAME_IS_DIRTY);
    }
  }

  
  aImage->SetAnimationMode(presContext->ImageAnimationMode());
  
  
  
  aRequest->IncrementAnimationConsumers();
  
  return NS_OK;
}

void
nsBulletFrame::GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup)
{
  if (!aPresContext)
    return;

  NS_PRECONDITION(nullptr != aLoadGroup, "null OUT parameter pointer");

  nsIPresShell *shell = aPresContext->GetPresShell();

  if (!shell)
    return;

  nsIDocument *doc = shell->GetDocument();
  if (!doc)
    return;

  *aLoadGroup = doc->GetDocumentLoadGroup().take();
}

union VoidPtrOrFloat {
  VoidPtrOrFloat() : p(nullptr) {}

  void *p;
  float f;
};

float
nsBulletFrame::GetFontSizeInflation() const
{
  if (!HasFontSizeInflation()) {
    return 1.0f;
  }
  VoidPtrOrFloat u;
  u.p = Properties().Get(FontSizeInflationProperty());
  return u.f;
}

void
nsBulletFrame::SetFontSizeInflation(float aInflation)
{
  if (aInflation == 1.0f) {
    if (HasFontSizeInflation()) {
      RemoveStateBits(BULLET_FRAME_HAS_FONT_INFLATION);
      Properties().Delete(FontSizeInflationProperty());
    }
    return;
  }

  AddStateBits(BULLET_FRAME_HAS_FONT_INFLATION);
  VoidPtrOrFloat u;
  u.f = aInflation;
  Properties().Set(FontSizeInflationProperty(), u.p);
}

already_AddRefed<imgIContainer>
nsBulletFrame::GetImage() const
{
  if (mImageRequest && StyleList()->GetListStyleImage()) {
    nsCOMPtr<imgIContainer> imageCon;
    mImageRequest->GetImage(getter_AddRefs(imageCon));
    return imageCon.forget();
  }

  return nullptr;
}

nscoord
nsBulletFrame::GetBaseline() const
{
  nscoord ascent = 0, bottomPadding;
  if (GetStateBits() & BULLET_FRAME_IMAGE_LOADING) {
    ascent = GetRect().height;
  } else {
    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                          GetFontSizeInflation());
    CounterStyle* listStyleType = StyleList()->GetCounterStyle();
    switch (listStyleType->GetStyle()) {
      case NS_STYLE_LIST_STYLE_NONE:
        break;

      case NS_STYLE_LIST_STYLE_DISC:
      case NS_STYLE_LIST_STYLE_CIRCLE:
      case NS_STYLE_LIST_STYLE_SQUARE:
        ascent = fm->MaxAscent();
        bottomPadding = NSToCoordRound(float(ascent) / 8.0f);
        ascent = std::max(nsPresContext::CSSPixelsToAppUnits(MIN_BULLET_SIZE),
                        NSToCoordRound(0.8f * (float(ascent) / 2.0f)));
        ascent += bottomPadding;
        break;

      default:
        ascent = fm->MaxAscent();
        break;
    }
  }
  return ascent + GetUsedBorderAndPadding().top;
}

void
nsBulletFrame::GetSpokenText(nsAString& aText)
{
  CounterStyle* style = StyleList()->GetCounterStyle();
  bool isBullet;
  style->GetSpokenCounterText(mOrdinal, aText, isBullet);
  if (isBullet) {
    aText.Append(' ');
  } else {
    nsAutoString prefix, suffix;
    style->GetPrefix(prefix);
    style->GetSuffix(suffix);
    aText = prefix + aText + suffix;
  }
}








NS_IMPL_ISUPPORTS(nsBulletListener, imgINotificationObserver)

nsBulletListener::nsBulletListener() :
  mFrame(nullptr)
{
}

nsBulletListener::~nsBulletListener()
{
}

NS_IMETHODIMP
nsBulletListener::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;
  return mFrame->Notify(aRequest, aType, aData);
}
