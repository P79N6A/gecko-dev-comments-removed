









#include "nsILayoutHistoryState.h"
#include "nsWeakReference.h"
#include "nsClassHashtable.h"
#include "nsPresState.h"
#include "mozilla/Attributes.h"

class nsLayoutHistoryState final : public nsILayoutHistoryState,
                                   public nsSupportsWeakReference
{
public:
  nsLayoutHistoryState()
    : mScrollPositionOnly(false)
  {
  }

  NS_DECL_ISUPPORTS

  
  virtual void
  AddState(const nsCString& aKey, nsPresState* aState) override;
  virtual nsPresState*
  GetState(const nsCString& aKey) override;
  virtual void
  RemoveState(const nsCString& aKey) override;
  virtual bool
  HasStates() const override;
  virtual void
  SetScrollPositionOnly(const bool aFlag) override;


private:
  ~nsLayoutHistoryState() {}
  bool mScrollPositionOnly;

  nsClassHashtable<nsCStringHashKey,nsPresState> mStates;
};


already_AddRefed<nsILayoutHistoryState>
NS_NewLayoutHistoryState()
{
  nsRefPtr<nsLayoutHistoryState> state = new nsLayoutHistoryState();
  return state.forget();
}

NS_IMPL_ISUPPORTS(nsLayoutHistoryState,
                  nsILayoutHistoryState,
                  nsISupportsWeakReference)

void
nsLayoutHistoryState::AddState(const nsCString& aStateKey, nsPresState* aState)
{
  mStates.Put(aStateKey, aState);
}

nsPresState*
nsLayoutHistoryState::GetState(const nsCString& aKey)
{
  nsPresState* state = nullptr;
  bool entryExists = mStates.Get(aKey, &state);

  if (entryExists && mScrollPositionOnly) {
    
    state->ClearNonScrollState();
  }

  return state;
}

void
nsLayoutHistoryState::RemoveState(const nsCString& aKey)
{
  mStates.Remove(aKey);
}

bool
nsLayoutHistoryState::HasStates() const
{
  return mStates.Count() != 0;
}

void
nsLayoutHistoryState::SetScrollPositionOnly(const bool aFlag)
{
  mScrollPositionOnly = aFlag;
}
