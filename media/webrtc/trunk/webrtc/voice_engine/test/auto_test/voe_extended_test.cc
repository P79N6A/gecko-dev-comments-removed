









#include "webrtc/voice_engine/test/auto_test/voe_extended_test.h"

#include <stdio.h>
#include <string.h>
#include <vector>

#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/voice_engine/test/auto_test/voe_test_defines.h"
#include "webrtc/voice_engine/voice_engine_defines.h"

#if defined(_WIN32)
#include <conio.h>
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
#include <netdb.h>
#endif

using namespace webrtc;
using namespace test;

namespace voetest {




#define _SEND_TO_REMOTE_IP_

#ifdef _SEND_TO_REMOTE_IP_
const int RemotePort = 12345; 
const char* RemoteIP = "192.168.200.1"; 
#endif

#ifdef WEBRTC_ANDROID

extern void* globalJavaVM;
extern void* globalContext;
#endif

static const int kTestMaxNumChannels = 100;






AudioDeviceModuleImpl* AudioDeviceModuleImpl::Create() {
  AudioDeviceModuleImpl* xADM = new AudioDeviceModuleImpl();
  if (xADM)
    xADM->AddRef();
  return xADM;
}


bool AudioDeviceModuleImpl::Destroy(AudioDeviceModuleImpl* adm) {
  if (!adm)
    return false;
  int32_t count = adm->Release();
  if (count != 0) {
    return false;
  } else {
    delete adm;
    return true;
  }
}

AudioDeviceModuleImpl::AudioDeviceModuleImpl() :
  _ref_count(0) {
}

AudioDeviceModuleImpl::~AudioDeviceModuleImpl() {
}

int32_t AudioDeviceModuleImpl::AddRef() {
  return ++_ref_count;
}

int32_t AudioDeviceModuleImpl::Release() {
  
  
  return --_ref_count;
}





ExtendedTestTransport::ExtendedTestTransport(VoENetwork* ptr) :
  myNetw(ptr), _thread(NULL), _lock(NULL), _event(NULL), _length(0),
  _channel(0) {
  const char* threadName = "voe_extended_test_external_thread";
  _lock = CriticalSectionWrapper::CreateCriticalSection();
  _event = EventWrapper::Create();
  _thread = ThreadWrapper::CreateThread(Run, this, kHighPriority, threadName);
  if (_thread) {
    unsigned int id;
    _thread->Start(id);
  }
}

ExtendedTestTransport::~ExtendedTestTransport() {
  if (_thread) {
    _thread->SetNotAlive();
    _event->Set();
    if (_thread->Stop()) {
      delete _thread;
      _thread = NULL;
      delete _event;
      _event = NULL;
      delete _lock;
      _lock = NULL;
    }
  }
}

bool ExtendedTestTransport::Run(void* ptr) {
  return static_cast<ExtendedTestTransport*> (ptr)->Process();
}

bool ExtendedTestTransport::Process() {
  switch (_event->Wait(500)) {
    case kEventSignaled:
      _lock->Enter();
      myNetw->ReceivedRTPPacket(_channel, _packetBuffer, _length);
      _lock->Leave();
      return true;
    case kEventTimeout:
      return true;
    case kEventError:
      break;
  }
  return true;
}

int ExtendedTestTransport::SendPacket(int channel, const void *data, int len) {
  _lock->Enter();
  if (len < 1612) {
    memcpy(_packetBuffer, (const unsigned char*) data, len);
    _length = len;
    _channel = channel;
  }
  _lock->Leave();
  _event->Set(); 
  return len;
}

int ExtendedTestTransport::SendRTCPPacket(int channel, const void *data, int len) {
  myNetw->ReceivedRTCPPacket(channel, data, len);
  return len;
}





XRTPObserver::XRTPObserver() :
  _SSRC(0) {
}

XRTPObserver::~XRTPObserver() {
}

void XRTPObserver::OnIncomingCSRCChanged(const int , const unsigned int ,
                                         const bool ) {
}

void XRTPObserver::OnIncomingSSRCChanged(const int , unsigned int SSRC) {
  
  
  
  

  _SSRC = SSRC; 

}





int VoEExtendedTest::PrepareTest(const char* str) const {
  TEST_LOG("\n\n================================================\n");
  TEST_LOG("\tExtended *%s* Test\n", str);
  TEST_LOG("================================================\n\n");

  return 0;
}

int VoEExtendedTest::TestPassed(const char* str) const {
  TEST_LOG("\n\n------------------------------------------------\n");
  TEST_LOG("\tExtended *%s* test passed!\n", str);
  TEST_LOG("------------------------------------------------\n\n");

  return 0;
}

void VoEExtendedTest::OnPeriodicDeadOrAlive(const int , bool alive) {
  _alive = alive;
  if (alive) {
    TEST_LOG("=> ALIVE ");
  } else {
    TEST_LOG("=> DEAD ");
  }
  fflush(NULL);
}

void VoEExtendedTest::CallbackOnError(int errCode, int) {
  _errCode = errCode;
  TEST_LOG("\n************************\n");
  TEST_LOG(" RUNTIME ERROR: %d \n", errCode);
  TEST_LOG("************************\n");
}

VoEExtendedTest::VoEExtendedTest(VoETestManager& mgr) :
  _mgr(mgr) {
  for (int i = 0; i < 32; i++) {
    _listening[i] = false;
    _playing[i] = false;
    _sending[i] = false;
  }
}

VoEExtendedTest::~VoEExtendedTest() {
}

void VoEExtendedTest::StartMedia(int channel, int rtpPort, bool listen,
                                 bool playout, bool send) {
  VoEBase* voe_base_ = _mgr.BasePtr();

  _listening[channel] = false;
  _playing[channel] = false;
  _sending[channel] = false;

  VoENetwork* voe_network = _mgr.NetworkPtr();

  voice_channel_transports_[channel].reset(
      new VoiceChannelTransport(voe_network, channel));

  voice_channel_transports_[channel]->SetSendDestination("127.0.0.1", rtpPort);
  voice_channel_transports_[channel]->SetLocalReceiver(rtpPort);

  if (listen) {
    _listening[channel] = true;
    voe_base_->StartReceive(channel);
  }
  if (playout) {
    _playing[channel] = true;
    voe_base_->StartPlayout(channel);
  }
  if (send) {
    _sending[channel] = true;
    voe_base_->StartSend(channel);
  }
}

void VoEExtendedTest::StopMedia(int channel) {
  VoEBase* voe_base_ = _mgr.BasePtr();

  if (_listening[channel]) {
    _listening[channel] = false;
    voe_base_->StopReceive(channel);
  }
  if (_playing[channel]) {
    _playing[channel] = false;
    voe_base_->StopPlayout(channel);
  }
  if (_sending[channel]) {
    _sending[channel] = false;
    voe_base_->StopSend(channel);
  }
  voice_channel_transports_[channel].reset(NULL);
}

void VoEExtendedTest::Play(int channel, unsigned int timeMillisec, bool addFileAsMicrophone,
                           bool addTimeMarker) {
  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEFile* file = _mgr.FilePtr();

  voe_base_->StartPlayout(channel);
  TEST_LOG("[playing]");
  fflush(NULL);
  if (addFileAsMicrophone) {
    file->StartPlayingFileAsMicrophone(channel, _mgr.AudioFilename(), true, true);
    TEST_LOG("[file as mic]");
    fflush(NULL);
  }
  if (addTimeMarker) {
    float dtSec = (float) ((float) timeMillisec / 1000.0);
    TEST_LOG("[dT=%.1f]", dtSec);
    fflush(NULL); 
  }
  SleepMs(timeMillisec);
  voe_base_->StopPlayout(channel);
  file->StopPlayingFileAsMicrophone(channel);
}

void VoEExtendedTest::Sleep(unsigned int timeMillisec, bool addMarker) {
  if (addMarker) {
    float dtSec = (float) ((float) timeMillisec / 1000.0);
    TEST_LOG("[dT=%.1f]", dtSec); 
  }
  webrtc::SleepMs(timeMillisec);
}

int VoEExtendedTest::TestBase() {
#ifndef _WIN32
  
#undef PAUSE
#define PAUSE SleepMs(2000);
#endif

  PrepareTest("Base");

  
  
  
  
  VoEBase* voe_base_ = _mgr.BasePtr();
  VoENetwork* voe_network = _mgr.NetworkPtr();
#ifdef _TEST_RTP_RTCP_
  VoERTP_RTCP* rtp = _mgr.RTP_RTCPPtr();
#endif

  
  

#ifdef _USE_EXTENDED_TRACE_
  TEST(SetTraceFileName - SetDebugTraceFileName); ANL();
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(NULL)); MARK();
  
  std::string output_path = webrtc::test::OutputPath();
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(
              (output_path + "VoEBase_trace_dont_use.txt").c_str())); MARK();
  
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(GetFilename(""
              (output_path + "VoEBase_trace.txt").c_str())); MARK();
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStream |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo)); MARK();

  ANL(); AOK(); ANL(); ANL();
#endif

  
  
  
  TEST(SetObserver);
  ANL();

  TEST_MUSTPASS(voe_base_->RegisterVoiceEngineObserver(*this));
  MARK();
  SleepMs(100);
  TEST_MUSTPASS(voe_base_->DeRegisterVoiceEngineObserver());
  MARK();

  ANL();
  AOK();
  ANL();
  ANL();

  
  
  TEST(GetVersion);
  ANL();

  char version[1024];
  
  
  TEST_MUSTPASS(voe_base_->GetVersion(version));
  MARK();
  TEST_LOG("\n-----\n%s\n-----\n", version);

  ANL();
  AOK();
  ANL();
  ANL();

  
  
  TEST(Init);
  ANL();

  TEST_MUSTPASS(voe_base_->Init());
  MARK();
  TEST_MUSTPASS(voe_base_->Terminate());

  TEST_MUSTPASS(voe_base_->Init());
  MARK();
  
  
  TEST_MUSTPASS(voe_base_->Init());
  MARK();
  TEST_MUSTPASS(voe_base_->Terminate());
#if (!defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID))
  
  TEST_MUSTPASS(voe_base_->Init());
  MARK(); 
  TEST_MUSTPASS(voe_base_->Terminate());
#endif

  ANL();
  AOK();
  ANL();
  ANL();

  
  
  TEST(Terminate);
  ANL();
  TEST_MUSTPASS(voe_base_->Terminate());
  MARK(); 
  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->Terminate());
  MARK(); 

  ANL();
  AOK();
  ANL();
  ANL();

  
  
  
  
  
  
  
  
  
  
  
  
  
  TEST_LOG("\nTesting: Init in combination with an external ADM\n");

  
  AudioDeviceModuleImpl* xADM = AudioDeviceModuleImpl::Create();
  ASSERT_FALSE(xADM == NULL);
  ASSERT_TRUE(xADM->ReferenceCounter() == 1);

  
  TEST_MUSTPASS(voe_base_->Init(xADM));MARK();
  ASSERT_TRUE(xADM->ReferenceCounter() == 2);
  TEST_MUSTPASS(voe_base_->Terminate());
  ASSERT_TRUE(xADM->ReferenceCounter() == 1);

  
  
  ASSERT_TRUE(AudioDeviceModuleImpl::Destroy(xADM));
  ANL();
  AOK();ANL();

  
  

  
  
  

  int i;
  int channel;
  static const int kTestMaxNumChannels = 100;

  TEST(CreateChannel);
  ANL();
  TEST(DeleteChannel);
  ANL();

  TEST_MUSTPASS(voe_base_->Init());

  channel = voe_base_->CreateChannel();
  MARK();
  TEST_MUSTPASS(channel != 0);
  channel = voe_base_->CreateChannel();
  MARK();
  TEST_MUSTPASS(channel != 1);

  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  MARK();
  TEST_MUSTPASS(voe_base_->DeleteChannel(1));
  MARK();

  
  for (i = 0; i < 10; i++) {
    channel = voe_base_->CreateChannel();
    MARK();
    TEST_MUSTPASS(channel != 0); 
    TEST_MUSTPASS(voe_base_->DeleteChannel(channel));
    MARK();
  }
  
  for (i = 0; i < kTestMaxNumChannels; i++) {
    channel = voe_base_->CreateChannel();
    MARK();
    TEST_MUSTPASS(channel != i);
  }
  channel = voe_base_->CreateChannel();
  MARK(); 
  TEST_MUSTPASS(channel != -1);

  int aChannel =
      (((kTestMaxNumChannels - 17) > 0) ? (kTestMaxNumChannels - 17) : 0);
  TEST_MUSTPASS(voe_base_->DeleteChannel(aChannel));
  MARK();
  channel = voe_base_->CreateChannel();
  MARK(); 
  TEST_MUSTPASS(channel != aChannel);

  
  for (i = 0; i < kTestMaxNumChannels; i++) {
    TEST_MUSTPASS(voe_base_->DeleteChannel(i));
    MARK();
  }

  
  TEST_MUSTPASS(-1 != voe_base_->DeleteChannel(aChannel));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);

  ANL();
  AOK();
  ANL();
  ANL();

  
  
  
  
  
  TEST(StartReceive);
  ANL();
  TEST(StopReceive);
  ANL();

  
  TEST_MUSTPASS(!voe_base_->StartReceive(0));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);
  TEST_MUSTPASS(!voe_base_->StopReceive(0));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);

  int ch = voe_base_->CreateChannel();

  
  TEST_MUSTPASS(!voe_base_->StartReceive(0));
  MARK();

  ExtendedTestTransport* ptrTransport = new ExtendedTestTransport(voe_network);
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(ch, *ptrTransport));
  MARK(); 

  
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  MARK();

  
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  MARK();
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  MARK();

  
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  Play(ch, 1000, true, true);
  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  MARK();

  voe_base_->DeleteChannel(0);
  delete ptrTransport;
  ch = voe_base_->CreateChannel();

  
  TEST_LOG("\nspeak after 2 seconds and ensure that no delay is added:\n");

  Sleep(2000, true); 

  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  Play(ch, 2000, true, true);
  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(0));

  TEST_MUSTPASS(voe_base_->DeleteChannel(ch));
  ANL();

  

  for (i = 0; i < kTestMaxNumChannels; i++) {
    ch = voe_base_->CreateChannel();
    MARK();
  }
  for (i = 0; i < kTestMaxNumChannels; i++) {
    voe_base_->DeleteChannel(i);
    MARK();
  }
  for (i = 0; i < kTestMaxNumChannels; i++) {
    ch = voe_base_->CreateChannel();
    ExtendedTestTransport* ptrTransport =
        new ExtendedTestTransport(voe_network);
    TEST_MUSTPASS(voe_network ->RegisterExternalTransport(ch, *ptrTransport));
    TEST_MUSTPASS(voe_base_->StartReceive(ch));
    MARK();
    TEST_MUSTPASS(voe_base_->StopReceive(ch));
    MARK();
    voe_base_->DeleteChannel(ch);
    delete ptrTransport;
  }

  ANL();
  AOK();
  ANL();
  ANL();

  
  

  
  
  
  
  
  TEST(StartPlayout);
  ANL();
  TEST(StopPlayout);
  ANL();

  
  TEST_MUSTPASS(!voe_base_->StartPlayout(0));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);
  TEST_MUSTPASS(!voe_base_->StopPlayout(0));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);

  ch = voe_base_->CreateChannel();

  TEST_MUSTPASS(voe_base_->StartPlayout(ch));
  MARK();
  TEST_MUSTPASS(voe_base_->StartPlayout(ch));
  MARK();
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  MARK();
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  MARK();

  voe_base_->DeleteChannel(ch);

  
  for (i = 0; i < kTestMaxNumChannels; i++) {
    ch = voe_base_->CreateChannel();
    TEST_MUSTPASS(voe_base_->StartPlayout(ch));
    MARK();
  }
  for (i = 0; i < kTestMaxNumChannels; i++) {
    TEST_MUSTPASS(voe_base_->StopPlayout(i));
    MARK();
    voe_base_->DeleteChannel(i);
  }
  for (i = 0; i < kTestMaxNumChannels; i++) {
    ch = voe_base_->CreateChannel();
    TEST_MUSTPASS(voe_base_->StartPlayout(ch));
    MARK();
    TEST_MUSTPASS(voe_base_->StopPlayout(ch));
    MARK();
    voe_base_->DeleteChannel(ch);
  }

  ANL();
  AOK();
  ANL();
  ANL();

  
  

  
  
  
  
  
  TEST(StartSend);
  ANL();
  TEST(StopSend);
  ANL();

  
  TEST_MUSTPASS(!voe_base_->StartSend(0));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);
  TEST_MUSTPASS(!voe_base_->StopSend(0));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);

  ch = voe_base_->CreateChannel();

  
  TEST_MUSTPASS(!voe_base_->StartSend(ch));
  MARK();
  TEST_ERROR(VE_DESTINATION_NOT_INITED);


  
  
  ptrTransport = new ExtendedTestTransport(voe_network);
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(ch, *ptrTransport));
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  MARK();
  SleepMs(100);

  TEST_MUSTPASS(voe_base_->StopSend(ch));
  MARK();

  voe_base_->DeleteChannel(ch);
  delete ptrTransport;
  ch = voe_base_->CreateChannel();

  ptrTransport = new ExtendedTestTransport(voe_network);
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(ch, *ptrTransport));
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  MARK();
  TEST_MUSTPASS(voe_base_->StartReceive(ch));
  Play(ch, 2000, true, true);
  TEST_MUSTPASS(voe_base_->StopSend(ch));
  MARK();
  TEST_MUSTPASS(voe_base_->StopReceive(ch));

  voe_base_->DeleteChannel(ch);

  ANL();
  AOK();
  ANL();
  ANL();

  
  

  
  
  
  TEST(SetNetEQPlayoutMode);
  ANL();
  TEST(GetNetEQPlayoutMode);
  ANL();

  NetEqModes mode;

  ch = voe_base_->CreateChannel();

  
  TEST_MUSTPASS(!voe_base_->GetNetEQPlayoutMode(ch+1, mode));
  MARK();
  TEST_MUSTPASS(!voe_base_->SetNetEQPlayoutMode(ch+1, kNetEqDefault));
  MARK();

  
  TEST_MUSTPASS(voe_base_->GetNetEQPlayoutMode(ch, mode));
  MARK();
  TEST_MUSTPASS(mode != kNetEqDefault);
  TEST_MUSTPASS(voe_base_->SetNetEQPlayoutMode(ch, kNetEqStreaming));
  MARK();
  voe_base_->DeleteChannel(ch);

  
  ch = voe_base_->CreateChannel();
  TEST_MUSTPASS(voe_base_->GetNetEQPlayoutMode(ch, mode));
  MARK();
  TEST_MUSTPASS(mode != kNetEqDefault);
  voe_base_->DeleteChannel(ch);

  
  for (i = 0; i < kTestMaxNumChannels; i++) {
    ch = voe_base_->CreateChannel();

    
    TEST_MUSTPASS(voe_base_->SetNetEQPlayoutMode(i, kNetEqDefault));
    MARK();
    TEST_MUSTPASS(voe_base_->GetNetEQPlayoutMode(i, mode));
    MARK();
    TEST_MUSTPASS(mode != kNetEqDefault);
    TEST_MUSTPASS(voe_base_->SetNetEQPlayoutMode(i, kNetEqStreaming));
    MARK();
    TEST_MUSTPASS(voe_base_->GetNetEQPlayoutMode(i, mode));
    MARK();
    TEST_MUSTPASS(mode != kNetEqStreaming);
    TEST_MUSTPASS(voe_base_->SetNetEQPlayoutMode(i, kNetEqFax));
    MARK();
    TEST_MUSTPASS(voe_base_->GetNetEQPlayoutMode(i, mode));
    MARK();
    TEST_MUSTPASS(mode != kNetEqFax);
    SleepMs(50);
  }

  for (i = 0; i < kTestMaxNumChannels; i++) {
    voe_base_->DeleteChannel(i);
  }

  ANL();
  AOK();
  ANL();
  ANL();

  

  ch = voe_base_->CreateChannel();

  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(voe_network, ch));

  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);
  TEST_MUSTPASS(voe_base_->StartReceive(ch));
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  TEST_MUSTPASS(voe_base_->StartPlayout(ch));

  TEST_MUSTPASS(voe_base_->SetNetEQPlayoutMode(ch, kNetEqDefault));
  MARK();
  TEST_LOG("\nenjoy full duplex using kNetEqDefault playout mode...\n");
  PAUSE

  TEST_MUSTPASS(voe_base_->SetNetEQPlayoutMode(ch, kNetEqStreaming));
  MARK();
  TEST_LOG("\nenjoy full duplex using kNetEqStreaming playout mode...\n");
  PAUSE

  TEST_MUSTPASS(voe_base_->SetNetEQPlayoutMode(ch, kNetEqFax));
  MARK();
  TEST_LOG("\nenjoy full duplex using kNetEqFax playout mode...\n");
  PAUSE

  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(ch));

  voe_base_->DeleteChannel(ch);

  ANL();
  AOK();
  ANL();
  ANL();

  
  

  ch = voe_base_->CreateChannel(); 
  

  
