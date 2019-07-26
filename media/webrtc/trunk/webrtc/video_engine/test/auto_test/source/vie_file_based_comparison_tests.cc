









#include "video_engine/test/auto_test/interface/vie_file_based_comparison_tests.h"

#include "video_engine/test/auto_test/interface/vie_autotest_defines.h"
#include "video_engine/test/auto_test/primitives/base_primitives.h"
#include "video_engine/test/auto_test/primitives/framedrop_primitives.h"
#include "video_engine/test/auto_test/primitives/general_primitives.h"
#include "video_engine/test/libvietest/include/tb_external_transport.h"
#include "video_engine/test/libvietest/include/tb_interfaces.h"
#include "video_engine/test/libvietest/include/vie_external_render_filter.h"
#include "video_engine/test/libvietest/include/vie_fake_camera.h"
#include "video_engine/test/libvietest/include/vie_to_file_renderer.h"

bool ViEFileBasedComparisonTests::TestCallSetup(
    const std::string& i420_video_file,
    int width,
    int height,
    ViEToFileRenderer* local_file_renderer,
    ViEToFileRenderer* remote_file_renderer) {

  TbInterfaces interfaces("TestCallSetup");

  int video_channel = -1;
  EXPECT_EQ(0, interfaces.base->CreateChannel(video_channel));

  ViEFakeCamera fake_camera(interfaces.capture);
  if (!fake_camera.StartCameraInNewThread(i420_video_file,
                                          width,
                                          height)) {
    
    ADD_FAILURE() << "Could not open input video " << i420_video_file <<
        ": aborting test...";
    return false;
  }
  int capture_id = fake_camera.capture_id();

  
  
  EXPECT_EQ(0, interfaces.capture->ConnectCaptureDevice(
      capture_id, video_channel));

  ConfigureRtpRtcp(interfaces.rtp_rtcp, kNack, video_channel);

  webrtc::ViERender* render_interface = interfaces.render;
  webrtc::ViEImageProcess* image_process = interfaces.image_process;

  RenderToFile(render_interface, video_channel, remote_file_renderer);

  
  
  
  webrtc::ExternalRendererEffectFilter renderer_filter(local_file_renderer);
  EXPECT_EQ(0, image_process->RegisterSendEffectFilter(video_channel,
                                                       renderer_filter));

  
  const char* device_name = "Fake Capture Device";

  ::TestI420CallSetup(interfaces.codec, interfaces.video_engine,
                      interfaces.base, interfaces.network, video_channel,
                      device_name);

  EXPECT_EQ(0, render_interface->StopRender(video_channel));
  EXPECT_EQ(0, render_interface->RemoveRenderer(video_channel));

  interfaces.capture->DisconnectCaptureDevice(video_channel);

  
  
  
  
  fake_camera.StopCamera();
  EXPECT_EQ(0, image_process->DeregisterSendEffectFilter(video_channel));

  EXPECT_EQ(0, interfaces.base->DeleteChannel(video_channel));
  return true;
}

void ViEFileBasedComparisonTests::TestFullStack(
    const std::string& i420_video_file,
    int width,
    int height,
    int bit_rate_kbps,
    ProtectionMethod protection_method,
    const NetworkParameters& network,
    ViEToFileRenderer* local_file_renderer,
    ViEToFileRenderer* remote_file_renderer,
    FrameDropDetector* frame_drop_detector) {
  TbInterfaces interfaces("TestFullStack");

  
  ViEFakeCamera fake_camera(interfaces.capture);
  if (!fake_camera.StartCameraInNewThread(i420_video_file, width, height)) {
    
    ADD_FAILURE() << "Could not open input video " << i420_video_file <<
        ": aborting test...";
    return;
  }
  int video_channel = -1;
  int capture_id = fake_camera.capture_id();
  EXPECT_EQ(0, interfaces.base->CreateChannel(video_channel));

  
  
  
  EXPECT_EQ(0, interfaces.rtp_rtcp->SetLocalSSRC(video_channel, 12345));

  EXPECT_EQ(0, interfaces.capture->ConnectCaptureDevice(
      capture_id, video_channel));
  ConfigureRtpRtcp(interfaces.rtp_rtcp, protection_method, video_channel);

  ::TestFullStack(interfaces, capture_id, video_channel, width, height,
                  bit_rate_kbps, network, frame_drop_detector,
                  remote_file_renderer, local_file_renderer);
  EXPECT_TRUE(fake_camera.StopCamera());
}
