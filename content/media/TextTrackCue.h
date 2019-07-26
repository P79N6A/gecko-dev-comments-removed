





#ifndef mozilla_dom_TextTrackCue_h
#define mozilla_dom_TextTrackCue_h

#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/dom/VTTCueBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIWebVTTParserWrapper.h"
#include "mozilla/StaticPtr.h"

namespace mozilla {
namespace dom {

class HTMLTrackElement;
class TextTrack;

class TextTrackCue MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TextTrackCue, nsDOMEventTargetHelper)

  
  
  static already_AddRefed<TextTrackCue>
  Constructor(GlobalObject& aGlobal,
              double aStartTime,
              double aEndTime,
              const nsAString& aText,
              ErrorResult& aRv)
  {
    nsRefPtr<TextTrackCue> ttcue = new TextTrackCue(aGlobal.GetAsSupports(), aStartTime,
                                                    aEndTime, aText, aRv);
    return ttcue.forget();
  }
  TextTrackCue(nsISupports* aGlobal, double aStartTime, double aEndTime,
               const nsAString& aText, ErrorResult& aRv);

  TextTrackCue(nsISupports* aGlobal, double aStartTime, double aEndTime,
               const nsAString& aText, HTMLTrackElement* aTrackElement,
               ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsINode* GetParentObject()
  {
    return mDocument;
  }

  TextTrack* GetTrack() const
  {
    return mTrack;
  }

  void GetId(nsAString& aId) const
  {
    aId = mId;
  }

  void SetId(const nsAString& aId)
  {
    if (mId == aId) {
      return;
    }

    mId = aId;
    CueChanged();
  }

  double StartTime() const
  {
    return mStartTime;
  }

  void SetStartTime(double aStartTime)
  {
    if (mStartTime == aStartTime)
      return;

    mStartTime = aStartTime;
    CueChanged();
  }

  double EndTime() const
  {
    return mEndTime;
  }

  void SetEndTime(double aEndTime)
  {
    if (mEndTime == aEndTime)
      return;

    mEndTime = aEndTime;
    CueChanged();
  }

  bool PauseOnExit()
  {
    return mPauseOnExit;
  }

  void SetPauseOnExit(bool aPauseOnExit)
  {
    if (mPauseOnExit == aPauseOnExit)
      return;

    mPauseOnExit = aPauseOnExit;
    CueChanged();
  }

  void GetRegionId(nsAString& aRegionId) const
  {
    aRegionId = mRegionId;
  }

  void SetRegionId(const nsAString& aRegionId)
  {
    if (mRegionId == aRegionId) {
      return;
    }

    mRegionId = aRegionId;
    CueChanged();
  }

  DirectionSetting Vertical() const
  {
    return mVertical;
  }

  void SetVertical(const DirectionSetting& aVertical)
  {
    if (mVertical == aVertical)
      return;

    mReset = true;
    mVertical = aVertical;
    CueChanged();
  }

  bool SnapToLines()
  {
    return mSnapToLines;
  }

  void SetSnapToLines(bool aSnapToLines)
  {
    if (mSnapToLines == aSnapToLines)
      return;

    mReset = true;
    mSnapToLines = aSnapToLines;
    CueChanged();
  }

  double Line() const
  {
    return mLine;
  }

  void SetLine(double aLine)
  {
    
    mReset = true;
    mLine = aLine;
  }

  int32_t Position() const
  {
    return mPosition;
  }

  void SetPosition(int32_t aPosition, ErrorResult& aRv)
  {
    
    if (mPosition == aPosition)
      return;

    if (aPosition > 100 || aPosition < 0){
      aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
      return;
    }

    mReset = true;
    mPosition = aPosition;
    CueChanged();
  }

  int32_t Size() const
  {
    return mSize;
  }

  void SetSize(int32_t aSize, ErrorResult& aRv)
  {
    if (mSize == aSize) {
      return;
    }

    if (aSize < 0 || aSize > 100) {
      aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
      return;
    }

    mReset = true;
    mSize = aSize;
    CueChanged();
  }

  TextTrackCueAlign Align() const
  {
    return mAlign;
  }

  void SetAlign(TextTrackCueAlign& aAlign)
  {
    if (mAlign == aAlign)
      return;

    mReset = true;
    mAlign = aAlign;
    CueChanged();
  }

  void GetText(nsAString& aText) const
  {
    aText = mText;
  }

  void SetText(const nsAString& aText)
  {
    if (mText == aText)
      return;

    mReset = true;
    mText = aText;
    CueChanged();
  }

  IMPL_EVENT_HANDLER(enter)
  IMPL_EVENT_HANDLER(exit)

  
  bool
  operator==(const TextTrackCue& rhs) const
  {
    return mId.Equals(rhs.mId);
  }

  const nsAString& Id() const
  {
    return mId;
  }

  























  



  void RenderCue();

  






  already_AddRefed<DocumentFragment> GetCueAsHTML();

  void SetTrackElement(HTMLTrackElement* aTrackElement);

private:
  void CueChanged();
  void SetDefaultCueSettings();
  void CreateCueOverlay();
  nsresult StashDocument(nsISupports* aGlobal);

  nsRefPtr<nsIDocument> mDocument;
  nsString mText;
  double mStartTime;
  double mEndTime;

  nsRefPtr<TextTrack> mTrack;
  nsRefPtr<HTMLTrackElement> mTrackElement;
  nsString mId;
  int32_t mPosition;
  int32_t mSize;
  bool mPauseOnExit;
  bool mSnapToLines;
  nsString mRegionId;
  DirectionSetting mVertical;
  int mLine;
  TextTrackCueAlign mAlign;

  
  
  nsCOMPtr<nsIContent> mDisplayState;
  
  
  
  bool mReset;

  static StaticRefPtr<nsIWebVTTParserWrapper> sParserWrapper;
};

} 
} 

#endif 