#ifdef _TEST_RTP_RTCP_
  TEST_MUSTPASS(rtp->SetRTCP_CNAME(ch, "Johnny"));
#endif
  voice_channel_transport.reset(new VoiceChannelTransport(voe_network, ch));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);

  TEST_MUSTPASS(voe_base_->StartReceive(ch));
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  TEST_MUSTPASS(voe_base_->StartPlayout(ch));

  TEST_LOG("full duplex is now activated (1)\n");
  TEST_LOG("waiting for RTCP packet...\n");

  SleepMs(7000); 
  PAUSE;

  
#ifdef _TEST_RTP_RTCP_
  char tmpStr[64] = { 0 };
  TEST_MUSTPASS(rtp->GetRemoteRTCP_CNAME(ch, tmpStr));
  TEST_MUSTPASS(_stricmp("Johnny", tmpStr));
#endif
  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(ch));

  
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  TEST_MUSTPASS(voe_base_->StartReceive(ch));
  TEST_MUSTPASS(voe_base_->StartPlayout(ch));

  TEST_LOG("\nfull duplex is now activated (2)\n");

  PAUSE

  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(ch));

  
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  TEST_MUSTPASS(voe_base_->StartReceive(ch));
  TEST_MUSTPASS(voe_base_->StartPlayout(ch));

  TEST_LOG("\nfull duplex is now activated (3)\n");
  TEST_LOG("waiting for RTCP packet...\n");

  SleepMs(7000); 
  PAUSE

  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(ch));

  voe_base_->DeleteChannel(ch);

  ch = voe_base_->CreateChannel();
  voice_channel_transport.reset(new VoiceChannelTransport(voe_network, ch));

  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);

  TEST_MUSTPASS(voe_base_->StartReceive(ch));
  TEST_MUSTPASS(voe_base_->StartSend(ch));
  TEST_MUSTPASS(voe_base_->StartPlayout(ch));

  TEST_LOG("\nfull duplex is now activated (5)\n");

  PAUSE

  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(ch));

  
  TEST_MUSTPASS(voe_base_->StartReceive(ch));
  TEST_MUSTPASS(voe_base_->StartPlayout(ch));
  TEST_MUSTPASS(voe_base_->StartSend(ch));

  TEST_LOG("\nfull duplex is now activated (6)\n");

  PAUSE

  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(ch));

  
  ch = voe_base_->CreateChannel();

  scoped_ptr<VoiceChannelTransport> voice_channel_transport_2(
      new VoiceChannelTransport(voe_network, ch));

  voice_channel_transport_2->SetSendDestination("127.0.0.1", 33221);
  voice_channel_transport_2->SetLocalReceiver(33221);

  TEST_MUSTPASS(voe_base_->StartReceive(ch));
  TEST_MUSTPASS(voe_base_->StartPlayout(ch));
  TEST_MUSTPASS(voe_base_->StartSend(ch));

  TEST_LOG("\nfull duplex is now activated (8)\n");

  PAUSE

  ANL();
  AOK();
  ANL();
  ANL();

  
  

#ifdef _USE_EXTENDED_TRACE_
  TEST(SetTraceFilter); ANL();

  TEST_MUSTPASS(VoiceEngine::SetTraceFile(GetFilename(""
              "VoEBase_trace_filter.txt").c_str())); MARK();
  SleepMs(100);

  
  
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceNone)); MARK();
  SleepMs(300);
  
  TEST_MUSTPASS(voe_base_->SetOnHoldStatus(0, true)); MARK();
  
  TEST_MUSTPASS(!voe_base_->SetOnHoldStatus(999, true)); MARK();

  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceApiCall |
          kTraceCritical |
          kTraceError |
          kTraceWarning)); MARK();
  SleepMs(300);
  
  TEST_MUSTPASS(voe_base_->SetOnHoldStatus(0, false)); MARK();
  
  TEST_MUSTPASS(!voe_base_->SetOnHoldStatus(999, true)); MARK();

  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceApiCall | kTraceInfo));
  MARK();
  SleepMs(300);
  
  TEST_MUSTPASS(voe_base_->SetOnHoldStatus(0, true)); MARK();
  
  TEST_MUSTPASS(!voe_base_->SetOnHoldStatus(999, true)); MARK();

  
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceAll)); MARK();
  SleepMs(300);

  AOK(); ANL();
#endif

  
  
  
  
  

  
  VoiceEngine* instVE[7];
  VoEBase* baseVE[7];
  for (int instNum = 0; instNum < 7; instNum++) {
    instVE[instNum] = VoiceEngine::Create();
    baseVE[instNum] = VoEBase::GetInterface(instVE[instNum]);
    TEST_MUSTPASS(baseVE[instNum]->Init());
    TEST_MUSTPASS(baseVE[instNum]->CreateChannel());
  }

  TEST_LOG("Created 7 more instances of VE, make sure audio is ok...\n\n");
  PAUSE

  for (int instNum = 0; instNum < 7; instNum++) {
    TEST_MUSTPASS(baseVE[instNum]->DeleteChannel(0));
    TEST_MUSTPASS(baseVE[instNum]->Terminate());
    baseVE[instNum]->Release();
    VoiceEngine::Delete(instVE[instNum]);
  }

  AOK();
  ANL();

  
  
  TEST_MUSTPASS(voe_base_->StopSend(ch));
  TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  TEST_MUSTPASS(voe_base_->StopReceive(ch));
  TEST_MUSTPASS(voe_base_->DeleteChannel(ch));

  voe_base_->DeleteChannel(0);
  TEST_MUSTPASS(voe_base_->Terminate());
  return 0;
}





int VoEExtendedTest::TestCallReport() {
  
  VoEBase* voe_base_ = _mgr.BasePtr();
  VoECallReport* report = _mgr.CallReportPtr();
  VoEFile* file = _mgr.FilePtr();
  VoEAudioProcessing* apm = _mgr.APMPtr();
  VoENetwork* voe_network = _mgr.NetworkPtr();

  PrepareTest("CallReport");

  
  if (!report) {
    TEST_LOG("VoECallReport is not supported!");
    return -1;
  }

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(
      GetFilename("VoECallReport_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo));
#endif

  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());
  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(voe_network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(0, _mgr.AudioFilename(),
          true, true));

  
  
  TEST(ResetCallReportStatistics);
  ANL();
  TEST_MUSTPASS(!report->ResetCallReportStatistics(-2));
  MARK(); 
  TEST_MUSTPASS(!report->ResetCallReportStatistics(1));
  MARK(); 
  TEST_MUSTPASS(report->ResetCallReportStatistics(0));
  MARK(); 
  TEST_MUSTPASS(report->ResetCallReportStatistics(-1));
  MARK(); 
  AOK();
  ANL();

  bool enabled = false;
  EchoStatistics echo;
  TEST(GetEchoMetricSummary);
  ANL();
  TEST_MUSTPASS(apm->GetEcMetricsStatus(enabled));
  TEST_MUSTPASS(enabled != false);
  TEST_MUSTPASS(apm->SetEcMetricsStatus(true));
  TEST_MUSTPASS(report->GetEchoMetricSummary(echo)); 
  
  AOK();
  ANL();

  
  





















  TEST(WriteReportToFile);
  ANL();

  
  char fileNameUTF8[64];

  fileNameUTF8[0] = (char) 0xce;
  fileNameUTF8[1] = (char) 0xba;
  fileNameUTF8[2] = (char) 0xce;
  fileNameUTF8[3] = (char) 0xbb;
  fileNameUTF8[4] = (char) 0xce;
  fileNameUTF8[5] = (char) 0xbd;
  fileNameUTF8[6] = (char) 0xce;
  fileNameUTF8[7] = (char) 0xbe;
  fileNameUTF8[8] = '.';
  fileNameUTF8[9] = 't';
  fileNameUTF8[10] = 'x';
  fileNameUTF8[11] = 't';
  fileNameUTF8[12] = 0;

  TEST_MUSTPASS(!report->WriteReportToFile(NULL));
  MARK();
  TEST_MUSTPASS(report->WriteReportToFile("call_report.txt"));
  MARK();
  TEST_MUSTPASS(report->WriteReportToFile(fileNameUTF8));
  MARK(); 
  AOK();
  ANL();

  TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  return 0;
}





int VoEExtendedTest::TestCodec() {
  PrepareTest("Codec");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoECodec* codec = _mgr.CodecPtr();
  VoEFile* file = _mgr.FilePtr();
  VoENetwork* voe_network = _mgr.NetworkPtr();

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(
      GetFilename("VoECodec_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo));
#endif

  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());
  ExtendedTestTransport* ptrTransport = new ExtendedTestTransport(voe_network);
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(0, *ptrTransport));
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));

  
  

  int i;
  int err;

  CodecInst cinst;

  
  

  int nCodecs;

  TEST(GetNumOfCodecs);
  ANL();
  
  nCodecs = codec->NumOfCodecs();
  MARK();
  TEST_MUSTPASS(nCodecs < 0);
  AOK();
  ANL();

  
  
  TEST(GetCodec);
  ANL();
  
  nCodecs = codec->NumOfCodecs();
  for (int index = 0; index < nCodecs; index++) {
    TEST_MUSTPASS(codec->GetCodec(index, cinst));
    TEST_LOG("[%2d] %16s: fs=%6d, pt=%4d, rate=%7d, ch=%2d, size=%5d", index, cinst.plname,
             cinst.plfreq, cinst.pltype, cinst.rate, cinst.channels, cinst.pacsize);
    if (cinst.pltype == -1) {
      TEST_LOG(" <= NOTE pt=-1\n");
    } else {
      ANL();
    }
  }

  
  TEST_MUSTPASS(-1 != codec->GetCodec(-1, cinst));
  nCodecs = codec->NumOfCodecs();
  TEST_MUSTPASS(-1 != codec->GetCodec(nCodecs, cinst));
  MARK();
  
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_LISTNR);
  AOK();
  ANL();

  
  
  TEST(GetSendCodec);
  ANL();

  CodecInst defaultCodec;

  
  TEST_MUSTPASS(-1 != codec->GetSendCodec(kTestMaxNumChannels-1, cinst));
  MARK(); 
  TEST_MUSTPASS(-1 != codec->GetSendCodec(kTestMaxNumChannels, cinst));
  MARK(); 
  TEST_MUSTPASS(-1 != codec->GetSendCodec(-1, cinst));
  MARK(); 
  TEST_MUSTPASS(codec->GetSendCodec(0, cinst));
  MARK(); 

  nCodecs = codec->NumOfCodecs();
  for (int index = 0; index < nCodecs; index++) {
    TEST_MUSTPASS(codec->GetCodec(index, defaultCodec));
    if (codec->SetSendCodec(0, defaultCodec) == 0) {
      TEST_MUSTPASS(codec->GetSendCodec(0, cinst));
      MARK();
      
      
      
      TEST_MUSTPASS(cinst.pacsize != defaultCodec.pacsize);
      TEST_MUSTPASS(cinst.plfreq != defaultCodec.plfreq);
      TEST_MUSTPASS(cinst.pltype != defaultCodec.pltype);
      TEST_MUSTPASS(cinst.rate != defaultCodec.rate);
      TEST_MUSTPASS(cinst.channels != defaultCodec.channels);
    }
  }

  ANL();
  AOK();
  ANL();

  
  
  TEST(SetSendCodec);
  ANL();

  

  nCodecs = codec->NumOfCodecs();
  for (int index = 0; index < nCodecs; index++) {
    
    TEST_MUSTPASS(codec->GetCodec(index, cinst));
    defaultCodec = cinst;
    TEST_LOG("[%2d] %s (default): fs=%d, pt=%d, rate=%d, ch=%d, size=%d\n",
             index, cinst.plname, cinst.plfreq, cinst.pltype, cinst.rate,
             cinst.channels, cinst.pacsize);

    
    if (!_stricmp("CN", cinst.plname) || !_stricmp("telephone-event",
                                                   cinst.plname)
        || !_stricmp("red", cinst.plname)) {
      
      
      TEST_MUSTPASS(!codec->SetSendCodec(0, cinst));
      err = voe_base_->LastError();
      TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
      continue;
    }

    
    
    
    if (cinst.pltype == -1) {
      cinst.pltype = 97;
    }

    
    TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

    
    TEST_LOG("\npacsize : ");

    for (int pacsize = 80; pacsize < 1440; pacsize += 80) {
      cinst.pacsize = pacsize;
      if (-1 != codec->SetSendCodec(0, cinst)) {
        
        TEST_LOG("%d ", pacsize);
      } else {
        err = voe_base_->LastError();
        TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
      }
    }
    cinst.pacsize = defaultCodec.pacsize;

    
    TEST_LOG("\nchannels: ");
    for (int channels = 1; channels < 4; channels++) {
      cinst.channels = channels;
      if (-1 != codec->SetSendCodec(0, cinst)) {
        
        
        
        TEST_LOG("%d ", channels);
      } else {
        
        
        
        err = voe_base_->LastError();
        ASSERT_TRUE((err == VE_INVALID_ARGUMENT)||
                    (err == VE_CANNOT_SET_SEND_CODEC));
      }
    }
    cinst.channels = defaultCodec.channels;

    
    TEST_LOG("\nplfreq  : ");
    cinst.plfreq = defaultCodec.plfreq;
    TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
    TEST_LOG("%d ", cinst.plfreq);

    

    strcpy(cinst.plname, "INVALID");
    TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst))
    {
      
      err = voe_base_->LastError();
      TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
    }

    
    strcpy(cinst.plname, defaultCodec.plname);

    
    TEST_LOG("\npltype  : ");
    
    cinst.pltype = defaultCodec.pltype;
    TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
    TEST_LOG("%d ", cinst.pltype);
    cinst.pltype = defaultCodec.pltype + 1;
    TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
    TEST_LOG("%d ", cinst.pltype);
    const int valid_pltypes[4] = { 0, 96, 117, 127 };
    for (i = 0; i < static_cast<int> (sizeof(valid_pltypes) / sizeof(int)); i++) {
      cinst.pltype = valid_pltypes[i];
      TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
      TEST_LOG("%d ", cinst.pltype);
    }
    
    cinst.pltype = defaultCodec.pltype;

    
    TEST_LOG("\nrate    : ");
    if (_stricmp("isac", cinst.plname) == 0) {
      
      if (cinst.plfreq == 16000) {
        int valid_rates[3] = { -1, 10000, 32000 };
        
        for (i = 0; i < static_cast<int> (sizeof(valid_rates) / sizeof(int)); i++) {
          cinst.rate = valid_rates[i];
          TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
          TEST_LOG("%d ", cinst.rate);
        }
        cinst.rate = 0; 
        TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst))
        {
          
          err = voe_base_->LastError();
          TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
        }
        ANL();
      } else 
      {
        
        int valid_rates[8] = { -1, 10000, 25000, 32000, 35000, 45000, 50000, 52000 };
        for (i = 0; i < static_cast<int> (sizeof(valid_rates) / sizeof(int)); i++) {
          cinst.rate = valid_rates[i];
          TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
          TEST_LOG("%d ", cinst.rate);
        }
        int invalid_rates[3] = { 0, 5000, 57000 }; 
        for (i = 0; i < static_cast<int> (sizeof(invalid_rates) / sizeof(int)); i++) {
          cinst.rate = invalid_rates[i];
          TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst))
          {
            
            err = voe_base_->LastError();
            TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
          }
        }
        ANL();
      }
    } else if (_stricmp("amr", cinst.plname) == 0) {
      int valid_rates[8] = { 4750, 5150, 5900, 6700, 7400, 7950, 10200, 12200 };
      for (i = 0;
          i < static_cast<int> (sizeof(valid_rates) / sizeof(int));
          i++) {
        cinst.rate = valid_rates[i];
        TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
        TEST_LOG("%d ", cinst.rate);
      }
      ANL();
    } else if (_stricmp("g7291", cinst.plname) == 0) {
      int valid_rates[12] = { 8000, 12000, 14000, 16000, 18000, 20000, 22000,
                              24000, 26000, 28000, 30000, 32000 };
      for (i = 0;
          i < static_cast<int> (sizeof(valid_rates) / sizeof(int));
          i++) {
        cinst.rate = valid_rates[i];
        TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
        TEST_LOG("%d ", cinst.rate);
      }
      ANL();
    } else if (_stricmp("amr-wb", cinst.plname) == 0) {
      int valid_rates[9] = { 7000, 9000, 12000, 14000, 16000, 18000, 20000,
                             23000, 24000 };
      for (i = 0;
          i < static_cast<int> (sizeof(valid_rates) / sizeof(int));
          i++) {
        cinst.rate = valid_rates[i];
        TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
        TEST_LOG("%d ", cinst.rate);
      }
      TEST_LOG(" <=> ");
      ANL();
    } else if (_stricmp("speex", cinst.plname) == 0) {
      
      int valid_rates[9] = { 2001, 4000, 7000, 11000, 15000, 20000, 25000,
          33000, 46000 };
      for (i = 0;
          i < static_cast<int> (sizeof(valid_rates) / sizeof(int));
          i++) {
        cinst.rate = valid_rates[i];
        TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
        TEST_LOG("%d ", cinst.rate);
      }
      cinst.rate = 2000; 
      TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst))
      {
        err = voe_base_->LastError();
        TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
      }
      ANL();
    } else if (_stricmp("silk", cinst.plname) == 0) {
      
      int valid_rates[7] = { 6000, 10000, 15000, 20000, 25000, 32000, 40000 };
      for (i = 0;
          i < static_cast<int> (sizeof(valid_rates) / sizeof(int));
          i++) {
        cinst.rate = valid_rates[i];
        TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
        TEST_LOG("%d ", cinst.rate);
      }
      cinst.rate = 5999; 
      TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst))
      {
        err = voe_base_->LastError();
        TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
      }
      cinst.rate = 40001; 
      TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst))
      {
        err = voe_base_->LastError();
        TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
      }
      ANL();
    } else {
      
      cinst.rate = defaultCodec.rate;
      TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
      TEST_LOG("%d ", cinst.rate);
      cinst.rate = defaultCodec.rate + 17;
      TEST_MUSTPASS(!codec->SetSendCodec(0, cinst));
      err = voe_base_->LastError();
      TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
      ANL();
    }
    cinst.rate = defaultCodec.rate;

    
    if (_stricmp("l16", cinst.plname) == 0) {
      if (8000 == cinst.plfreq) {
        
        cinst.pacsize = 480; 
        TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst));
        err = voe_base_->LastError();
        TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
        cinst.pacsize = 640; 
        TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst));
        err = voe_base_->LastError();
        TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
      } else {
        
        cinst.pacsize = 80; 
        TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst));
        err = voe_base_->LastError();
        TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
        cinst.pacsize = 240; 
        TEST_MUSTPASS(-1 != codec->SetSendCodec(0, cinst));
        err = voe_base_->LastError();
        TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
      }
    }
    ANL();
  }  

  
  const CodecInst tmp = { 0, "PCMU", 8000, 160, 1, 64000 };
  TEST_MUSTPASS(codec->SetSendCodec(0, tmp));

  ANL();
  AOK();
  ANL();

  
  

  const int VADSleep = 0;

  bool disabledDTX;
  VadModes mode;
  bool enabled;

  
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(enabled != false);
  TEST_MUSTPASS(mode != kVadConventional);
  TEST_MUSTPASS(disabledDTX != true);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, true));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(enabled != true);
  TEST_MUSTPASS(mode != kVadConventional);
  TEST_MUSTPASS(disabledDTX != false);
  SleepMs(VADSleep);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, true, kVadConventional));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(mode != kVadConventional);
  SleepMs(VADSleep);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, true, kVadAggressiveLow));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(mode != kVadAggressiveLow);
  SleepMs(VADSleep);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, true, kVadAggressiveMid));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(mode != kVadAggressiveMid);
  SleepMs(VADSleep);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, true, kVadAggressiveHigh));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(mode != kVadAggressiveHigh);
  SleepMs(VADSleep);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, true, kVadConventional, true));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(disabledDTX != true);
  SleepMs(VADSleep);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, false, kVadConventional, false));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(disabledDTX == false);
  SleepMs(VADSleep);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, false));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(enabled != false);
  SleepMs(VADSleep);

  
  TEST_MUSTPASS(codec->SetVADStatus(0, true));
  TEST_MUSTPASS(codec->SetVADStatus(0, false));
  TEST_MUSTPASS(codec->GetVADStatus(0, enabled, mode, disabledDTX));
  TEST_LOG("VAD: enabled=%d, mode=%d, disabledDTX=%d\n", enabled, mode,
           disabledDTX);
  TEST_MUSTPASS(enabled != false);
  TEST_MUSTPASS(mode != kVadConventional);
  TEST_MUSTPASS(disabledDTX != true);
  SleepMs(VADSleep);

  AOK();
  ANL();
  ANL();

  
  
  TEST(GetRecCodec);
  ANL();

  
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));

  
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  SleepMs(100); 

  
  CodecInst newCodec;
  for (i = 0; i < codec->NumOfCodecs(); i++) {
    TEST_MUSTPASS(codec->GetCodec(i, newCodec));
    
    if (!_stricmp("red", newCodec.plname) || !_stricmp("cn", newCodec.plname)
        || !_stricmp("telephone-event", newCodec.plname)) {
      continue; 
    }
    if (-1 != codec->SetSendCodec(0, newCodec)) {
      SleepMs(150);
      
      TEST_MUSTPASS(codec->GetRecCodec(0, cinst));
      TEST_LOG("%s %s ", newCodec.plname, cinst.plname);
      TEST_MUSTPASS(_stricmp(newCodec.plname, cinst.plname) != 0);
      TEST_MUSTPASS(cinst.pltype != newCodec.pltype);
      TEST_MUSTPASS(cinst.plfreq != newCodec.plfreq);
    }
  }

  
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));

  ANL();
  AOK();
  ANL();
  ANL();

