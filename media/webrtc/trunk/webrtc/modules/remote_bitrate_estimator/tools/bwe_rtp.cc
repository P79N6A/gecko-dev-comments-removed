









#include "webrtc/modules/remote_bitrate_estimator/tools/bwe_rtp.h"

#include <stdio.h>
#include <string>

#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_payload_registry.h"
#include "webrtc/test/rtp_file_reader.h"

const int kMinBitrateBps = 30000;

bool ParseArgsAndSetupEstimator(int argc,
                                char** argv,
                                webrtc::Clock* clock,
                                webrtc::RemoteBitrateObserver* observer,
                                webrtc::test::RtpFileReader** rtp_reader,
                                webrtc::RtpHeaderParser** parser,
                                webrtc::RemoteBitrateEstimator** estimator,
                                std::string* estimator_used) {
  *rtp_reader = webrtc::test::RtpFileReader::Create(
      webrtc::test::RtpFileReader::kRtpDump, argv[3]);
  if (!*rtp_reader) {
    fprintf(stderr, "Cannot open input file %s\n", argv[3]);
    return false;
  }
  fprintf(stderr, "Input file: %s\n\n", argv[3]);
  webrtc::RTPExtensionType extension = webrtc::kRtpExtensionAbsoluteSendTime;

  if (strncmp("tsoffset", argv[1], 8) == 0) {
    extension = webrtc::kRtpExtensionTransmissionTimeOffset;
    fprintf(stderr, "Extension: toffset\n");
  } else {
    fprintf(stderr, "Extension: abs\n");
  }
  int id = atoi(argv[2]);

  
  *parser = webrtc::RtpHeaderParser::Create();
  (*parser)->RegisterRtpHeaderExtension(extension, id);
  if (estimator) {
    switch (extension) {
      case webrtc::kRtpExtensionAbsoluteSendTime: {
          webrtc::AbsoluteSendTimeRemoteBitrateEstimatorFactory factory;
          *estimator = factory.Create(observer, clock, webrtc::kAimdControl,
                                      kMinBitrateBps);
          *estimator_used = "AbsoluteSendTimeRemoteBitrateEstimator";
          break;
        }
      case webrtc::kRtpExtensionTransmissionTimeOffset: {
          webrtc::RemoteBitrateEstimatorFactory factory;
          *estimator = factory.Create(observer, clock, webrtc::kAimdControl,
                                      kMinBitrateBps);
          *estimator_used = "RemoteBitrateEstimator";
          break;
        }
      default:
        assert(false);
    }
  }
  return true;
}
