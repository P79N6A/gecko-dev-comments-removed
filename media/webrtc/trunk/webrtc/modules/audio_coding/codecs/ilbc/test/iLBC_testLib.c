

















#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ilbc.h"


#ifdef JUNK_DATA
#define SEED_FILE "randseed.txt"
#endif










int main(int argc, char* argv[])
{
  FILE *ifileid,*efileid,*ofileid, *chfileid;
  short encoded_data[55], data[240], speechType;
  short len, mode, pli;
  int blockcount = 0;

  iLBC_encinst_t *Enc_Inst;
  iLBC_decinst_t *Dec_Inst;
#ifdef JUNK_DATA
  int i;
  FILE *seedfile;
  unsigned int random_seed = (unsigned int) time(NULL);
#endif

  
  WebRtcIlbcfix_EncoderCreate(&Enc_Inst);
  WebRtcIlbcfix_DecoderCreate(&Dec_Inst);

  

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
    exit(2);
  }
  
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

#ifdef JUNK_DATA
  srand(random_seed);

  if ( (seedfile = fopen(SEED_FILE, "a+t") ) == NULL ) {
    fprintf(stderr, "Error: Could not open file %s\n", SEED_FILE);
  }
  else {
    fprintf(seedfile, "%u\n", random_seed);
    fclose(seedfile);
  }
#endif

  
  WebRtcIlbcfix_EncoderInit(Enc_Inst, mode);
  WebRtcIlbcfix_DecoderInit(Dec_Inst, mode);

  
#ifdef SPLIT_10MS
  while(fread(data, sizeof(short), 80, ifileid) == 80) {
#else
  while((short)fread(data,sizeof(short),(mode<<3),ifileid)==(mode<<3)) {
#endif
    blockcount++;

    
    fprintf(stderr, "--- Encoding block %i --- ",blockcount);
#ifdef SPLIT_10MS
    len=WebRtcIlbcfix_Encode(Enc_Inst, data, 80, encoded_data);
#else
    len=WebRtcIlbcfix_Encode(Enc_Inst, data, (short)(mode<<3), encoded_data);
#endif
    fprintf(stderr, "\r");

#ifdef JUNK_DATA
    for ( i = 0; i < len; i++) {
      encoded_data[i] = (short) (encoded_data[i] + (short) rand());
    }
#endif
    
    if(len != 0){ 
      fwrite(encoded_data,1,len,efileid);
    }

    if(len != 0){ 
      
      if (argc==6) {
        if (fread(&pli, sizeof(WebRtc_Word16), 1, chfileid)) {
          if ((pli!=0)&&(pli!=1)) {
            fprintf(stderr, "Error in channel file\n");
            exit(0);
          }
          if (pli==0) {
            
            memset(encoded_data, 0, sizeof(WebRtc_Word16)*25);
          }
        } else {
          fprintf(stderr, "Error. Channel file too short\n");
          exit(0);
        }
      } else {
        pli=1;
      }

      
      fprintf(stderr, "--- Decoding block %i --- ",blockcount);
      if (pli==1) {
        len=WebRtcIlbcfix_Decode(Dec_Inst, encoded_data, len, data, &speechType);
      } else {
        len=WebRtcIlbcfix_DecodePlc(Dec_Inst, data, 1);
      }
      fprintf(stderr, "\r");

      
      fwrite(data,sizeof(short),len,ofileid);
    }
  }

#ifdef JUNK_DATA
  if ( (seedfile = fopen(SEED_FILE, "a+t") ) == NULL ) {
    fprintf(stderr, "Error: Could not open file %s\n", SEED_FILE);
  }
  else {
    fprintf(seedfile, "ok\n\n");
    fclose(seedfile);
  }
#endif

  
  WebRtcIlbcfix_EncoderFree(Enc_Inst);
  WebRtcIlbcfix_DecoderFree(Dec_Inst);

  
  fclose(ifileid);
  fclose(efileid);
  fclose(ofileid);

  return 0;
}
