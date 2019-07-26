


#ifdef XP_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "TestHarness.h"

#include "AudioChannelService.h"
#include "AudioChannelAgent.h"

#define TEST_ENSURE_BASE(_test, _msg)       \
  PR_BEGIN_MACRO                            \
    if (!_test) {                           \
      fail(_msg);                           \
      return NS_ERROR_FAILURE;              \
    } else {                                \
      passed(_msg);                         \
    }                                       \
  PR_END_MACRO

using namespace mozilla::dom;

class Agent : public nsIAudioChannelAgentCallback
{
public:
  NS_DECL_ISUPPORTS

  Agent(AudioChannelType aType)
  : mType(aType)
  , mWaitCallback(false)
  , mRegistered(false)
  , mCanPlay(false)
  {
    mAgent = do_CreateInstance("@mozilla.org/audiochannelagent;1");
  }

  virtual ~Agent() {}

  nsresult Init()
  {
    nsresult rv = mAgent->Init(mType, this);
    NS_ENSURE_SUCCESS(rv, rv);

    return mAgent->SetVisibilityState(false);
  }

  nsresult StartPlaying(bool *_ret)
  {
    if (mRegistered)
      StopPlaying();

    nsresult rv = mAgent->StartPlaying(_ret);
    mRegistered = true;
    return rv;
  }

  nsresult StopPlaying()
  {
    mRegistered = false;
    int loop = 0;
    while (mWaitCallback) {
      #ifdef XP_WIN
      Sleep(1000);
      #else
      sleep(1);
      #endif
      if (loop++ == 5) {
        TEST_ENSURE_BASE(false, "StopPlaying timeout");
      }
    }
    return mAgent->StopPlaying();
  }

  nsresult SetVisibilityState(bool visible)
  {
    if (mRegistered) {
      mWaitCallback = true;
    }
    return mAgent->SetVisibilityState(visible);
  }

  NS_IMETHODIMP CanPlayChanged(bool canPlay)
  {
    mCanPlay = canPlay;
    mWaitCallback = false;
    return NS_OK;
  }

  nsresult GetCanPlay(bool *_ret)
  {
    int loop = 0;
    while (mWaitCallback) {
      #ifdef XP_WIN
      Sleep(1000);
      #else
      sleep(1);
      #endif
      if (loop++ == 5) {
        TEST_ENSURE_BASE(false, "GetCanPlay timeout");
      }
    }
    *_ret = mCanPlay;
    return NS_OK;
  }

  nsCOMPtr<AudioChannelAgent> mAgent;
  AudioChannelType mType;
  bool mWaitCallback;
  bool mRegistered;
  bool mCanPlay;
};

NS_IMPL_ISUPPORTS1(Agent, nsIAudioChannelAgentCallback)

nsresult
TestDoubleStartPlaying()
{
  nsCOMPtr<Agent> agent = new Agent(AUDIO_CHANNEL_NORMAL);

  nsresult rv = agent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;
  rv = agent->mAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent->mAgent->StartPlaying(&playing);
  TEST_ENSURE_BASE(NS_FAILED(rv), "Test0: StartPlaying calling twice should return error");

  return NS_OK;
}

nsresult
TestOneNormalChannel()
{
  nsCOMPtr<Agent> agent = new Agent(AUDIO_CHANNEL_NORMAL);
  nsresult rv = agent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;
  rv = agent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test1: A normal channel unvisible agent must not be playing");

  rv = agent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test1: A normal channel visible agent should be playing");

  return rv;
}

nsresult
TestTwoNormalChannels()
{
  nsCOMPtr<Agent> agent1 = new Agent(AUDIO_CHANNEL_NORMAL);
  nsresult rv = agent1->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<Agent> agent2 = new Agent(AUDIO_CHANNEL_NORMAL);
  rv = agent2->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;
  rv = agent1->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test2: A normal channel unvisible agent1 must not be playing");

  rv = agent2->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test2: A normal channel unvisible agent2 must not be playing");

  rv = agent1->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent2->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent1->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test2: A normal channel visible agent1 should be playing");

  rv = agent2->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test2: A normal channel visible agent2 should be playing");

  return rv;
}