#ifdef WEBRTC_CODEC_AMR
  
  

  
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->CreateChannel());

  TEST(SetAMREncFormat); ANL();

  
  TEST_MUSTPASS(codec->GetCodec(0, cinst));
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  
  TEST_MUSTPASS(-1 != codec->SetAMREncFormat(0)); MARK();
  TEST_MUSTPASS(-1 != codec->SetAMREncFormat(0, kRfc3267BwEfficient));
  MARK();
  TEST_MUSTPASS(-1 != codec->SetAMREncFormat(0, kRfc3267OctetAligned));
  MARK();
  TEST_MUSTPASS(-1 != codec->SetAMREncFormat(0, kRfc3267FileStorage));
  MARK();

  
  strcpy(cinst.plname,"AMR");
  cinst.channels=1; cinst.plfreq=8000; cinst.rate=12200; cinst.pltype=112;
  cinst.pacsize=160;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  
  TEST_MUSTPASS(codec->SetAMREncFormat(0)); MARK();
  TEST_MUSTPASS(codec->SetAMREncFormat(0, kRfc3267BwEfficient)); MARK();
  TEST_MUSTPASS(codec->SetAMREncFormat(0, kRfc3267OctetAligned)); MARK();
  TEST_MUSTPASS(codec->SetAMREncFormat(0, kRfc3267FileStorage)); MARK();
  TEST_MUSTPASS(-1 != codec->SetAMREncFormat(-1)); MARK();
  TEST_MUSTPASS(codec->SetAMREncFormat(0)); MARK(); 

  ANL();
  AOK();
  ANL();

  
  

  TEST(SetAMRDecFormat); ANL();

  
  
  TEST_MUSTPASS(!codec->SetAMRDecFormat(0)); MARK();
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_AUDIO_CODING_MODULE_ERROR);

  
  TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));

  
  TEST_MUSTPASS(codec->SetAMRDecFormat(0)); MARK();
  TEST_MUSTPASS(codec->SetAMRDecFormat(0, kRfc3267BwEfficient)); MARK();
  TEST_MUSTPASS(codec->SetAMRDecFormat(0, kRfc3267OctetAligned)); MARK();
  TEST_MUSTPASS(codec->SetAMRDecFormat(0, kRfc3267FileStorage)); MARK();
  TEST_MUSTPASS(-1 != codec->SetAMRDecFormat(-1)); MARK();
  TEST_MUSTPASS(codec->SetAMRDecFormat(0)); MARK(); 

  ANL();
  AOK();
  ANL();
#endif 
#ifdef WEBRTC_CODEC_AMRWB
  
  

  
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->CreateChannel());

  TEST(SetAMRWbEncFormat); ANL();

  
  TEST_MUSTPASS(codec->GetCodec(0, cinst));
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  
  TEST_MUSTPASS(-1 != codec->SetAMRWbEncFormat(0)); MARK();
  TEST_MUSTPASS(-1 != codec->SetAMRWbEncFormat(0, kRfc3267BwEfficient));
  MARK();
  TEST_MUSTPASS(-1 != codec->SetAMRWbEncFormat(0, kRfc3267OctetAligned));
  MARK();
  TEST_MUSTPASS(-1 != codec->SetAMRWbEncFormat(0, kRfc3267FileStorage));
  MARK();

  
  strcpy(cinst.plname,"AMR-WB");
  cinst.channels=1; cinst.plfreq=16000; cinst.rate=20000;
  cinst.pltype=112; cinst.pacsize=320;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  
  TEST_MUSTPASS(codec->SetAMRWbEncFormat(0)); MARK();
  TEST_MUSTPASS(codec->SetAMRWbEncFormat(0, kRfc3267BwEfficient)); MARK();
  TEST_MUSTPASS(codec->SetAMRWbEncFormat(0, kRfc3267OctetAligned)); MARK();
  TEST_MUSTPASS(codec->SetAMRWbEncFormat(0, kRfc3267FileStorage)); MARK();
  TEST_MUSTPASS(-1 != codec->SetAMRWbEncFormat(-1)); MARK();
  TEST_MUSTPASS(codec->SetAMRWbEncFormat(0)); MARK(); 

  ANL();
  AOK();
  ANL();

  
  

  TEST(SetAMRWbDecFormat); ANL();

  
  
  TEST_MUSTPASS(!codec->SetAMRWbDecFormat(0)); MARK();
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_AUDIO_CODING_MODULE_ERROR);

  
  TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));

  
  TEST_MUSTPASS(codec->SetAMRWbDecFormat(0)); MARK();
  TEST_MUSTPASS(codec->SetAMRWbDecFormat(0, kRfc3267BwEfficient)); MARK();
  TEST_MUSTPASS(codec->SetAMRWbDecFormat(0, kRfc3267OctetAligned)); MARK();
  TEST_MUSTPASS(codec->SetAMRWbDecFormat(0, kRfc3267FileStorage)); MARK();
  TEST_MUSTPASS(-1 != codec->SetAMRWbDecFormat(-1)); MARK();
  TEST_MUSTPASS(codec->SetAMRWbDecFormat(0)); MARK(); 

  ANL();
  AOK();
  ANL();
#endif 
  
  
  TEST(SetSendCNPayloadType);
  ANL();

  TEST_MUSTPASS(-1 != codec->SetSendCNPayloadType(-1, 0));
  MARK(); 

  
  TEST_MUSTPASS(-1 != codec->SetSendCNPayloadType(0, 0));
  MARK(); 
  TEST_MUSTPASS(-1 != codec->SetSendCNPayloadType(0, 95));
  MARK(); 
  TEST_MUSTPASS(-1 != codec->SetSendCNPayloadType(0, 128));
  MARK(); 
  TEST_MUSTPASS(-1 != codec->SetSendCNPayloadType(0, -1));
  MARK(); 

  
  TEST_MUSTPASS(!codec->SetSendCNPayloadType(0, 96, kFreq8000Hz));
  MARK();
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_PLFREQ);

  
  TEST_MUSTPASS(codec->SetSendCNPayloadType(0, 96, kFreq16000Hz));
  MARK();
  TEST_MUSTPASS(codec->SetSendCNPayloadType(0, 96, kFreq32000Hz));
  MARK(); 
  TEST_MUSTPASS(codec->SetSendCNPayloadType(0, 127, kFreq16000Hz));
  MARK();
  TEST_MUSTPASS(codec->SetSendCNPayloadType(0, 127, kFreq32000Hz));
  MARK();
  TEST_MUSTPASS(codec->SetSendCNPayloadType(0, 100, kFreq32000Hz));
  MARK();

  ANL();
  AOK();
  ANL();

  
  
  TEST(SetRecPayloadType);
  ANL();

  
  nCodecs = codec->NumOfCodecs();
  for (i = 0; i < nCodecs; i++) {
    TEST_MUSTPASS(codec->GetCodec(i, newCodec));
    
    if (-1 == newCodec.pltype) {
      newCodec.pltype = 127;
    }
    TEST_MUSTPASS(codec->SetRecPayloadType(0, newCodec));
    MARK(); 
    newCodec.pltype = 99;
    TEST_MUSTPASS(codec->SetRecPayloadType(0, newCodec));
    MARK(); 
    newCodec.pltype = -1;
    TEST_MUSTPASS(codec->SetRecPayloadType(0, newCodec));
    MARK(); 
  }

  ANL();
  AOK();
  ANL();

  
  
  TEST(GetRecPayloadType);
  ANL();

  CodecInst extraCodec;
  for (i = 0; i < nCodecs; i++) {
    
    TEST_MUSTPASS(codec->GetCodec(i, newCodec));
    
    if (-1 == newCodec.pltype) {
      newCodec.pltype = 127;
    }
    TEST_MUSTPASS(codec->SetRecPayloadType(0, newCodec));
    
    
    
    extraCodec.pltype = -1; 
    extraCodec.plfreq = newCodec.plfreq;
    extraCodec.rate = newCodec.rate;
    extraCodec.channels = newCodec.channels;
    strcpy(extraCodec.plname, newCodec.plname);
    
    TEST_MUSTPASS(codec->GetRecPayloadType(0, extraCodec));
    
    
    
    TEST_MUSTPASS(newCodec.pltype != extraCodec.pltype);
    TEST_MUSTPASS(newCodec.plfreq != extraCodec.plfreq);
    TEST_MUSTPASS(newCodec.channels != extraCodec.channels);
  }

  AOK();
  ANL();

  
  
  TEST(SetRecPayloadType - removing receive codecs);
  ANL();

  TEST_MUSTPASS(voe_base_->StartSend(0));
  if (file) {
    TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
        0, _mgr.AudioFilename(), true, true));
  }

  
  
  nCodecs = codec->NumOfCodecs();
  for (i = 0; i < nCodecs; i++) {
    TEST_MUSTPASS(codec->GetCodec(i, cinst));
    if (!_stricmp("red", cinst.plname) || !_stricmp("cn", cinst.plname)
        || !_stricmp("telephone-event", cinst.plname)) {
      continue; 
    }
    TEST_LOG("Testing codec: %s", cinst.plname);
    fflush(NULL);

    if (-1 == cinst.pltype) {
      
      
      cinst.pltype = 127;
    } else {
      
      memcpy(&extraCodec, &cinst, sizeof(CodecInst));
      extraCodec.pltype = -1;
      TEST_MUSTPASS(codec->SetRecPayloadType(0, extraCodec));
    }

    
    TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

    
    TEST_MUSTPASS(voe_base_->StartReceive(0));
    TEST_MUSTPASS(voe_base_->StartPlayout(0));
    TEST_LOG("  silence");
    fflush(NULL);
    SleepMs(800);
    TEST_MUSTPASS(voe_base_->StopPlayout(0));
    TEST_MUSTPASS(voe_base_->StopReceive(0));

    
    TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));

    
    TEST_MUSTPASS(voe_base_->StartReceive(0));
    TEST_MUSTPASS(voe_base_->StartPlayout(0));
    TEST_LOG("  audio");
    fflush(NULL);
    SleepMs(800);
    TEST_MUSTPASS(voe_base_->StopPlayout(0));
    TEST_MUSTPASS(voe_base_->StopReceive(0));

    if (127 == cinst.pltype) {
      
      
      
      cinst.pltype = -1;
      TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));
    }

    ANL();
  }

  
  TEST_LOG("Removing receive codecs:");
  for (i = 0; i < nCodecs; i++) {
    TEST_MUSTPASS(codec->GetCodec(i, cinst));
    if (!_stricmp("ipcmwb", cinst.plname) || !_stricmp("pcmu", cinst.plname)
        || !_stricmp("eg711a", cinst.plname)) {
      TEST_LOG(" %s", cinst.plname);
      memcpy(&extraCodec, &cinst, sizeof(CodecInst));
      extraCodec.pltype = -1;
      TEST_MUSTPASS(codec->SetRecPayloadType(0, extraCodec));
    }
  }
  ANL();

  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));

  
  TEST_LOG("Looping through send codecs \n");
  TEST_LOG("Verify that removed codecs are not audible and the other are \n");
  for (i = 0; i < nCodecs; i++) {
    TEST_MUSTPASS(codec->GetCodec(i, cinst));
    if (!_stricmp("red", cinst.plname) || !_stricmp("cn", cinst.plname)
        || !_stricmp("telephone-event", cinst.plname)) {
      continue; 
    }
    TEST_LOG("Testing codec: %s \n", cinst.plname);

    
    
    if (-1 == cinst.pltype) {
      cinst.pltype = 127;
      TEST_MUSTPASS(voe_base_->StopPlayout(0));
      TEST_MUSTPASS(voe_base_->StopReceive(0));
      TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));
      TEST_MUSTPASS(voe_base_->StartReceive(0));
      TEST_MUSTPASS(voe_base_->StartPlayout(0));
    }

    
    TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

    
    SleepMs(800);
  }

  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));

  
  TEST_LOG("Restoring receive codecs:");
  for (i = 0; i < nCodecs; i++) {
    TEST_MUSTPASS(codec->GetCodec(i, cinst));
    if (!_stricmp("ipcmwb", cinst.plname) || !_stricmp("pcmu", cinst.plname)
        || !_stricmp("eg711a", cinst.plname)) {
      TEST_LOG(" %s", cinst.plname);
      memcpy(&extraCodec, &cinst, sizeof(CodecInst));
      TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));
    }
  }
  ANL();

  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));

  
  TEST_LOG("Looping through send codecs \n");
  TEST_LOG("Verify that all codecs are audible \n");
  for (i = 0; i < nCodecs; i++) {
    TEST_MUSTPASS(codec->GetCodec(i, cinst));
    if (!_stricmp("red", cinst.plname) || !_stricmp("cn", cinst.plname)
        || !_stricmp("telephone-event", cinst.plname)) {
      continue; 
    }
    TEST_LOG("Testing codec: %s \n", cinst.plname);

    
    
    if (-1 == cinst.pltype) {
      cinst.pltype = 127;
      TEST_MUSTPASS(voe_base_->StopPlayout(0));
      TEST_MUSTPASS(voe_base_->StopReceive(0));
      TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));
      TEST_MUSTPASS(voe_base_->StartReceive(0));
      TEST_MUSTPASS(voe_base_->StartPlayout(0));
    }

    
    TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

    
    SleepMs(800);
  }

  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));

  
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->CreateChannel());

