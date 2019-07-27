




#ifndef mozilla_imagelib_DiscardTracker_h_
#define mozilla_imagelib_DiscardTracker_h_

#include "mozilla/Atomics.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "prlock.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"

class nsITimer;

namespace mozilla {
namespace image {

class RasterImage;












class DiscardTracker
{
  public:
    









    struct Node : public LinkedListElement<Node>
    {
      RasterImage *img;
      TimeStamp timestamp;
    };

    



    static nsresult Reset(struct Node* node);

    



    static void Remove(struct Node* node);

    


    static nsresult Initialize();

    




    static void Shutdown();

    



    static void DiscardAll();

    






    static bool TryAllocation(uint64_t aBytes);

    




    static void InformDeallocation(uint64_t aBytes);

  private:
    


    friend void DiscardTimeoutChangedCallback(const char* aPref, void *aClosure);

    



    class DiscardRunnable : public nsRunnable
    {
      NS_IMETHOD Run();
    };

    static void ReloadTimeout();
    static nsresult EnableTimer();
    static void DisableTimer();
    static void MaybeDiscardSoon();
    static void TimerCallback(nsITimer *aTimer, void *aClosure);
    static void DiscardNow();

    static LinkedList<Node> sDiscardableImages;
    static nsCOMPtr<nsITimer> sTimer;
    static bool sInitialized;
    static bool sTimerOn;
    static Atomic<bool> sDiscardRunnablePending;
    static uint64_t sCurrentDecodedImageBytes;
    static uint32_t sMinDiscardTimeoutMs;
    static uint32_t sMaxDecodedImageKB;
    static uint32_t sHardLimitDecodedImageKB;
    
    static PRLock *sAllocationLock;
    static Mutex* sNodeListMutex;
    static Atomic<bool> sShutdown;
};

} 
} 

#endif 
