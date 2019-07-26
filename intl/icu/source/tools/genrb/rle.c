















#include "rle.h"




static const uint16_t ESCAPE = 0xA5A5;





static const uint8_t ESCAPE_BYTE = (uint8_t)0xA5;









static uint16_t*
appendEncodedByte(uint16_t* buffer, uint16_t* buffLimit, uint8_t value, uint8_t state[],UErrorCode* status) {
    if(!status || U_FAILURE(*status)){
        return NULL;
    }
    if (state[0] != 0) {
        uint16_t c = (uint16_t) ((state[1] << 8) | (((int32_t) value) & 0xFF));
        if(buffer < buffLimit){
            *buffer++ = c;
        }else{
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
        state[0] = 0;
        return buffer;
    }
    else {
        state[0] = 1;
        state[1] = value;
        return buffer;
    }
}




static uint16_t*
encodeRunByte(uint16_t* buffer,uint16_t* bufLimit, uint8_t value, int32_t length, uint8_t state[], UErrorCode* status) {
    if(!status || U_FAILURE(*status)){
        return NULL;
    }
    if (length < 4) {
        int32_t j=0;
        for (; j<length; ++j) {
            if (value == ESCAPE_BYTE) {
                buffer = appendEncodedByte(buffer,bufLimit, ESCAPE_BYTE, state,status);
            }
            buffer = appendEncodedByte(buffer,bufLimit, value, state, status);
        }
    }
    else {
        if (length == ESCAPE_BYTE) {
            if (value == ESCAPE_BYTE){
               buffer =  appendEncodedByte(buffer, bufLimit,ESCAPE_BYTE, state,status);
            }
            buffer = appendEncodedByte(buffer,bufLimit, value, state, status);
            --length;
        }
        buffer = appendEncodedByte(buffer,bufLimit, ESCAPE_BYTE, state,status);
        buffer = appendEncodedByte(buffer,bufLimit, (char)length, state, status);
        buffer = appendEncodedByte(buffer,bufLimit, value, state, status); 
    }
    return buffer;
}

#define APPEND( buffer, bufLimit, value, num, status){  \
    if(buffer<bufLimit){                    \
        *buffer++=(value);                  \
    }else{                                  \
        *status = U_BUFFER_OVERFLOW_ERROR;  \
    }                                       \
    num++;                                  \
}





static uint16_t*
encodeRunShort(uint16_t* buffer,uint16_t* bufLimit, uint16_t value, int32_t length,UErrorCode* status) {
    int32_t num=0;
    if (length < 4) {
        int j=0;
        for (; j<length; ++j) {
            if (value == (int32_t) ESCAPE){
                APPEND(buffer,bufLimit,ESCAPE, num, status);

            }
            APPEND(buffer,bufLimit,value,num, status);
        }
    }
    else {
        if (length == (int32_t) ESCAPE) {
            if (value == (int32_t) ESCAPE){
                APPEND(buffer,bufLimit,ESCAPE,num,status);

            }
            APPEND(buffer,bufLimit,value,num,status);
            --length;
        }
        APPEND(buffer,bufLimit,ESCAPE,num,status);
        APPEND(buffer,bufLimit,(uint16_t) length, num,status);
        APPEND(buffer,bufLimit,(uint16_t)value, num, status); 
    }
    return buffer;
}














