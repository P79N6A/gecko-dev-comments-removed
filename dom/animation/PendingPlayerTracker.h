




#ifndef mozilla_dom_PendingPlayerTracker_h
#define mozilla_dom_PendingPlayerTracker_h

#include "mozilla/dom/Animation.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDocument.h"
#include "nsTHashtable.h"

class nsIFrame;

namespace mozilla {

class PendingPlayerTracker final
{
public:
  explicit PendingPlayerTracker(nsIDocument* aDocument)
    : mDocument(aDocument)
  { }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(PendingPlayerTracker)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(PendingPlayerTracker)

  void AddPlayPending(dom::Animation& aPlayer)
  {
    MOZ_ASSERT(!IsWaitingToPause(aPlayer),
               "Player is already waiting to pause");
    AddPending(aPlayer, mPlayPendingSet);
  }
  void RemovePlayPending(dom::Animation& aPlayer)
  {
    RemovePending(aPlayer, mPlayPendingSet);
  }
  bool IsWaitingToPlay(const dom::Animation& aPlayer) const
  {
    return IsWaiting(aPlayer, mPlayPendingSet);
  }

  void AddPausePending(dom::Animation& aPlayer)
  {
    MOZ_ASSERT(!IsWaitingToPlay(aPlayer),
               "Player is already waiting to play");
    AddPending(aPlayer, mPausePendingSet);
  }
  void RemovePausePending(dom::Animation& aPlayer)
  {
    RemovePending(aPlayer, mPausePendingSet);
  }
  bool IsWaitingToPause(const dom::Animation& aPlayer) const
  {
    return IsWaiting(aPlayer, mPausePendingSet);
  }

  void TriggerPendingPlayersOnNextTick(const TimeStamp& aReadyTime);
  void TriggerPendingPlayersNow();
  bool HasPendingPlayers() const {
    return mPlayPendingSet.Count() > 0 || mPausePendingSet.Count() > 0;
  }

private:
  ~PendingPlayerTracker() { }

  void EnsurePaintIsScheduled();

  typedef nsTHashtable<nsRefPtrHashKey<dom::Animation>>
    AnimationPlayerSet;

  void AddPending(dom::Animation& aPlayer,
                  AnimationPlayerSet& aSet);
  void RemovePending(dom::Animation& aPlayer,
                     AnimationPlayerSet& aSet);
  bool IsWaiting(const dom::Animation& aPlayer,
                 const AnimationPlayerSet& aSet) const;

  AnimationPlayerSet mPlayPendingSet;
  AnimationPlayerSet mPausePendingSet;
  nsCOMPtr<nsIDocument> mDocument;
};

} 

#endif 
