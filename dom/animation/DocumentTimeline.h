





#ifndef mozilla_dom_DocumentTimeline_h
#define mozilla_dom_DocumentTimeline_h

#include "mozilla/TimeStamp.h"
#include "AnimationTimeline.h"
#include "nsIDocument.h"
#include "nsRefreshDriver.h"

struct JSContext;

namespace mozilla {
namespace dom {

class DocumentTimeline final : public AnimationTimeline
{
public:
  explicit DocumentTimeline(nsIDocument* aDocument)
    : AnimationTimeline(aDocument->GetParentObject())
    , mDocument(aDocument)
  {
  }

protected:
  virtual ~DocumentTimeline() { }

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(DocumentTimeline,
                                                         AnimationTimeline)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  
  virtual Nullable<TimeDuration> GetCurrentTime() const override;

  bool TracksWallclockTime() const override
  {
    nsRefreshDriver* refreshDriver = GetRefreshDriver();
    return !refreshDriver ||
           !refreshDriver->IsTestControllingRefreshesEnabled();
  }
  Nullable<TimeDuration> ToTimelineTime(const TimeStamp& aTimeStamp) const
                                                                     override;
  TimeStamp ToTimeStamp(const TimeDuration& aTimelineTime) const override;

protected:
  TimeStamp GetCurrentTimeStamp() const;
  nsRefreshDriver* GetRefreshDriver() const;

  nsCOMPtr<nsIDocument> mDocument;

  
  
  
  mutable TimeStamp mLastRefreshDriverTime;
};

} 
} 

#endif 