int32_t
usArrayToRLEString(const uint16_t* src,int32_t srcLen,uint16_t* buffer, int32_t bufLen,UErrorCode* status) {
    uint16_t* bufLimit =  buffer+bufLen;
    uint16_t* saveBuffer = buffer;
    if(buffer < bufLimit){
        *buffer++ =  (uint16_t)(srcLen>>16);
        if(buffer<bufLimit){
            uint16_t runValue = src[0];
            int32_t runLength = 1;
            int i=1;
            *buffer++ = (uint16_t) srcLen;

            for (; i<srcLen; ++i) {
                uint16_t s = src[i];
                if (s == runValue && runLength < 0xFFFF){
                    ++runLength;
                }else {
                    buffer = encodeRunShort(buffer,bufLimit, (uint16_t)runValue, runLength,status);
                    runValue = s;
                    runLength = 1;
                }
            }
            buffer= encodeRunShort(buffer,bufLimit,(uint16_t)runValue, runLength,status);
        }else{
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
    }else{
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    return (int32_t)(buffer - saveBuffer);
}















int32_t
byteArrayToRLEString(const uint8_t* src,int32_t srcLen, uint16_t* buffer,int32_t bufLen, UErrorCode* status) {
    const uint16_t* saveBuf = buffer;
    uint16_t* bufLimit =  buffer+bufLen;
    if(buffer < bufLimit){
        *buffer++ = ((uint16_t) (srcLen >> 16));

        if(buffer<bufLimit){
            uint8_t runValue = src[0];
            int runLength = 1;
            uint8_t state[2]= {0};
            int i=1;
            *buffer++=((uint16_t) srcLen);
            for (; i<srcLen; ++i) {
                uint8_t b = src[i];
                if (b == runValue && runLength < 0xFF){
                    ++runLength;
                }
                else {
                    buffer = encodeRunByte(buffer, bufLimit,runValue, runLength, state,status);
                    runValue = b;
                    runLength = 1;
                }
            }
            buffer = encodeRunByte(buffer,bufLimit, runValue, runLength, state, status);

            


            if (state[0] != 0) {
                buffer = appendEncodedByte(buffer,bufLimit, 0, state ,status);
            }
        }else{
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
    }else{
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    return (int32_t) (buffer - saveBuf);
}





int32_t
rleStringToUCharArray(uint16_t* src, int32_t srcLen, uint16_t* target, int32_t tgtLen, UErrorCode* status) {
    int32_t length = 0;
    int32_t ai = 0;
    int i=2;

    if(!status || U_FAILURE(*status)){
        return 0;
    }
    
    if(srcLen == -1){
        srcLen = u_strlen(src);
    }
    if(srcLen <= 2){
        return 2;
    }
    length = (((int32_t) src[0]) << 16) | ((int32_t) src[1]);

    if(target == NULL){
        return length;
    }
    if(tgtLen < length){
        *status = U_BUFFER_OVERFLOW_ERROR;
        return length;
    }

    for (; i<srcLen; ++i) {
        uint16_t c = src[i];
        if (c == ESCAPE) {
            c = src[++i];
            if (c == ESCAPE) {
                target[ai++] = c;
            } else {
                int32_t runLength = (int32_t) c;
                uint16_t runValue = src[++i];
                int j=0;
                for (; j<runLength; ++j) {
                    target[ai++] = runValue;
                }
            }
        }
        else {
            target[ai++] = c;
        }
    }

    if (ai != length){
        *status = U_INTERNAL_PROGRAM_ERROR;
    }

    return length;
}




int32_t
rleStringToByteArray(uint16_t* src, int32_t srcLen, uint8_t* target, int32_t tgtLen, UErrorCode* status) {

    int32_t length = 0;
    UBool nextChar = TRUE;
    uint16_t c = 0;
    int32_t node = 0;
    int32_t runLength = 0;
    int32_t i = 2;
    int32_t ai=0;

    if(!status || U_FAILURE(*status)){
        return 0;
    }
    
    if(srcLen == -1){
        srcLen = u_strlen(src);
    }
    if(srcLen <= 2){
        return 2;
    }
    length = (((int32_t) src[0]) << 16) | ((int32_t) src[1]);

    if(target == NULL){
        return length;
    }
    if(tgtLen < length){
        *status = U_BUFFER_OVERFLOW_ERROR;
        return length;
    }

    for (; ai<tgtLen; ) {
       




        uint8_t b;
        if (nextChar) {
            c = src[i++];
            b = (uint8_t) (c >> 8);
            nextChar = FALSE;
        }
        else {
            b = (uint8_t) (c & 0xFF);
            nextChar = TRUE;
        }

       




        switch (node) {
        case 0:
            
            if (b == ESCAPE_BYTE) {
                node = 1;
            }
            else {
                target[ai++] = b;
            }
            break;
        case 1:
           


            if (b == ESCAPE_BYTE) {
                target[ai++] = ESCAPE_BYTE;
                node = 0;
            }
            else {
                runLength = b;
                node = 2;
            }
            break;
        case 2:
            {
                int j=0;
               


                for (; j<runLength; ++j){
                    if(ai<tgtLen){
                        target[ai++] = b;
                    }else{
                        *status = U_BUFFER_OVERFLOW_ERROR;
                        return ai;
                    }
                }
                node = 0;
                break;
            }
        }
    }

    if (node != 0){
        *status = U_INTERNAL_PROGRAM_ERROR;
        
        return 0;
    }


    if (i != srcLen){
        
        *status = U_INTERNAL_PROGRAM_ERROR;
        return ai;
    }

    return ai;
}

