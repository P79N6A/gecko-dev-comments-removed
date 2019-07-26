













#include <iostream>
#include "isac.h"
#include "utility.h"
#include "signal_processing_library.h"

#define MAX_FILE_NAME  500
#define MAX_NUM_CLIENTS 2


#define NUM_CLIENTS 2

using namespace std;

int main(int argc, char* argv[])
{
  char fileNameWB[MAX_FILE_NAME];
  char fileNameSWB[MAX_FILE_NAME];

  char outFileName[MAX_NUM_CLIENTS][MAX_FILE_NAME];

  FILE* inFile[MAX_NUM_CLIENTS];
  FILE* outFile[MAX_NUM_CLIENTS];

  ISACStruct* codecInstance[MAX_NUM_CLIENTS];
  WebRtc_Word32 resamplerState[MAX_NUM_CLIENTS][8];

  int encoderSampRate[MAX_NUM_CLIENTS];

  int minBn = 16000;
  int maxBn = 56000;

  int bnWB = 32000;
  int bnSWB = 56000;

  strcpy(outFileName[0], "switchSampRate_out1.pcm");
  strcpy(outFileName[1], "switchSampRate_out2.pcm");

  short clientCntr;

  unsigned int lenEncodedInBytes[MAX_NUM_CLIENTS];
  unsigned int lenAudioIn10ms[MAX_NUM_CLIENTS];
  unsigned int lenEncodedInBytesTmp[MAX_NUM_CLIENTS];
  unsigned int lenAudioIn10msTmp[MAX_NUM_CLIENTS];
  BottleNeckModel* packetData[MAX_NUM_CLIENTS];

  char versionNumber[100];
  short samplesIn10ms[MAX_NUM_CLIENTS];
  int bottleneck[MAX_NUM_CLIENTS];

  printf("\n\n");
  printf("____________________________________________\n\n");
  WebRtcIsac_version(versionNumber);
  printf("    iSAC-swb version %s\n", versionNumber);
  printf("____________________________________________\n");


  fileNameWB[0]  = '\0';
  fileNameSWB[0] = '\0';

  char myFlag[20];
  strcpy(myFlag, "-wb");
  
  if(readParamString(argc, argv, myFlag, fileNameWB, MAX_FILE_NAME) <= 0)
  {
    printf("No wideband file is specified");
  }

  strcpy(myFlag, "-swb");
  if(readParamString(argc, argv, myFlag, fileNameSWB, MAX_FILE_NAME) <= 0)
  {
    printf("No super-wideband file is specified");
  }

  
  encoderSampRate[0] = 16000;
  OPEN_FILE_RB(inFile[0], fileNameWB);

  
  encoderSampRate[1] = 32000;
  OPEN_FILE_RB(inFile[1], fileNameSWB);

  strcpy(myFlag, "-I");
  short codingMode = readSwitch(argc, argv, myFlag);

  for(clientCntr = 0; clientCntr < NUM_CLIENTS; clientCntr++)
  {
    codecInstance[clientCntr] = NULL;

    printf("\n");
    printf("Client %d\n", clientCntr + 1);
    printf("---------\n");
    printf("Starting %s",
           (encoderSampRate[clientCntr] == 16000)
           ? "wideband":"super-wideband");

    
    OPEN_FILE_WB(outFile[clientCntr], outFileName[clientCntr]);
    printf("Output File...................... %s\n", outFileName[clientCntr]);

    samplesIn10ms[clientCntr] = encoderSampRate[clientCntr] * 10;

    if(codingMode == 1)
    {
      bottleneck[clientCntr] = (clientCntr)? bnSWB:bnWB;
    }
    else
    {
      bottleneck[clientCntr] = (clientCntr)? minBn:maxBn;
    }

    printf("Bottleneck....................... %0.3f kbits/sec \n",
           bottleneck[clientCntr] / 1000.0);

    
    printf("Encoding Mode.................... %s\n",
           (codingMode == 1)? "Channel-Independent (Instantaneous)":"Adaptive");

    lenEncodedInBytes[clientCntr] = 0;
    lenAudioIn10ms[clientCntr] = 0;
    lenEncodedInBytesTmp[clientCntr] = 0;
    lenAudioIn10msTmp[clientCntr] = 0;

    packetData[clientCntr] = (BottleNeckModel*)new(BottleNeckModel);
    if(packetData[clientCntr] == NULL)
    {
      printf("Could not allocate memory for packetData \n");
      return -1;
    }
    memset(packetData[clientCntr], 0, sizeof(BottleNeckModel));
    memset(resamplerState[clientCntr], 0, sizeof(WebRtc_Word32) * 8);
  }

  for(clientCntr = 0; clientCntr < NUM_CLIENTS; clientCntr++)
  {
    
    if(WebRtcIsac_Create(&codecInstance[clientCntr]))
    {
      printf("Could not creat client %d\n", clientCntr + 1);
      return -1;
    }

    WebRtcIsac_SetEncSampRate(codecInstance[clientCntr], encoderSampRate[clientCntr]);

    WebRtcIsac_SetDecSampRate(codecInstance[clientCntr],
                              encoderSampRate[clientCntr + (1 - ((clientCntr & 1)<<1))]);

    
    if(WebRtcIsac_EncoderInit(codecInstance[clientCntr],
                              codingMode) < 0)
    {
      printf("Could not initialize client, %d\n", clientCntr + 1);
      return -1;
    }

    
    if(WebRtcIsac_DecoderInit(codecInstance[clientCntr]) < 0)
    {
      printf("Could not initialize decoder of client %d\n",
             clientCntr + 1);
      return -1;
    }

    
    if(codingMode != 0)
    {
      
      if(WebRtcIsac_Control(codecInstance[clientCntr],
                            bottleneck[clientCntr], 30) < 0)
      {
        printf("Could not setup bottleneck and frame-size for client %d\n",
               clientCntr + 1);
        return -1;
      }
    }
  }


  short streamLen;
  short numSamplesRead;
  short lenDecodedAudio;
  short senderIdx;
  short receiverIdx;

  printf("\n");
  short num10ms[MAX_NUM_CLIENTS];
  memset(num10ms, 0, sizeof(short)*MAX_NUM_CLIENTS);
  FILE* arrivalTimeFile1 = fopen("arrivalTime1.dat", "wb");
  FILE* arrivalTimeFile2 = fopen("arrivalTime2.dat", "wb");
  short numPrint[MAX_NUM_CLIENTS];
  memset(numPrint, 0, sizeof(short) * MAX_NUM_CLIENTS);

  
  short silence10ms[10 * 32];
  memset(silence10ms, 0, 320 * sizeof(short));
  short audioBuff10ms[10 * 32];
  short audioBuff60ms[60 * 32];
  short resampledAudio60ms[60 * 32];

  unsigned short bitStream[600+600];
  short speechType[1];

  short numSampFreqChanged = 0;
  while(numSampFreqChanged < 10)
  {
    for(clientCntr = 0; clientCntr < NUM_CLIENTS; clientCntr++)
    {
      
      
      
      
      
      
      senderIdx = clientCntr; 
      receiverIdx = 1 - clientCntr;

      
      
      
      
      
      

      numSamplesRead = (short)fread(audioBuff10ms, sizeof(short),
                                    samplesIn10ms[senderIdx], inFile[senderIdx]);
      if(numSamplesRead != samplesIn10ms[senderIdx])
      {
        
        printf("Changing Encoder Sampling frequency in client %d to ", senderIdx+1);
        fclose(inFile[senderIdx]);
        numSampFreqChanged++;
        if(encoderSampRate[senderIdx] == 16000)
        {
          printf("super-wideband.\n");
          OPEN_FILE_RB(inFile[senderIdx], fileNameSWB);
          encoderSampRate[senderIdx] = 32000;
        }
        else
        {
          printf("wideband.\n");
          OPEN_FILE_RB(inFile[senderIdx], fileNameWB);
          encoderSampRate[senderIdx] = 16000;
        }
        WebRtcIsac_SetEncSampRate(codecInstance[senderIdx], encoderSampRate[senderIdx]);
        WebRtcIsac_SetDecSampRate(codecInstance[receiverIdx], encoderSampRate[senderIdx]);

        samplesIn10ms[clientCntr] = encoderSampRate[clientCntr] * 10;

        numSamplesRead = (short)fread(audioBuff10ms, sizeof(short),
                                      samplesIn10ms[senderIdx], inFile[senderIdx]);
        if(numSamplesRead != samplesIn10ms[senderIdx])
        {
          printf(" File %s for client %d has not enough audio\n",
                 (encoderSampRate[senderIdx]==16000)? "wideband":"super-wideband",
                 senderIdx + 1);
          return -1;
        }
      }
      num10ms[senderIdx]++;

      
      
      
      
      
      
      

      


      streamLen = WebRtcIsac_Encode(codecInstance[senderIdx],
                                    audioBuff10ms, (short*)bitStream);
      WebRtc_Word16 ggg;
      if (streamLen > 0) {
        if((  WebRtcIsac_ReadFrameLen(codecInstance[receiverIdx],
                                      (short *) bitStream, &ggg))<0)
          printf("ERROR\n");
      }

      
      if(streamLen < 0)
      {
        printf(" Encoder error in client %d \n", senderIdx + 1);
        return -1;
      }


      if(streamLen > 0)
      {
        
        
        lenEncodedInBytes[senderIdx] += streamLen;
        lenAudioIn10ms[senderIdx] += (unsigned int)num10ms[senderIdx];
        lenEncodedInBytesTmp[senderIdx] += streamLen;
        lenAudioIn10msTmp[senderIdx] += (unsigned int)num10ms[senderIdx];

        
        if(lenAudioIn10msTmp[senderIdx] >= 100)
        {
          numPrint[senderIdx]++;
          printf("  %d,  %6.3f => %6.3f ", senderIdx+1,
                 bottleneck[senderIdx] / 1000.0,
                 lenEncodedInBytesTmp[senderIdx] * 0.8 /
                 lenAudioIn10msTmp[senderIdx]);

          if(codingMode == 0)
          {
            WebRtc_Word32 bn;
            WebRtcIsac_GetUplinkBw(codecInstance[senderIdx], &bn);
            printf("[%d] ", bn);
          }
          
          
          
          
          

          cout << flush;
          lenEncodedInBytesTmp[senderIdx] = 0;
          lenAudioIn10msTmp[senderIdx]    = 0;
          
          
          printf("  %0.1f \n", lenAudioIn10ms[senderIdx] * 10. /1000);
          

          
          
          
          
          
          
          
          

          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
        }

        
        get_arrival_time(num10ms[senderIdx] * samplesIn10ms[senderIdx],
                         streamLen, bottleneck[senderIdx], packetData[senderIdx],
                         encoderSampRate[senderIdx]*1000, encoderSampRate[senderIdx]*1000);

        
        if(senderIdx == 0)
        {
          if (fwrite(&(packetData[senderIdx]->arrival_time),
                     sizeof(unsigned int),
                     1, arrivalTimeFile1) != 1) {
            return -1;
          }
        }
        else
        {
          if (fwrite(&(packetData[senderIdx]->arrival_time),
                     sizeof(unsigned int),
                     1, arrivalTimeFile2) != 1) {
            return -1;
          }
        }

        
        if(WebRtcIsac_UpdateBwEstimate(codecInstance[receiverIdx],
                                       bitStream,  streamLen, packetData[senderIdx]->rtp_number,
                                       packetData[senderIdx]->sample_count,
                                       packetData[senderIdx]->arrival_time) < 0)
        {
          printf(" BWE Error at client %d \n", receiverIdx + 1);
          return -1;
        }
        
        
        lenDecodedAudio = WebRtcIsac_Decode(
            codecInstance[receiverIdx], bitStream, streamLen,
            audioBuff60ms, speechType);
        if(lenDecodedAudio < 0)
        {
          printf(" Decoder error in client %d \n", receiverIdx + 1);
          return -1;
        }


        if(encoderSampRate[senderIdx] == 16000)
        {
          WebRtcSpl_UpsampleBy2(audioBuff60ms, lenDecodedAudio, resampledAudio60ms,
                                resamplerState[receiverIdx]);
          if (fwrite(resampledAudio60ms, sizeof(short), lenDecodedAudio << 1,
                     outFile[receiverIdx]) !=
              static_cast<size_t>(lenDecodedAudio << 1)) {
            return -1;
          }
        }
        else
        {
          if (fwrite(audioBuff60ms, sizeof(short), lenDecodedAudio,
                     outFile[receiverIdx]) !=
              static_cast<size_t>(lenDecodedAudio)) {
            return -1;
          }
        }
        num10ms[senderIdx] = 0;
      }
      
      
    }
  }
}
