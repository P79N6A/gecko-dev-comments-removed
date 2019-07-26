

















#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ilbc.h"
















#define BLOCKL_MAX			240
#define ILBCNOOFWORDS_MAX	25


int main(int argc, char* argv[])
{

  FILE *ifileid,*efileid,*ofileid, *cfileid;
  WebRtc_Word16 data[BLOCKL_MAX];
  WebRtc_Word16 encoded_data[ILBCNOOFWORDS_MAX], decoded_data[BLOCKL_MAX];
  int len;
  short pli, mode;
  int blockcount = 0;
  int packetlosscount = 0;
  int frameLen;
  WebRtc_Word16 speechType;
  iLBC_encinst_t *Enc_Inst;
  iLBC_decinst_t *Dec_Inst;

#ifdef __ILBC_WITH_40BITACC
  
  if (sizeof(long)>=sizeof(long long)) {
    fprintf(stderr, "40-bit simulation is not be supported on this platform\n");
    exit(0);
  }
#endif

  

  if ((argc!=5) && (argc!=6)) {
    fprintf(stderr,
            "\n*-----------------------------------------------*\n");
    fprintf(stderr,
            "   %s <20,30> input encoded decoded (channel)\n\n",
            argv[0]);
    fprintf(stderr,
            "   mode    : Frame size for the encoding/decoding\n");
    fprintf(stderr,
            "                 20 - 20 ms\n");
    fprintf(stderr,
            "                 30 - 30 ms\n");
    fprintf(stderr,
            "   input   : Speech for encoder (16-bit pcm file)\n");
    fprintf(stderr,
            "   encoded : Encoded bit stream\n");
    fprintf(stderr,
            "   decoded : Decoded speech (16-bit pcm file)\n");
    fprintf(stderr,
            "   channel : Packet loss pattern, optional (16-bit)\n");
    fprintf(stderr,
            "                  1 - Packet received correctly\n");
    fprintf(stderr,
            "                  0 - Packet Lost\n");
    fprintf(stderr,
            "*-----------------------------------------------*\n\n");
    exit(1);
  }
  mode=atoi(argv[1]);
  if (mode != 20 && mode != 30) {
    fprintf(stderr,"Wrong mode %s, must be 20, or 30\n",
            argv[1]);
    exit(2);
  }
  if ( (ifileid=fopen(argv[2],"rb")) == NULL) {
    fprintf(stderr,"Cannot open input file %s\n", argv[2]);
    exit(2);}
  if ( (efileid=fopen(argv[3],"wb")) == NULL) {
    fprintf(stderr, "Cannot open encoded file file %s\n",
            argv[3]); exit(1);}
  if ( (ofileid=fopen(argv[4],"wb")) == NULL) {
    fprintf(stderr, "Cannot open decoded file %s\n",
            argv[4]); exit(1);}
  if (argc==6) {
    if( (cfileid=fopen(argv[5],"rb")) == NULL) {
      fprintf(stderr, "Cannot open channel file %s\n",
              argv[5]);
      exit(1);
    }
  } else {
    cfileid=NULL;
  }

  

  fprintf(stderr, "\n");
  fprintf(stderr,
          "*---------------------------------------------------*\n");
  fprintf(stderr,
          "*                                                   *\n");
  fprintf(stderr,
          "*      iLBC test program                            *\n");
  fprintf(stderr,
          "*                                                   *\n");
  fprintf(stderr,
          "*                                                   *\n");
  fprintf(stderr,
          "*---------------------------------------------------*\n");
  fprintf(stderr,"\nMode           : %2d ms\n", mode);
  fprintf(stderr,"Input file     : %s\n", argv[2]);
  fprintf(stderr,"Encoded file   : %s\n", argv[3]);
  fprintf(stderr,"Output file    : %s\n", argv[4]);
  if (argc==6) {
    fprintf(stderr,"Channel file   : %s\n", argv[5]);
  }
  fprintf(stderr,"\n");

  
  WebRtcIlbcfix_EncoderCreate(&Enc_Inst);
  WebRtcIlbcfix_DecoderCreate(&Dec_Inst);


  

  WebRtcIlbcfix_EncoderInit(Enc_Inst, mode);
  WebRtcIlbcfix_DecoderInit(Dec_Inst, mode);
  frameLen = mode*8;

  

  while (((WebRtc_Word16)fread(data,sizeof(WebRtc_Word16),frameLen,ifileid))==
         frameLen) {

    blockcount++;

    

    fprintf(stderr, "--- Encoding block %i --- ",blockcount);
    len=WebRtcIlbcfix_Encode(Enc_Inst, data, (WebRtc_Word16)frameLen, encoded_data);
    fprintf(stderr, "\r");

    

    if (fwrite(encoded_data, sizeof(WebRtc_Word16),
               ((len+1)/sizeof(WebRtc_Word16)), efileid) !=
        (size_t)(((len+1)/sizeof(WebRtc_Word16)))) {
      return -1;
    }

    
    if (argc==6) {
      if (fread(&pli, sizeof(WebRtc_Word16), 1, cfileid)) {
        if ((pli!=0)&&(pli!=1)) {
          fprintf(stderr, "Error in channel file\n");
          exit(0);
        }
        if (pli==0) {
          
          memset(encoded_data, 0,
                 sizeof(WebRtc_Word16)*ILBCNOOFWORDS_MAX);
          packetlosscount++;
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
      len=WebRtcIlbcfix_Decode(Dec_Inst, encoded_data,
                               (WebRtc_Word16)len, decoded_data,&speechType);
    } else {
      len=WebRtcIlbcfix_DecodePlc(Dec_Inst, decoded_data, 1);
    }
    fprintf(stderr, "\r");

    

    if (fwrite(decoded_data, sizeof(WebRtc_Word16), len,
               ofileid) != (size_t)len) {
      return -1;
    }
  }

  

  fclose(ifileid);  fclose(efileid); fclose(ofileid);
  if (argc==6) {
    fclose(cfileid);
  }

  
  WebRtcIlbcfix_EncoderFree(Enc_Inst);
  WebRtcIlbcfix_DecoderFree(Dec_Inst);


  printf("\nDone with simulation\n\n");

  return(0);
}
