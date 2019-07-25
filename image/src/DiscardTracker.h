




#ifndef mozilla_imagelib_DiscardTracker_h_
#define mozilla_imagelib_DiscardTracker_h_

#include "mozilla/LinkedList.h"
#include "mozilla/TimeStamp.h"

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

    





    static void InformAllocation(int64_t bytes);

  private:
    


    friend int DiscardTimeoutChangedCallback(const char* aPref, void *aClosure);

    



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
    static int32_t sDiscardRunnablePending;
    static int64_t sCurrentDecodedImageBytes;
    static uint32_t sMinDiscardTimeoutMs;
    static uint32_t sMaxDecodedImageKB;
    
    static PRLock *sAllocationLock;
};

} 
} 

#endif 
