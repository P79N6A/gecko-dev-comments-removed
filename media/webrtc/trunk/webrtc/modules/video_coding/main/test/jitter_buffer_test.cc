









#include <math.h>
#include <stdio.h>

#include "common_types.h"
#include "../source/event.h"
#include "frame_buffer.h"
#include "inter_frame_delay.h"
#include "jitter_buffer.h"
#include "jitter_estimate_test.h"
#include "jitter_estimator.h"
#include "media_opt_util.h"
#include "modules/video_coding/main/source/tick_time_base.h"
#include "packet.h"
#include "test_util.h"
#include "test_macros.h"


using namespace webrtc;



int CheckOutFrame(VCMEncodedFrame* frameOut, unsigned int size, bool startCode)
{
    if (frameOut == 0)
    {
        return -1;
    }

    const WebRtc_UWord8* outData = frameOut->Buffer();

    unsigned int i = 0;

    if(startCode)
    {
        if (outData[0] != 0 || outData[1] != 0 || outData[2] != 0 ||
            outData[3] != 1)
        {
            return -2;
        }
        i+= 4;
    }

    
    int count = 3;

    
    if (frameOut->Length() != size)
    {
        return -3;
    }

    for(; i < size; i++)
    {
        if (outData[i] == 0 && outData[i + 1] == 0 && outData[i + 2] == 0x80)
        {
            i += 2;
        }
        else if(startCode && outData[i] == 0 && outData[i + 1] == 0)
        {
            if (outData[i] != 0 || outData[i + 1] != 0 ||
                outData[i + 2] != 0 || outData[i + 3] != 1)
            {
                return -3;
            }
            i += 3;
        }
        else
        {
            if (outData[i] != count)
            {
                return -4;
            }
            count++;
            if(count == 10)
            {
                count = 3;
            }
        }
    }
    return 0;
}


