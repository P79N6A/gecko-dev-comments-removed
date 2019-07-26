





#ifndef mozilla_dom_AudioParent_h
#define mozilla_dom_AudioParent_h

#include "mozilla/dom/PAudioParent.h"
#include "nsAudioStream.h"
#include "nsITimer.h"

namespace mozilla {
namespace dom {
class AudioParent : public PAudioParent, public nsITimerCallback
{
 public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSITIMERCALLBACK

    virtual bool
    RecvWrite(const nsCString& data, const uint32_t& count);

    virtual bool
    RecvSetVolume(const float& aVolume);

    virtual bool
    RecvMinWriteSize();

    virtual bool
    RecvDrain();

    virtual bool
    RecvPause();

    virtual bool
    RecvResume();

    virtual bool
    RecvShutdown();

    virtual bool
    SendMinWriteSizeDone(int32_t minFrames);

    virtual bool
    SendDrainDone();

    virtual bool
    SendWriteDone();

    AudioParent(int32_t aNumChannels, int32_t aRate, int32_t aFormat);
    virtual ~AudioParent();
    virtual void ActorDestroy(ActorDestroyReason);

    nsRefPtr<nsAudioStream> mStream;
    nsCOMPtr<nsITimer> mTimer;

private:
    void Shutdown();

    bool mIPCOpen;
};
} 
} 

#endif
