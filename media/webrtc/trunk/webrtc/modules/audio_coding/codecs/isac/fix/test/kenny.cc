









#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "webrtc/modules/audio_coding/codecs/isac/fix/interface/isacfix.h"
#include "webrtc/test/testsupport/perf_test.h"





#define SEED_FILE "randseed.txt"  /* Used when running decoder on garbage data */
#define MAX_FRAMESAMPLES    960   /* max number of samples per frame (= 60 ms frame) */
#define FRAMESAMPLES_10ms 160   /* number of samples per 10ms frame */
#define FS           16000 /* sampling frequency (Hz) */


int readframe(WebRtc_Word16 *data, FILE *inp, int length) {

  short k, rlen, status = 0;

  rlen = fread(data, sizeof(WebRtc_Word16), length, inp);
  if (rlen < length) {
    for (k = rlen; k < length; k++)
      data[k] = 0;
    status = 1;
  }

  return status;
}


typedef struct {
  WebRtc_UWord32 send_time;            
  WebRtc_UWord32 arrival_time;         
  WebRtc_UWord32 sample_count;         
  WebRtc_UWord16 rtp_number;
} BottleNeckModel;

void get_arrival_time(int current_framesamples,   
                      int packet_size,            
                      int bottleneck,             
                      BottleNeckModel *BN_data)
{
  const int HeaderSize = 35;
  int HeaderRate;

  HeaderRate = HeaderSize * 8 * FS / current_framesamples;     

  
  BN_data->sample_count = BN_data->sample_count + current_framesamples;

  BN_data->arrival_time += ((packet_size + HeaderSize) * 8 * FS) / (bottleneck + HeaderRate);
  BN_data->send_time += current_framesamples;

  if (BN_data->arrival_time < BN_data->sample_count)
    BN_data->arrival_time = BN_data->sample_count;

  BN_data->rtp_number++;
}

void get_arrival_time2(int current_framesamples,
                       int current_delay,
                       BottleNeckModel *BN_data)
{
  if (current_delay == -1)
    
  {
    BN_data->arrival_time += current_framesamples;
  }
  else if (current_delay != -2)
  {
    
    BN_data->arrival_time += (current_framesamples + ((FS/1000) * current_delay));
  }
  
  

  BN_data->rtp_number++;
}

