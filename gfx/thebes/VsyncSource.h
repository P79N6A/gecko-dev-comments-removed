




#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"

namespace mozilla {
class CompositorVsyncDispatcher;

namespace gfx {



class VsyncSource
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncSource)
public:
  
  class Display {
    public:
      Display();
      virtual ~Display();
      void AddCompositorVsyncDispatcher(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
      void RemoveCompositorVsyncDispatcher(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
      
      void NotifyVsync(mozilla::TimeStamp aVsyncTimestamp);

      
      virtual void EnableVsync() = 0;
      virtual void DisableVsync() = 0;
      virtual bool IsVsyncEnabled() = 0;

    private:
      nsTArray<nsRefPtr<mozilla::CompositorVsyncDispatcher>> mCompositorVsyncDispatchers;
  }; 

  void AddCompositorVsyncDispatcher(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
  void RemoveCompositorVsyncDispatcher(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);

protected:
  virtual Display& GetGlobalDisplay() = 0; 
  virtual Display& FindDisplay(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
  virtual ~VsyncSource() {}
}; 
} 
} 
