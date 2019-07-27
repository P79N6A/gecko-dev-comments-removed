



#ifndef NfcGonkMessage_h
#define NfcGonkMessage_h

namespace mozilla {

#define NFCD_MAJOR_VERSION 1
#define NFCD_MINOR_VERSION 7

enum NfcRequest {
  eNfcRequest_Config = 0,
  eNfcRequest_Connect,
  eNfcRequest_Close,
  eNfcRequest_GetDetailsNDEF,
  eNfcRequest_ReadNDEF,
  eNfcRequest_WriteNDEF,
  eNfcRequest_MakeReadOnlyNDEF,
};

enum NfcResponse {
  eNfcResponse_General = 1000,
  eNfcResponse_Config,
  eNfcResponse_GetDetailsNDEF,
  eNfcResponse_ReadNDEF,
};

enum NfcNotification {
  eNfcNotification_Initialized = 2000,
  eNfcNotification_TechDiscovered,
  eNfcNotification_TechLost,
};

enum NfcTechlogy {
  eNfcTechlogy_NDEF = 0,
  eNfcTechlogy_NDEFWritable,
  eNfcTechlogy_NDEFFormattable,
  eNfcTechlogy_P2P,
};

} 

#endif 
