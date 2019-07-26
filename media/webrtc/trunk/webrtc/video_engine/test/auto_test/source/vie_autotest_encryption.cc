













#include "vie_autotest_defines.h"
#include "vie_autotest.h"
#include "engine_configurations.h"

#include "tb_capture_device.h"
#include "tb_external_transport.h"
#include "tb_interfaces.h"
#include "tb_video_channel.h"

class ViEAutotestEncryption: public webrtc::Encryption
{
public:
    ViEAutotestEncryption()
    {
    }
    ~ViEAutotestEncryption()
    {
    }

    virtual void encrypt(int channel_no, unsigned char* in_data,
                         unsigned char* out_data, int bytes_in, int* bytes_out)
    {
        for (int i = 0; i < bytes_in; i++)
        {
            out_data[i] = ~in_data[i];
        }
        assert(*bytes_out >= bytes_in + 2);
        *bytes_out = bytes_in + 2;
        out_data[bytes_in] = 'a';
        out_data[bytes_in + 1] = 'b';
    }

    virtual void decrypt(int channel_no, unsigned char* in_data,
                         unsigned char* out_data, int bytes_in, int* bytes_out)
    {
        for (int i = 0; i < bytes_in - 2; i++)
        {
            out_data[i] = ~in_data[i];
        }
        assert(*bytes_out >= bytes_in - 2);
        *bytes_out = bytes_in - 2;
    }

    virtual void encrypt_rtcp(int channel_no, unsigned char* in_data,
                              unsigned char* out_data, int bytes_in,
                              int* bytes_out)
    {
        for (int i = 0; i < bytes_in; i++)
        {
            out_data[i] = ~in_data[i];
        }
        assert(*bytes_out >= bytes_in + 2);
        *bytes_out = bytes_in + 2;
        out_data[bytes_in] = 'a';
        out_data[bytes_in + 1] = 'b';
    }

    virtual void decrypt_rtcp(int channel_no, unsigned char* in_data,
                              unsigned char* out_data, int bytes_in,
                              int* bytes_out)
    {
        for (int i = 0; i < bytes_in - 2; i++)
        {
            out_data[i] = ~in_data[i];
        }
        assert(*bytes_out >= bytes_in - 2);
        *bytes_out = bytes_in - 2;
    }
};

void ViEAutoTest::ViEEncryptionStandardTest()
{
    
    
    

    
    TbInterfaces ViE("ViEEncryptionStandardTest");
    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);

    
    TbCaptureDevice tbCapture(ViE);
    tbCapture.ConnectTo(tbChannel.videoChannel);

    tbChannel.StartReceive();

    tbChannel.StartSend();

    RenderCaptureDeviceAndOutputStream(&ViE, &tbChannel, &tbCapture);

    
    
    
    ViEAutotestEncryption testEncryption;
    
    EXPECT_NE(0, ViE.base->StartSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->RegisterExternalEncryption(
        tbChannel.videoChannel, testEncryption));
    ViETest::Log(
        "External encryption/decryption added, you should still see video");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DeregisterExternalEncryption(
        tbChannel.videoChannel));

    
    
    
}

void ViEAutoTest::ViEEncryptionExtendedTest()
{
    
    
    

    
    TbInterfaces ViE("ViEEncryptionExtendedTest");
    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);

    
    TbCaptureDevice tbCapture(ViE);
    tbCapture.ConnectTo(tbChannel.videoChannel);

    tbChannel.StartReceive();
    tbChannel.StartSend();

    RenderCaptureDeviceAndOutputStream(&ViE, &tbChannel, &tbCapture);

    
    
    

    
    
    
    ViEAutotestEncryption testEncryption;
    EXPECT_EQ(0, ViE.encryption->RegisterExternalEncryption(
        tbChannel.videoChannel, testEncryption));
    ViETest::Log(
        "External encryption/decryption added, you should still see video");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DeregisterExternalEncryption(
        tbChannel.videoChannel));

    
    
    
}

void ViEAutoTest::ViEEncryptionAPITest()
{
    
    
    

    
    
    

    
    TbInterfaces ViE("ViEEncryptionAPITest");
    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);

    
    TbCaptureDevice tbCapture(ViE);
    
    tbCapture.ConnectTo(tbChannel.videoChannel);

    
    
    

    ViEAutotestEncryption testEncryption;
    EXPECT_EQ(0, ViE.encryption->RegisterExternalEncryption(
        tbChannel.videoChannel, testEncryption));
    EXPECT_NE(0, ViE.encryption->RegisterExternalEncryption(
        tbChannel.videoChannel, testEncryption));
    EXPECT_EQ(0, ViE.encryption->DeregisterExternalEncryption(
        tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DeregisterExternalEncryption(
        tbChannel.videoChannel));

    
    
    
}
