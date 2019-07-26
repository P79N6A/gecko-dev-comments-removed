





#ifndef mozilla_dom_TextTrackCue_h
#define mozilla_dom_TextTrackCue_h

#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/dom/TextTrack.h"
#include "mozilla/dom/TextTrackCueBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDocument.h"

struct webvtt_node;

namespace mozilla {
namespace dom {

class HTMLTrackElement;
class TextTrack;

class TextTrackCue MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(TextTrackCue,
                                                         nsDOMEventTargetHelper)

  
  
  static already_AddRefed<TextTrackCue>
  Constructor(GlobalObject& aGlobal,
              double aStartTime,
              double aEndTime,
              const nsAString& aText,
              ErrorResult& aRv)
  {
    nsRefPtr<TextTrackCue> ttcue = new TextTrackCue(aGlobal.Get(), aStartTime,
                                                    aEndTime, aText, aRv);
    return ttcue.forget();
  }
  TextTrackCue(nsISupports* aGlobal, double aStartTime, double aEndTime,
               const nsAString& aText, ErrorResult& aRv);

  TextTrackCue(nsISupports* aGlobal, double aStartTime, double aEndTime,
               const nsAString& aText, HTMLTrackElement* aTrackElement,
               webvtt_node* head, ErrorResult& aRv);

  ~TextTrackCue();

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsISupports* GetParentObject()
  {
    return mGlobal;
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

  void GetVertical(nsAString& aVertical)
  {
    aVertical = mVertical;
  }

  void SetVertical(const nsAString& aVertical)
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

  void SetPosition(int32_t aPosition)
  {
    
    if (mPosition == aPosition)
      return;

    mReset = true;
    mPosition = aPosition;
    CueChanged();
  }

  int32_t Size() const
  {
    return mSize;
  }

  void SetSize(int32_t aSize)
  {
    if (mSize == aSize) {
      return;
    }

    if (aSize < 0 || aSize > 100) {
      
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

  already_AddRefed<DocumentFragment> GetCueAsHTML() const
  {
    
    return nullptr;
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

  








  void
  ConvertNodeTreeToDOMTree(nsIContent* aParentContent);

  



  already_AddRefed<nsIContent>
  ConvertInternalNodeToContent(const webvtt_node* aWebVTTNode);

  



  already_AddRefed<nsIContent>
  ConvertLeafNodeToContent(const webvtt_node* aWebVTTNode);

private:
  void CueChanged();
  void SetDefaultCueSettings();
  void CreateCueOverlay();
  nsresult StashDocument();

  nsCOMPtr<nsISupports> mGlobal;
  nsRefPtr<nsIDocument> mDocument;
  nsString mText;
  double mStartTime;
  double mEndTime;

  nsRefPtr<TextTrack> mTrack;
  nsRefPtr<HTMLTrackElement> mTrackElement;
  webvtt_node *mHead;
  nsString mId;
  int32_t mPosition;
  int32_t mSize;
  bool mPauseOnExit;
  bool mSnapToLines;
  nsString mVertical;
  int mLine;
  TextTrackCueAlign mAlign;

  
  
  nsCOMPtr<nsIContent> mDisplayState;
  
  
  
  bool mReset;
};

} 
} 

#endif 
