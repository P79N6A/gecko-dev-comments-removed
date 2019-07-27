




#ifndef mozilla_dom_PendingPlayerTracker_h
#define mozilla_dom_PendingPlayerTracker_h

#include "mozilla/dom/AnimationPlayer.h"
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

  void AddPlayPending(dom::AnimationPlayer& aPlayer)
  {
    MOZ_ASSERT(!IsWaitingToPause(aPlayer),
               "Player is already waiting to pause");
    AddPending(aPlayer, mPlayPendingSet);
  }
  void RemovePlayPending(dom::AnimationPlayer& aPlayer)
  {
    RemovePending(aPlayer, mPlayPendingSet);
  }
  bool IsWaitingToPlay(const dom::AnimationPlayer& aPlayer) const
  {
    return IsWaiting(aPlayer, mPlayPendingSet);
  }

  void AddPausePending(dom::AnimationPlayer& aPlayer)
  {
    MOZ_ASSERT(!IsWaitingToPlay(aPlayer),
               "Player is already waiting to play");
    AddPending(aPlayer, mPausePendingSet);
  }
  void RemovePausePending(dom::AnimationPlayer& aPlayer)
  {
    RemovePending(aPlayer, mPausePendingSet);
  }
  bool IsWaitingToPause(const dom::AnimationPlayer& aPlayer) const
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

  typedef nsTHashtable<nsRefPtrHashKey<dom::AnimationPlayer>>
    AnimationPlayerSet;

  void AddPending(dom::AnimationPlayer& aPlayer,
                  AnimationPlayerSet& aSet);
  void RemovePending(dom::AnimationPlayer& aPlayer,
                     AnimationPlayerSet& aSet);
  bool IsWaiting(const dom::AnimationPlayer& aPlayer,
                 const AnimationPlayerSet& aSet) const;

  AnimationPlayerSet mPlayPendingSet;
  AnimationPlayerSet mPausePendingSet;
  nsCOMPtr<nsIDocument> mDocument;
};

} 

#endif 
