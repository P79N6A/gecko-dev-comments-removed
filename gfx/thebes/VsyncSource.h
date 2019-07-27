




#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"

namespace mozilla {
class VsyncDispatcher;

namespace gfx {



class VsyncSource
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncSource)
public:
  
  class Display {
    public:
      Display();
      virtual ~Display();
      void AddVsyncDispatcher(mozilla::VsyncDispatcher* aVsyncDispatcher);
      void RemoveVsyncDispatcher(mozilla::VsyncDispatcher* aVsyncDispatcher);
      
      void NotifyVsync(mozilla::TimeStamp aVsyncTimestamp);

      
      virtual void EnableVsync() = 0;
      virtual void DisableVsync() = 0;
      virtual bool IsVsyncEnabled() = 0;

    private:
      nsTArray<nsRefPtr<mozilla::VsyncDispatcher>> mVsyncDispatchers;
  }; 

  void AddVsyncDispatcher(mozilla::VsyncDispatcher* aVsyncDispatcher);
  void RemoveVsyncDispatcher(mozilla::VsyncDispatcher* aVsyncDispatcher);

protected:
  virtual Display& GetGlobalDisplay() = 0; 
  virtual Display& FindDisplay(mozilla::VsyncDispatcher* aVsyncDispatcher);
  virtual ~VsyncSource() {}
}; 
} 
} 