int main(int argc, char* argv[])
{

  char inname[100], outname[100],  outbitsname[100], bottleneck_file[100];
  FILE *inp, *outp, *f_bn, *outbits;
  int endfile;

  int i, errtype, h = 0, k, packetLossPercent = 0;
  WebRtc_Word16 CodingMode;
  WebRtc_Word16 bottleneck;
  WebRtc_Word16 framesize = 30;           
  int cur_framesmpls, err = 0, lostPackets = 0;

  
  double starttime, runtime, length_file;

  WebRtc_Word16 stream_len = 0;
  WebRtc_Word16 framecnt, declen = 0;
  WebRtc_Word16 shortdata[FRAMESAMPLES_10ms];
  WebRtc_Word16 decoded[MAX_FRAMESAMPLES];
  WebRtc_UWord16 streamdata[500];
  WebRtc_Word16 speechType[1];
  WebRtc_Word16 prevFrameSize = 1;
  WebRtc_Word16 rateBPS = 0;
  WebRtc_Word16 fixedFL = 0;
  WebRtc_Word16 payloadSize = 0;
  WebRtc_Word32 payloadRate = 0;
  int setControlBWE = 0;
  int readLoss;
  FILE  *plFile = NULL;

  char version_number[20];
  char tmpBit[5] = ".bit";

  int totalbits =0;
  int totalsmpls =0;
  WebRtc_Word16 testNum, testCE;

  FILE *fp_gns = NULL;
  int gns = 0;
  int cur_delay = 0;
  char gns_file[100];

  int nbTest = 0;
  WebRtc_Word16 lostFrame;
  float scale = (float)0.7;
  
  ISACFIX_MainStruct *ISAC_main_inst = NULL;

  
  FILE *seedfile;
  unsigned int random_seed = (unsigned int) time(NULL);

  BottleNeckModel       BN_data;
  f_bn  = NULL;

  readLoss = 0;
  packetLossPercent = 0;

  
  if ((argc<3) || (argc>21))  {
    printf("\n\nWrong number of arguments or flag values.\n\n");

    printf("\n");
    WebRtcIsacfix_version(version_number);
    printf("iSAC version %s \n\n", version_number);

    printf("Usage:\n\n");
    printf("./kenny.exe [-F num][-I] bottleneck_value infile outfile \n\n");
    printf("with:\n");
    printf("[-I]             :if -I option is specified, the coder will use\n");
    printf("                  an instantaneous Bottleneck value. If not, it\n");
    printf("                  will be an adaptive Bottleneck value.\n\n");
    printf("bottleneck_value :the value of the bottleneck provided either\n");
    printf("                  as a fixed value (e.g. 25000) or\n");
    printf("                  read from a file (e.g. bottleneck.txt)\n\n");
    printf("[-INITRATE num]  :Set a new value for initial rate. Note! Only used"
           " in adaptive mode.\n\n");
    printf("[-FL num]        :Set (initial) frame length in msec. Valid length"
           " are 30 and 60 msec.\n\n");
    printf("[-FIXED_FL]      :Frame length to be fixed to initial value.\n\n");
    printf("[-MAX num]       :Set the limit for the payload size of iSAC"
           " in bytes. \n");
    printf("                  Minimum 100, maximum 400.\n\n");
    printf("[-MAXRATE num]   :Set the maxrate for iSAC in bits per second. \n");
    printf("                  Minimum 32000, maximum 53400.\n\n");
    printf("[-F num]         :if -F option is specified, the test function\n");
    printf("                  will run the iSAC API fault scenario specified"
           " by the\n");
    printf("                  supplied number.\n");
    printf("                  F 1 - Call encoder prior to init encoder call\n");
    printf("                  F 2 - Call decoder prior to init decoder call\n");
    printf("                  F 3 - Call decoder prior to encoder call\n");
    printf("                  F 4 - Call decoder with a too short coded"
           " sequence\n");
    printf("                  F 5 - Call decoder with a too long coded"
           " sequence\n");
    printf("                  F 6 - Call decoder with random bit stream\n");
    printf("                  F 7 - Call init encoder/decoder at random"
           " during a call\n");
    printf("                  F 8 - Call encoder/decoder without having"
           " allocated memory for \n");
    printf("                        encoder/decoder instance\n");
    printf("                  F 9 - Call decodeB without calling decodeA\n");
    printf("                  F 10 - Call decodeB with garbage data\n");
    printf("[-PL num]       : if -PL option is specified 0<num<100 will "
           "specify the\n");
    printf("                  percentage of packet loss\n\n");
    printf("[-G file]       : if -G option is specified the file given is"
           " a .gns file\n");
    printf("                  that represents a network profile\n\n");
    printf("[-NB num]       : if -NB option, use the narrowband interfaces\n");
    printf("                  num=1 => encode with narrowband encoder"
           " (infile is narrowband)\n");
    printf("                  num=2 => decode with narrowband decoder"
           " (outfile is narrowband)\n\n");
    printf("[-CE num]       : Test of APIs used by Conference Engine.\n");
    printf("                  CE 1 - createInternal, freeInternal,"
           " getNewBitstream \n");
    printf("                  CE 2 - transcode, getBWE \n");
    printf("                  CE 3 - getSendBWE, setSendBWE.  \n\n");
    printf("[-RTP_INIT num] : if -RTP_INIT option is specified num will be"
           " the initial\n");
    printf("                  value of the rtp sequence number.\n\n");
    printf("infile          : Normal speech input file\n\n");
    printf("outfile         : Speech output file\n\n");
    printf("Example usage   : \n\n");
    printf("./kenny.exe -I bottleneck.txt speechIn.pcm speechOut.pcm\n\n");
    exit(0);

  }

  
  WebRtcIsacfix_version(version_number);
  printf("iSAC version %s \n\n", version_number);

  
  CodingMode = 0;
  testNum = 0;
  testCE = 0;
  for (i = 1; i < argc-2;i++) {
    
    if (!strcmp ("-I", argv[i])) {
      printf("\nInstantaneous BottleNeck\n");
      CodingMode = 1;
      i++;
    }

    
    if (!strcmp ("-INITRATE", argv[i])) {
      rateBPS = atoi(argv[i + 1]);
      setControlBWE = 1;
      if ((rateBPS < 10000) || (rateBPS > 32000)) {
        printf("\n%d is not a initial rate. "
               "Valid values are in the range 10000 to 32000.\n", rateBPS);
        exit(0);
      }
      printf("\nNew initial rate: %d\n", rateBPS);
      i++;
    }

    
    if (!strcmp ("-FL", argv[i])) {
      framesize = atoi(argv[i + 1]);
      if ((framesize != 30) && (framesize != 60)) {
        printf("\n%d is not a valid frame length. "
               "Valid length are 30 and 60 msec.\n", framesize);
        exit(0);
      }
      printf("\nFrame Length: %d\n", framesize);
      i++;
    }

    
    if (!strcmp ("-FIXED_FL", argv[i])) {
      fixedFL = 1;
      setControlBWE = 1;
    }

    
    if (!strcmp ("-MAX", argv[i])) {
      payloadSize = atoi(argv[i + 1]);
      printf("Maximum Payload Size: %d\n", payloadSize);
      i++;
    }

    
    if (!strcmp ("-MAXRATE", argv[i])) {
      payloadRate = atoi(argv[i + 1]);
      printf("Maximum Rate in kbps: %d\n", payloadRate);
      i++;
    }

    
    if (!strcmp ("-F", argv[i])) {
      testNum = atoi(argv[i + 1]);
      printf("\nFault test: %d\n", testNum);
      if (testNum < 1 || testNum > 10) {
        printf("\n%d is not a valid Fault Scenario number."
               " Valid Fault Scenarios are numbered 1-10.\n", testNum);
        exit(0);
      }
      i++;
    }

    
    if (!strcmp ("-PL", argv[i])) {
      if( isdigit( *argv[i+1] ) ) {
        packetLossPercent = atoi( argv[i+1] );
        if( (packetLossPercent < 0) | (packetLossPercent > 100) ) {
          printf( "\nInvalid packet loss perentage \n" );
          exit( 0 );
        }
        if( packetLossPercent > 0 ) {
          printf( "\nSimulating %d %% of independent packet loss\n",
                  packetLossPercent );
        } else {
          printf( "\nNo Packet Loss Is Simulated \n" );
        }
        readLoss = 0;
      } else {
        readLoss = 1;
        plFile = fopen( argv[i+1], "rb" );
        if( plFile == NULL ) {
          printf( "\n couldn't open the frameloss file: %s\n", argv[i+1] );
          exit( 0 );
        }
        printf( "\nSimulating packet loss through the given "
                "channel file: %s\n", argv[i+1] );
      }
      i++;
    }

    
    if (!strcmp ("-rnd", argv[i])) {
      srand(time(NULL) );
      printf( "\n Random pattern in lossed packets \n" );
    }

    
    if (!strcmp ("-G", argv[i])) {
      sscanf(argv[i + 1], "%s", gns_file);
      fp_gns = fopen(gns_file, "rb");
      if (fp_gns  == NULL) {
        printf("Cannot read file %s.\n", gns_file);
        exit(0);
      }
      gns = 1;
      i++;
    }

    
    if (!strcmp ("-NB", argv[i])) {
      nbTest = atoi(argv[i + 1]);
      i++;
    }

    
    if (!strcmp ("-CE", argv[i])) {
      testCE = atoi(argv[i + 1]);
      if (testCE==1 || testCE==2) {
        i++;
        scale = (float)atof( argv[i+1] );
      } else if (testCE < 1 || testCE > 3) {
        printf("\n%d is not a valid CE-test number, valid Fault "
               "Scenarios are numbered 1-3\n", testCE);
        exit(0);
      }
      i++;
    }

    
    if (!strcmp ("-RTP_INIT", argv[i])) {
      i++;
    }
  }

  
  
  bottleneck = atoi(argv[CodingMode+1]);
  if (bottleneck == 0 && gns == 0) {
    sscanf(argv[CodingMode+1], "%s", bottleneck_file);
    f_bn = fopen(bottleneck_file, "rb");
    if (f_bn  == NULL) {
      printf("No value provided for BottleNeck and cannot read file %s\n",
             bottleneck_file);
      exit(0);
    } else {
      int aux_var;
      printf("reading bottleneck rates from file %s\n\n",bottleneck_file);
      if (fscanf(f_bn, "%d", &aux_var) == EOF) {
        
        fseek(f_bn, 0L, SEEK_SET);
        if (fscanf(f_bn, "%d", &aux_var) == EOF) {
          exit(0);
        }
      }
      bottleneck = (WebRtc_Word16)aux_var;
      





    }
  } else {
    f_bn = NULL;
    printf("\nfixed bottleneck rate of %d bits/s\n\n", bottleneck);
  }

  if (CodingMode == 0) {
    printf("\nAdaptive BottleNeck\n");
  }

  
  sscanf(argv[argc-2], "%s", inname);
  sscanf(argv[argc-1], "%s", outname);

  
  while ((int)outname[h] != 0) {
    outbitsname[h] = outname[h];
    h++;
  }
  for (k=0; k<5; k++) {
    outbitsname[h] = tmpBit[k];
    h++;
  }
  if ((inp = fopen(inname,"rb")) == NULL) {
    printf("  iSAC: Cannot read file %s\n", inname);
    exit(1);
  }
  if ((outp = fopen(outname,"wb")) == NULL) {
    printf("  iSAC: Cannot write file %s\n", outname);
    exit(1);
  }

  if ((outbits = fopen(outbitsname,"wb")) == NULL) {
    printf("  iSAC: Cannot write file %s\n", outbitsname);
    exit(1);
  }
  printf("\nInput:%s\nOutput:%s\n\n", inname, outname);

  
  if (testNum == 10) {
    
    srand(random_seed);

    if ( (seedfile = fopen(SEED_FILE, "a+t") ) == NULL ) {
      printf("Error: Could not open file %s\n", SEED_FILE);
    }
    else {
      fprintf(seedfile, "%u\n", random_seed);
      fclose(seedfile);
    }
  }

  
  starttime = clock()/(double)CLOCKS_PER_SEC;

  
  if (testNum != 8)
  {
    if(1){
      err =WebRtcIsacfix_Create(&ISAC_main_inst);
    }else{
      
      int sss;
      void *ppp;
      err =WebRtcIsacfix_AssignSize(&sss);
      ppp=malloc(sss);
      err =WebRtcIsacfix_Assign(&ISAC_main_inst,ppp);
    }
    
    if (err < 0) {
      printf("\n\n Error in create.\n\n");
    }
    if (testCE == 1) {
      err = WebRtcIsacfix_CreateInternal(ISAC_main_inst);
      
      if (err < 0) {
        printf("\n\n Error in createInternal.\n\n");
      }
    }
  }

  
  BN_data.send_time     = 0;
  BN_data.arrival_time  = 0;
  BN_data.sample_count  = 0;
  BN_data.rtp_number    = 0;

  
  framecnt= 0;
  endfile = 0;
  if (testNum != 1) {
    WebRtcIsacfix_EncoderInit(ISAC_main_inst, CodingMode);
  }
  if (testNum != 2) {
    WebRtcIsacfix_DecoderInit(ISAC_main_inst);
  }

  if (CodingMode == 1) {
    err = WebRtcIsacfix_Control(ISAC_main_inst, bottleneck, framesize);
    if (err < 0) {
      
      errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
      printf("\n\n Error in control: %d.\n\n", errtype);
    }
  } else if(setControlBWE == 1) {
    err = WebRtcIsacfix_ControlBwe(ISAC_main_inst, rateBPS, framesize, fixedFL);
  }

  if (payloadSize != 0) {
    err = WebRtcIsacfix_SetMaxPayloadSize(ISAC_main_inst, payloadSize);
    if (err < 0) {
      
      errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
      printf("\n\n Error in SetMaxPayloadSize: %d.\n\n", errtype);
      exit(EXIT_FAILURE);
    }
  }
  if (payloadRate != 0) {
    err = WebRtcIsacfix_SetMaxRate(ISAC_main_inst, payloadRate);
    if (err < 0) {
      
      errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
      printf("\n\n Error in SetMaxRateInBytes: %d.\n\n", errtype);
      exit(EXIT_FAILURE);
    }
  }

  *speechType = 1;


  while (endfile == 0) {

    if(testNum == 7 && (rand()%2 == 0)) {
      err = WebRtcIsacfix_EncoderInit(ISAC_main_inst, CodingMode);
      
      if (err < 0) {
        errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
        printf("\n\n Error in encoderinit: %d.\n\n", errtype);
      }

      err = WebRtcIsacfix_DecoderInit(ISAC_main_inst);
      
      if (err < 0) {
        errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
        printf("\n\n Error in decoderinit: %d.\n\n", errtype);
      }
    }


    cur_framesmpls = 0;
    while (1) {
      
      if (nbTest != 1) {
        endfile = readframe(shortdata, inp, FRAMESAMPLES_10ms);
      } else {
        endfile = readframe(shortdata, inp, (FRAMESAMPLES_10ms/2));
      }

      if (testNum == 7) {
        srand(time(NULL));
      }

      
      if (!(testNum == 3 && framecnt == 0)) {
        if (nbTest != 1) {
          short bwe;

          
          stream_len = WebRtcIsacfix_Encode(ISAC_main_inst,
                                            shortdata,
                                            (WebRtc_Word16*)streamdata);

          

          if (stream_len>0) {
            if (testCE == 1) {
              err = WebRtcIsacfix_ReadBwIndex((WebRtc_Word16*)streamdata, &bwe);
              stream_len = WebRtcIsacfix_GetNewBitStream(
                  ISAC_main_inst,
                  bwe,
                  scale,
                  (WebRtc_Word16*)streamdata);
            } else if (testCE == 2) {
              
            } else if (testCE == 3) {
              


              err = WebRtcIsacfix_GetDownLinkBwIndex(ISAC_main_inst, &bwe);
              
              if (err < 0) {
                errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
                printf("\nError in getSendBWE: %d.\n", errtype);
              }

              err = WebRtcIsacfix_UpdateUplinkBw(ISAC_main_inst, bwe);
              
              if (err < 0) {
                errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
                printf("\nError in setBWE: %d.\n", errtype);
              }

            }
          }
        } else {
#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
          stream_len = WebRtcIsacfix_EncodeNb(ISAC_main_inst,
                                              shortdata,
                                              streamdata);
#else
          stream_len = -1;
#endif
        }
      }
      else
      {
        break;
      }

      if (stream_len < 0 || err < 0) {
        
        errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
        printf("\nError in encoder: %d.\n", errtype);
      } else {
        if (fwrite(streamdata, sizeof(char),
                   stream_len, outbits) != (size_t)stream_len) {
          return -1;
        }
      }

      cur_framesmpls += FRAMESAMPLES_10ms;

      
      if (f_bn != NULL) {
        int aux_var;
        if (fscanf(f_bn, "%d", &aux_var) == EOF) {
          
          fseek(f_bn, 0L, SEEK_SET);
          if (fscanf(f_bn, "%d", &aux_var) == EOF) {
            exit(0);
          }
        }
        bottleneck = (WebRtc_Word16)aux_var;
        if (CodingMode == 1) {
          WebRtcIsacfix_Control(ISAC_main_inst, bottleneck, framesize);
        }
      }

      
      if (stream_len != 0) break;
    }

    
    
    if (testNum == 4) {
      stream_len += 10;
    }

    
    
    if (testNum == 5) {
      stream_len -= 10;
    }

    if (testNum == 6) {
      srand(time(NULL));
      for (i = 0; i < stream_len; i++ ) {
        streamdata[i] = rand();
      }
    }

    
    if (fp_gns != NULL) {
      if (fscanf(fp_gns, "%d", &cur_delay) == EOF) {
        fseek(fp_gns, 0L, SEEK_SET);
        if (fscanf(fp_gns, "%d", &cur_delay) == EOF) {
          exit(0);
        }
      }
    }

    
    if (!(testNum == 3 && framecnt == 0)) {
      if (gns == 0) {
        get_arrival_time(cur_framesmpls, stream_len, bottleneck,
                         &BN_data);
      } else {
        get_arrival_time2(cur_framesmpls, cur_delay, &BN_data);
      }
    }

    
    if (cur_delay != -1) {

      
      if (testNum == 10) {
        for ( i = 0; i < stream_len; i++) {
          streamdata[i] = (short) (streamdata[i] + (short) rand());
        }
      }

      if (testNum != 9) {
        err = WebRtcIsacfix_UpdateBwEstimate(ISAC_main_inst,
                                             streamdata,
                                             stream_len,
                                             BN_data.rtp_number,
                                             BN_data.send_time,
                                             BN_data.arrival_time);

        if (err < 0) {
          
          errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
          printf("\nError in decoder: %d.\n", errtype);
        }
      }

      if( readLoss == 1 ) {
        if( fread( &lostFrame, sizeof(WebRtc_Word16), 1, plFile ) != 1 ) {
          rewind( plFile );
        }
        lostFrame = !lostFrame;
      } else {
        lostFrame = (rand()%100 < packetLossPercent);
      }



      
      if( lostFrame && framecnt >  0) {
        if (nbTest !=2) {
          declen = WebRtcIsacfix_DecodePlc(ISAC_main_inst,
                                           decoded, prevFrameSize );
        } else {
#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
          declen = WebRtcIsacfix_DecodePlcNb(ISAC_main_inst, decoded,
                                             prevFrameSize );
#else
          declen = -1;
#endif
        }
        lostPackets++;
      } else {
        if (nbTest !=2 ) {
          short FL;
          
          err = WebRtcIsacfix_ReadFrameLen((WebRtc_Word16*)streamdata, &FL);
          declen = WebRtcIsacfix_Decode( ISAC_main_inst, streamdata, stream_len,
                                         decoded, speechType );
          
          if (err<0 || declen<0 || FL!=declen) {
            errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
            printf("\nError in decode_B/or getFrameLen: %d.\n", errtype);
          }
          prevFrameSize = declen/480;

        } else {
#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
          declen = WebRtcIsacfix_DecodeNb( ISAC_main_inst, streamdata,
                                           stream_len, decoded, speechType );
#else
          declen = -1;
#endif
          prevFrameSize = declen/240;
        }
      }

      if (declen <= 0) {
        
        errtype=WebRtcIsacfix_GetErrorCode(ISAC_main_inst);
        printf("\nError in decoder: %d.\n", errtype);
      }

      
      if (fwrite(decoded, sizeof(WebRtc_Word16),
                 declen, outp) != (size_t)declen) {
        return -1;
      }
      
      
    } else {
      lostPackets++;
    }
    framecnt++;

    totalsmpls += declen;
    totalbits += 8 * stream_len;

    
    if (testNum == 10) {
      if ( (seedfile = fopen(SEED_FILE, "a+t") ) == NULL ) {
        printf( "Error: Could not open file %s\n", SEED_FILE);
      }
      else {
        fprintf(seedfile, "ok\n\n");
        fclose(seedfile);
      }
    }
  }
  printf("\nLost Frames %d ~ %4.1f%%\n", lostPackets,
         (double)lostPackets/(double)framecnt*100.0 );
  printf("\n\ntotal bits                          = %d bits", totalbits);
  printf("\nmeasured average bitrate              = %0.3f kbits/s",
         (double)totalbits *(FS/1000) / totalsmpls);
  printf("\n");

  


  runtime = (double)(((double)clock()/(double)CLOCKS_PER_SEC)-starttime);
  length_file = ((double)framecnt*(double)declen/FS);
  printf("\n\nLength of speech file: %.1f s\n", length_file);
  printf("Time to run iSAC:      %.2f s (%.2f %% of realtime)\n\n",
         runtime, (100*runtime/length_file));
  printf("\n\n_______________________________________________\n");

  
  webrtc::test::PrintResult("time_per_10ms_frame", "", "isac",
                            (runtime * 10000) / length_file, "us", false);

  fclose(inp);
  fclose(outp);
  fclose(outbits);

  if ( testCE == 1) {
    WebRtcIsacfix_FreeInternal(ISAC_main_inst);
  }
  WebRtcIsacfix_Free(ISAC_main_inst);
  return 0;
}
