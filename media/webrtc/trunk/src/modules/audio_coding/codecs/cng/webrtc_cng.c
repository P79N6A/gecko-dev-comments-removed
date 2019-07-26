










#include <string.h>
#include <stdlib.h>

#include "webrtc_cng.h"
#include "signal_processing_library.h"
#include "cng_helpfuns.h"
#include "stdio.h"


typedef struct WebRtcCngDecInst_t_ {

    WebRtc_UWord32 dec_seed;
    WebRtc_Word32 dec_target_energy;
    WebRtc_Word32 dec_used_energy;
    WebRtc_Word16 dec_target_reflCoefs[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 dec_used_reflCoefs[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 dec_filtstate[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 dec_filtstateLow[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 dec_Efiltstate[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 dec_EfiltstateLow[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 dec_order;
    WebRtc_Word16 dec_target_scale_factor; 
    WebRtc_Word16 dec_used_scale_factor;  
    WebRtc_Word16 target_scale_factor; 
    WebRtc_Word16 errorcode;
    WebRtc_Word16 initflag; 

} WebRtcCngDecInst_t;


typedef struct WebRtcCngEncInst_t_ {

    WebRtc_Word16 enc_nrOfCoefs;
    WebRtc_Word16 enc_sampfreq;
    WebRtc_Word16 enc_interval;
    WebRtc_Word16 enc_msSinceSID;
    WebRtc_Word32 enc_Energy;
    WebRtc_Word16 enc_reflCoefs[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word32 enc_corrVector[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 enc_filtstate[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 enc_filtstateLow[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_UWord32 enc_seed;    
    WebRtc_Word16 errorcode;
    WebRtc_Word16 initflag;

} WebRtcCngEncInst_t;

const WebRtc_Word32 WebRtcCng_kDbov[94]={
    1081109975,  858756178,  682134279,  541838517,  430397633,  341876992,
    271562548,  215709799,  171344384,  136103682,  108110997,   85875618,
    68213428,   54183852,   43039763,   34187699,   27156255,   21570980,
    17134438,   13610368,   10811100,    8587562,    6821343,    5418385,
    4303976,    3418770,    2715625,    2157098,    1713444,    1361037,
    1081110,     858756,     682134,     541839,     430398,     341877,
    271563,     215710,     171344,     136104,     108111,      85876,
    68213,      54184,      43040,      34188,      27156,      21571,
    17134,      13610,      10811,       8588,       6821,       5418,
    4304,       3419,       2716,       2157,       1713,       1361,
    1081,        859,        682,        542,        430,        342,
    272,        216,        171,        136,        108,         86, 
    68,         54,         43,         34,         27,         22, 
    17,         14,         11,          9,          7,          5, 
    4,          3,          3,          2,          2,           1, 
    1,          1,          1,          1
};
const WebRtc_Word16 WebRtcCng_kCorrWindow[WEBRTC_CNG_MAX_LPC_ORDER] = {
    32702, 32636, 32570, 32505, 32439, 32374, 
    32309, 32244, 32179, 32114, 32049, 31985
}; 














WebRtc_Word16 WebRtcCng_Version(char *version)
{
    strcpy((char*)version,(const char*)"1.2.0\n");
    return(0);
}














WebRtc_Word16 WebRtcCng_AssignSizeEnc(int *sizeinbytes)
{
    *sizeinbytes=sizeof(WebRtcCngEncInst_t)*2/sizeof(WebRtc_Word16);
    return(0);
}

WebRtc_Word16 WebRtcCng_AssignSizeDec(int *sizeinbytes)
{
    *sizeinbytes=sizeof(WebRtcCngDecInst_t)*2/sizeof(WebRtc_Word16);
    return(0);
}
















WebRtc_Word16 WebRtcCng_AssignEnc(CNG_enc_inst **inst, void *CNG_inst_Addr)
{
    if (CNG_inst_Addr!=NULL) {
        *inst = (CNG_enc_inst*)CNG_inst_Addr;
        (*(WebRtcCngEncInst_t**) inst)->errorcode = 0;
        (*(WebRtcCngEncInst_t**) inst)->initflag = 0;
        return(0);
    } else {
        
        return(-1);
    }
}

WebRtc_Word16 WebRtcCng_AssignDec(CNG_dec_inst **inst, void *CNG_inst_Addr)
{
    if (CNG_inst_Addr!=NULL) {
        *inst = (CNG_dec_inst*)CNG_inst_Addr;
        (*(WebRtcCngDecInst_t**) inst)->errorcode = 0;
        (*(WebRtcCngDecInst_t**) inst)->initflag = 0;
        return(0);
    } else {
        
        return(-1);
    }
}














WebRtc_Word16 WebRtcCng_CreateEnc(CNG_enc_inst **cng_inst)
{
    *cng_inst=(CNG_enc_inst*)malloc(sizeof(WebRtcCngEncInst_t));
    if(*cng_inst!=NULL) {
        (*(WebRtcCngEncInst_t**) cng_inst)->errorcode = 0;
        (*(WebRtcCngEncInst_t**) cng_inst)->initflag = 0;
        return(0);
    }
    else {
        
        return(-1);
    }
}

WebRtc_Word16 WebRtcCng_CreateDec(CNG_dec_inst **cng_inst)
{
    *cng_inst=(CNG_dec_inst*)malloc(sizeof(WebRtcCngDecInst_t));
    if(*cng_inst!=NULL) {
        (*(WebRtcCngDecInst_t**) cng_inst)->errorcode = 0;
        (*(WebRtcCngDecInst_t**) cng_inst)->initflag = 0;
        return(0);
    }
    else {
        
        return(-1);
    }
}






















WebRtc_Word16 WebRtcCng_InitEnc(CNG_enc_inst *cng_inst,
                                WebRtc_Word16 fs,
                                WebRtc_Word16 interval,
                                WebRtc_Word16 quality)
{
    int i;

    WebRtcCngEncInst_t* inst=(WebRtcCngEncInst_t*)cng_inst;

    memset(inst, 0, sizeof(WebRtcCngEncInst_t));

     

    if (quality>WEBRTC_CNG_MAX_LPC_ORDER) {
        inst->errorcode = CNG_DISALLOWED_LPC_ORDER;
        return (-1);
    }

    if (fs<=0) {
        inst->errorcode = CNG_DISALLOWED_SAMPLING_FREQUENCY;
        return (-1);
    }

    inst->enc_sampfreq=fs;
    inst->enc_interval=interval;
    inst->enc_nrOfCoefs=quality;
    inst->enc_msSinceSID=0;
    inst->enc_seed=7777; 
    inst->enc_Energy=0;
    for(i=0;i<(WEBRTC_CNG_MAX_LPC_ORDER+1);i++){
        inst->enc_reflCoefs[i]=0;
        inst->enc_corrVector[i]=0;
    }
    inst->initflag=1;

    return(0);
}

WebRtc_Word16 WebRtcCng_InitDec(CNG_dec_inst *cng_inst)
{
    int i;

    WebRtcCngDecInst_t* inst=(WebRtcCngDecInst_t*)cng_inst;

    memset(inst, 0, sizeof(WebRtcCngDecInst_t));
    inst->dec_seed=7777; 
    inst->dec_order=5;
    inst->dec_target_scale_factor=0;
    inst->dec_used_scale_factor=0;
    for(i=0;i<(WEBRTC_CNG_MAX_LPC_ORDER+1);i++){
        inst->dec_filtstate[i]=0;
        inst->dec_target_reflCoefs[i]=0;
        inst->dec_used_reflCoefs[i]=0;
    }
    inst->dec_target_reflCoefs[0]=0;
    inst->dec_used_reflCoefs[0]=0;
    inst ->dec_used_energy=0;
    inst->initflag=1;

    return(0);
}














WebRtc_Word16 WebRtcCng_FreeEnc(CNG_enc_inst *cng_inst)
{
    free(cng_inst);
    return(0);
}

WebRtc_Word16 WebRtcCng_FreeDec(CNG_dec_inst *cng_inst)
{
    free(cng_inst);
    return(0);
}

















WebRtc_Word16 WebRtcCng_Encode(CNG_enc_inst *cng_inst, 
                               WebRtc_Word16 *speech,
                               WebRtc_Word16 nrOfSamples,
                               WebRtc_UWord8* SIDdata,
                               WebRtc_Word16* bytesOut,
                               WebRtc_Word16 forceSID)
{
    WebRtcCngEncInst_t* inst=(WebRtcCngEncInst_t*)cng_inst;

    WebRtc_Word16 arCoefs[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word32 corrVector[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 refCs[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 hanningW[WEBRTC_CNG_MAX_OUTSIZE_ORDER];
    WebRtc_Word16 ReflBeta=19661; 
    WebRtc_Word16 ReflBetaComp=13107;  
    WebRtc_Word32 outEnergy;
    int outShifts;
    int i, stab;
    int acorrScale;
    int index;
    WebRtc_Word16 ind,factor;
    WebRtc_Word32 *bptr, blo, bhi;
    WebRtc_Word16 negate;
    const WebRtc_Word16 *aptr;

    WebRtc_Word16 speechBuf[WEBRTC_CNG_MAX_OUTSIZE_ORDER];


        
    if (inst->initflag != 1) {
        inst->errorcode = CNG_ENCODER_NOT_INITIATED;
        return (-1);
    }


        
    if (nrOfSamples>WEBRTC_CNG_MAX_OUTSIZE_ORDER) {
        inst->errorcode = CNG_DISALLOWED_FRAME_SIZE;
        return (-1);
    }


    for(i=0;i<nrOfSamples;i++){
        speechBuf[i]=speech[i];
    }

    factor=nrOfSamples;

    
    outEnergy =WebRtcSpl_Energy(speechBuf, nrOfSamples, &outShifts);
    while(outShifts>0){
        if(outShifts>5){ 
            outEnergy<<=(outShifts-5);
            outShifts=5;
        }
        else{
            factor/=2;
            outShifts--;
        }
    }
    outEnergy=WebRtcSpl_DivW32W16(outEnergy,factor);

    if (outEnergy > 1){
        
        WebRtcSpl_GetHanningWindow(hanningW, nrOfSamples/2);
        for( i=0;i<(nrOfSamples/2);i++ )
            hanningW[nrOfSamples-i-1]=hanningW[i];

        WebRtcSpl_ElementwiseVectorMult(speechBuf, hanningW, speechBuf, nrOfSamples, 14);

        WebRtcSpl_AutoCorrelation( speechBuf, nrOfSamples, inst->enc_nrOfCoefs, corrVector, &acorrScale );

        if( *corrVector==0 )
            *corrVector = WEBRTC_SPL_WORD16_MAX;

        
        aptr = WebRtcCng_kCorrWindow;
        bptr = corrVector;

        
        for( ind=0; ind<inst->enc_nrOfCoefs; ind++ )
        {
            
            
                 
            negate = *bptr<0;
            if( negate )
                *bptr = -*bptr;

            blo = (WebRtc_Word32)*aptr * (*bptr & 0xffff);
            bhi = ((blo >> 16) & 0xffff) + ((WebRtc_Word32)(*aptr++) * ((*bptr >> 16) & 0xffff));
            blo = (blo & 0xffff) | ((bhi & 0xffff) << 16);

            *bptr = (( (bhi>>16) & 0x7fff) << 17) | ((WebRtc_UWord32)blo >> 15);
            if( negate )
                *bptr = -*bptr;
            bptr++;
        }

        

        stab=WebRtcSpl_LevinsonDurbin(corrVector, arCoefs, refCs, inst->enc_nrOfCoefs);
        
        if(!stab){
            
            *bytesOut=0;
            return(0);
        }

    }
    else {
        for(i=0;i<inst->enc_nrOfCoefs; i++)
            refCs[i]=0;
    }

    if(forceSID){
        
        for(i=0;i<inst->enc_nrOfCoefs;i++)
            inst->enc_reflCoefs[i]=refCs[i];
        inst->enc_Energy=outEnergy;
    }
    else{
        
        for(i=0;i<(inst->enc_nrOfCoefs);i++){
            inst->enc_reflCoefs[i]=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(inst->enc_reflCoefs[i],ReflBeta,15);
            inst->enc_reflCoefs[i]+=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(refCs[i],ReflBetaComp,15);
        }
        inst->enc_Energy=(outEnergy>>2)+(inst->enc_Energy>>1)+(inst->enc_Energy>>2);
    }


    if(inst->enc_Energy<1){
        inst->enc_Energy=1;
    }

    if((inst->enc_msSinceSID>(inst->enc_interval-1))||forceSID){

        
        index=0;
        for(i=1;i<93;i++){
            
            if((inst->enc_Energy-WebRtcCng_kDbov[i])>0){
                index=i;
                break;
            }
        }
        if((i==93)&&(index==0))
            index=94;
        SIDdata[0]=index;


        
        if(inst->enc_nrOfCoefs==WEBRTC_CNG_MAX_LPC_ORDER){ 
            for(i=0;i<inst->enc_nrOfCoefs;i++){
                SIDdata[i+1]=((inst->enc_reflCoefs[i]+128)>>8);  
            }
        }else{
            for(i=0;i<inst->enc_nrOfCoefs;i++){
                SIDdata[i+1]=(127+((inst->enc_reflCoefs[i]+128)>>8));  
            }
        }

        inst->enc_msSinceSID=0;
        *bytesOut=inst->enc_nrOfCoefs+1;

        inst->enc_msSinceSID+=(1000*nrOfSamples)/inst->enc_sampfreq;
        return(inst->enc_nrOfCoefs+1);
    }else{
        inst->enc_msSinceSID+=(1000*nrOfSamples)/inst->enc_sampfreq;
        *bytesOut=0;
    return(0);
    }
}
















WebRtc_Word16 WebRtcCng_UpdateSid(CNG_dec_inst *cng_inst,
                                  WebRtc_UWord8 *SID,
                                  WebRtc_Word16 length)
{

    WebRtcCngDecInst_t* inst=(WebRtcCngDecInst_t*)cng_inst;
    WebRtc_Word16 refCs[WEBRTC_CNG_MAX_LPC_ORDER];
    WebRtc_Word32 targetEnergy;
    int i;

    if (inst->initflag != 1) {
        inst->errorcode = CNG_DECODER_NOT_INITIATED;
        return (-1);
    }

    
    if(length> (WEBRTC_CNG_MAX_LPC_ORDER+1))
        length=WEBRTC_CNG_MAX_LPC_ORDER+1;

    inst->dec_order=length-1;

    if(SID[0]>93)
        SID[0]=93;
    targetEnergy=WebRtcCng_kDbov[SID[0]];
    
    targetEnergy=targetEnergy>>1;
    targetEnergy+=targetEnergy>>2;

    inst->dec_target_energy=targetEnergy;

    
    if(inst->dec_order==WEBRTC_CNG_MAX_LPC_ORDER){ 
        for(i=0;i<(inst->dec_order);i++){
            refCs[i]=SID[i+1]<<8; 
            inst->dec_target_reflCoefs[i]=refCs[i];
        }
    }else{
        for(i=0;i<(inst->dec_order);i++){
            refCs[i]=(SID[i+1]-127)<<8; 
            inst->dec_target_reflCoefs[i]=refCs[i];
        }
    }
    
    for(i=(inst->dec_order);i<WEBRTC_CNG_MAX_LPC_ORDER;i++){
            refCs[i]=0; 
            inst->dec_target_reflCoefs[i]=refCs[i];
        }

    return(0);
}















WebRtc_Word16 WebRtcCng_Generate(CNG_dec_inst *cng_inst,
                                 WebRtc_Word16 *outData,
                                 WebRtc_Word16 nrOfSamples,
                                 WebRtc_Word16 new_period)
{
    WebRtcCngDecInst_t* inst=(WebRtcCngDecInst_t*)cng_inst;
    
    int i;
    WebRtc_Word16 excitation[WEBRTC_CNG_MAX_OUTSIZE_ORDER];
    WebRtc_Word16 low[WEBRTC_CNG_MAX_OUTSIZE_ORDER];
    WebRtc_Word16 lpPoly[WEBRTC_CNG_MAX_LPC_ORDER+1];
    WebRtc_Word16 ReflBetaStd=26214; 
    WebRtc_Word16 ReflBetaCompStd=6553; 
    WebRtc_Word16 ReflBetaNewP=19661; 
    WebRtc_Word16 ReflBetaCompNewP=13107; 
    WebRtc_Word16 Beta,BetaC, tmp1, tmp2, tmp3;
    WebRtc_Word32 targetEnergy;
    WebRtc_Word16 En;
    WebRtc_Word16 temp16;

    if (nrOfSamples>WEBRTC_CNG_MAX_OUTSIZE_ORDER) {
        inst->errorcode = CNG_DISALLOWED_FRAME_SIZE;
        return (-1);
    }


    if (new_period) {
        inst->dec_used_scale_factor=inst->dec_target_scale_factor;
        Beta=ReflBetaNewP;
        BetaC=ReflBetaCompNewP;
    } else {
        Beta=ReflBetaStd;
        BetaC=ReflBetaCompStd;
    }

    
    tmp1=inst->dec_used_scale_factor<<2; 
    tmp2=inst->dec_target_scale_factor<<2; 
    tmp3=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(tmp1,Beta,15);
    tmp3+=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(tmp2,BetaC,15);
    inst->dec_used_scale_factor=tmp3>>2; 

    inst->dec_used_energy=inst->dec_used_energy>>1;
    inst->dec_used_energy+=inst->dec_target_energy>>1;

    
    
    for (i=0;i<WEBRTC_CNG_MAX_LPC_ORDER;i++) {
        inst->dec_used_reflCoefs[i]=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(inst->dec_used_reflCoefs[i],Beta,15);
        inst->dec_used_reflCoefs[i]+=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(inst->dec_target_reflCoefs[i],BetaC,15);        
    }

    
    WebRtcCng_K2a16(inst->dec_used_reflCoefs, WEBRTC_CNG_MAX_LPC_ORDER, lpPoly);

     

    targetEnergy=inst->dec_used_energy;

    
    En=8192; 
    for (i=0; i<(WEBRTC_CNG_MAX_LPC_ORDER); i++) {

        
        

        
        temp16=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(inst->dec_used_reflCoefs[i],inst->dec_used_reflCoefs[i],15); 
        temp16=0x7fff - temp16; 
        En=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(En,temp16,15);

    }

    

    

    targetEnergy=WebRtcSpl_Sqrt(inst->dec_used_energy);

    En=(WebRtc_Word16)WebRtcSpl_Sqrt(En)<<6; 
    En=(En*3)>>1; 

    inst->dec_used_scale_factor=(WebRtc_Word16)((En*targetEnergy)>>12);


    

    
    
    for(i=0;i<nrOfSamples;i++){
        excitation[i]=WebRtcSpl_RandN(&inst->dec_seed)>>1;
    }

    
    WebRtcSpl_ScaleVector(excitation, excitation, inst->dec_used_scale_factor, nrOfSamples, 13);

    WebRtcSpl_FilterAR(
        lpPoly,    
        WEBRTC_CNG_MAX_LPC_ORDER+1, 
        excitation,            
        nrOfSamples, 
        inst->dec_filtstate,        
        WEBRTC_CNG_MAX_LPC_ORDER, 
        inst->dec_filtstateLow,        
        WEBRTC_CNG_MAX_LPC_ORDER, 
        outData,    
        low,
        nrOfSamples
    );

    return(0);

}

















WebRtc_Word16 WebRtcCng_GetErrorCodeEnc(CNG_enc_inst *cng_inst)
{

    
    WebRtcCngEncInst_t* inst=(WebRtcCngEncInst_t*)cng_inst;

    return inst->errorcode;
}

WebRtc_Word16 WebRtcCng_GetErrorCodeDec(CNG_dec_inst *cng_inst)
{

    
    WebRtcCngDecInst_t* inst=(WebRtcCngDecInst_t*)cng_inst;

    return inst->errorcode;
}