#if defined(WEBRTC_CODEC_ISAC)

  
  
  TEST(SetISACInitTargetRate);
  ANL();

  
  cinst.channels = 1;
  cinst.pacsize = 160;
  cinst.plfreq = 8000;
  strcpy(cinst.plname, "PCMU");
  cinst.pltype = 0;
  cinst.rate = 64000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACInitTargetRate(0, 10000));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_CODEC_ERROR);

  
  cinst.channels = 1;
  cinst.plfreq = 16000;
  strcpy(cinst.plname, "ISAC");
  cinst.pltype = 103;
  cinst.rate = -1; 
  cinst.pacsize = 480; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACInitTargetRate(1, 10000));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_CHANNEL_NOT_VALID);

  TEST_MUSTPASS(!codec->SetISACInitTargetRate(0, 500));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(!codec->SetISACInitTargetRate(0, 33000));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 10000));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 0));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 32000));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 32000, true));
  MARK();
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 32000, false));
  MARK();

  cinst.pacsize = 960; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 32000, false));
  MARK();

  cinst.rate = 20000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  TEST_MUSTPASS(!codec->SetISACInitTargetRate(0, 32000));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_AUDIO_CODING_MODULE_ERROR);

  cinst.rate = -1;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 32000));
  MARK(); 

  ANL();
  AOK();
  ANL();

  
  
  TEST(ISACSWB SetISACInitTargetRate);
  ANL();

  
  cinst.channels = 1;
  cinst.plfreq = 32000;
  strcpy(cinst.plname, "ISAC");
  cinst.pltype = 104;
  cinst.rate = -1; 
  cinst.pacsize = 960; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACInitTargetRate(1, 10000));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_CHANNEL_NOT_VALID);

  TEST_MUSTPASS(!codec->SetISACInitTargetRate(0, -1));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);
  TEST_MUSTPASS(!codec->SetISACInitTargetRate(0, -1));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(!codec->SetISACInitTargetRate(0, 500));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(!codec->SetISACInitTargetRate(0, 57000));
  MARK(); 

  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 10000));
  MARK();
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 0));
  MARK();
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 56000));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 56000, true));
  MARK();
  TEST_MUSTPASS(codec->SetISACInitTargetRate(0, 56000, false));
  MARK();

  ANL();
  AOK();
  ANL();

  
  
  TEST(SetISACMaxRate);
  ANL();

  
  cinst.channels = 1;
  cinst.pacsize = 160;
  cinst.plfreq = 8000;
  strcpy(cinst.plname, "PCMU");
  cinst.pltype = 0;
  cinst.rate = 64000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACMaxRate(0, 48000));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_CODEC_ERROR);

  
  cinst.channels = 1;
  cinst.plfreq = 16000;
  strcpy(cinst.plname, "ISAC");
  cinst.pltype = 103;
  cinst.rate = -1; 
  cinst.pacsize = 480; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACMaxRate(1, 48000));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_CHANNEL_NOT_VALID);

  TEST_MUSTPASS(!codec->SetISACMaxRate(0, 31900));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(!codec->SetISACMaxRate(0, 53500));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(codec->SetISACMaxRate(0, 32000));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 40000));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 48000));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 53400));
  MARK(); 

  cinst.pacsize = 960; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 48000));
  MARK();

  cinst.rate = 20000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 40000));
  MARK(); 

  ANL();
  AOK();
  ANL();

  TEST(ISACSWB SetISACMaxRate);
  ANL();
  
  cinst.channels = 1;
  cinst.plfreq = 32000;
  strcpy(cinst.plname, "ISAC");
  cinst.pltype = 104;
  cinst.rate = 45000; 
  cinst.pacsize = 960; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACMaxRate(1, 48000));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_CHANNEL_NOT_VALID);

  TEST_MUSTPASS(!codec->SetISACMaxRate(0, 31900));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(!codec->SetISACMaxRate(0, 107500));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(codec->SetISACMaxRate(0, 32000));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 40000));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 55000));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 80000));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 107000));
  MARK(); 


  cinst.rate = -1; 
  cinst.pacsize = 960; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACMaxRate(1, 48000));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_CHANNEL_NOT_VALID);

  TEST_MUSTPASS(!codec->SetISACMaxRate(0, 31900));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(!codec->SetISACMaxRate(0, 107500));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(codec->SetISACMaxRate(0, 32000));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 40000));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 55000));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 80000));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxRate(0, 107000));
  MARK(); 

  ANL();
  AOK();
  ANL();

  
  
  TEST(SetISACMaxPayloadSize);
  ANL();

  
  cinst.channels = 1;
  cinst.pacsize = 160;
  cinst.plfreq = 8000;
  strcpy(cinst.plname, "PCMU");
  cinst.pltype = 0;
  cinst.rate = 64000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACMaxPayloadSize(0, 120));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_CODEC_ERROR);

  
  cinst.channels = 1;
  cinst.plfreq = 16000;
  strcpy(cinst.plname, "ISAC");
  cinst.pltype = 103;
  cinst.rate = -1; 
  cinst.pacsize = 480; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACMaxPayloadSize(1, 120));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_CHANNEL_NOT_VALID);

  TEST_MUSTPASS(!codec->SetISACMaxPayloadSize(0, 100));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(!codec->SetISACMaxPayloadSize(0, 410));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(codec->SetISACMaxPayloadSize(0, 200));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACMaxPayloadSize(0, 120));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxPayloadSize(0, 400));
  MARK();

  ANL();
  AOK();
  ANL();

  TEST(ISACSWB SetISACMaxPayloadSize);
  ANL();
  
  cinst.channels = 1;
  cinst.plfreq = 32000;
  strcpy(cinst.plname, "ISAC");
  cinst.pltype = 104;
  cinst.rate = 45000; 
  cinst.pacsize = 960; 
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));

  TEST_MUSTPASS(!codec->SetISACMaxPayloadSize(1, 100));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_CHANNEL_NOT_VALID);

  TEST_MUSTPASS(!codec->SetISACMaxPayloadSize(0, 100));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(!codec->SetISACMaxPayloadSize(0, 610));
  MARK(); 
  err = voe_base_->LastError();
  TEST_MUSTPASS(err != VE_INVALID_ARGUMENT);

  TEST_MUSTPASS(codec->SetISACMaxPayloadSize(0, 200));
  MARK(); 
  TEST_MUSTPASS(codec->SetISACMaxPayloadSize(0, 120));
  MARK();
  TEST_MUSTPASS(codec->SetISACMaxPayloadSize(0, 600));
  MARK();

  ANL();
  AOK();
  ANL();

  
  
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(0, *ptrTransport));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  std::string output_path = webrtc::test::OutputPath();
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
          0, (output_path + "audio_long16.pcm").c_str(), true , true));
  cinst.channels = 1;
  TEST_LOG("Testing codec: Switch between iSAC-wb and iSAC-swb \n");
  TEST_LOG("Testing codec: iSAC wideband \n");
  strcpy(cinst.plname, "ISAC");
  cinst.pltype = 103;
  cinst.rate = -1; 
  cinst.pacsize = 480; 
  cinst.plfreq = 16000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  SleepMs(2000);
  TEST_LOG("             : iSAC superwideband \n");
  cinst.pltype = 104;
  cinst.rate = -1; 
  cinst.pacsize = 960; 
  cinst.plfreq = 32000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  SleepMs(2000);
  TEST_LOG("             : iSAC wideband \n");
  strcpy(cinst.plname, "ISAC");
  cinst.pltype = 103;
  cinst.rate = -1; 
  cinst.pacsize = 480; 
  cinst.plfreq = 16000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  SleepMs(2000);
  TEST_LOG("             : iSAC superwideband \n");
  cinst.pltype = 104;
  cinst.rate = -1; 
  cinst.pacsize = 960; 
  cinst.plfreq = 32000;
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  SleepMs(2000);
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
#else
  TEST_LOG("Skipping extended iSAC API tests - "
      "WEBRTC_CODEC_ISAC not defined\n");
#endif 
  TEST_MUSTPASS(voe_network ->DeRegisterExternalTransport(0));
  delete ptrTransport;

  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  return 0;
}





int VoEExtendedTest::TestDtmf() {
  PrepareTest("Dtmf");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEDtmf* dtmf = _mgr.DtmfPtr();
  VoECodec* codec = _mgr.CodecPtr();
  VoEVolumeControl* volume = _mgr.VolumeControlPtr();
  VoENetwork* voe_network = _mgr.NetworkPtr();

  std::string output_path = webrtc::test::OutputPath();
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(
      (output_path + "VoEDtmf_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo));
  
  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());
  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(voe_network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));

  
  

  
  TEST(SetDtmfFeedbackStatus & GetDtmfFeedbackStatus);
  ANL();
  bool dtmfFeedback = false, dtmfDirectFeedback = true;
  TEST_MUSTPASS(dtmf->GetDtmfFeedbackStatus(dtmfFeedback,
          dtmfDirectFeedback));
  TEST_MUSTPASS(!dtmfFeedback);
  TEST_MUSTPASS(dtmfDirectFeedback);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0));
  MARK();
  SleepMs(500);

  TEST_MUSTPASS(dtmf->SetDtmfFeedbackStatus(false, false));
  TEST_MUSTPASS(dtmf->GetDtmfFeedbackStatus(dtmfFeedback,
          dtmfDirectFeedback));
  TEST_MUSTPASS(dtmfFeedback);
  TEST_MUSTPASS(dtmfDirectFeedback);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0));
  MARK();
  SleepMs(500);

  TEST_MUSTPASS(dtmf->SetDtmfFeedbackStatus(false, true));
  TEST_MUSTPASS(dtmf->GetDtmfFeedbackStatus(dtmfFeedback,
          dtmfDirectFeedback));
  TEST_MUSTPASS(dtmfFeedback);
  TEST_MUSTPASS(!dtmfDirectFeedback);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0));
  MARK();
  SleepMs(500);

  TEST_MUSTPASS(dtmf->SetDtmfFeedbackStatus(true, false));
  TEST_MUSTPASS(dtmf->GetDtmfFeedbackStatus(dtmfFeedback,
          dtmfDirectFeedback));
  TEST_MUSTPASS(!dtmfFeedback);
  TEST_MUSTPASS(dtmfDirectFeedback);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0));
  MARK();
  SleepMs(500);

  TEST_MUSTPASS(dtmf->SetDtmfFeedbackStatus(true, true));
  TEST_MUSTPASS(dtmf->GetDtmfFeedbackStatus(dtmfFeedback,
          dtmfDirectFeedback));
  TEST_MUSTPASS(!dtmfFeedback);
  TEST_MUSTPASS(!dtmfDirectFeedback);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SetDtmfFeedbackStatus(false, false));

  AOK();
  ANL();

  
  TEST(SendDtmf);
  ANL();

  
  
  
  
  TEST_MUSTPASS(!dtmf->SendTelephoneEvent(0, -1, false, 160, 10));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!dtmf->SendTelephoneEvent(0, 16, false, 160, 10));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  
  TEST_MUSTPASS(!dtmf->SendTelephoneEvent(0, 0, true, 99, 10));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!dtmf->SendTelephoneEvent(0, 0, true, 60001, 10));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!dtmf->SendTelephoneEvent(0, 20, true, -1, 10));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  
  TEST_MUSTPASS(!dtmf->SendTelephoneEvent(0, 0, true, 160, -1));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!dtmf->SendTelephoneEvent(0, 0, true, 160, 37));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(!dtmf->SendTelephoneEvent(0, 0, true));
  MARK();
  TEST_MUSTPASS(VE_NOT_SENDING != voe_base_->LastError());
  TEST_MUSTPASS(voe_base_->StartSend(0));

  
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 16, true));
  MARK();
  SleepMs(500); 
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true, 100, 10));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true, 400, 10));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true, 160, 0));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true, 160, 36));
  MARK();
  SleepMs(500);

  
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, false));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 15, false));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, false, 100, 10));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, false, 400, 10));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, false, 160, 0));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, false, 160, 36));
  MARK();
  SleepMs(500);

  
  
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 17, true, 100, 10));
  MARK();
  SleepMs(200);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 32, true, 100, 10));
  MARK();
  SleepMs(200);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 78, true, 100, 10));
  MARK();
  SleepMs(200);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 255, true, 100, 10));
  MARK();
  SleepMs(200);
  
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 32, true, 100, 10));
  MARK();
  SleepMs(200);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 32, true, 1000, 10));
  MARK();
  SleepMs(1200);

  AOK();
  ANL();

  
  TEST(PlayDtmfTone);
  ANL();
  TEST_MUSTPASS(!dtmf->PlayDtmfTone(-1, 200, 10));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!dtmf->PlayDtmfTone(16, 200, 10));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!dtmf->PlayDtmfTone(0, 9, 10));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!dtmf->PlayDtmfTone(0, 200, -1));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!dtmf->PlayDtmfTone(0, 200, 37));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());

  TEST_MUSTPASS(dtmf->PlayDtmfTone(0));
  MARK();
  SleepMs(500);
  
  TEST_MUSTPASS(dtmf->PlayDtmfTone(0, 100, 10));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->PlayDtmfTone(0, 2000, 10));
  MARK();
  SleepMs(2300);
  TEST_MUSTPASS(dtmf->PlayDtmfTone(0, 200, 0));
  MARK();
  SleepMs(500);
  TEST_MUSTPASS(dtmf->PlayDtmfTone(0, 200, 36));
  MARK();
  SleepMs(500);

  AOK();
  ANL();

  
  TEST(SetTelephoneEventDetection);
  ANL();
  AOK();
  ANL();

  
  TEST(SendDtmf - with VAD enabled);
  ANL();
  
  TEST_MUSTPASS(volume->SetInputMute(0, true));
  MARK();
  
  TEST_MUSTPASS(codec->SetVADStatus(0, true));
  MARK();
  
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true, 400));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 9, true, 400));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true, 400));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 9, true, 400));
  MARK();
  SleepMs(1000);
  
  CodecInst ci;
