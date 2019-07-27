



#ifndef NfcGonkMessage_h
#define NfcGonkMessage_h

namespace mozilla {

#define NFCD_MAJOR_VERSION 1
#define NFCD_MINOR_VERSION 22

enum NfcTechlogy {
  NDEF = 0,
  NDEFWritable,
  NDEFFormattable,
  P2P,
};

enum NfcErrorCode {
  Success = 0,
  IOErr = 1,
  Timeout = 2,
  BusyErr = 3,
  ConnectErr = 4,
  DisconnectErr = 5,
  ReadErr = 6,
  WriteErr = 7,
  InvalidParam = 8,
  InsufficientResources = 9,
  SocketCreation = 10,
  FailEnableDiscovery = 11,
  FailDisableDiscovery = 12,
  NotInitialized = 13,
  InitializeFail = 14,
  DeinitializeFail = 15,
  NotSupported = 16,
  FailEnableLowPowerMode = 17,
  FailDisableLowPowerMode = 18,
};

} 

#endif 
