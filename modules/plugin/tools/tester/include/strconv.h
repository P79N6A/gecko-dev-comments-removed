




































#ifndef __STRCONV_H__
#define __STRCONV_H__

DWORD convertStringToLPSTR1(DWORD * pdw1);
DWORD convertStringToLPSTR2(DWORD * pdw1, DWORD * pdw2);
DWORD convertStringToLPSTR3(DWORD * pdw1, DWORD * pdw2, DWORD * pdw3);
void convertStringToDWORD1(DWORD * pdw1);
void convertStringToDWORD2(DWORD * pdw1, DWORD * pdw2);
void convertStringToDWORD3(DWORD * pdw1, DWORD * pdw2, DWORD * pdw3);
void convertStringToDWORD4(DWORD * pdw1, DWORD * pdw2, DWORD * pdw3, DWORD * pdw4);
DWORD convertStringToBOOL1(DWORD * pdw1);
DWORD convertStringToNPReason1(DWORD * pdw1);
DWORD convertStringToNPNVariable1(DWORD * pdw1);
DWORD convertStringToNPPVariable1(DWORD * pdw1);


#define fTNV1     0x00000001
#define fTNV2     0x00000010
#define fTNV3     0x00000100
#define fTNV4     0x00001000

#endif 
