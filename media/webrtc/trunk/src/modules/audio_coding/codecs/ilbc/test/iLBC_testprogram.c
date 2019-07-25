

















#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "nit_encode.h"
#include "encode.h"
#include "init_decode.h"
#include "decode.h"
#include "constants.h"
#include "ilbc.h"

#define ILBCNOOFWORDS_MAX (NO_OF_BYTES_30MS)/2


#include <time.h>






short encode(                         
    iLBC_Enc_Inst_t *iLBCenc_inst,    
    WebRtc_Word16 *encoded_data,      
    WebRtc_Word16 *data               
                                                        ){

  
  WebRtcIlbcfix_Encode((WebRtc_UWord16 *)encoded_data, data, iLBCenc_inst);

  return (iLBCenc_inst->no_of_bytes);
}





short decode( 
    iLBC_Dec_Inst_t *iLBCdec_inst, 
    short *decoded_data, 
    short *encoded_data, 
    short mode           
              ){

  

  if (mode<0 || mode>1) {
    printf("\nERROR - Wrong mode - 0, 1 allowed\n"); exit(3);}

  

  WebRtcIlbcfix_Decode(decoded_data, (WebRtc_UWord16 *)encoded_data,
                       iLBCdec_inst, mode);

  return (iLBCdec_inst->blockl);
}









#define MAXFRAMES   10000
#define MAXFILELEN (BLOCKL_MAX*MAXFRAMES)

int main(int argc, char* argv[])
{

  

  float starttime1, starttime2;
  float runtime1, runtime2;
  float outtime;

  FILE *ifileid,*efileid,*ofileid, *chfileid;
  short *inputdata, *encodeddata, *decodeddata;
  short *channeldata;
  int blockcount = 0, noOfBlocks=0, i, noOfLostBlocks=0;
  short mode;
  iLBC_Enc_Inst_t Enc_Inst;
  iLBC_Dec_Inst_t Dec_Inst;

  short frameLen;
  short count;
#ifdef SPLIT_10MS
  short size;
#endif

  inputdata=(short*) malloc(MAXFILELEN*sizeof(short));
  if (inputdata==NULL) {
    fprintf(stderr,"Could not allocate memory for vector\n");
    exit(0);
  }
  encodeddata=(short*) malloc(ILBCNOOFWORDS_MAX*MAXFRAMES*sizeof(short));
  if (encodeddata==NULL) {
    fprintf(stderr,"Could not allocate memory for vector\n");
    free(inputdata);
    exit(0);
  }
  decodeddata=(short*) malloc(MAXFILELEN*sizeof(short));
  if (decodeddata==NULL) {
    fprintf(stderr,"Could not allocate memory for vector\n");
    free(inputdata);
    free(encodeddata);
    exit(0);
  }
  channeldata=(short*) malloc(MAXFRAMES*sizeof(short));
  if (channeldata==NULL) {
    fprintf(stderr,"Could not allocate memory for vector\n");
    free(inputdata);
    free(encodeddata);
    free(decodeddata);
    exit(0);
  }

  

  if (argc != 6 ) {
    fprintf(stderr, "%s mode inputfile bytefile outputfile channelfile\n",
            argv[0]);
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "%s <30,20> in.pcm byte.dat out.pcm T30.0.dat\n", argv[0]);
    exit(1);
  }
  mode=atoi(argv[1]);
  if (mode != 20 && mode != 30) {
    fprintf(stderr,"Wrong mode %s, must be 20, or 30\n", argv[1]);
    exit(2);
  }
  if ( (ifileid=fopen(argv[2],"rb")) == NULL) {
    fprintf(stderr,"Cannot open input file %s\n", argv[2]);
    exit(2);}
  if ( (efileid=fopen(argv[3],"wb")) == NULL) {
    fprintf(stderr, "Cannot open channelfile file %s\n",
            argv[3]); exit(3);}
  if( (ofileid=fopen(argv[4],"wb")) == NULL) {
    fprintf(stderr, "Cannot open output file %s\n",
            argv[4]); exit(3);}
  if ( (chfileid=fopen(argv[5],"rb")) == NULL) {
    fprintf(stderr,"Cannot open channel file file %s\n", argv[5]);
    exit(2);}


  
#ifndef PRINT_MIPS
  fprintf(stderr, "\n");
  fprintf(stderr,
          "*---------------------------------------------------*\n");
  fprintf(stderr,
          "*                                                   *\n");
  fprintf(stderr,
          "*      iLBCtest                                     *\n");
  fprintf(stderr,
          "*                                                   *\n");
  fprintf(stderr,
          "*                                                   *\n");
  fprintf(stderr,
          "*---------------------------------------------------*\n");
#ifdef SPLIT_10MS
  fprintf(stderr,"\n10ms split with raw mode: %2d ms\n", mode);
#else
  fprintf(stderr,"\nMode          : %2d ms\n", mode);
