






































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
    RecvWrite(
            const nsCString& data,
            const PRUint32& count);

    virtual bool
    RecvSetVolume(const float& aVolume);

    virtual bool
    RecvMinWriteSample();

    virtual bool
    RecvDrain();

    virtual bool
    RecvPause();

    virtual bool
    RecvResume();

    virtual bool
    RecvShutdown();

    virtual bool
    SendMinWriteSampleDone(PRInt32 minSamples);

    virtual bool
    SendDrainDone(nsresult status);

    AudioParent(PRInt32 aNumChannels, PRInt32 aRate, PRInt32 aFormat);
    virtual ~AudioParent();
    virtual void ActorDestroy(ActorDestroyReason);

    nsRefPtr<nsAudioStream> mStream;
    nsCOMPtr<nsITimer> mTimer;

private:
    void Shutdown();

    PRPackedBool mIPCOpen;
};
} 
} 

#endif