int JitterBufferTest(CmdArgs& args)
{
    
#if defined(EVENT_DEBUG)
    return -1;
#endif
    TickTimeBase clock;

    
    WebRtc_UWord16 seqNum = 1234;
    WebRtc_UWord32 timeStamp = 0;
    int size = 1400;
    WebRtc_UWord8 data[1500];
    VCMPacket packet(data, size, seqNum, timeStamp, true);

    VCMJitterBuffer jb(&clock);

    seqNum = 1234;
    timeStamp = 123*90;
    FrameType incomingFrameType(kVideoFrameKey);
    VCMEncodedFrame* frameOut=NULL;
    WebRtc_Word64 renderTimeMs = 0;
    packet.timestamp = timeStamp;
    packet.seqNum = seqNum;

    
    data[0] = 0;
    data[1] = 0;
    data[2] = 0x80;
    int count = 3;
    for (unsigned int i = 3; i < sizeof(data) - 3; ++i)
    {
        data[i] = count;
        count++;
        if(count == 10)
        {
            data[i+1] = 0;
            data[i+2] = 0;
            data[i+3] = 0x80;
            count = 3;
            i += 3;
        }
    }

    
    TEST(0 == jb.GetFrame(packet));
    TEST(-1 == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));
    TEST(0 == jb.GetCompleteFrameForDecoding(10));
    TEST(0 == jb.GetFrameForDecoding());

    
    jb.Start();

    
    VCMEncodedFrame* frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(0 == jb.GetCompleteFrameForDecoding(10));


    
    
    
    
    
    

    
    
    
    
    
    
    
    
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    

    
    
    
    
    
    
    

    seqNum++;
    timeStamp += 33*90;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*2, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    


    
    
    
    
    
    

    
    timeStamp += 33*90;
    seqNum++;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameKey);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    
    int loop = 0;
    do
    {
        seqNum++;
        packet.isFirstPacket = false;
        packet.markerBit = false;
        packet.seqNum = seqNum;

        frameIn = jb.GetFrame(packet);
        TEST(frameIn != 0);

        
        TEST(kIncomplete == jb.InsertPacket(frameIn, packet));
        loop++;
    } while (loop < 98);

    
    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*100, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameKey);

    
    jb.ReleaseFrame(frameOut);

    

    
    
    
    
    
    

    
    timeStamp += 33*90;
    seqNum++;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    
    loop = 0;
    do
    {
        seqNum++;
        packet.isFirstPacket = false;
        packet.markerBit = false;
        packet.seqNum = seqNum;

        frameIn = jb.GetFrame(packet);
        TEST(frameIn != 0);

        
        TEST(kIncomplete == jb.InsertPacket(frameIn, packet));
        loop++;
    } while (loop < 98);

    
    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*100, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    

    
    
    
    
    
    
    

    
    timeStamp += 33*90;
    seqNum += 100;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    
    loop = 0;
    do
    {
        seqNum--;
        packet.isFirstPacket = false;
        packet.markerBit = false;
        packet.seqNum = seqNum;

        frameIn = jb.GetFrame(packet);
        TEST(frameIn != 0);

        
        TEST(kIncomplete == jb.InsertPacket(frameIn, packet));
        loop++;
    } while (loop < 98);

    
    seqNum--;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*100, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    

    seqNum+= 100;

    
    
    
    
    
    

    seqNum += 2;
    timeStamp += 2* 33 * 90;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);
    TEST(frameOut == 0);

    seqNum -= 3;
    timeStamp -= 33*90;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*2, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*2, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    seqNum += 2;
    

    
    packet.dataPtr = data;
    packet.codec = kVideoCodecUnknown;

    
    
    
    
    
    
    

   seqNum++;
    timeStamp += 2*33*90;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    packet.isFirstPacket = false;
    packet.markerBit = true;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kDuplicatePacket == jb.InsertPacket(frameIn, packet));

    seqNum++;
    packet.seqNum = seqNum;

    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*2, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    

    
    
    
    
    
    
    

    seqNum++;
    timeStamp += 33 * 90;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.insertStartCode = true;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size * 2 + 4 * 2, true) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    
    packet.insertStartCode = false;
    

    
    
    
    WebRtc_UWord32 numDeltaFrames = 0;
    WebRtc_UWord32 numKeyFrames = 0;
    jb.FrameStatistics(&numDeltaFrames, &numKeyFrames);

    TEST(numDeltaFrames == 8);
    TEST(numKeyFrames == 1);

    WebRtc_UWord32 frameRate;
    WebRtc_UWord32 bitRate;
    jb.IncomingRateStatistics(&frameRate, &bitRate);

    
    TEST(frameRate > 30);
    TEST(bitRate > 10000000);


    jb.Flush();

    
    
    
    
    
    

    
    
    
    seqNum = 0xffff - 4;
    seqNum++;
    timeStamp += 33*90;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.completeNALU = kNaluStart;
    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);
    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    for (int i = 0; i < 11; ++i) {
      webrtc::FrameType frametype = kVideoFrameDelta;
      seqNum++;
      timeStamp += 33*90;
      packet.frameType = frametype;
      packet.isFirstPacket = true;
      packet.markerBit = false;
      packet.seqNum = seqNum;
      packet.timestamp = timeStamp;
      packet.completeNALU = kNaluStart;

      frameIn = jb.GetFrame(packet);
      TEST(frameIn != 0);

      
      TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

      
      TEST(timeStamp - 33 * 90 == jb.NextTimestamp(10, &incomingFrameType,
                                                   &renderTimeMs));

      
      if (i == 0)
      {
          TEST(incomingFrameType == kVideoFrameKey);
      }
      else
      {
          TEST(incomingFrameType == frametype);
      }

      
      frameOut = jb.GetCompleteFrameForDecoding(10);

      
      TEST(frameOut == 0);

      seqNum += 2;
      packet.isFirstPacket = false;
      packet.markerBit = true;
      packet.seqNum = seqNum;
      packet.completeNALU = kNaluEnd;

      frameIn = jb.GetFrame(packet);
      TEST(frameIn != 0);

      
      TEST(kIncomplete == jb.InsertPacket(frameIn, packet));


      
      seqNum++;
      packet.isFirstPacket = false;
      packet.markerBit = false;
      packet.seqNum = seqNum;
      packet.completeNALU = kNaluEnd;
      packet.frameType = kFrameEmpty;

      frameIn = jb.GetFrame(packet);
      TEST(frameIn != 0);

      
      TEST(kIncomplete == jb.InsertPacket(frameIn, packet));

      
      frameOut = jb.GetFrameForDecoding();

      
      
      if (i < 10)
      {
          TEST(CheckOutFrame(frameOut, size, false) == 0);

          
          if (i == 0)
          {
              TEST(frameOut->FrameType() == kVideoFrameKey);
          }
         else
         {
             TEST(frameOut->FrameType() == frametype);
         }
          TEST(frameOut->Complete() == false);
          TEST(frameOut->MissingFrame() == false);
      }

      
      jb.ReleaseFrame(frameOut);
    }

    TEST(jb.num_not_decodable_packets() == 10);

    
    
    timeStamp -= 33 * 90;
    packet.timestamp = timeStamp - 1000;
    frameIn = jb.GetFrame(packet);
    TEST(frameIn == NULL);

    packet.timestamp = timeStamp - 500;
    frameIn = jb.GetFrame(packet);
    TEST(frameIn == NULL);

    packet.timestamp = timeStamp - 100;
    frameIn = jb.GetFrame(packet);
    TEST(frameIn == NULL);

    TEST(jb.num_discarded_packets() == 3);

    jb.Flush();

    
    TEST(jb.num_discarded_packets() == 3);

    


    
    timeStamp += 33*90;
    seqNum += 4;


    
    
    
    
    
    
    

    jb.Flush();

    
    timeStamp += 33*90;
    seqNum = 0xfff0;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    
    loop = 0;
    do
    {
        seqNum++;
        packet.isFirstPacket = false;
        packet.markerBit = false;
        packet.seqNum = seqNum;

        frameIn = jb.GetFrame(packet);
        TEST(frameIn != 0);

        
        TEST(kIncomplete == jb.InsertPacket(frameIn, packet));

        
        TEST(timeStamp == jb.NextTimestamp(2, &incomingFrameType,
                                           &renderTimeMs));

        
        TEST(incomingFrameType == kVideoFrameDelta);

        
        frameOut = jb.GetCompleteFrameForDecoding(2);

        
        TEST(frameOut == 0);

        loop++;
    } while (loop < 98);

    
    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*100, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    

    
    
    
    
    
    
    

    
    jb.Flush();

    
    timeStamp += 33*90;
    seqNum = 10;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    
    loop = 0;
    do
    {
        seqNum--;
        packet.isFirstPacket = false;
        packet.markerBit = false;
        packet.seqNum = seqNum;

        frameIn = jb.GetFrame(packet);
        TEST(frameIn != 0);

        
        TEST(kIncomplete == jb.InsertPacket(frameIn, packet));

        
        TEST(timeStamp == jb.NextTimestamp(2, &incomingFrameType,
                                           &renderTimeMs));

        
        TEST(incomingFrameType == kVideoFrameDelta);

        
        frameOut = jb.GetCompleteFrameForDecoding(2);

        
        TEST(frameOut == 0);

        loop++;
    } while (loop < 98);

    
    seqNum--;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*100, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    

    
    jb.Flush();

    
    
    
    
    
    

    
    timeStamp += 33*90;
    seqNum = 1;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    
    seqNum -= 2;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kIncomplete == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = false;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*3, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    

    
    jb.Flush();

    
    
    
    
    
    
    

    seqNum = 2;
    timeStamp = 3000;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(3000 == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));
    TEST(kVideoFrameDelta == incomingFrameType);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);
    TEST(3000 == frameOut->TimeStamp());

    TEST(CheckOutFrame(frameOut, size, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    jb.ReleaseFrame(frameOut);

    seqNum--;
    timeStamp = 2000;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    
    
    TEST(frameIn == 0);

    

    jb.Flush();

   
    
    
    
    
    
    

    seqNum = 2;
    timeStamp = 3000;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));
    TEST(kVideoFrameDelta == incomingFrameType);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);
    TEST(timeStamp == frameOut->TimeStamp());

    TEST(CheckOutFrame(frameOut, size, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    jb.ReleaseFrame(frameOut);

    seqNum--;
    timeStamp = 0xffffff00;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    
    TEST(frameIn == 0);

    jb.Flush();

    
    
    
    
    
    
    

    seqNum = 1;
    timeStamp = 0xffffff00;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*2, false) == 0);

    jb.ReleaseFrame(frameOut);

    seqNum++;
    timeStamp += 33*90;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = false;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameDelta);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    
    TEST(frameOut == 0);

    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kCompleteSession == jb.InsertPacket(frameIn, packet));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);

    TEST(CheckOutFrame(frameOut, size*2, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);

    

    jb.Flush();

    
    
    
    
    
    
    

    seqNum = 1;
    timeStamp = 0xffffff00;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(0xffffff00 == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));
    TEST(kVideoFrameDelta == incomingFrameType);

    
    seqNum++;
    timeStamp = 2700;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(0xffffff00 == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));
    TEST(kVideoFrameDelta == incomingFrameType);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);
    TEST(0xffffff00 == frameOut->TimeStamp());

    TEST(CheckOutFrame(frameOut, size, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    TEST(2700 == jb.NextTimestamp(0, &incomingFrameType, &renderTimeMs));
    TEST(kVideoFrameDelta == incomingFrameType);

    
    VCMEncodedFrame* frameOut2 = jb.GetCompleteFrameForDecoding(10);
    TEST(2700 == frameOut2->TimeStamp());

    TEST(CheckOutFrame(frameOut2, size, false) == 0);

    
    TEST(frameOut2->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);
    jb.ReleaseFrame(frameOut2);

    

    jb.Flush();

    
    
    
    
    
    
    

    seqNum = 2;
    timeStamp = 2700;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(2700 == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));
    TEST(kVideoFrameDelta == incomingFrameType);

    
    seqNum--;
    timeStamp = 0xffffff00;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(0xffffff00 == jb.NextTimestamp(10, &incomingFrameType, &renderTimeMs));
    TEST(kVideoFrameDelta == incomingFrameType);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);
    TEST(0xffffff00 == frameOut->TimeStamp());

    TEST(CheckOutFrame(frameOut, size, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameDelta);

    
    TEST(2700 == jb.NextTimestamp(0, &incomingFrameType, &renderTimeMs));
    TEST(kVideoFrameDelta == incomingFrameType);

    
    frameOut2 = jb.GetCompleteFrameForDecoding(10);
    TEST(2700 == frameOut2->TimeStamp());

    TEST(CheckOutFrame(frameOut2, size, false) == 0);

    
    TEST(frameOut2->FrameType() == kVideoFrameDelta);

    
    jb.ReleaseFrame(frameOut);
    jb.ReleaseFrame(frameOut2);

    

    
    
    

    jb.Start();

    loop = 0;
    packet.timestamp += 33*90;
    bool firstPacket = true;
    
    do
    {
        seqNum++;
        packet.isFirstPacket = false;
        packet.markerBit = false;
        packet.seqNum = seqNum;

        frameIn = jb.GetFrame(packet);
        TEST(frameIn != 0);

        
        if (firstPacket)
        {
            TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));
            firstPacket = false;
        }
        else
        {
            TEST(kIncomplete == jb.InsertPacket(frameIn, packet));
        }

        
        TEST(packet.timestamp == jb.NextTimestamp(10, &incomingFrameType,
                                                  &renderTimeMs));

        
        TEST(incomingFrameType == kVideoFrameDelta);

        loop++;
    } while (loop < kMaxPacketsInSession);

    

    
    seqNum++;
    packet.isFirstPacket = false;
    packet.markerBit = true;
    packet.seqNum = seqNum;

    frameIn = jb.GetFrame(packet);
    TEST(frameIn != 0);

    
    TEST(kSizeError == jb.InsertPacket(frameIn, packet));

    TEST(0 == jb.GetCompleteFrameForDecoding(10));

    

    
    
    
    
    
    
    
    

    jb.Flush();

    loop = 0;
    seqNum = 65485;
    WebRtc_UWord32 timeStampStart = timeStamp +  33*90;
    WebRtc_UWord32 timeStampFirstKey = 0;
    VCMEncodedFrame* ptrLastDeltaFrame = NULL;
    VCMEncodedFrame* ptrFirstKeyFrame = NULL;
    
    do
    {
        timeStamp += 33*90;
        seqNum++;
        packet.isFirstPacket = true;
        packet.markerBit = true;
        packet.seqNum = seqNum;
        packet.timestamp = timeStamp;

        frameIn = jb.GetFrame(packet);
        TEST(frameIn != 0);

        if (loop == 49)  
        {
            ptrLastDeltaFrame = frameIn;
        }
        if (loop == 50)  
        {
            ptrFirstKeyFrame = frameIn;
            packet.frameType = kVideoFrameKey;
            timeStampFirstKey = packet.timestamp;
        }

        
        TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

        
        TEST(timeStampStart == jb.NextTimestamp(10, &incomingFrameType,
                                                &renderTimeMs));

        
        TEST(incomingFrameType == kVideoFrameDelta);

        loop++;
    } while (loop < kMaxNumberOfFrames);

    

    
    timeStamp += 33*90;
    seqNum++;
    packet.isFirstPacket = true;
    packet.markerBit = true;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;

    
    frameIn = jb.GetFrame(packet);
    
    TEST(frameIn != 0 && frameIn && ptrLastDeltaFrame);

    
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStampFirstKey == jb.NextTimestamp(10, &incomingFrameType,
                                               &renderTimeMs));

    
    TEST(incomingFrameType == kVideoFrameKey);

    
    frameOut = jb.GetCompleteFrameForDecoding(10);
    TEST(ptrFirstKeyFrame == frameOut);

    TEST(CheckOutFrame(frameOut, size, false) == 0);

    
    TEST(frameOut->FrameType() == kVideoFrameKey);

    
    jb.ReleaseFrame(frameOut);

    jb.Flush();

    
    

    
    seqNum = 3;
    
    
    int maxSize = 1000;
    for (int i = 0; i < maxSize + 10; i++)
    {
        timeStamp += 33 * 90;
        seqNum++;
        packet.isFirstPacket = false;
        packet.markerBit = false;
        packet.seqNum = seqNum;
        packet.timestamp = timeStamp;
        packet.frameType = kFrameEmpty;
        VCMEncodedFrame* testFrame = jb.GetFrameForDecoding();
        
        if (testFrame != NULL)
        {
            TEST(testFrame->TimeStamp() < timeStamp);
            printf("Not null TS = %d\n",testFrame->TimeStamp());
        }
    }

    jb.Flush();


    


    
    

    jb.Flush();
    jb.SetNackMode(kNoNack, -1, -1);
    seqNum ++;
    timeStamp += 33 * 90;
    int insertedLength = 0;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = true;
    packet.completeNALU = kNaluStart;
    packet.markerBit = false;

    frameIn = jb.GetFrame(packet);

     
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    seqNum += 2; 
    packet.seqNum = seqNum;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = false;
    packet.completeNALU = kNaluIncomplete;
    packet.markerBit = false;

     
    TEST(kIncomplete == jb.InsertPacket(frameIn, packet));

    seqNum++;
    packet.seqNum = seqNum;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = false;
    packet.completeNALU = kNaluEnd;
    packet.markerBit = false;

    
    TEST(kIncomplete == jb.InsertPacket(frameIn, packet));

    seqNum++;
    packet.seqNum = seqNum;
    packet.completeNALU = kNaluComplete;
    packet.markerBit = true; 
    TEST(kIncomplete == jb.InsertPacket(frameIn, packet));


    
    
    
    
    
    packet.seqNum = 1;
    packet.timestamp = timeStamp + 33 * 90 * 10;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = false;
    packet.completeNALU = kNaluStart;
    packet.markerBit = false;
    frameIn = jb.GetFrame(packet);
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    
    TEST(timeStamp == jb.NextTimestamp(10, &incomingFrameType,
                                       &renderTimeMs));
    frameOut = jb.GetFrameForDecoding();

    
    
    
    TEST(CheckOutFrame(frameOut, packet.sizeBytes * 2, false) == 0);
    jb.ReleaseFrame(frameOut);

    
    seqNum += 2; 
    timeStamp += 33*90;
    insertedLength = 0;

    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = false;
    packet.completeNALU = kNaluEnd;
    packet.markerBit = false;

    TEST(frameIn = jb.GetFrame(packet));
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));
    insertedLength += packet.sizeBytes; 

    seqNum--;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = true;
    packet.completeNALU = kNaluStart;
    packet.markerBit = false;
    TEST(kIncomplete == jb.InsertPacket(frameIn, packet));
    insertedLength += packet.sizeBytes; 

    seqNum += 3; 
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = false;
    packet.completeNALU = kNaluComplete;
    packet.markerBit = false;
    TEST(kIncomplete == jb.InsertPacket(frameIn, packet));
    insertedLength += packet.sizeBytes; 

    seqNum += 1;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = false;
    packet.completeNALU = kNaluStart;
    packet.markerBit = false;
    TEST(kIncomplete == jb.InsertPacket(frameIn, packet));
    
    insertedLength += packet.sizeBytes;

    seqNum += 2;
    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = false;
    packet.completeNALU = kNaluEnd;
    packet.markerBit = true;
    TEST(kIncomplete == jb.InsertPacket(frameIn, packet));
    
    

    frameOut = jb.GetFrameForDecoding();
    
    TEST(CheckOutFrame(frameOut, insertedLength, false) == 0);
    jb.ReleaseFrame(frameOut);


    
    seqNum += 1;
    timeStamp += 33 * 90;
    VCMPacket emptypacket(data, 0, seqNum, timeStamp, true);
    emptypacket.seqNum = seqNum;
    emptypacket.timestamp = timeStamp;
    emptypacket.frameType = kVideoFrameKey;
    emptypacket.isFirstPacket = true;
    emptypacket.completeNALU = kNaluComplete;
    emptypacket.markerBit = true;
    TEST(frameIn = jb.GetFrame(emptypacket));
    TEST(kFirstPacket == jb.InsertPacket(frameIn, emptypacket));
    
    
    insertedLength += 0;

    
    
    frameOut = jb.GetFrameForDecoding();


    
    seqNum += 1;
    timeStamp += 33 * 90;

    packet.seqNum = seqNum;
    packet.timestamp = timeStamp;
    packet.frameType = kVideoFrameKey;
    packet.isFirstPacket = true;
    packet.completeNALU = kNaluComplete;
    packet.markerBit = false;
    TEST(frameIn = jb.GetFrame(packet));
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    seqNum += 1;
    emptypacket.seqNum = seqNum;
    emptypacket.timestamp = timeStamp;
    emptypacket.frameType = kVideoFrameKey;
    emptypacket.isFirstPacket = true;
    emptypacket.completeNALU = kNaluComplete;
    emptypacket.markerBit = true;
    TEST(kCompleteSession == jb.InsertPacket(frameIn, emptypacket));

    
    frameOut = jb.GetCompleteFrameForDecoding(10);
    
    TEST(CheckOutFrame(frameOut, packet.sizeBytes, false) == 0);

    jb.ReleaseFrame(frameOut);

    jb.Flush();

    
    
    

    packet.seqNum += 2;
    packet.frameType = kVideoFrameDelta;
    packet.isFirstPacket = false;
    packet.markerBit = false;

    TEST(frameIn = jb.GetFrame(packet));
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    frameOut = jb.GetFrameForDecoding();
    TEST(frameOut == NULL);

    packet.seqNum += 2;
    packet.timestamp += 33 * 90;

    TEST(frameIn = jb.GetFrame(packet));
    TEST(kFirstPacket == jb.InsertPacket(frameIn, packet));

    frameOut = jb.GetFrameForDecoding();

    TEST(frameOut != NULL);
    TEST(CheckOutFrame(frameOut, packet.sizeBytes, false) == 0);
    jb.ReleaseFrame(frameOut);

    jb.Stop();

    printf("DONE !!!\n");

    printf("\nVCM Jitter Buffer Test: \n\n%i tests completed\n",
           vcmMacrosTests);
    if (vcmMacrosErrors > 0)
    {
        printf("%i FAILED\n\n", vcmMacrosErrors);
    }
    else
    {
        printf("ALL PASSED\n\n");
    }

    return 0;

}