#endif
  fprintf(stderr,"\nInput file    : %s\n", argv[2]);
  fprintf(stderr,"Coded file    : %s\n", argv[3]);
  fprintf(stderr,"Output file   : %s\n\n", argv[4]);
  fprintf(stderr,"Channel file  : %s\n\n", argv[5]);
#endif

  

  WebRtcIlbcfix_EncoderInit(&Enc_Inst, mode);
  WebRtcIlbcfix_DecoderInit(&Dec_Inst, mode, 1);

  

#ifdef SPLIT_10MS
  frameLen = (mode==20)? 80:160;
  fread(Enc_Inst.past_samples, sizeof(short), frameLen, ifileid);
  Enc_Inst.section = 0;

  while( fread(&inputdata[noOfBlocks*80], sizeof(short),
               80, ifileid) == 80 ) {
    noOfBlocks++;
  }

  noOfBlocks += frameLen/80;
  frameLen = 80;
#else
  frameLen = Enc_Inst.blockl;

  while( fread(&inputdata[noOfBlocks*Enc_Inst.blockl],sizeof(short),
               Enc_Inst.blockl,ifileid)==(WebRtc_UWord16)Enc_Inst.blockl){
    noOfBlocks++;
  }
#endif


  while ((fread(&channeldata[blockcount],sizeof(short), 1,chfileid)==1)
            && ( blockcount < noOfBlocks/(Enc_Inst.blockl/frameLen) )) {
    blockcount++;
  }

  if ( blockcount < noOfBlocks/(Enc_Inst.blockl/frameLen) ) {
    fprintf(stderr,"Channel file %s is too short\n", argv[4]);
    free(inputdata);
    free(encodeddata);
    free(decodeddata);
    free(channeldata);
    exit(0);
  }

  count=0;

  

  starttime1 = clock()/(float)CLOCKS_PER_SEC;

  
#ifdef PRINT_MIPS
  printf("-1 -1\n");
#endif

#ifdef SPLIT_10MS
  


  
  while( count < blockcount * (Enc_Inst.blockl/frameLen) )    {

    encode(&Enc_Inst, &encodeddata[Enc_Inst.no_of_words *
                                   (count/(Enc_Inst.nsub/2))],
           &inputdata[frameLen * count] );
#else
    while (count < noOfBlocks) {
      encode( &Enc_Inst, &encodeddata[Enc_Inst.no_of_words * count],
              &inputdata[frameLen * count] );
#endif

#ifdef PRINT_MIPS
      printf("-1 -1\n");
#endif

      count++;
    }

    count=0;

    

    starttime2=clock()/(float)CLOCKS_PER_SEC;
    runtime1 = (float)(starttime2-starttime1);

    

    while (count < blockcount) {
      if (channeldata[count]==1) {
        
        decode(&Dec_Inst, &decodeddata[count * Dec_Inst.blockl],
               &encodeddata[Dec_Inst.no_of_words * count], 1);
      } else if (channeldata[count]==0) {
        
        short emptydata[ILBCNOOFWORDS_MAX];
        memset(emptydata, 0, Dec_Inst.no_of_words*sizeof(short));
        decode(&Dec_Inst, &decodeddata[count*Dec_Inst.blockl],
               emptydata, 0);
        noOfLostBlocks++;
      } else {
        printf("Error in channel file (values have to be either 1 or 0)\n");
        exit(0);
      }
#ifdef PRINT_MIPS
      printf("-1 -1\n");
#endif

      count++;
    }

    

    runtime2 = (float)(clock()/(float)CLOCKS_PER_SEC-starttime2);

    outtime = (float)((float)blockcount*
                      (float)mode/1000.0);

#ifndef PRINT_MIPS
    printf("\nLength of speech file: %.1f s\n", outtime);
    printf("Lost frames          : %.1f%%\n\n", 100*(float)noOfLostBlocks/(float)blockcount);

    printf("Time to run iLBC_encode+iLBC_decode:");
    printf(" %.1f s (%.1f%% of realtime)\n", runtime1+runtime2,
           (100*(runtime1+runtime2)/outtime));

    printf("Time in iLBC_encode                :");
    printf(" %.1f s (%.1f%% of total runtime)\n",
           runtime1, 100.0*runtime1/(runtime1+runtime2));

    printf("Time in iLBC_decode                :");
    printf(" %.1f s (%.1f%% of total runtime)\n\n",
           runtime2, 100.0*runtime2/(runtime1+runtime2));
#endif

    
    for (i=0; i<blockcount; i++) {
      fwrite(&encodeddata[i*Enc_Inst.no_of_words], sizeof(short),
             Enc_Inst.no_of_words, efileid);
    }
    for (i=0;i<blockcount;i++) {
      fwrite(&decodeddata[i*Enc_Inst.blockl],sizeof(short),Enc_Inst.blockl,ofileid);
    }

    

    free(inputdata);
    free(encodeddata);
    free(decodeddata);
    free(channeldata);
    fclose(ifileid);  fclose(efileid); fclose(ofileid);
    return(0);
  }
