





#ifndef mozilla_dom_TVProgram_h__
#define mozilla_dom_TVProgram_h__

#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class TVChannel;

class TVProgram MOZ_FINAL : public nsISupports
                          , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TVProgram)

  explicit TVProgram(nsISupports* aOwner);

  

  nsISupports* GetParentObject() const
  {
    return mOwner;
  }

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_OVERRIDE;

  

  void GetAudioLanguages(nsTArray<nsString>& aLanguages) const;

  void GetSubtitleLanguages(nsTArray<nsString>& aLanguages) const;

  void GetEventId(nsAString& aEventId) const;

  already_AddRefed<TVChannel> Channel() const;

  void GetTitle(nsAString& aTitle) const;

  uint64_t StartTime() const;

  uint64_t Duration() const;

  void GetDescription(nsAString& aDescription) const;

  void GetRating(nsAString& aRating) const;

private:
  ~TVProgram();

  nsCOMPtr<nsISupports> mOwner;
};

} 
} 

#endif 