#if (!defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID))
  ci.channels = 1;
  ci.pacsize = 480;
  ci.plfreq = 16000;
  strcpy(ci.plname, "ISAC");
  ci.pltype = 103;
  ci.rate = -1;
#else
  ci.pltype = 119;
  strcpy(ci.plname, "isaclc");
  ci.plfreq = 16000;
  ci.pacsize = 320;
  ci.channels = 1;
  ci.rate = 40000;
#endif
  TEST_MUSTPASS(codec->SetSendCodec(0, ci));
  MARK();
  
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true, 400));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 9, true, 400));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 0, true, 400));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(dtmf->SendTelephoneEvent(0, 9, true, 400));
  MARK();
  SleepMs(1000);
  SleepMs(4000);
  
  TEST_MUSTPASS(codec->SetVADStatus(0, false));
  MARK();
  
  TEST_MUSTPASS(volume->SetInputMute(0, false));
  MARK();

  AOK();
  ANL();

  
  TEST(SetSendTelephoneEventPayloadType);
  ANL();
  TEST_MUSTPASS(!dtmf->SetSendTelephoneEventPayloadType(0, 128));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());

  TEST_MUSTPASS(dtmf->SetSendTelephoneEventPayloadType(0, 96));
  MARK();
  TEST_MUSTPASS(dtmf->SetSendTelephoneEventPayloadType(0, 127));
  MARK();
  TEST_MUSTPASS(dtmf->SetSendTelephoneEventPayloadType(0, 106));
  MARK(); 

  AOK();
  ANL();

  TEST_MUSTPASS(dtmf->SetDtmfFeedbackStatus(true, false));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  return 0;
}





int VoEExtendedTest::TestEncryption() {
  PrepareTest("Encryption");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEFile* file = _mgr.FilePtr();
  VoENetwork* voe_network = _mgr.NetworkPtr();

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(
          GetFilename("VoEEncryption_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo));
#endif
  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());
  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(voe_network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(0, _mgr.AudioFilename(),
          true, true));

  
  

  TEST(SRTP disabled - Fail tests);
  ANL();

  
  
  
  
  

  TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  return 0;
}





int VoEExtendedTest::TestExternalMedia() {
  PrepareTest("VoEExternalMedia");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEExternalMedia* xmedia = _mgr.ExternalMediaPtr();
  VoENetwork* voe_network = _mgr.NetworkPtr();

  
  if (!xmedia) {
    TEST_LOG("VoEExternalMedia is not supported!");
    return -1;
  }

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(
          GetFilename("VoEExternalMedia_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(
          kTraceStateInfo | kTraceStateInfo | kTraceWarning |
          kTraceError | kTraceCritical | kTraceApiCall |
          kTraceMemory | kTraceInfo));
#endif
  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());
  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(voe_network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));

  int getLen = 0;
  int16_t vector[32000];
  memset(vector, 0, 32000 * sizeof(short));

#ifdef WEBRTC_VOE_EXTERNAL_REC_AND_PLAYOUT

  
  TEST(ExternalPlayoutGetData);
  ANL();

  TEST_MUSTPASS(!xmedia->SetExternalPlayoutStatus(true));
  TEST_MUSTPASS(VE_ALREADY_SENDING != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalPlayoutGetData(vector, 16000, 100, getLen));
  TEST_MUSTPASS(VE_INVALID_OPERATION != voe_base_->LastError());

  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(xmedia->SetExternalPlayoutStatus(true));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));

  TEST_MUSTPASS(xmedia->ExternalPlayoutGetData(vector, 48000, 0, getLen));
  TEST_MUSTPASS(480 != getLen);
  SleepMs(10);
  TEST_MUSTPASS(xmedia->ExternalPlayoutGetData(vector, 16000, 3000, getLen));
  TEST_MUSTPASS(160 != getLen);
  SleepMs(10);

  TEST_MUSTPASS(!xmedia->ExternalPlayoutGetData(vector, 8000, 100, getLen));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalPlayoutGetData(vector, 16000, -1, getLen));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());

  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(xmedia->SetExternalPlayoutStatus(false));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));

  
  TEST(SetExternalRecording);
  ANL();

  TEST_MUSTPASS(!xmedia->SetExternalRecordingStatus(true));
  TEST_MUSTPASS(VE_ALREADY_SENDING != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalRecordingInsertData(vector, 160, 16000, 20));
  TEST_MUSTPASS(VE_INVALID_OPERATION != voe_base_->LastError());

  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(xmedia->SetExternalRecordingStatus(true));
  TEST_MUSTPASS(voe_base_->StartSend(0));

  TEST_MUSTPASS(xmedia->ExternalRecordingInsertData(vector, 480, 48000, 0));
  SleepMs(10);
  TEST_MUSTPASS(xmedia->ExternalRecordingInsertData(vector, 640, 16000, 0));
  SleepMs(40);

  TEST_MUSTPASS(!xmedia->ExternalRecordingInsertData(vector, 160, 16000, -1));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalRecordingInsertData(vector, 80, 8000, 20));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalRecordingInsertData(vector, 0, 16000, 20));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalRecordingInsertData(vector, 80, 16000, 20));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalRecordingInsertData(vector, 500, 16000, 20));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());

  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(xmedia->SetExternalRecordingStatus(false));
  TEST_MUSTPASS(voe_base_->StartSend(0));

#else 
  TEST_MUSTPASS(!xmedia->SetExternalPlayoutStatus(true));
  TEST_MUSTPASS(VE_FUNC_NOT_SUPPORTED != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalPlayoutGetData(vector, 16000, 100, getLen));
  TEST_MUSTPASS(VE_FUNC_NOT_SUPPORTED != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->SetExternalRecordingStatus(true));
  TEST_MUSTPASS(VE_FUNC_NOT_SUPPORTED != voe_base_->LastError());
  TEST_MUSTPASS(!xmedia->ExternalRecordingInsertData(vector, 160, 16000, 20));
  TEST_MUSTPASS(VE_FUNC_NOT_SUPPORTED != voe_base_->LastError());

#endif 
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  ANL();
  AOK();
  return 0;
}





int VoEExtendedTest::TestFile() {
  PrepareTest("File");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEFile* file = _mgr.FilePtr();
  VoECodec* codec = _mgr.CodecPtr();
  VoENetwork* voe_network = _mgr.NetworkPtr();

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(
          GetFilename("VoEFile_trace.txt").c_str())); MARK();
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo));
#endif

  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());
  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(voe_network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));

  
  

  const int dT(100);

  TEST(StartPlayingFileLocally);
  ANL();
  TEST(StopPlayingFileLocally);
  ANL();

  voe_base_->StopPlayout(0);
  std::string output_path = webrtc::test::OutputPath();
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long16.pcm").c_str()));MARK();
  voe_base_->StartPlayout(0);
  MARK(); 
  SleepMs(dT);
  TEST_MUSTPASS(!file->StartPlayingFileLocally(
          0, (output_path + "audio_long16.pcm").c_str()));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_ALREADY_PLAYING);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long16.pcm").c_str()));
  MARK(); 
  SleepMs(dT);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long16.pcm").c_str(),
          false, kFileFormatPcm16kHzFile));
  MARK();
  SleepMs(dT);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long8.pcm").c_str(),
          false, kFileFormatPcm8kHzFile));
  MARK();
  SleepMs(dT);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long16.wav").c_str(),
          false, kFileFormatPcm8kHzFile));
  MARK();
  SleepMs(dT);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long8mulaw.wav").c_str(), false,
          kFileFormatPcm8kHzFile));
  MARK();
  SleepMs(dT);

  

  
  
  
  
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_short16.pcm").c_str(), false,
          kFileFormatPcm16kHzFile, 1.0, 0, 2000));
  MARK(); 
  SleepMs(2500);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  TEST_MUSTPASS(!file->StartPlayingFileLocally(
          0, (output_path + "audio_short16.pcm").c_str(), false,
          kFileFormatPcm16kHzFile, 1.0, 2000, 1000));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_BAD_FILE);
  TEST_MUSTPASS(!file->StartPlayingFileLocally(
          0, (output_path + "audio_short16.pcm").c_str(), false,
          kFileFormatPcm16kHzFile, 1.0, 21000, 30000));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_BAD_FILE);
  TEST_MUSTPASS(!file->StartPlayingFileLocally(
          0, (output_path + "audio_short16.pcm").c_str(), false,
          kFileFormatPcm16kHzFile, 1.0, 100, 100));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_BAD_FILE);
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long16.pcm").c_str()));
  MARK(); 
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  TEST_MUSTPASS(!file->StartPlayingFileLocally(0, (InStream*)NULL));
  MARK(); 
  TEST_MUSTPASS(voe_base_->LastError() != VE_BAD_FILE);

  AOK();
  ANL();

  TEST(IsPlayingFileLocally);
  ANL();

  TEST_MUSTPASS(0 != file->IsPlayingFileLocally(0));
  MARK(); 
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long16.pcm").c_str()));
  MARK();
  TEST_MUSTPASS(1 != file->IsPlayingFileLocally(0));
  MARK(); 
  AOK();
  ANL();

  TEST(ScaleLocalFilePlayout);
  ANL();
  TEST_MUSTPASS(file->ScaleLocalFilePlayout(0, 1.0));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(file->ScaleLocalFilePlayout(0, 0.0));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(file->ScaleLocalFilePlayout(0, 0.5));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(file->ScaleLocalFilePlayout(0, 0.25));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  MARK();
  AOK();
  ANL();

  
  
  
  TEST(StartPlayingFileAsMicrophone);
  ANL();
  TEST(IsPlayingFileAsMicrophone);
  ANL();
  TEST(ScaleFileAsMicrophonePlayout);
  ANL();
  CodecInst tempCodec;
  for (int ch = -1; ch < 1; ++ch) 
  {
    TEST_LOG("Testing channel = %d \n", ch);
    for (int fs = 1; fs < 4; ++fs) 
    {
      switch (fs) {
        case 1: 
          TEST_LOG("Testing with nb codec \n");
          tempCodec.channels = 1;
          tempCodec.pacsize = 160;
          tempCodec.plfreq = 8000;
          strcpy(tempCodec.plname, "PCMU");
          tempCodec.pltype = 0;
          tempCodec.rate = 64000;
          break;
        case 2: 
#ifdef WEBRTC_CODEC_ISAC
          TEST_LOG("Testing with wb codec \n");
          tempCodec.channels = 1;
          tempCodec.pacsize = 480;
          tempCodec.plfreq = 16000;
          strcpy(tempCodec.plname, "ISAC");
          tempCodec.pltype = 103;
          tempCodec.rate = 32000;
          break;
#else
          TEST_LOG("NOT testing with wb codec - "
              "WEBRTC_CODEC_ISAC not defined \n");
          continue;
#endif
        case 3: 
#ifdef WEBRTC_CODEC_PCM16
          TEST_LOG("Testing with swb codec \n");
          tempCodec.channels = 1;
          tempCodec.pacsize = 640;
          tempCodec.plfreq = 32000;
          strcpy(tempCodec.plname, "L16");
          tempCodec.pltype = 125;
          tempCodec.rate = 512000;
          break;
#else
          TEST_LOG("NOT testing with swb codec -"
              " WEBRTC_CODEC_PCM16 not defined \n");
          continue;
#endif
      }
      TEST_MUSTPASS(voe_base_->StopSend(0));
      TEST_MUSTPASS(voe_base_->StopPlayout(0));
      TEST_MUSTPASS(voe_base_->StopReceive(0));
      TEST_MUSTPASS(codec->SetRecPayloadType(0, tempCodec));
      TEST_MUSTPASS(voe_base_->StartReceive(0));
      TEST_MUSTPASS(voe_base_->StartPlayout(0));
      TEST_MUSTPASS(voe_base_->StartSend(0));
      TEST_MUSTPASS(codec->SetSendCodec(0, tempCodec));

      TEST_LOG("File 1 in 16 kHz no mix, 2 in 16 kHz mix,"
        " 3 in 8 kHz no mix, 4 in 8 kHz mix \n");

      TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
              ch, (output_path + "audio_long16.pcm").c_str()));
      MARK(); 
      SleepMs(2000);
      TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(ch));
      MARK();
      TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
              ch, (output_path + "audio_long16.wav").c_str(), false, true,
              kFileFormatWavFile));
      MARK(); 
      SleepMs(2000);
      TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(ch));
      MARK();
      TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
              ch, (output_path + "audio_long8.pcm").c_str(), false, false,
              kFileFormatPcm8kHzFile));
      MARK(); 
      SleepMs(2000);
      TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(ch));
      MARK();
      TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
              ch, (output_path + "audio_long8.pcm").c_str(), false, true,
              kFileFormatPcm8kHzFile));
      MARK(); 
      SleepMs(2000);
      TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(ch));
      MARK();
      TEST_MUSTPASS(!file->StartPlayingFileAsMicrophone(
              ch, (InStream*)NULL));
      MARK(); 
      AOK();
      ANL();

      TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
              ch, (output_path + "audio_long16.pcm").c_str()));
      TEST_MUSTPASS(1 != file->IsPlayingFileAsMicrophone(ch));
      TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(ch));
      TEST_MUSTPASS(0 != file->IsPlayingFileAsMicrophone(ch));
      AOK();
      ANL();

      TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
              ch, (output_path + "audio_long16.pcm").c_str()));
      TEST_MUSTPASS(file->ScaleFileAsMicrophonePlayout(ch, 1.0));
      MARK();
      SleepMs(1000);
      TEST_MUSTPASS(file->ScaleFileAsMicrophonePlayout(ch, 0.5));
      MARK();
      SleepMs(1000);
      TEST_MUSTPASS(file->ScaleFileAsMicrophonePlayout(ch, 0.25));
      MARK();
      SleepMs(1000);
      TEST_MUSTPASS(file->ScaleFileAsMicrophonePlayout(ch, 0.0));
      MARK();
      SleepMs(1000);
      TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(ch));
      MARK();
      AOK();
      ANL();
    }
  }

  

  CodecInst fcomp = { 0, "L16", 8000, 80, 1, 128000 };

  TEST(StartRecordingPlayout);
  ANL();
  TEST(StopRecordingPlayout);
  ANL();

  TEST_MUSTPASS(file->StartRecordingPlayout(0,
          (output_path + "rec_play16.pcm").c_str()));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingPlayout(0));
  MARK();

  fcomp.plfreq = 8000;
  strcpy(fcomp.plname, "L16");
  TEST_MUSTPASS(file->StartRecordingPlayout(0,
      (output_path + "rec_play8.wav").c_str(), &fcomp));
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingPlayout(0));
  MARK();

    fcomp.plfreq = 16000;
  strcpy(fcomp.plname, "L16");
  TEST_MUSTPASS(file->StartRecordingPlayout(0,
      (output_path + "rec_play16.wav").c_str(), &fcomp));
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingPlayout(0));
  MARK();

  fcomp.pltype = 0;
  fcomp.plfreq = 8000;
  strcpy(fcomp.plname, "PCMU");
  fcomp.rate = 64000;
  fcomp.pacsize = 160;
  fcomp.channels = 1;

  TEST_MUSTPASS(file->StartRecordingPlayout(0,
          (output_path + "rec_play_pcmu.wav").c_str(),
          &fcomp));
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingPlayout(0));
  MARK();

  fcomp.pltype = 8;
  fcomp.plfreq = 8000;
  strcpy(fcomp.plname, "PCMA");
  TEST_MUSTPASS(file->StartRecordingPlayout(0,
          (output_path + "rec_play_pcma.wav").c_str(),
          &fcomp));
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingPlayout(0));
  MARK();

  fcomp.pltype = 97;
  fcomp.pacsize = 240;
  fcomp.rate = 13300;
  fcomp.plfreq = 8000;
  strcpy(fcomp.plname, "ILBC");
  TEST_MUSTPASS(file->StartRecordingPlayout(0,
          (output_path + "rec_play.ilbc").c_str(),
          &fcomp));
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingPlayout(0));
  MARK();

  TEST_MUSTPASS(file->StartRecordingPlayout(
          -1, (output_path + "rec_play16_mixed.pcm").c_str()));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingPlayout(-1));
  MARK();

  
  TEST_LOG("\nplaying out...\n");
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "rec_play.ilbc").c_str(), false,
          kFileFormatCompressedFile));
  MARK();
  SleepMs(2000);

  AOK();
  ANL();

  
  TEST(StartRecordingMicrophone);
  ANL();
  TEST(StopRecordingMicrophone);
  ANL();

  TEST_MUSTPASS(file->StartRecordingMicrophone(
      (output_path + "rec_mic16.pcm").c_str()));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingMicrophone());
  MARK();

  voe_base_->StopSend(0);
  TEST_MUSTPASS(file->StartRecordingMicrophone(
      (output_path + "rec_mic16.pcm").c_str()));
  MARK(); 
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingMicrophone());
  MARK();
  voe_base_->StartSend(0); 

  fcomp.plfreq = 8000;
  strcpy(fcomp.plname, "L16");
  TEST_MUSTPASS(file->StartRecordingMicrophone(
          (output_path + "rec_play8.wav").c_str(), &fcomp));
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingMicrophone());
  MARK();

  fcomp.plfreq = 16000;
  strcpy(fcomp.plname, "L16");
  TEST_MUSTPASS(file->StartRecordingMicrophone(
          (output_path + "rec_play16.wav").c_str(), &fcomp));
  SleepMs(1000);
  TEST_MUSTPASS(file->StopRecordingMicrophone());
  MARK();

  
  
  TEST_LOG("StartRecordingCall, record both mic and file in specific"
    " channels \n");
  TEST_LOG("Create maxnumofchannels \n");
  for (int i = 1; i < kTestMaxNumChannels; i++) {
    int ch = voe_base_->CreateChannel();
    TEST_MUSTPASS(ch == -1);
    TEST_MUSTPASS(voe_base_->StopPlayout(ch));
  }
  scoped_ptr<VoiceChannelTransport> voice_channel_transport_1(
      new VoiceChannelTransport(voe_network, 1));
  voice_channel_transport_1->SetSendDestination("127.0.0.1", 12356);
  voice_channel_transport_1->SetLocalReceiver(12356);
  TEST_MUSTPASS(voe_base_->StartReceive(1));
  TEST_MUSTPASS(voe_base_->StopPlayout(1));
  TEST_MUSTPASS(voe_base_->StartSend(1));
  TEST_MUSTPASS(voe_base_->StartPlayout(1));

  TEST_LOG("ALways playing audio_long16.pcm for "
    "channel 0 in background \n");
  fcomp.plfreq = 16000;
  strcpy(fcomp.plname, "L16");
  TEST_LOG("Recording microphone to L16, please speak \n");
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
          0, (output_path + "audio_long16.pcm").c_str(), true , true));
  TEST_MUSTPASS(file->StartRecordingMicrophone(
          (output_path + "rec_play_ch.wav").c_str(), &fcomp));
  MARK();
  SleepMs(3000);
  TEST_MUSTPASS(file->StopRecordingMicrophone());
  MARK();
  TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(0));
  TEST_LOG("Playing recording file, you should only hear what you said \n");
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "rec_play_ch.wav").c_str(),
          false, kFileFormatWavFile));
  SleepMs(2500);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  TEST_LOG("Recording microphone 0 to L16, please speak \n");
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
          -1, (output_path + "audio_long16.pcm").c_str(), true , true));
  TEST_MUSTPASS(file->StartRecordingMicrophone(
          (output_path + "rec_play_ch_0.wav").c_str(), &fcomp));
  MARK();
  SleepMs(3000);
  TEST_MUSTPASS(file->StopRecordingMicrophone());
  MARK();
  TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(-1));
  TEST_LOG("Playing recording file, you should hear what you said and"
    " audio_long16.pcm \n");
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "rec_play_ch_0.wav").c_str(),
          false, kFileFormatWavFile));
  SleepMs(2500);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  TEST_LOG("Recording microphone to ilbc, please speak \n");
  strcpy(fcomp.plname, "ilbc");
  fcomp.plfreq = 8000;
  fcomp.pacsize = 160;
  fcomp.rate = 15200;
  fcomp.channels = 1;
  fcomp.pltype = 97;
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(
          0, (output_path + "audio_long16.pcm").c_str(), true , true));
  TEST_MUSTPASS(file->StartRecordingMicrophone(
          (output_path + "rec_play_ch_0.ilbc").c_str(), &fcomp));
  MARK();
  SleepMs(3000);
  TEST_MUSTPASS(file->StopRecordingMicrophone());
  MARK();
  TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(0));
  TEST_LOG("Playing recording file, you should only hear what you said \n");
  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "rec_play_ch_0.ilbc").c_str(), false,
          kFileFormatCompressedFile));
  SleepMs(2500);
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  for (int i = 1; i < kTestMaxNumChannels; i++) {
    TEST_MUSTPASS(voe_base_->DeleteChannel(i));
  }

  AOK();
  ANL();

  


