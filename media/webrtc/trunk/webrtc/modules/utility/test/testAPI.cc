















#include <iostream>
#include <stdio.h>
#include <assert.h>

#ifdef WIN32
    #include <windows.h>
    #include <tchar.h>
#endif

#include "common_types.h"
#include "trace.h"

#include "Engineconfigurations.h"
#include "media_file.h"
#include "file_player.h"
#include "file_recorder.h"


bool notify = false, playing = false, recording = false;


class MyFileModuleCallback : public FileCallback
{
public:
    virtual void PlayNotification( const WebRtc_Word32 id,
                                   const WebRtc_UWord32 durationMs )
    {
        printf("\tReceived PlayNotification from module %ld, durationMs = %ld\n",
               id, durationMs);
        notify = true;
    };

    virtual void RecordNotification( const WebRtc_Word32 id,
                                     const WebRtc_UWord32 durationMs )
    {
        printf("\tReceived RecordNotification from module %ld, durationMs = %ld\n",
               id, durationMs);
        notify = true;
    };

    virtual void PlayFileEnded(const WebRtc_Word32 id)
    {
        printf("\tReceived PlayFileEnded notification from module %ld.\n", id);
        playing = false;
    };

    virtual void RecordFileEnded(const WebRtc_Word32 id)
    {
        printf("\tReceived RecordFileEnded notification from module %ld.\n", id);
        recording = false;
    }
};


