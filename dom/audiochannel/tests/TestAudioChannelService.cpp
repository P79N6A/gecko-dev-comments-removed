



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

class Agent
{
public:
  Agent(AudioChannelType aType)
  : mType(aType)
  , mRegistered(false)
  {
    mAgent = do_CreateInstance("@mozilla.org/audiochannelagent;1");
  }

  nsresult Init()
  {
    nsresult rv = mAgent->Init(mType, nullptr);
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
    return mAgent->StopPlaying();
  }

  nsCOMPtr<AudioChannelAgent> mAgent;
  AudioChannelType mType;
  bool mRegistered;
};

nsresult
TestDoubleStartPlaying()
{
  Agent agent(AUDIO_CHANNEL_NORMAL);
  nsresult rv = agent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;
  rv = agent.mAgent->StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent.mAgent->StartPlaying(&playing);
  TEST_ENSURE_BASE(NS_FAILED(rv), "Test0: StartPlaying calling twice should return error");

  return NS_OK;
}

nsresult
TestOneNormalChannel()
{
  Agent agent(AUDIO_CHANNEL_NORMAL);
  nsresult rv = agent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;
  rv = agent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test1: A normal channel unvisible agent must not be playing");

  rv = agent.mAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test1: A normal channel visible agent should be playing");

  return rv;
}

nsresult
TestTwoNormalChannels()
{
  Agent agent1(AUDIO_CHANNEL_NORMAL);
  nsresult rv = agent1.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  Agent agent2(AUDIO_CHANNEL_NORMAL);
  rv = agent2.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;
  rv = agent1.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test2: A normal channel unvisible agent1 must not be playing");

  rv = agent2.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test2: A normal channel unvisible agent2 must not be playing");

  rv = agent1.mAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent2.mAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent1.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test2: A normal channel visible agent1 should be playing");

  rv = agent2.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test2: A normal channel visible agent2 should be playing");

  return rv;
}

nsresult
TestContentChannels()
{
  Agent agent1(AUDIO_CHANNEL_CONTENT);
  nsresult rv = agent1.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  Agent agent2(AUDIO_CHANNEL_CONTENT);
  rv = agent2.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;
  rv = agent1.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel unvisible agent1 should be playing");

  rv = agent2.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel unvisible agent2 should be playing");

  rv = agent1.mAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent2.mAgent->SetVisibilityState(false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent1.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel visible agent1 should be playing");

  rv = agent2.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test3: A content channel unvisible agent2 should not be playing");

  rv = agent1.mAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent2.mAgent->SetVisibilityState(true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agent1.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel visible agent1 should be playing");

  rv = agent2.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test3: A content channel visible agent2 should be playing");

  return rv;
}

nsresult
TestPriorities()
{
  Agent normalAgent(AUDIO_CHANNEL_NORMAL);
  nsresult rv = normalAgent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  Agent contentAgent(AUDIO_CHANNEL_CONTENT);
  rv = contentAgent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  Agent notificationAgent(AUDIO_CHANNEL_NOTIFICATION);
  rv = notificationAgent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  Agent alarmAgent(AUDIO_CHANNEL_ALARM);
  rv = alarmAgent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  Agent telephonyAgent(AUDIO_CHANNEL_TELEPHONY);
  rv = telephonyAgent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  Agent ringerAgent(AUDIO_CHANNEL_RINGER);
  rv = ringerAgent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  Agent pNotificationAgent(AUDIO_CHANNEL_PUBLICNOTIFICATION);
  rv = pNotificationAgent.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  bool playing;

  
  rv = normalAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A normal channel unvisible agent should not be playing");

  
  rv = contentAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: A content channel unvisible agent should be playing");

  
  rv = notificationAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An notification channel unvisible agent should be playing");

  
  rv = contentAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A content channel unvisible agent should not be playing when notification channel is playing");

  
  rv = alarmAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An alarm channel unvisible agent should be playing");

  
  rv = notificationAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A notification channel unvisible agent should not be playing when an alarm is playing");

  
  rv = telephonyAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An telephony channel unvisible agent should be playing");

  
  rv = alarmAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A alarm channel unvisible agent should not be playing when a telephony is playing");

  
  rv = ringerAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An ringer channel unvisible agent should be playing");

  
  rv = telephonyAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A telephony channel unvisible agent should not be playing when a riger is playing");

  
  rv = pNotificationAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(playing, "Test4: An pNotification channel unvisible agent should be playing");

  
  rv = ringerAgent.StartPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  TEST_ENSURE_BASE(!playing, "Test4: A ringer channel unvisible agent should not be playing when a public notification is playing");

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