#if !defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID)
  TEST(StartRecordingSpeakerStereo);
  ANL();
  TEST(StopRecordingSpeakerStereo);
  ANL();

  VoEHardware* hardware = _mgr.HardwarePtr();
  TEST_MUSTPASS(NULL == hardware);
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
#if defined(_WIN32)
  TEST_MUSTPASS(hardware->SetRecordingDevice(-1));
  TEST_MUSTPASS(hardware->SetPlayoutDevice(-1));
#else
  TEST_MUSTPASS(hardware->SetRecordingDevice(0));
  TEST_MUSTPASS(hardware->SetPlayoutDevice(0));
#endif
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  MARK();

  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
#if defined(_WIN32)
  TEST_MUSTPASS(hardware->SetRecordingDevice(-1));
  TEST_MUSTPASS(hardware->SetPlayoutDevice(-1));
#else
  TEST_MUSTPASS(hardware->SetRecordingDevice(0));
  TEST_MUSTPASS(hardware->SetPlayoutDevice(0));
#endif
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));

  AOK();
  ANL();
#else
  TEST_LOG("Skipping stereo record tests -"
      " WEBRTC_IOS or WEBRTC_ANDROID is defined \n");
#endif 
  

#if defined(WEBRTC_IOS) || defined(WEBRTC_ANDROID)
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
#endif

  TEST(ConvertPCMToWAV);
  ANL();

  TEST_MUSTPASS(file->ConvertPCMToWAV(
          (output_path + "audio_long16.pcm").c_str(),
          (output_path + "singleUserDemoConv.wav").c_str()));
  MARK();
  TEST_MUSTPASS(!file->ConvertPCMToWAV((InStream*)NULL,
          (OutStream*)NULL));MARK(); 
  AOK();
  ANL();

  TEST(ConvertWAVToPCM);
  ANL();

  TEST_MUSTPASS(file->ConvertWAVToPCM(
          (output_path + "audio_long16.wav").c_str(),
          (output_path + "singleUserDemoConv.pcm").c_str()));
  MARK();
  TEST_MUSTPASS(!file->ConvertWAVToPCM((InStream*)NULL, (OutStream*)NULL));
  MARK(); 
  AOK();
  ANL();

  TEST(ConvertPCMToCompressed);
  ANL();

  fcomp.plfreq = 16000;
  strcpy(fcomp.plname, "L16");
  TEST_MUSTPASS(!file->ConvertPCMToCompressed(
          (output_path + "audio_long16.pcm").c_str(),
          (output_path + "singleUserDemoConv16_dummy.wav").c_str(), &fcomp));
  MARK(); 

  fcomp.plfreq = 8000;
  strcpy(fcomp.plname, "ilbc");
  fcomp.pacsize = 160;
  fcomp.rate = 15200;
  fcomp.pltype = 97;
  fcomp.channels = 1;
  TEST_MUSTPASS(file->ConvertPCMToCompressed(
          (output_path + "audio_long16.pcm").c_str(),
          (output_path + "singleUserDemoConv.ilbc").c_str(), &fcomp));MARK();
  AOK();ANL();

  TEST(ConvertCompressedToPCM);
  ANL();

  TEST_MUSTPASS(file->ConvertCompressedToPCM(
          (output_path + "singleUserDemoConv.ilbc").c_str(),
          (output_path + "singleUserDemoConv_ilbc.pcm").c_str()));MARK();
  TEST_MUSTPASS(!file->ConvertCompressedToPCM(
          (output_path + "audio_long16.pcm").c_str(),
          (output_path + "singleUserDemoConv_dummy.pcm").c_str()));MARK();
  AOK();ANL();

#if defined(WEBRTC_IOS) || defined(WEBRTC_ANDROID)
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
#endif

  
  TEST(GetFileDuration);
  ANL();

  int dur;

  TEST_MUSTPASS(file->GetFileDuration(
          (output_path + "audio_long16.pcm").c_str(), dur));
  TEST_MUSTPASS(file->GetFileDuration(
          (output_path + "audio_long8.pcm").c_str(),
          dur, kFileFormatPcm8kHzFile));
  TEST_MUSTPASS(file->GetFileDuration(
          (output_path + "audio_long16.pcm").c_str(),
          dur, kFileFormatPcm16kHzFile));
  TEST_MUSTPASS(file->GetFileDuration(
          (output_path + "audio_long16.wav").c_str(),
          dur, kFileFormatPcm8kHzFile));
  TEST_MUSTPASS(file->GetFileDuration(
          (output_path + "singleUserDemoConv.ilbc").c_str(), dur,
          kFileFormatCompressedFile));

  AOK();
  ANL();

  TEST(GetPlaybackPosition);
  ANL();

  int pos;

  TEST_MUSTPASS(file->StartPlayingFileLocally(
          0, (output_path + "audio_long16.pcm").c_str()));
  SleepMs(1000);
  TEST_MUSTPASS(file->GetPlaybackPosition(0, pos));
  MARK(); 
  SleepMs(1000);
  TEST_MUSTPASS(file->GetPlaybackPosition(0, pos));
  MARK(); 
  
  
  
  TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  AOK();
  ANL();

  
  
  char localFiles[7][50] = { "audio_tiny8.wav", "audio_tiny11.wav",
      "audio_tiny16.wav", "audio_tiny22.wav", "audio_tiny32.wav",
      "audio_tiny44.wav", "audio_tiny48.wav" };
  char freq[7][5] = { "8", "11", "16", "22", "32", "44.1", "48" };
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  for (int i = 0; i < 7; i++) {
    TEST_LOG("Playing file %s, in %s KHz \n", localFiles[i], freq[i]);
    TEST_MUSTPASS(file->StartPlayingFileLocally(
            0, (output_path + localFiles[i]).c_str(),
            false, kFileFormatWavFile, 1));
    SleepMs(4500); 
    TEST_MUSTPASS(file->StopPlayingFileLocally(0));
  }

  
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  AOK();
  ANL();

  return 0;
}





