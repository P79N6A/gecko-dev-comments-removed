














#if defined(THREAD_SANITIZER)





char kTSanDefaultSuppressions[] =




"race:rtc::MessageQueue::Quit\n"
"race:FileVideoCapturerTest::VideoCapturerListener::OnFrameCaptured\n"
"race:vp8cx_remove_encoder_threads\n"
"race:third_party/libvpx/source/libvpx/vp9/common/vp9_scan.h\n"



"race:webrtc::TraceImpl::WriteToFile\n"
"race:webrtc::VideoEngine::SetTraceFilter\n"
"race:webrtc::VoiceEngine::SetTraceFilter\n"
"race:webrtc::Trace::set_level_filter\n"
"race:webrtc::GetStaticInstance<webrtc::TraceImpl>\n"



"race:webrtc/modules/audio_processing/aec/aec_core.c\n"
"race:webrtc/modules/audio_processing/aec/aec_rdft.c\n"



"race:rtc::FireAndForgetAsyncClosure<FunctorB>::Execute\n"
"race:rtc::MessageQueueManager::Clear\n"
"race:rtc::Thread::Clear\n"



"race:webrtc/base/testclient.cc\n"
"race:webrtc/base/virtualsocketserver.cc\n"
"race:talk/p2p/base/stunserver_unittest.cc\n"



"race:webrtc/base/logging.cc\n"
"race:webrtc/base/sharedexclusivelock_unittest.cc\n"
"race:webrtc/base/signalthread_unittest.cc\n"



"race:user_sctp_timer_iterate\n"



"deadlock:webrtc::ProcessThreadImpl::RegisterModule\n"
"deadlock:webrtc::RTCPReceiver::SetSsrcs\n"
"deadlock:webrtc::RTPSenderAudio::RegisterAudioPayload\n"
"deadlock:webrtc::test::UdpSocketManagerPosixImpl::RemoveSocket\n"
"deadlock:webrtc::vcm::VideoReceiver::RegisterPacketRequestCallback\n"
"deadlock:webrtc::ViECaptureImpl::ConnectCaptureDevice\n"
"deadlock:webrtc::ViEChannel::StartSend\n"
"deadlock:webrtc::ViECodecImpl::GetSendSideDelay\n"
"deadlock:webrtc::ViEEncoder::OnLocalSsrcChanged\n"
"deadlock:webrtc::ViESender::RegisterSendTransport\n"


;  

#endif  
