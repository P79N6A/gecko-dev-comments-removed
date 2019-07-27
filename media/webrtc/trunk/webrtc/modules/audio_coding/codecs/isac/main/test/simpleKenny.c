











#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef WIN32
#include "windows.h"
#define CLOCKS_PER_SEC  1000
#endif

#include <ctype.h>
#include <math.h>


#include "isac.h"
#include "utility.h"



#define MAX_FRAMESAMPLES_SWB                1920

#define FRAMESAMPLES_SWB_10ms               320
#define FRAMESAMPLES_WB_10ms                160


#define FS_SWB                               32000
#define FS_WB                                16000



#ifdef HAVE_DEBUG_INFO
    #include "debugUtility.h"
    debugStruct debugInfo;
#endif

unsigned long framecnt = 0;

int main(int argc, char* argv[])
{
    
    FILE* inp;
    FILE* outp;
    char inname[500];
    char outname[500];

    
    double        rate;
    double        rateRCU;
    unsigned long totalbits = 0;
    unsigned long totalBitsRCU = 0;
    unsigned long totalsmpls =0;

    int32_t   bottleneck = 39;
    int16_t   frameSize = 30;           
    int16_t   codingMode = 1;
    int16_t   shortdata[FRAMESAMPLES_SWB_10ms];
    int16_t   decoded[MAX_FRAMESAMPLES_SWB];
    
    int16_t   speechType[1];
    int16_t   payloadLimit;
    int32_t   rateLimit;
    ISACStruct*   ISAC_main_inst;

    int16_t   stream_len = 0;
    int16_t   declen = 0;
    int16_t   err;
    int16_t   cur_framesmpls;
    int           endfile;
#ifdef WIN32
    double        length_file;
    double        runtime;
    char          outDrive[10];
    char          outPath[500];
    char          outPrefix[500];
    char          outSuffix[500];
    char          bitrateFileName[500];
    FILE*         bitrateFile;
    double        starttime;
    double        rateLB = 0;
    double        rateUB = 0;
#endif
    FILE*         histFile;
    FILE*         averageFile;
    int           sampFreqKHz;
    int           samplesIn10Ms;
    int16_t   maxStreamLen = 0;
    char          histFileName[500];
    char          averageFileName[500];
    unsigned int  hist[600];
    unsigned int  tmpSumStreamLen = 0;
    unsigned int  packetCntr = 0;
    unsigned int  lostPacketCntr = 0;
    uint8_t  payload[1200];
    uint8_t  payloadRCU[1200];
    uint16_t  packetLossPercent = 0;
    int16_t   rcuStreamLen = 0;
	int onlyEncode;
	int onlyDecode;


    BottleNeckModel packetData;
	packetData.arrival_time  = 0;
	packetData.sample_count  = 0;
	packetData.rtp_number    = 0;
    memset(hist, 0, sizeof(hist));

    
    if(argc < 5)
    {
		int size;
		WebRtcIsac_AssignSize(&size);

        printf("\n\nWrong number of arguments or flag values.\n\n");

        printf("Usage:\n\n");
        printf("%s infile outfile -bn bottelneck [options] \n\n", argv[0]);
        printf("with:\n");
        printf("-I................... indicates encoding in instantaneous mode.\n");
        printf("-bn bottleneck....... the value of the bottleneck in bit/sec, e.g. 39742,\n");
		printf("                      in instantaneous (channel-independent) mode.\n\n");
        printf("infile............... Normal speech input file\n\n");
        printf("outfile.............. Speech output file\n\n");
        printf("OPTIONS\n");
        printf("-------\n");
        printf("-fs sampFreq......... sampling frequency of codec 16 or 32 (default) kHz.\n");
        printf("-plim payloadLim..... payload limit in bytes,\n");
        printf("                      default is the maximum possible.\n");
        printf("-rlim rateLim........ rate limit in bits/sec, \n");
        printf("                      default is the maimum possible.\n");
        printf("-h file.............. record histogram and *append* to 'file'.\n");
        printf("-ave file............ record average rate of 3 sec intervales and *append* to 'file'.\n");
        printf("-ploss............... packet-loss percentage.\n");
		printf("-enc................. do only encoding and store the bit-stream\n");
		printf("-dec................. the input file is a bit-stream, decode it.\n");

        printf("\n");
        printf("Example usage:\n\n");
        printf("%s speechIn.pcm speechOut.pcm -B 40000 -fs 32 \n\n", argv[0]);

		printf("structure size %d bytes\n", size);

        exit(0);
    }



    
    bottleneck = readParamInt(argc, argv, "-bn", 50000);
    fprintf(stderr,"\nfixed bottleneck rate of %d bits/s\n\n", bottleneck);

    
    sscanf(argv[1], "%s", inname);
    sscanf(argv[2], "%s", outname);
    codingMode = readSwitch(argc, argv, "-I");
    sampFreqKHz = (int16_t)readParamInt(argc, argv, "-fs", 32);
    if(readParamString(argc, argv, "-h", histFileName, 500) > 0)
    {
        histFile = fopen(histFileName, "a");
        if(histFile == NULL)
        {
            printf("cannot open hist file %s", histFileName);
            exit(0);
        }
    }
    else
    {
        
        histFile = NULL;
    }


    packetLossPercent = readParamInt(argc, argv, "-ploss", 0);

    if(readParamString(argc, argv, "-ave", averageFileName, 500) > 0)
    {
        averageFile = fopen(averageFileName, "a");
        if(averageFile == NULL)
        {
            printf("cannot open file to write rate %s", averageFileName);
            exit(0);
        }
    }
    else
    {
        averageFile = NULL;
    }

	onlyEncode = readSwitch(argc, argv, "-enc");
	onlyDecode = readSwitch(argc, argv, "-dec");


    switch(sampFreqKHz)
    {
    case 16:
        {
            samplesIn10Ms = 160;
            break;
        }
    case 32:
        {
            samplesIn10Ms = 320;
            break;
        }
    default:
        printf("A sampling frequency of %d kHz is not supported,\
valid values are 8 and 16.\n", sampFreqKHz);
        exit(-1);
    }
    payloadLimit = (int16_t)readParamInt(argc, argv, "-plim", 400);
    rateLimit = readParamInt(argc, argv, "-rlim", 106800);

    if ((inp = fopen(inname,"rb")) == NULL) {
        printf("  iSAC: Cannot read file %s.\n", inname);
        exit(1);
    }
    if ((outp = fopen(outname,"wb")) == NULL) {
        printf("  iSAC: Cannot write file %s.\n", outname);
        exit(1);
    }

#ifdef WIN32
    _splitpath(outname, outDrive, outPath, outPrefix, outSuffix);
    _makepath(bitrateFileName, outDrive, outPath, "bitrate", ".txt");

    bitrateFile = fopen(bitrateFileName, "a");
    fprintf(bitrateFile, "%  %%s  \n", inname);
#endif

    printf("\n");
    printf("Input.................... %s\n", inname);
    printf("Output................... %s\n", outname);
    printf("Encoding Mode............ %s\n",
        (codingMode == 1)? "Channel-Independent":"Channel-Adaptive");
    printf("Bottleneck............... %d bits/sec\n", bottleneck);
    printf("Packet-loss Percentage... %d\n", packetLossPercent);
    printf("\n");

#ifdef WIN32
    starttime = clock()/(double)CLOCKS_PER_SEC; 
#endif

    
    err = WebRtcIsac_Create(&ISAC_main_inst);

    WebRtcIsac_SetEncSampRate(ISAC_main_inst, sampFreqKHz * 1000);
    WebRtcIsac_SetDecSampRate(ISAC_main_inst, sampFreqKHz >= 32 ? 32000 :
        16000);
    
    if (err < 0) {
        fprintf(stderr,"\n\n Error in create.\n\n");
        exit(EXIT_FAILURE);
    }

    framecnt = 0;
    endfile     = 0;

    
    if(WebRtcIsac_EncoderInit(ISAC_main_inst, codingMode) < 0)
    {
        printf("cannot initialize encoder\n");
        return -1;
    }
    if(WebRtcIsac_DecoderInit(ISAC_main_inst) < 0)
    {
        printf("cannot initialize decoder\n");
        return -1;
    }

    
    
    
    
    
    
    
    
    
    
    
    

    if(codingMode == 1)
    {
        if(WebRtcIsac_Control(ISAC_main_inst, bottleneck, frameSize) < 0)
        {
            printf("cannot set bottleneck\n");
            return -1;
        }
    }
    else
    {
        if(WebRtcIsac_ControlBwe(ISAC_main_inst, 15000, 30, 1) < 0)
        {
            printf("cannot configure BWE\n");
            return -1;
        }
    }

    if(WebRtcIsac_SetMaxPayloadSize(ISAC_main_inst, payloadLimit) < 0)
    {
        printf("cannot set maximum payload size %d.\n", payloadLimit);
        return -1;
    }

    if (rateLimit < 106800) {
        if(WebRtcIsac_SetMaxRate(ISAC_main_inst, rateLimit) < 0)
        {
            printf("cannot set the maximum rate %d.\n", rateLimit);
            return -1;
        }
    }

    







    while (endfile == 0)
    {
        fprintf(stderr,"  \rframe = %7li", framecnt);

        
        cur_framesmpls = 0;
        stream_len = 0;


		if(onlyDecode)
		{
			uint8_t auxUW8;
                        size_t auxSizet;
			if(fread(&auxUW8, sizeof(uint8_t), 1, inp) < 1)
			{
				break;
			}
			stream_len = ((uint8_t)auxUW8) << 8;
			if(fread(&auxUW8, sizeof(uint8_t), 1, inp) < 1)
			{
				break;
			}
			stream_len |= (uint16_t)auxUW8;
                        auxSizet = (size_t)stream_len;
                        if(fread(payload, 1, auxSizet, inp) < auxSizet)
			{
				printf("last payload is corrupted\n");
				break;
			}
		}
		else
		{
			while(stream_len == 0)
			{
				
				endfile = readframe(shortdata, inp, samplesIn10Ms);
				if(endfile)
				{
					break;
				}
				cur_framesmpls += samplesIn10Ms;

				
                                stream_len = WebRtcIsac_Encode(
                                        ISAC_main_inst,
                                        shortdata,
                                        payload);

				if(stream_len < 0)
				{
					
					
					fprintf(stderr,"\nError in encoder\n");
					getc(stdin);
					exit(EXIT_FAILURE);
				}


			}
			
			if(endfile)
			{
				break;
			}

                        rcuStreamLen = WebRtcIsac_GetRedPayload(
                            ISAC_main_inst, payloadRCU);

			get_arrival_time(cur_framesmpls, stream_len, bottleneck, &packetData,
				sampFreqKHz * 1000, sampFreqKHz * 1000);
			if(WebRtcIsac_UpdateBwEstimate(ISAC_main_inst,
                                                       payload,
                                                       stream_len,
                                                       packetData.rtp_number,
                                                       packetData.sample_count,
                                                       packetData.arrival_time)
                           < 0)
			{
				printf(" BWE Error at client\n");
				return -1;
			}
		}

        if(endfile)
        {
            break;
        }

        maxStreamLen = (stream_len > maxStreamLen)? stream_len:maxStreamLen;
        packetCntr++;

        hist[stream_len]++;
        if(averageFile != NULL)
        {
            tmpSumStreamLen += stream_len;
            if(packetCntr == 100)
            {
                
                fprintf(averageFile, "%8.3f ", (double)tmpSumStreamLen * 8.0 / (30.0 * packetCntr));
                packetCntr = 0;
                tmpSumStreamLen = 0;
            }
        }

		if(onlyEncode)
		{
                  uint8_t auxUW8;
                  auxUW8 = (uint8_t)(((stream_len & 0x7F00) >> 8) & 0xFF);
                  if (fwrite(&auxUW8, sizeof(uint8_t), 1, outp) != 1) {
                    return -1;
                  }

                  auxUW8 = (uint8_t)(stream_len & 0xFF);
                  if (fwrite(&auxUW8, sizeof(uint8_t), 1, outp) != 1) {
                    return -1;
                  }
                  if (fwrite(payload, 1, stream_len,
                             outp) != (size_t)stream_len) {
                    return -1;
                  }
		}
		else
		{

			

			if((rand() % 100) < packetLossPercent)
			{
                                declen = WebRtcIsac_DecodeRcu(
                                    ISAC_main_inst,
                                    payloadRCU,
                                    rcuStreamLen,
                                    decoded,
                                    speechType);
				lostPacketCntr++;
			}
			else
			{
                                declen = WebRtcIsac_Decode(
                                    ISAC_main_inst,
                                    payload,
                                    stream_len,
                                    decoded,
                                    speechType);
                        }
                        if(declen <= 0)
			{
				
				fprintf(stderr,"\nError in decoder.\n");
				getc(stdin);
				exit(1);
			}

			
                        if (fwrite(decoded, sizeof(int16_t),
                                   declen, outp) != (size_t)declen) {
                          return -1;
                        }
			cur_framesmpls = declen;
		}
        
        framecnt++;
        totalsmpls += cur_framesmpls;
        if(stream_len > 0)
        {
            totalbits += 8 * stream_len;
        }
        if(rcuStreamLen > 0)
        {
            totalBitsRCU += 8 * rcuStreamLen;
        }
    }

    rate =    ((double)totalbits    * (sampFreqKHz)) / (double)totalsmpls;
    rateRCU = ((double)totalBitsRCU * (sampFreqKHz)) / (double)totalsmpls;

    printf("\n\n");
    printf("Sampling Rate......................... %d kHz\n", sampFreqKHz);
    printf("Payload Limit......................... %d bytes \n", payloadLimit);
    printf("Rate Limit............................ %d bits/sec \n", rateLimit);

#ifdef WIN32
#ifdef HAVE_DEBUG_INFO
    rateLB = ((double)debugInfo.lbBytes * 8. *
              (sampFreqKHz)) / (double)totalsmpls;
    rateUB = ((double)debugInfo.ubBytes * 8. *
              (sampFreqKHz)) / (double)totalsmpls;
#endif

    fprintf(bitrateFile, "%d  %10u     %d     %6.3f  %6.3f    %6.3f\n",
        sampFreqKHz,
        framecnt,
        bottleneck,
        rateLB,
        rateUB,
        rate);
    fclose(bitrateFile);
#endif   

    printf("\n");
    printf("Measured bit-rate..................... %0.3f kbps\n", rate);
    printf("Measured RCU bit-ratre................ %0.3f kbps\n", rateRCU);
    printf("Maximum bit-rate/payloadsize.......... %0.3f / %d\n",
        maxStreamLen * 8 / 0.03, maxStreamLen);
    printf("Measured packet-loss.................. %0.1f%% \n",
        100.0f * (float)lostPacketCntr / (float)packetCntr);













    printf("\n");

    
#ifdef WIN32
    runtime = (double)(clock()/(double)CLOCKS_PER_SEC-starttime);
    length_file = ((double)framecnt*(double)declen/(sampFreqKHz*1000));
    printf("Length of speech file................ %.1f s\n", length_file);
    printf("Time to run iSAC..................... %.2f s (%.2f %% of realtime)\n\n",
        runtime, (100*runtime/length_file));
#endif
    printf("\n\n_______________________________________________\n");

    if(histFile != NULL)
    {
        int n;
        for(n = 0; n < 600; n++)
        {
            fprintf(histFile, "%6d ", hist[n]);
        }
        fprintf(histFile, "\n");
        fclose(histFile);
    }
    if(averageFile != NULL)
    {
        if(packetCntr > 0)
        {
            fprintf(averageFile, "%8.3f ", (double)tmpSumStreamLen * 8.0 / (30.0 * packetCntr));
        }
        fprintf(averageFile, "\n");
        fclose(averageFile);
    }

    fclose(inp);
    fclose(outp);

    WebRtcIsac_Free(ISAC_main_inst);


#ifdef CHANGE_OUTPUT_NAME
    {
        char* p;
        char myExt[50];
        char bitRateStr[10];
        char newOutName[500];
        strcpy(newOutName, outname);

        myExt[0] = '\0';
        p = strchr(newOutName, '.');
        if(p != NULL)
        {
            strcpy(myExt, p);
            *p = '_';
            p++;
            *p = '\0';
        }
        else
        {
            strcat(newOutName, "_");
        }
        sprintf(bitRateStr, "%0.0fkbps", rate);
        strcat(newOutName, bitRateStr);
        strcat(newOutName, myExt);
        rename(outname, newOutName);
    }
#endif
    exit(0);
}


