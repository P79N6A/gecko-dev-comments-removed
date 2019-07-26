













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

#ifdef WEBRTC_SRTP
    
    
    

    
    
    
    unsigned char srtpKey1[30] =
    {   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3,
        4, 5, 6, 7, 8, 9};

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 0, 0, webrtc::kEncryption, srtpKey1));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 0, 0, webrtc::kEncryption, srtpKey1));
    ViETest::Log("SRTP encryption only");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kAuthentication, srtpKey1));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kAuthentication, srtpKey1));

    ViETest::Log("SRTP authentication only");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey1));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey1));

    ViETest::Log("SRTP full protection");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));
#endif  

    
    
    
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

    
    
    

#ifdef WEBRTC_SRTP

    
    
    
    unsigned char srtpKey1[30] =
    {   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3,
        4, 5, 6, 7, 8, 9};
    unsigned char srtpKey2[30] =
    {   9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 9, 8, 7, 6,
        5, 4, 3, 2, 1, 0};
    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthNull, 0, 0,
        webrtc::kNoProtection, srtpKey1));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthNull, 0, 0,
        webrtc::kNoProtection, srtpKey1));

    ViETest::Log("SRTP NULL encryption/authentication");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 0, 0, webrtc::kEncryption, srtpKey1));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 0, 0, webrtc::kEncryption, srtpKey1));

    ViETest::Log("SRTP encryption only");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kAuthentication, srtpKey1));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kAuthentication, srtpKey1));

    ViETest::Log("SRTP authentication only");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey1));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey1));

    ViETest::Log("SRTP full protection");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey2));

    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey1));

    ViETest::Log(
        "\nSRTP receive key changed, you should not see any remote images");
    AutoTestSleep(kAutoTestSleepTimeMs);

    
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey2));

    ViETest::Log("\nSRTP send key changed too, you should see remote video "
                 "again with some decoding artefacts at start");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));

    
    ViETest::Log("SRTP receive disabled , you shouldn't see any video");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

#endif 
    
    
    
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

#ifdef WEBRTC_SRTP
    unsigned char srtpKey[30] =
    {   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3,
        4, 5, 6, 7, 8, 9};

    
    
    

    
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kNoProtection, srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryption, srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kAuthentication, srtpKey));

    
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 15,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 257,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 15, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kEncryptionAndAuthentication, srtpKey));

    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 257, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kEncryptionAndAuthentication, srtpKey));

    
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode,
        30, webrtc::kAuthHmacSha1, 21, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 257, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 21, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 20, 13, webrtc::kEncryptionAndAuthentication,
        srtpKey));

    
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        NULL));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));

    
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthNull, 0, 0,
        webrtc::kNoProtection, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kAuthentication, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        1, 4, webrtc::kAuthentication, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        20, 20, webrtc::kAuthentication, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        1, 1, webrtc::kAuthentication, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 0, 0, webrtc::kEncryption, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 16,
        webrtc::kAuthNull, 0, 0, webrtc::kEncryption, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    
    

    
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kNoProtection, srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryption, srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kAuthentication, srtpKey));

    
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 15,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 257,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 15, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kEncryptionAndAuthentication, srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 257, webrtc::kAuthHmacSha1,
        20, 4, webrtc::kEncryptionAndAuthentication, srtpKey));

    
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 21, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 257, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 21, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 20, 13, webrtc::kEncryptionAndAuthentication,
        srtpKey));

    
    EXPECT_NE(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        NULL));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_NE(0, ViE.encryption->EnableSRTPSend(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPSend(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthNull, 0, 0,
        webrtc::kNoProtection, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        1, 4, webrtc::kAuthentication, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 0,
        webrtc::kAuthHmacSha1, 20, 20, webrtc::kAuthentication, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherNull, 0, webrtc::kAuthHmacSha1,
        1, 1, webrtc::kAuthentication, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthNull, 0, 0, webrtc::kEncryption, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 16,
        webrtc::kAuthNull, 0, 0, webrtc::kEncryption, srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));

    
    EXPECT_EQ(0, ViE.encryption->EnableSRTPReceive(
        tbChannel.videoChannel, webrtc::kCipherAes128CounterMode, 30,
        webrtc::kAuthHmacSha1, 20, 4, webrtc::kEncryptionAndAuthentication,
        srtpKey));
    EXPECT_EQ(0, ViE.encryption->DisableSRTPReceive(tbChannel.videoChannel));
#endif 
    
    
    

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
