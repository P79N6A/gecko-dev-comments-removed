





#include "mozilla/dom/TVProgramBinding.h"
#include "TVProgram.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(TVProgram, mOwner)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TVProgram)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TVProgram)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TVProgram)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

TVProgram::TVProgram(nsISupports* aOwner)
  : mOwner(aOwner)
{
}

TVProgram::~TVProgram()
{
}

 JSObject*
TVProgram::WrapObject(JSContext* aCx)
{
  return TVProgramBinding::Wrap(aCx, this);
}

void
TVProgram::GetAudioLanguages(nsTArray<nsString>& aLanguages) const
{
  
}

void
TVProgram::GetSubtitleLanguages(nsTArray<nsString>& aLanguages) const
{
  
}

void
TVProgram::GetEventId(nsAString& aEventId) const
{
  
}

already_AddRefed<TVChannel>
TVProgram::Channel() const
{
  
  return nullptr;
}

void
TVProgram::GetTitle(nsAString& aTitle) const
{
  
}

uint64_t
TVProgram::StartTime() const
{
  
  return 0;
}

uint64_t
TVProgram::Duration() const
{
  
  return 0;
}

void
TVProgram::GetDescription(nsAString& aDescription) const
{
  
}

void
TVProgram::GetRating(nsAString& aRating) const
{
  
}

} 
} 