nsresult
TestContentChannels()
{
  nsCOMPtr<Agent> agent1 = new Agent(AUDIO_CHANNEL_CONTENT);
  nsresult rv = agent1->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<Agent> agent2 = new Agent(AUDIO_CHANNEL_CONTENT);
  rv = agent2->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = agent1->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent2->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;
  rv = agent1->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel visible agent1 should be playing");

  rv = agent2->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel visible agent2 should be playing");

  

  rv = agent1->SetVisibilityState(false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent1->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel unvisible agent1 should be playing from foreground to background");

  
  rv = agent2->SetVisibilityState(false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent2->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel unvisible agent2 should be playing from foreground to background");

  



  rv = agent1->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = agent2->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = agent1->StopPlaying();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = agent2->StopPlaying();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = agent1->SetVisibilityState(false);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = agent2->SetVisibilityState(false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent1->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test3: A content channel unvisible agent1 must not be playing from background state");

  rv = agent2->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test3: A content channel unvisible agent2 must not be playing from background state");

  return rv;
}

nsresult
TestPriorities()
{
  nsCOMPtr<Agent> normalAgent = new Agent(AUDIO_CHANNEL_NORMAL);
  nsresult rv = normalAgent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<Agent> contentAgent = new Agent(AUDIO_CHANNEL_CONTENT);
  rv = contentAgent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<Agent> notificationAgent = new Agent(AUDIO_CHANNEL_NOTIFICATION);
  rv = notificationAgent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<Agent> alarmAgent = new Agent(AUDIO_CHANNEL_ALARM);
  rv = alarmAgent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<Agent> telephonyAgent = new Agent(AUDIO_CHANNEL_TELEPHONY);
  rv = telephonyAgent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<Agent> ringerAgent = new Agent(AUDIO_CHANNEL_RINGER);
  rv = ringerAgent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<Agent> pNotificationAgent = new Agent(AUDIO_CHANNEL_PUBLICNOTIFICATION);
  rv = pNotificationAgent->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;

  
  rv = normalAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A normal channel unvisible agent must not be playing");

  
  rv = contentAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A content channel unvisible agent must not be playing from background state");

  
  rv = notificationAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An notification channel unvisible agent should be playing");

  
  rv = contentAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A content channel unvisible agent should not be playing when notification channel is playing");

  
  rv = alarmAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An alarm channel unvisible agent should be playing");

  
  rv = notificationAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A notification channel unvisible agent should not be playing when an alarm is playing");

  
  rv = telephonyAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An telephony channel unvisible agent should be playing");

  
  rv = alarmAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A alarm channel unvisible agent should not be playing when a telephony is playing");

  
  rv = ringerAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An ringer channel unvisible agent should be playing");

  
  rv = telephonyAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A telephony channel unvisible agent should not be playing when a riger is playing");

  
  rv = pNotificationAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An pNotification channel unvisible agent should be playing");

  
  rv = ringerAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A ringer channel unvisible agent should not be playing when a public notification is playing");

  
  rv = normalAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = normalAgent->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: A normal channel visible agent should be playing");

  
  rv = contentAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = contentAgent->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: A content channel visible agent should be playing");

  
  rv = notificationAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = notificationAgent->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: A notification channel visible agent should be playing");

  
  rv = alarmAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = alarmAgent->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: A alarm channel visible agent should be playing");

  
  rv = telephonyAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = telephonyAgent->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: A telephony channel visible agent should be playing");

  
  rv = ringerAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = ringerAgent->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: A ringer channel visible agent should be playing");

  
  rv = pNotificationAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = pNotificationAgent->GetCanPlay(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: A pNotification channel visible agent should be playing");

  return rv;
}

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("AudioChannelService");
  if (xpcom.failed())
    return 1;

  if (NS_FAILED(TestDoubleStartPlaying()))
    return 1;

  if (NS_FAILED(TestOneNormalChannel()))
    return 1;

  if (NS_FAILED(TestTwoNormalChannels()))
    return 1;

  if (NS_FAILED(TestContentChannels()))
    return 1;

  if (NS_FAILED(TestPriorities()))
    return 1;

  return 0;
}