int VoEExtendedTest::TestHardware() {
  PrepareTest("Hardware");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEHardware* hardware = _mgr.HardwarePtr();

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile((output_path +
              "VoEHardware_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo));
#endif

  
  TEST(Set/GetAudioDeviceLayer);
  ANL();
  AudioLayers wantedLayer = kAudioPlatformDefault;
  AudioLayers givenLayer;

#if defined(_WIN32)
  wantedLayer = kAudioWindowsCore;
  hardware->SetAudioDeviceLayer(wantedLayer);
  TEST_LOG("If you run on XP or below, CoreAudio "
      "should not be able to set.\n");
  TEST_LOG("If you run on Vista or above, CoreAudio "
      "should be able to set.\n");
  TEST_LOG("Verify that this is the case.\n");

  TEST_MUSTPASS(voe_base_->Init());

  TEST_MUSTPASS(hardware->GetAudioDeviceLayer(givenLayer));
  if(givenLayer == kAudioWindowsCore)
  {
    TEST_LOG("CoreAudio was set\n");
  }
  else
  {
    TEST_LOG("CoreAudio was *not* set\n");
  }

  TEST_MUSTPASS(voe_base_->Terminate());

  wantedLayer = kAudioWindowsWave;
  TEST_MUSTPASS(hardware->SetAudioDeviceLayer(wantedLayer));
  TEST_LOG("Wave audio should always be able to set.\n");

  TEST_MUSTPASS(voe_base_->Init());

  TEST_MUSTPASS(hardware->GetAudioDeviceLayer(givenLayer));
  if(givenLayer == kAudioWindowsWave)
  {
    TEST_LOG("Wave audio was set\n");
  }
  else
  {
    TEST_LOG("Wave audio was not set\n");
  }

  TEST_MUSTPASS(voe_base_->Terminate());
  
#elif defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
  wantedLayer = kAudioLinuxPulse;
  TEST_MUSTPASS(hardware->SetAudioDeviceLayer(wantedLayer));
  TEST_LOG("If you run on Linux with no/unsupported PA version, PulseAudio "
      "7should not be able to set.\n");
  TEST_LOG("If you run on Linux with supported PA version running, PulseAudio"
      " should be able to set.\n");
  TEST_LOG("Verify that this is the case.\n");

  TEST_MUSTPASS(voe_base_->Init());

  TEST_MUSTPASS(hardware->GetAudioDeviceLayer(givenLayer));
  if(givenLayer == kAudioLinuxPulse)
  {
    TEST_LOG("\nPulseAudio was set\n");
  }
  else
  {
    TEST_LOG("\nPulseAudio was not set\n");
  }

  TEST_MUSTPASS(voe_base_->Terminate());

  wantedLayer = kAudioLinuxAlsa;
  TEST_MUSTPASS(hardware->SetAudioDeviceLayer(wantedLayer));
  TEST_LOG("ALSA audio should always be able to set.\n");

  TEST_MUSTPASS(voe_base_->Init());

  TEST_MUSTPASS(hardware->GetAudioDeviceLayer(givenLayer));
  if(givenLayer == kAudioLinuxAlsa)
  {
    TEST_LOG("\nALSA audio was set\n");
  }
  else
  {
    TEST_LOG("\nALSA audio was not set\n");
  }

  TEST_MUSTPASS(voe_base_->Terminate());
#endif 
  
  wantedLayer = (AudioLayers) 17;
  TEST_MUSTPASS(hardware->SetAudioDeviceLayer(wantedLayer));
  TEST_MUSTPASS(hardware->GetAudioDeviceLayer(givenLayer));
  ASSERT_TRUE(givenLayer == kAudioPlatformDefault);
  MARK();

  
  wantedLayer = kAudioPlatformDefault;
  TEST_MUSTPASS(hardware->SetAudioDeviceLayer(wantedLayer));
  TEST_MUSTPASS(hardware->GetAudioDeviceLayer(givenLayer));
  TEST_MUSTPASS(givenLayer != wantedLayer);
  MARK();

  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());

  wantedLayer = kAudioPlatformDefault;
  TEST_MUSTPASS(-1 != hardware->SetAudioDeviceLayer(wantedLayer));
  TEST_MUSTPASS(VE_ALREADY_INITED != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(hardware->GetAudioDeviceLayer(givenLayer));
  MARK();
  switch (givenLayer) {
    case kAudioPlatformDefault:
      
      break;
    case kAudioWindowsCore:
      TEST_LOG("\nRunning kAudioWindowsCore\n");
      break;
    case kAudioWindowsWave:
      TEST_LOG("\nRunning kAudioWindowsWave\n");
      break;
    case kAudioLinuxAlsa:
      TEST_LOG("\nRunning kAudioLinuxAlsa\n");
      break;
    case kAudioLinuxPulse:
      TEST_LOG("\nRunning kAudioLinuxPulse\n");
      break;
    default:
      TEST_LOG("\nERROR: Running unknown audio layer!!\n");
      return -1;
  }
  ANL();

#if !defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID)
  
  TEST(Getrecording/PlayoutDeviceStatus);
  ANL();
  bool isRecAvailable = false;
  bool isPlayAvailable = false;
  TEST_MUSTPASS(hardware->GetRecordingDeviceStatus(isRecAvailable));
  TEST_MUSTPASS(!isRecAvailable);
  MARK();
  TEST_MUSTPASS(hardware->GetPlayoutDeviceStatus(isPlayAvailable));
  TEST_MUSTPASS(!isPlayAvailable);
  MARK();

  ANL();

  int nRec = 0, nPlay = 0;
  char devName[128];
  char guidName[128];
  int idx;

  TEST_MUSTPASS(hardware->GetNumOfPlayoutDevices(nPlay));

  
  TEST(GetPlayoutDeviceName);
  ANL();
  TEST_MUSTPASS(-1 != hardware->GetPlayoutDeviceName(nPlay, devName,
          guidName));
  TEST_MUSTPASS(VE_CANNOT_RETRIEVE_DEVICE_NAME != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(-1 != hardware->GetPlayoutDeviceName(-2, devName, guidName));
  TEST_MUSTPASS(VE_CANNOT_RETRIEVE_DEVICE_NAME != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(-1 != hardware->GetPlayoutDeviceName(nPlay+1, devName,
          guidName));
  TEST_MUSTPASS(VE_CANNOT_RETRIEVE_DEVICE_NAME != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(-1 != hardware->GetPlayoutDeviceName(0, NULL, guidName));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(hardware->GetPlayoutDeviceName(0, devName, NULL));

  
  for (idx = 0; idx < nPlay; idx++) {
    TEST_MUSTPASS(hardware->GetPlayoutDeviceName(idx, devName, guidName));
    MARK();
    TEST_MUSTPASS(hardware->SetPlayoutDevice(idx));
  }

  ANL();

  TEST_MUSTPASS(hardware->GetNumOfRecordingDevices(nRec));

  
  TEST(GetRecordingDeviceName);
  ANL();
  TEST_MUSTPASS(-1 != hardware->GetRecordingDeviceName(nRec, devName,
          guidName));
  TEST_MUSTPASS(VE_CANNOT_RETRIEVE_DEVICE_NAME != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(-1 != hardware->GetRecordingDeviceName(-2, devName, guidName));
  TEST_MUSTPASS(VE_CANNOT_RETRIEVE_DEVICE_NAME != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(-1 != hardware->GetRecordingDeviceName(nRec+1, devName,
          guidName));
  TEST_MUSTPASS(VE_CANNOT_RETRIEVE_DEVICE_NAME != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(-1 != hardware->GetRecordingDeviceName(0, NULL, guidName));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(hardware->GetRecordingDeviceName(0, devName, NULL));

  
  for (idx = 0; idx < nRec; idx++) {
    TEST_MUSTPASS(hardware->GetRecordingDeviceName(idx, devName, guidName));
    MARK();
    TEST_MUSTPASS(hardware->SetRecordingDevice(idx));
  }
  ANL();

    
  TEST(SetRecordingDevice);
  ANL();
  TEST_MUSTPASS(hardware->SetRecordingDevice(0));
  MARK();
  TEST_MUSTPASS(hardware->SetRecordingDevice(0, kStereoLeft));
  MARK();
  TEST_MUSTPASS(hardware->SetRecordingDevice(0, kStereoRight));
  MARK();
  ANL();

  
  TEST(SetPlayoutDevice);
  ANL();
#if defined(_WIN32)
  TEST_MUSTPASS(hardware->SetPlayoutDevice(-1)); MARK();
#else
  TEST_MUSTPASS(hardware->SetPlayoutDevice(0));
  MARK();
#endif
  ANL();
#endif 
#if defined(WEBRTC_IOS)
  TEST(ResetSoundDevice); ANL();

  for (int p=0; p<=60; p+=20)
  {
    TEST_LOG("Resetting sound device several times with pause %d ms\n", p);
    for (int l=0; l<50; ++l)
    {
      TEST_MUSTPASS(hardware->ResetAudioDevice()); MARK();
      SleepMs(p);
    }
    ANL();
  }

  TEST_LOG("Start streaming - verify the audio after each batch of resets \n");
  TEST_MUSTPASS(voe_base_->SetSendDestination(0, 8000, "127.0.0.1"));
  TEST_MUSTPASS(voe_base_->SetLocalReceiver(0,8000));
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  SleepMs(2000);

  SleepMs(2000);
  for (int p=0; p<=60; p+=20)
  {
    TEST_LOG("Resetting sound device several time with pause %d ms\n", p);
    for (int l=0; l<20; ++l)
    {
      TEST_MUSTPASS(hardware->ResetAudioDevice()); MARK();
      SleepMs(p);
    }
    ANL();
    SleepMs(2000);
  }

  TEST_LOG("Stop streaming \n");
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartReceive(0));
#endif 
#ifdef WEBRTC_IOS
  TEST_LOG("\nNOTE: Always run hardware tests also without extended tests "
      "enabled,\nsince the extended tests are pre-streaming tests only.\n");
#endif

  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  ANL();
  AOK();

  return 0;
}





int VoEExtendedTest::TestNetEqStats() {
  PrepareTest("NetEqStats (!EMPTY!)");

  AOK();
  ANL();

  return 0;
}






int VoEExtendedTest::TestNetwork() {
  PrepareTest("Network");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoENetwork* voe_network = _mgr.NetworkPtr();

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile((output_path +
              "VoENetwork_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo));
#endif

  TEST_MUSTPASS(voe_base_->Init());

  
  
  
  
  
  
  
  TEST(SetExternalTransport);
  ANL();

  ExtendedTestTransport* ptrTransport = new ExtendedTestTransport(voe_network);

  
  TEST_MUSTPASS(!voe_network ->DeRegisterExternalTransport(0));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);

  TEST_MUSTPASS(voe_base_->CreateChannel());

  
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(0, *ptrTransport));
  MARK();
  TEST_MUSTPASS(voe_network ->DeRegisterExternalTransport(0));
  MARK();
  TEST_MUSTPASS(voe_network ->DeRegisterExternalTransport(0));
  MARK();
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(0, *ptrTransport));
  MARK();
  TEST_MUSTPASS(!voe_network ->RegisterExternalTransport(0, *ptrTransport));
  MARK(); 
  TEST_MUSTPASS(voe_network ->DeRegisterExternalTransport(0));
  MARK();

  
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->CreateChannel());

  
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(0, *ptrTransport));
  MARK();
  TEST_MUSTPASS(voe_base_->StartSend(0)); 
  TEST_MUSTPASS(!voe_network ->RegisterExternalTransport(0, *ptrTransport));
  MARK(); 
  TEST_MUSTPASS(voe_network ->DeRegisterExternalTransport(0));
  MARK();
  TEST_MUSTPASS(voe_network ->RegisterExternalTransport(0, *ptrTransport));
  MARK();
  Play(0, 2000, true, true); 
  TEST_MUSTPASS(voe_network ->DeRegisterExternalTransport(0));
  MARK();

  
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));

  
  
  TEST_MUSTPASS(voe_network ->DeRegisterExternalTransport(0));
  MARK();
  delete ptrTransport;

  TEST_MUSTPASS(voe_base_->DeleteChannel(0));

  ANL();
  AOK();
  ANL();
  ANL();

  
  
  return 0;
}






class RTPAudioTransport: public Transport {
 public:
  RTPAudioTransport() :
    mute_(false) {
  }

  virtual ~RTPAudioTransport() {
  }

  void set_mute(bool mute) {
    mute_ = mute;
  }
  bool mute() const {
    return mute_;
  }

  
  virtual int SendPacket(int channel, const void* data, int length) {
    const uint8_t* packet = static_cast<const uint8_t*> (data);

    
    assert(packet[0] & 0x10);
    int index = 12; 
    
    assert(packet[index++] == 0xBE);
    assert(packet[index++] == 0xDE);
    
    assert(packet[index++] == 0x00);
    assert(packet[index++] == 0x01);

    
    assert(((packet[index] & 0xf0) >> 4) == 1);
    
    assert((packet[index++] & 0x0f) == 0);

    int vad = packet[index] >> 7;
    int level = packet[index] & 0x7f;
    if (channel == 0) {
      printf("%d    -%d\n", vad, level);
    } else if (channel == 1) {
      printf("             %d    -%d\n", vad, level);
    } else {
      assert(false);
    }

    if (mute_) {
      assert(vad == 0);
      assert(level == 127);
    } else {
      assert(vad == 0 || vad == 1);
      assert(level >= 0 && level <= 127);
    }
    return 0;
  }

  virtual int SendRTCPPacket(int , const void* ,
                             int ) {
    return 0;
  }

 private:
  bool mute_;
};

int VoEExtendedTest::TestRTP_RTCP() {
  PrepareTest("RTP_RTCP");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEFile* file = _mgr.FilePtr();
  VoERTP_RTCP* rtp_rtcp = _mgr.RTP_RTCPPtr();
  VoENetwork* network = _mgr.NetworkPtr();
  VoEVolumeControl* volume = _mgr.VolumeControlPtr();
  VoECodec* codec = _mgr.CodecPtr();

  XRTPObserver rtpObserver;

#ifdef WEBRTC_ANDROID
  int sleepTime = 200;
#else
  int sleepTime = 100;
#endif

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile((output_path +
              "VoERTP_RTCP_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
          kTraceStateInfo |
          kTraceWarning |
          kTraceError |
          kTraceCritical |
          kTraceApiCall |
          kTraceMemory |
          kTraceInfo));
#endif

  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());

  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(network, 0));

  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);

  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));

  
  

  
  
  TEST(SetRTPAudioLevelIndicationStatus);
  ANL();
  TEST(GetRTPAudioLevelIndicationStatus);

  
  TEST_MUSTPASS(-1 != rtp_rtcp->SetRTPAudioLevelIndicationStatus(0, true, 0));
  MARK();
  TEST_ERROR(VE_INVALID_ARGUMENT);
  TEST_MUSTPASS(-1 != rtp_rtcp->SetRTPAudioLevelIndicationStatus(0, true, 15));
  MARK();
  TEST_ERROR(VE_INVALID_ARGUMENT);
  TEST_MUSTPASS(-1 != rtp_rtcp->SetRTPAudioLevelIndicationStatus(1, true, 5));
  MARK();
  TEST_ERROR(VE_CHANNEL_NOT_VALID);

  
  TEST_MUSTPASS(0 != rtp_rtcp->SetRTPAudioLevelIndicationStatus(0, false, 0));
  MARK();

  
  bool audioLevelEnabled(false);
  unsigned char ID(0);
  for (int id = 1; id < 15; id++) {
    TEST_MUSTPASS(rtp_rtcp->SetRTPAudioLevelIndicationStatus(0, true, id));
    MARK();
    TEST_MUSTPASS(rtp_rtcp->GetRTPAudioLevelIndicationStatus(
            0, audioLevelEnabled, ID));
    MARK();
    TEST_MUSTPASS(audioLevelEnabled != true);
    TEST_MUSTPASS(rtp_rtcp->SetRTPAudioLevelIndicationStatus(0, false, id));
    MARK();
    TEST_MUSTPASS(rtp_rtcp->GetRTPAudioLevelIndicationStatus(
            0, audioLevelEnabled, ID));
    MARK();
    TEST_MUSTPASS(audioLevelEnabled != false);
    TEST_MUSTPASS(ID != id);
  }
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));

  RTPAudioTransport rtpAudioTransport;
  TEST_MUSTPASS(voe_base_->CreateChannel());
  TEST_MUSTPASS(network->RegisterExternalTransport(0, rtpAudioTransport));
  TEST_MUSTPASS(rtp_rtcp->SetRTPAudioLevelIndicationStatus(0, true));
  TEST_MUSTPASS(codec->SetVADStatus(0, true));

  printf("\n\nReceving muted packets (expect VAD = 0, Level = -127)...\n");
  printf("VAD  Level [dbFS]\n");
  SleepMs(2000);
  rtpAudioTransport.set_mute(true);
  TEST_MUSTPASS(volume->SetInputMute(0, true));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  SleepMs(5000);
  TEST_MUSTPASS(voe_base_->StopSend(0));
  rtpAudioTransport.set_mute(false);
  TEST_MUSTPASS(volume->SetInputMute(0, false));

  printf("\nReceiving packets from mic (should respond to mic level)...\n");
  printf("VAD  Level [dbFS]\n");
  SleepMs(2000);
  TEST_MUSTPASS(voe_base_->StartSend(0));
  SleepMs(5000);
  TEST_MUSTPASS(voe_base_->StopSend(0));

  printf("\nReceiving packets from file (expect mostly VAD = 1)...\n");
  printf("VAD  Level [dbFS]\n");
  SleepMs(2000);
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(0, _mgr.AudioFilename(),
          true, true));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  SleepMs(5000);
  TEST_MUSTPASS(voe_base_->StopSend(0));

  printf("\nMuted and mic on independent channels...\n");
  printf("Muted        Mic\n");
  SleepMs(2000);
  ASSERT_TRUE(1 == voe_base_->CreateChannel());
  TEST_MUSTPASS(network->RegisterExternalTransport(1, rtpAudioTransport));
  TEST_MUSTPASS(rtp_rtcp->SetRTPAudioLevelIndicationStatus(1, true));
  TEST_MUSTPASS(codec->SetVADStatus(1, true));
  TEST_MUSTPASS(volume->SetInputMute(0, true));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartSend(1));
  SleepMs(5000);
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopSend(1));

  TEST_MUSTPASS(network->DeRegisterExternalTransport(0));
  TEST_MUSTPASS(network->DeRegisterExternalTransport(1));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(1));
  voice_channel_transport.reset(NULL);

  TEST_MUSTPASS(voe_base_->CreateChannel());
  voice_channel_transport.reset(new VoiceChannelTransport(network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12347);
  voice_channel_transport->SetLocalReceiver(12347);

  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));

  MARK();
  ANL();

  
  

  int i(0);

  TEST(SetLocalSSRC);
  TEST_MUSTPASS(!rtp_rtcp->SetLocalSSRC(0, 5678));
  MARK();
  TEST_MUSTPASS(VE_ALREADY_SENDING != voe_base_->LastError());
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(rtp_rtcp->SetLocalSSRC(0, 5678)); 
  TEST_MUSTPASS(voe_base_->StartSend(0));
  MARK();
  ANL();

  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(0, _mgr.AudioFilename(),
          true, true));

  
  
  TEST(Start/StopRtpDump);
  ANL();
  TEST(Start/RTPDumpIsActive);

  TEST_MUSTPASS(-1 != rtp_rtcp->RTPDumpIsActive(-1, kRtpIncoming));
  MARK(); 
  TEST_ERROR(VE_CHANNEL_NOT_VALID);
  TEST_MUSTPASS(false != rtp_rtcp->RTPDumpIsActive(0, kRtpIncoming));
  MARK(); 
  TEST_MUSTPASS(false != rtp_rtcp->RTPDumpIsActive(0, kRtpOutgoing));
  MARK(); 

  TEST_MUSTPASS(-1 != rtp_rtcp->StartRTPDump(-1, NULL));
  MARK(); 
  TEST_ERROR(VE_CHANNEL_NOT_VALID);
  TEST_MUSTPASS(-1 != rtp_rtcp->StartRTPDump(0, NULL));
  MARK(); 
  TEST_ERROR(VE_BAD_FILE);

  

  
  
  
  TEST_MUSTPASS(rtp_rtcp->StopRTPDump(0));
  MARK();
  TEST_MUSTPASS(rtp_rtcp->StopRTPDump(0, kRtpIncoming));
  MARK();
  TEST_MUSTPASS(rtp_rtcp->StopRTPDump(0, kRtpOutgoing));
  MARK();
  std::string output_path = webrtc::test::OutputPath();
  TEST_MUSTPASS(rtp_rtcp->StartRTPDump(
      0, (output_path + "dump_in_1sec.rtp").c_str(), kRtpIncoming));
  MARK();
  TEST_MUSTPASS(rtp_rtcp->StartRTPDump(
      0, (output_path + "dump_out_2sec.rtp").c_str(), kRtpOutgoing));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(rtp_rtcp->StopRTPDump(0, kRtpIncoming));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(rtp_rtcp->StopRTPDump(0, kRtpOutgoing));
  MARK();

  
  
  
  
  for (i = 0; i < 10; i++) {
    TEST_MUSTPASS(rtp_rtcp->StartRTPDump(0,
            (output_path + "dump_in_200ms.rtp").c_str()));
    MARK();
    SleepMs(200);
    TEST_MUSTPASS(rtp_rtcp->StopRTPDump(0));
    MARK();
  }

  
  
  ANL();

  TEST(GetRTCPStatus);
  bool enabled;
  TEST_MUSTPASS(!rtp_rtcp->GetRTCPStatus(-1, enabled));
  MARK();
  TEST_MUSTPASS(rtp_rtcp->GetRTCPStatus(0, enabled));
  MARK(); 
  TEST_MUSTPASS(enabled != true);
  ANL();

  TEST(SetRTCPStatus);
  TEST_MUSTPASS(rtp_rtcp->SetRTCPStatus(0, false));
  MARK();
  TEST_MUSTPASS(rtp_rtcp->GetRTCPStatus(0, enabled));
  TEST_MUSTPASS(enabled != false);
  MARK();
  SleepMs(2000);
  TEST_MUSTPASS(rtp_rtcp->SetRTCPStatus(0, true));
  MARK();
  TEST_MUSTPASS(rtp_rtcp->GetRTCPStatus(0, enabled));
  TEST_MUSTPASS(enabled != true);
  MARK();
  SleepMs(6000); 
  ANL();

  TEST(CNAME);
  TEST_MUSTPASS(!rtp_rtcp->SetRTCP_CNAME(0, NULL));
  MARK();
  TEST_MUSTPASS(VE_RTP_RTCP_MODULE_ERROR != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(!rtp_rtcp->GetRemoteRTCP_CNAME(0, NULL));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  MARK();
  ANL();

  TEST(GetRemoteSSRC);
  unsigned int ssrc(0);
  TEST_MUSTPASS(rtp_rtcp->GetRemoteSSRC(0, ssrc));
  MARK();
  TEST_MUSTPASS(ssrc != 5678);
  ANL();

  TEST(GetRemoteCSRC); 
  unsigned int csrcs[2];
  int n(0);
  TEST_MUSTPASS(!rtp_rtcp->GetRemoteCSRCs(1, csrcs));
  MARK();
  n = rtp_rtcp->GetRemoteCSRCs(0, csrcs);
  MARK();
  TEST_MUSTPASS(n != 0); 
  ANL();

  TEST(SetRTPObserver);
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(rtp_rtcp->RegisterRTPObserver(0, rtpObserver));
  TEST_MUSTPASS(rtp_rtcp->DeRegisterRTPObserver(0));
  TEST_MUSTPASS(rtp_rtcp->RegisterRTPObserver(0, rtpObserver));
  TEST_MUSTPASS(rtp_rtcp->SetLocalSSRC(0, 7777)); 
  TEST_MUSTPASS(voe_base_->StartSend(0));
  SleepMs(sleepTime);
  
  TEST_MUSTPASS(rtpObserver._SSRC != 7777);
  TEST_MUSTPASS(rtp_rtcp->DeRegisterRTPObserver(0));
  ANL();

  
  TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  voice_channel_transport.reset(NULL);

  SleepMs(100);

  TEST_MUSTPASS(voe_base_->CreateChannel());

  voice_channel_transport.reset(new VoiceChannelTransport(network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);

  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(0, _mgr.AudioFilename(),
          true, true));

  SleepMs(8000);

  TEST(GetRemoteRTCPData);
  
  
  unsigned int NTPHigh(0), NTPLow(0), timestamp(0), playoutTimestamp(0),
      jitter(0);
  unsigned short fractionLost(0);
  TEST_MUSTPASS(rtp_rtcp->GetRemoteRTCPData(0, NTPHigh, NTPLow,
          timestamp, playoutTimestamp));
  TEST_LOG("\n    NTPHigh = %u \n    NTPLow = %u \n    timestamp = %u \n  "
    "  playoutTimestamp = %u \n    jitter = %u \n    fractionLost = %hu \n",
    NTPHigh, NTPLow, timestamp, playoutTimestamp, jitter, fractionLost);

  unsigned int NTPHigh2(0), NTPLow2(0), timestamp2(0);
  unsigned int playoutTimestamp2(0), jitter2(0);
  unsigned short fractionLost2(0);

  TEST_LOG("take a new sample and ensure that the playout timestamp is "
    "maintained");
  SleepMs(100);
  TEST_MUSTPASS(rtp_rtcp->GetRemoteRTCPData(0, NTPHigh2, NTPLow2, timestamp2,
          playoutTimestamp2, &jitter2,
          &fractionLost2));
  TEST_LOG("\n    NTPHigh = %u \n    NTPLow = %u \n    timestamp = %u \n  "
    "  playoutTimestamp = %u \n    jitter = %u \n    fractionLost = %hu \n",
    NTPHigh2, NTPLow2, timestamp2, playoutTimestamp2, jitter2, fractionLost2);
  TEST_MUSTPASS(playoutTimestamp != playoutTimestamp2);

  TEST_LOG("wait for 8 seconds and ensure that the RTCP statistics is"
    " updated...");
  SleepMs(8000);
  TEST_MUSTPASS(rtp_rtcp->GetRemoteRTCPData(0, NTPHigh2, NTPLow2,
          timestamp2, playoutTimestamp2,
          &jitter2, &fractionLost2));
  TEST_LOG("\n    NTPHigh = %u \n    NTPLow = %u \n    timestamp = %u \n  "
    "  playoutTimestamp = %u \n    jitter = %u \n    fractionLost = %hu \n",
    NTPHigh2, NTPLow2, timestamp2, playoutTimestamp2, jitter2, fractionLost2);
  TEST_MUSTPASS((NTPHigh == NTPHigh2) && (NTPLow == NTPLow2));
  TEST_MUSTPASS(timestamp == timestamp2);
  TEST_MUSTPASS(playoutTimestamp == playoutTimestamp2);
  CodecInst cinst;
#ifdef WEBRTC_CODEC_RED
  TEST_LOG("Turn FEC and VAD on and wait for 4 seconds and ensure that "
    "the jitter is still small...");
#if (!defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID))
  cinst.pltype = 104;
  strcpy(cinst.plname, "isac");
  cinst.plfreq = 32000;
  cinst.pacsize = 960;
  cinst.channels = 1;
  cinst.rate = 45000;
#else
  cinst.pltype = 119;
  strcpy(cinst.plname, "isaclc");
  cinst.plfreq = 16000;
  cinst.pacsize = 320;
  cinst.channels = 1;
  cinst.rate = 40000;
#endif
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));
  TEST_MUSTPASS(codec->SetSendCodec(0, cinst));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(rtp_rtcp->SetFECStatus(0, true, 126));
  MARK();
  TEST_MUSTPASS(codec->SetVADStatus(0,true));
  SleepMs(4000);
  TEST_MUSTPASS(rtp_rtcp->GetRemoteRTCPData(0, NTPHigh2, NTPLow2, timestamp2,
          playoutTimestamp2, &jitter2,
          &fractionLost2));
  TEST_LOG("\n    NTPHigh = %u \n    NTPLow = %u \n    timestamp = %u \n "
    "   playoutTimestamp = %u \n    jitter = %u \n   fractionLost = %hu \n",
    NTPHigh2, NTPLow2, timestamp2, playoutTimestamp2, jitter2, fractionLost2);
  TEST_MUSTPASS(jitter2 > 1000)
  TEST_MUSTPASS(rtp_rtcp->SetFECStatus(0, false));
  MARK();