#ifdef HAVE_DEBUG_INFO
int setupDebugStruct(debugStruct* str)
{
    str->prevPacketLost = 0;
    str->currPacketLost = 0;

    OPEN_FILE_WB(str->res0to4FilePtr,     "Res0to4.dat");
    OPEN_FILE_WB(str->res4to8FilePtr,     "Res4to8.dat");
    OPEN_FILE_WB(str->res8to12FilePtr,    "Res8to12.dat");
    OPEN_FILE_WB(str->res8to16FilePtr,    "Res8to16.dat");

    OPEN_FILE_WB(str->res0to4DecFilePtr,  "Res0to4Dec.dat");
    OPEN_FILE_WB(str->res4to8DecFilePtr,  "Res4to8Dec.dat");
    OPEN_FILE_WB(str->res8to12DecFilePtr, "Res8to12Dec.dat");
    OPEN_FILE_WB(str->res8to16DecFilePtr, "Res8to16Dec.dat");

    OPEN_FILE_WB(str->in0to4FilePtr,      "in0to4.dat");
    OPEN_FILE_WB(str->in4to8FilePtr,      "in4to8.dat");
    OPEN_FILE_WB(str->in8to12FilePtr,     "in8to12.dat");
    OPEN_FILE_WB(str->in8to16FilePtr,     "in8to16.dat");

    OPEN_FILE_WB(str->out0to4FilePtr,     "out0to4.dat");
    OPEN_FILE_WB(str->out4to8FilePtr,     "out4to8.dat");
    OPEN_FILE_WB(str->out8to12FilePtr,    "out8to12.dat");
    OPEN_FILE_WB(str->out8to16FilePtr,    "out8to16.dat");
    OPEN_FILE_WB(str->fftFilePtr,         "riFFT.dat");
    OPEN_FILE_WB(str->fftDecFilePtr,      "riFFTDec.dat");

    OPEN_FILE_WB(str->arrivalTime,        NULL);
    str->lastArrivalTime = 0;

    str->maxPayloadLB = 0;
    str->maxPayloadUB = 0;
    str->lbBytes = 0;
    str->ubBytes = 0;

    return 0;
};
#endif