#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int , char** )
#endif
{
    Trace::CreateTrace();
    Trace::SetTraceFile("testTrace.txt");
    Trace::SetEncryptedTraceFile("testTraceDebug.txt");

    int playId = 1;
    int recordId = 2;

    printf("Welcome to test of FilePlayer and FileRecorder\n");


    
    
    
    
    


    

    {
        FilePlayer& filePlayer(*FilePlayer::CreateFilePlayer(1, webrtc::kFileFormatAviFile));
        FileRecorder& fileRecorder(*FileRecorder::CreateFileRecorder(1, webrtc::kFileFormatAviFile));

        const char* KFileName = "./tmpAviFileTestCase1_audioI420CIF30fps.avi";

        printf("\tReading from an avi file and writing the information to another \n");
        printf("\tin the same format (I420 CIF 30fps) \n");
        printf("\t\t check file named %s\n", KFileName);

        assert(filePlayer.StartPlayingVideoFile(
           "../../../MediaFile/main/test/files/aviTestCase1_audioI420CIF30fps.avi",
           false, false) == 0);

        
         webrtc::VideoCodec videoCodec;
        webrtc::VideoCodec recVideoCodec;
        webrtc::CodecInst audioCodec;
        assert(filePlayer.VideoCodec( videoCodec ) == 0);
        assert(filePlayer.AudioCodec( audioCodec) == 0);

        recVideoCodec = videoCodec;

        assert( fileRecorder.StartRecordingVideoFile(KFileName,
                                                     audioCodec,
                                                     recVideoCodec) == 0);

        assert(fileRecorder.IsRecording());

        webrtc::I420VideoFrame videoFrame;
        videoFrame.CreateEmptyFrame(videoCodec.width, videoCodec.height,
                                    videoCodec.width,
                                    (videoCodec.width + 1) / 2,
                                    (videoCodec.width + 1) / 2);

        int  frameCount   = 0;
        bool audioNotDone = true;
        bool videoNotDone =    true;
        AudioFrame audioFrame;

        while( audioNotDone || videoNotDone)
        {
            if(filePlayer.TimeUntilNextVideoFrame() <= 0)
            {
                if(filePlayer.GetVideoFromFile( videoFrame) != 0)
                {
                    
                    break;
                }
                frameCount++;
                videoNotDone = !videoFrame.IsZeroSize();
                videoFrame.SetRenderTime(TickTime::MillisecondTimestamp());
                if( videoNotDone)
                {
                    assert(fileRecorder.RecordVideoToFile(videoFrame) == 0);
                    ::Sleep(10);
                }
            }
             WebRtc_UWord32 decodedDataLengthInSamples;
            if( 0 !=  filePlayer.Get10msAudioFromFile( audioFrame.data_, decodedDataLengthInSamples, audioCodec.plfreq))
            {
                audioNotDone = false;
            } else
            {
                audioFrame.sample_rate_hz_ = filePlayer.Frequency();
                audioFrame.samples_per_channel_ = (WebRtc_UWord16)decodedDataLengthInSamples;
                fileRecorder.RecordAudioToFile(audioFrame, &TickTime::Now());
            }
       }
        ::Sleep(100);
        assert(fileRecorder.StopRecording() == 0);
        assert( !fileRecorder.IsRecording());
        assert(frameCount == 135);
        printf("\tGenerated %s\n\n", KFileName);
    }
    
    
    
    
    
    {
        FilePlayer& filePlayer(*FilePlayer::CreateFilePlayer(2, webrtc::kFileFormatAviFile));
        FileRecorder& fileRecorder(*FileRecorder::CreateFileRecorder(2, webrtc::kFileFormatAviFile));

        const char* KFileName = "./tmpAviFileTestCase2_audioI420CIF20fps.avi";

        printf("\tWriting information to a avi file and check the written file by \n");
        printf("\treopening it and control codec information.\n");
        printf("\t\t check file named %s all frames should be light green.\n", KFileName);
        
        webrtc::VideoCodec videoCodec;
        webrtc::CodecInst      audioCodec;

        memset(&videoCodec, 0, sizeof(videoCodec));

        const char* KVideoCodecName = "I420";
        strcpy(videoCodec.plName, KVideoCodecName);
        videoCodec.plType    = 124;
        videoCodec.maxFramerate = 20;
        videoCodec.height    = 288;
        videoCodec.width     = 352;

        const char* KAudioCodecName = "PCMU";
        strcpy(audioCodec.plname, KAudioCodecName);
        audioCodec.pltype   = 0;
        audioCodec.plfreq   = 8000;
        audioCodec.pacsize  = 80;
        audioCodec.channels = 1;
        audioCodec.rate     = 64000;

        assert( fileRecorder.StartRecordingVideoFile(
            KFileName,
            audioCodec,
            videoCodec) == 0);

        assert(fileRecorder.IsRecording());

        const WebRtc_UWord32 KVideoWriteSize = static_cast< WebRtc_UWord32>( (videoCodec.width * videoCodec.height * 3) / 2);
        webrtc::VideoFrame videoFrame;

        
        AudioFrame audioFrame;
        audioFrame.samples_per_channel_ = audioCodec.plfreq/100;
        memset(audioFrame.data_, 0, 2*audioFrame.samples_per_channel_);
        audioFrame.sample_rate_hz_ = 8000;

        
        int half_width = (videoCodec.width + 1) / 2;
        int half_height = (videoCodec.height + 1) / 2;
        videoFrame.CreateEmptyFrame(videoCodec.width, videoCodec.height,
                                    videoCodec.width, half_width, half_width);
        memset(videoFrame.buffer(kYPlane), 127,
               videoCodec.width * videoCodec.height);
        memset(videoFrame.buffer(kUPlane), 0, half_width * half_height);
        memset(videoFrame.buffer(kVPlane), 0, half_width * half_height);

        
        const int KWriteNumFrames = 20;
        int       writeFrameCount = 0;
        while(writeFrameCount < KWriteNumFrames)
        {
            
            assert(fileRecorder.RecordVideoToFile(videoFrame) == 0);

            
            for(int i=0; i<5; i++)
            {
                assert( fileRecorder.RecordAudioToFile(audioFrame) == 0);
            }
            writeFrameCount++;
        }
        ::Sleep(10); 
        assert(writeFrameCount == 20);
        assert(fileRecorder.StopRecording() == 0);
        assert( ! fileRecorder.IsRecording());

        assert(filePlayer.StartPlayingVideoFile(KFileName,false, false) == 0);
        assert(filePlayer.IsPlayingFile( ));

        
        webrtc::VideoCodec readVideoCodec;
        assert(filePlayer.VideoCodec( readVideoCodec ) == 0);
        assert(strcmp(readVideoCodec.plName, videoCodec.plName) == 0);
        assert(readVideoCodec.width      == videoCodec.width);
        assert(readVideoCodec.height     == videoCodec.height);
        assert(readVideoCodec.maxFramerate  == videoCodec.maxFramerate);

        webrtc::CodecInst readAudioCodec;
        assert(filePlayer.AudioCodec( readAudioCodec) == 0);
        assert(strcmp(readAudioCodec.plname, audioCodec.plname) == 0);
        assert(readAudioCodec.pltype     == audioCodec.pltype);
        assert(readAudioCodec.plfreq     == audioCodec.plfreq);
        assert(readAudioCodec.pacsize    == audioCodec.pacsize);
        assert(readAudioCodec.channels   == audioCodec.channels);
        assert(readAudioCodec.rate       == audioCodec.rate);

        assert(filePlayer.StopPlayingFile() == 0);
        assert( ! filePlayer.IsPlayingFile());
        printf("\tGenerated %s\n\n", KFileName);
    }
    
    
    
    
    

    {
        FilePlayer& filePlayer(*FilePlayer::CreateFilePlayer(2, webrtc::kFileFormatAviFile));
        FileRecorder& fileRecorder(*FileRecorder::CreateFileRecorder(3, webrtc::kFileFormatAviFile));

        printf("\tReading from an avi file and writing the information to another \n");
        printf("\tin a different format (H.263 CIF 30fps) \n");
        printf("\t\t check file named tmpAviFileTestCase1_audioH263CIF30fps.avi\n");

        assert(filePlayer.StartPlayingVideoFile(
           "../../../MediaFile/main/test/files/aviTestCase1_audioI420CIF30fps.avi",
           false,
           false) == 0);

        
         webrtc::VideoCodec videoCodec;
        webrtc::VideoCodec recVideoCodec;
        webrtc::CodecInst      audioCodec;
        assert(filePlayer.VideoCodec( videoCodec ) == 0);
        assert(filePlayer.AudioCodec( audioCodec) == 0);
        recVideoCodec = videoCodec;

        memcpy(recVideoCodec.plName, "H263",5);
        recVideoCodec.startBitrate = 1000;
        recVideoCodec.codecSpecific.H263.quality = 1;
        recVideoCodec.plType = 34;
        recVideoCodec.codecType = webrtc::kVideoCodecH263;

        assert( fileRecorder.StartRecordingVideoFile(
            "./tmpAviFileTestCase1_audioH263CIF30fps.avi",
            audioCodec,
            recVideoCodec) == 0);

        assert(fileRecorder.IsRecording());

        webrtc::I420VideoFrame videoFrame;
        videoFrame.CreateEmptyFrame(videoCodec.width, videoCodec.height,
                                    videoCodec.width, half_width,half_width);

        int  videoFrameCount   = 0;
        int  audioFrameCount   = 0;
        bool audioNotDone = true;
        bool videoNotDone =    true;
        AudioFrame audioFrame;

        while( audioNotDone || videoNotDone)
        {
            if(filePlayer.TimeUntilNextVideoFrame() <= 0)
            {
                if(filePlayer.GetVideoFromFile(videoFrame) != 0)
                {
                    break;
                }
                videoFrameCount++;
                videoNotDone = !videoFrame.IsZeroSize();
                if( videoNotDone)
                {
                    assert(fileRecorder.RecordVideoToFile(videoFrame) == 0);
                }
            }

            WebRtc_UWord32 decodedDataLengthInSamples;
            if( 0 != filePlayer.Get10msAudioFromFile( audioFrame.data_, decodedDataLengthInSamples, audioCodec.plfreq))
            {
                audioNotDone = false;

            } else
            {
                ::Sleep(5);
                audioFrame.sample_rate_hz_ = filePlayer.Frequency();
                audioFrame.samples_per_channel_ = (WebRtc_UWord16)decodedDataLengthInSamples;
                assert(0 == fileRecorder.RecordAudioToFile(audioFrame));

                audioFrameCount++;
            }
        }
        assert(videoFrameCount == 135);
        assert(audioFrameCount == 446); 

        assert(fileRecorder.StopRecording() == 0);
        assert( !fileRecorder.IsRecording());
        printf("\tGenerated ./tmpAviFileTestCase1_audioH263CIF30fps.avi\n\n");
    }


    printf("\nTEST completed.\n");

    Trace::ReturnTrace();
    return 0;
}