#endif 

  TEST(GetRTPStatistics);
  ANL();
  
  CallStatistics stats;
  
  
  unsigned int averageJitterMs, maxJitterMs, discardedPackets;
  SleepMs(1000);
  for (i = 0; i < 8; i++) {
    TEST_MUSTPASS(rtp_rtcp->GetRTPStatistics(0, averageJitterMs,
            maxJitterMs,
            discardedPackets));
    TEST_LOG("    %i) averageJitterMs = %u \n    maxJitterMs = %u \n  "
      "  discardedPackets = %u \n", i, averageJitterMs, maxJitterMs,
      discardedPackets);
    SleepMs(1000);
  }

  TEST(RTCPStatistics #1);
  ANL();
  unsigned int packetsSent(0);
  unsigned int packetsReceived(0);
  for (i = 0; i < 8; i++)
  {
    TEST_MUSTPASS(rtp_rtcp->GetRTCPStatistics(0, stats));
    TEST_LOG("    %i) fractionLost = %hu \n    cumulativeLost = %u \n  "
        "  extendedMax = %u \n    jitterSamples = %u \n    rttMs = %d \n",
        i, stats.fractionLost, stats.cumulativeLost,
        stats.extendedMax, stats.jitterSamples, stats.rttMs);
    TEST_LOG( "    bytesSent = %d \n    packetsSent = %d \n   "
        " bytesReceived = %d \n    packetsReceived = %d \n",
        stats.bytesSent, stats.packetsSent, stats.bytesReceived,
        stats.packetsReceived);
    if (i > 0)
    {
      TEST_LOG("    diff sent packets    : %u (~50)\n",
               stats.packetsSent - packetsSent);
      TEST_LOG("    diff received packets: %u (~50)\n",
               stats.packetsReceived - packetsReceived);
    }
    packetsSent = stats.packetsSent;
    packetsReceived = stats.packetsReceived;
    SleepMs(1000);
  }

  TEST(RTCPStatistics #2);
  ANL();
  TEST_LOG("restart sending and ensure that the statistics is reset");
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  SleepMs(50); 
  TEST_MUSTPASS(rtp_rtcp->GetRTCPStatistics(0, stats));
  TEST_LOG("\n    fractionLost = %hu \n    cumulativeLost = %u \n  "
      "  extendedMax = %u \n    jitterSamples = %u \n    rttMs = %d \n",
      stats.fractionLost, stats.cumulativeLost,
      stats.extendedMax, stats.jitterSamples, stats.rttMs);
  TEST_LOG( "    bytesSent = %d \n    packetsSent = %d \n   "
      " bytesReceived = %d \n    packetsReceived = %d \n",
      stats.bytesSent, stats.packetsSent, stats.bytesReceived,
      stats.packetsReceived);

  TEST(RTCPStatistics #3);
  ANL();
  TEST_LOG("disable RTCP and verify that statistics is not corrupt");
  TEST_MUSTPASS(rtp_rtcp->SetRTCPStatus(0, false));
  SleepMs(250);
  TEST_MUSTPASS(rtp_rtcp->GetRTCPStatistics(0, stats));
  TEST_LOG("\n    fractionLost = %hu \n    cumulativeLost = %u \n   "
      " extendedMax = %u \n    jitterSamples = %u \n    rttMs = %d \n",
      stats.fractionLost, stats.cumulativeLost,
      stats.extendedMax, stats.jitterSamples, stats.rttMs);
  TEST_LOG("    bytesSent = %d \n    packetsSent = %d \n    "
      "bytesReceived = %d \n    packetsReceived = %d \n",
      stats.bytesSent, stats.packetsSent,
      stats.bytesReceived, stats.packetsReceived);
  TEST_MUSTPASS(rtp_rtcp->SetRTCPStatus(0, true));

  TEST(RTCPStatistics #4);
  ANL();
  TEST_LOG("restart receiving and check RX statistics");
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  SleepMs(50); 
  TEST_MUSTPASS(rtp_rtcp->GetRTCPStatistics(0, stats));
  TEST_LOG("\n    fractionLost = %hu \n    cumulativeLost = %u \n   "
      " extendedMax = %u \n    jitterSamples = %u \n    rttMs = %d \n",
      stats.fractionLost, stats.cumulativeLost,
      stats.extendedMax, stats.jitterSamples,
      stats.rttMs);
  TEST_LOG("    bytesSent = %d \n    packetsSent = %d \n   "
      " bytesReceived = %d \n    packetsReceived = %d \n",
      stats.bytesSent, stats.packetsSent,
      stats.bytesReceived, stats.packetsReceived);

  TEST(SendApplicationDefinedRTCPPacket);
  
  TEST_MUSTPASS(voe_base_->StopSend(0));
  
  TEST_MUSTPASS(!rtp_rtcp->SendApplicationDefinedRTCPPacket(
      0, 0, 0, "abcdabcdabcdabcdabcdabcdabcdabcd", 32));
  MARK();
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_MUSTPASS(rtp_rtcp->SendApplicationDefinedRTCPPacket(
      0, 0, 0, "abcdabcdabcdabcdabcdabcdabcdabcd", 32));
  MARK();
  TEST_MUSTPASS(rtp_rtcp->SetRTCPStatus(0, false));
  
  TEST_MUSTPASS(!rtp_rtcp->SendApplicationDefinedRTCPPacket(
      0, 0, 0, "abcdabcdabcdabcdabcdabcdabcdabcd", 32));
  MARK();
  TEST_MUSTPASS(rtp_rtcp->SetRTCPStatus(0, true));
  TEST_MUSTPASS(rtp_rtcp->SendApplicationDefinedRTCPPacket(
      0, 0, 0, "abcdabcdabcdabcdabcdabcdabcdabcd", 32));
  MARK();
  
  TEST_MUSTPASS(!rtp_rtcp->SendApplicationDefinedRTCPPacket(
      0, 0, 0, "abcdabcdabcdabcdabcdabcdabcdabc", 31));
  MARK();
  
  TEST_MUSTPASS(!rtp_rtcp->SendApplicationDefinedRTCPPacket(0, 0, 0, NULL, 0));
  MARK();
  ANL();

#ifdef WEBRTC_CODEC_RED
  TEST(SetFECStatus);
  ANL();
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  cinst.pltype = 126;
  strcpy(cinst.plname, "red");
  cinst.plfreq = 8000;
  cinst.pacsize = 0;
  cinst.channels = 1;
  cinst.rate = 0;
  TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));
#if (!defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID))
  cinst.pltype = 104;
  strcpy(cinst.plname, "isac");
  cinst.plfreq = 32000;
  cinst.pacsize = 960;
  cinst.channels = 1;
  cinst.rate = 45000;
#else
  cinst.pltype = 119;
  strcpy(cinst.plname, "isaclc");
  cinst.plfreq = 16000;
  cinst.pacsize = 320;
  cinst.channels = 1;
  cinst.rate = 40000;
#endif
  
  
  TEST_MUSTPASS(codec->SetRecPayloadType(0, cinst));
  voice_channel_transport.reset(NULL);

  voice_channel_transport.reset(new VoiceChannelTransport(network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 8000);
  voice_channel_transport->SetLocalReceiver(8000);

  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  TEST_LOG("Start playing a file as microphone again \n");
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(0, _mgr.AudioFilename(),
                                                   true, true));
  TEST_MUSTPASS(rtp_rtcp->SetFECStatus(0, true, 126));
  MARK();
  TEST_LOG("Should sound OK with FEC enabled\n");
  SleepMs(4000);
  TEST_MUSTPASS(rtp_rtcp->SetFECStatus(0, false));
  MARK();
#endif 
  TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(0));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  ANL();
  AOK();
  return 0;
}





int VoEExtendedTest::TestVideoSync()
{
  PrepareTest("VideoSync");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEVideoSync* vsync = _mgr.VideoSyncPtr();
  VoENetwork* network = _mgr.NetworkPtr();

  
  if (!vsync)
  {
    TEST_LOG("VoEVideoSync is not supported!");
    return -1;
  }

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile((output_path +
      "VoEVideoSync_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
                                            kTraceStateInfo |
                                            kTraceWarning |
                                            kTraceError |
                                            kTraceCritical |
                                            kTraceApiCall |
                                            kTraceMemory |
                                            kTraceInfo));
#endif

  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());

  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(network, 0));

  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);

  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));

  
  

  TEST(SetInitTimestamp);
  ANL();
  TEST_MUSTPASS(!vsync->SetInitTimestamp(0, 12345));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(vsync->SetInitTimestamp(0, 12345));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  MARK();
  SleepMs(1000);
  AOK();
  ANL();

  TEST(SetInitSequenceNumber);
  ANL();
  TEST_MUSTPASS(!vsync->SetInitSequenceNumber(0, 123));
  TEST_MUSTPASS(voe_base_->StopSend(0));
  MARK();
  SleepMs(1000);
  TEST_MUSTPASS(vsync->SetInitSequenceNumber(0, 123));
  TEST_MUSTPASS(voe_base_->StartSend(0));
  MARK();
  SleepMs(1000);
  AOK();
  ANL();

  unsigned int timeStamp;
  TEST(GetPlayoutTimestamp);
  ANL();
  TEST_MUSTPASS(vsync->GetPlayoutTimestamp(0, timeStamp));
  TEST_LOG("GetPlayoutTimestamp: %u", timeStamp);
  SleepMs(1000);
  TEST_MUSTPASS(vsync->GetPlayoutTimestamp(0, timeStamp));
  TEST_LOG(" %u", timeStamp);
  SleepMs(1000);
  TEST_MUSTPASS(vsync->GetPlayoutTimestamp(0, timeStamp));
  TEST_LOG(" %u\n", timeStamp);
  AOK();
  ANL();

  TEST(SetMinimumPlayoutDelay);
  ANL();
  TEST_MUSTPASS(!vsync->SetMinimumPlayoutDelay(0, -1));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  MARK();
  TEST_MUSTPASS(!vsync->SetMinimumPlayoutDelay(0, 5000));
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  MARK();

  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  AOK();
  ANL();
  return 0;
}





int VoEExtendedTest::TestVolumeControl()
{
  PrepareTest("TestVolumeControl");

  VoEBase* voe_base_ = _mgr.BasePtr();
  VoEVolumeControl* volume = _mgr.VolumeControlPtr();
  VoENetwork* network = _mgr.NetworkPtr();
#ifdef _TEST_FILE_
  VoEFile* file = _mgr.FilePtr();
#endif
#ifdef _TEST_HARDWARE_
  VoEHardware* hardware = _mgr.HardwarePtr();
#endif

#ifdef _USE_EXTENDED_TRACE_
  TEST_MUSTPASS(VoiceEngine::SetTraceFile(
      (output_path + "VoEVolumeControl_trace.txt").c_str()));
  TEST_MUSTPASS(VoiceEngine::SetTraceFilter(kTraceStateInfo |
                                            kTraceStateInfo |
                                            kTraceWarning |
                                            kTraceError |
                                            kTraceCritical |
                                            kTraceApiCall |
                                            kTraceMemory |
                                            kTraceInfo));
#endif

  TEST_MUSTPASS(voe_base_->Init());
  TEST_MUSTPASS(voe_base_->CreateChannel());
#if (defined _TEST_HARDWARE_ && (!defined(WEBRTC_IOS)))
#if defined(_WIN32)
  TEST_MUSTPASS(hardware->SetRecordingDevice(-1));
  TEST_MUSTPASS(hardware->SetPlayoutDevice(-1));
#else
  TEST_MUSTPASS(hardware->SetRecordingDevice(0));
  TEST_MUSTPASS(hardware->SetPlayoutDevice(0));
#endif
#endif
  scoped_ptr<VoiceChannelTransport> voice_channel_transport(
      new VoiceChannelTransport(network, 0));
  voice_channel_transport->SetSendDestination("127.0.0.1", 12345);
  voice_channel_transport->SetLocalReceiver(12345);
  TEST_MUSTPASS(voe_base_->StartReceive(0));
  TEST_MUSTPASS(voe_base_->StartPlayout(0));
  TEST_MUSTPASS(voe_base_->StartSend(0));
#ifdef _TEST_FILE_
  TEST_MUSTPASS(file->StartPlayingFileAsMicrophone(0, _mgr.AudioFilename(),
                                                   true, true));
#endif

  
  

#if !defined(WEBRTC_IOS)
  TEST(SetSpeakerVolume);
  ANL();
  TEST_MUSTPASS(-1 != volume->SetSpeakerVolume(256));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  ANL();
#endif 

#if (!defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID))
  TEST(SetMicVolume); ANL();
  TEST_MUSTPASS(-1 != volume->SetMicVolume(256)); MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  ANL();
#endif 

#if !defined(WEBRTC_IOS)
  TEST(SetChannelOutputVolumeScaling);
  ANL();
  TEST_MUSTPASS(-1 != volume->SetChannelOutputVolumeScaling(0, (float)-0.1));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(-1 != volume->SetChannelOutputVolumeScaling(0, (float)10.1));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  ANL();
#endif 
#if (!defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID))
  TEST(SetOutputVolumePan);
  ANL();
  TEST_MUSTPASS(-1 != volume->SetOutputVolumePan(-1, (float)-0.1,
                                                 (float)1.0));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(-1 != volume->SetOutputVolumePan(-1, (float)1.1,
                                                 (float)1.0));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(-1 != volume->SetOutputVolumePan(-1, (float)1.0,
                                                 (float)-0.1));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(-1 != volume->SetOutputVolumePan(-1, (float)1.0,
                                                 (float)1.1));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  ANL();

  TEST(SetChannelOutputVolumePan);
  ANL();
  TEST_MUSTPASS(-1 != volume->SetOutputVolumePan(0, (float)-0.1,
                                                 (float)1.0));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(-1 != volume->SetOutputVolumePan(0, (float)1.1,
                                                 (float)1.0));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(-1 != volume->SetOutputVolumePan(0, (float)1.0,
                                                 (float)-0.1));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  TEST_MUSTPASS(-1 != volume->SetOutputVolumePan(0, (float)1.0,
                                                 (float)1.1));
  MARK();
  TEST_MUSTPASS(VE_INVALID_ARGUMENT != voe_base_->LastError());
  ANL();
#endif 
#ifdef _TEST_FILE_
  TEST_MUSTPASS(file->StopPlayingFileAsMicrophone(0));
#endif
  TEST_MUSTPASS(voe_base_->StopSend(0));
  TEST_MUSTPASS(voe_base_->StopPlayout(0));
  TEST_MUSTPASS(voe_base_->StopReceive(0));
  TEST_MUSTPASS(voe_base_->DeleteChannel(0));
  TEST_MUSTPASS(voe_base_->Terminate());

  AOK();
  ANL();
  return 0;
}

}  
