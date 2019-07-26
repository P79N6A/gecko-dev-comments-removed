






#ifndef SkCountdown_DEFINED
#define SkCountdown_DEFINED

#include "SkCondVar.h"
#include "SkRunnable.h"
#include "SkTypes.h"

class SkCountdown : public SkRunnable {
public:
    explicit SkCountdown(int32_t count);

    


    void reset(int32_t count);

    virtual void run() SK_OVERRIDE;

    


    void wait();

private:
    SkCondVar fReady;
    int32_t   fCount;
};

#endif
