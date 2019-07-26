





#ifndef mozilla_dom_bluetooth_obexbase_h__
#define mozilla_dom_bluetooth_obexbase_h__

#include "BluetoothCommon.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

BEGIN_BLUETOOTH_NAMESPACE

const char FINAL_BIT = 0x80;




enum ObexHeaderId {
  Count = 0xC0,
  Name = 0x01,
  Type = 0x42,
  Length = 0xC3,
  TimeISO8601 = 0x44,
  Time4Byte = 0xC4,
  Description = 0x05,
  Target = 0x46,
  HTTP = 0x47,
  Body = 0x48,
  EndOfBody = 0x49,
  Who = 0x4A,
  ConnectionId = 0xCB,
  AppParameters = 0x4C,
  AuthChallenge =0x4D,
  AuthResponse = 0x4E,
  ObjectClass = 0x4F
};





enum ObexRequestCode {
  Connect = 0x80,
  Disconnect = 0x81,
  Put = 0x02,
  PutFinal = 0x82,
  Get = 0x03,
  GetFinal = 0x83,
  SetPath = 0x85,
  Abort = 0xFF
};




enum ObexResponseCode {
  Continue = 0x90,

  Success = 0xA0,
  Created = 0xA1,
  Accepted = 0xA2,
  NonAuthoritativeInfo = 0xA3,
  NoContent = 0xA4,
  ResetContent = 0xA5,
  PartialContent = 0xA6,

  MultipleChoices = 0xB0,
  MovedPermanently = 0xB1,
  MovedTemporarily = 0xB2,
  SeeOther = 0xB3,
  NotModified = 0xB4,
  UseProxy = 0xB5,

  BadRequest = 0xC0,
  Unauthorized = 0xC1,
  PaymentRequired = 0xC2,
  Forbidden = 0xC3,
  NotFound = 0xC4,
  MethodNotAllowed = 0xC5,
  NotAcceptable = 0xC6,
  ProxyAuthenticationRequired = 0xC7,
  RequestTimeOut = 0xC8,
  Conflict = 0xC9,
  Gone = 0xCA,
  LengthRequired = 0xCB,
  PreconditionFailed = 0xCC,
  RequestedEntityTooLarge = 0xCD,
  RequestUrlTooLarge = 0xCE,
  UnsupprotedMediaType = 0xCF,

  InternalServerError = 0xD0,
  NotImplemented = 0xD1,
  BadGateway = 0xD2,
  ServiceUnavailable = 0xD3,
  GatewayTimeout = 0xD4,
  HttpVersionNotSupported = 0xD5,

  DatabaseFull = 0xE0,
  DatabaseLocked = 0xE1,
};

class ObexHeader {
public:
  ObexHeader(ObexHeaderId aId, int aDataLength, uint8_t* aData)
    : mId(aId)
    , mDataLength(aDataLength)
    , mData(nullptr)
  {
    mData = new uint8_t[mDataLength];
    memcpy(mData, aData, aDataLength);
  }

  ~ObexHeader()
  {
  }

  ObexHeaderId mId;
  int mDataLength;
  nsAutoPtr<uint8_t> mData;
};

class ObexHeaderSet {
public:
  uint8_t mOpcode;
  nsTArray<nsAutoPtr<ObexHeader> > mHeaders;

  ObexHeaderSet(uint8_t aOpcode) : mOpcode(aOpcode)
  {
  }

  ~ObexHeaderSet()
  {
  }

  void AddHeader(ObexHeader* aHeader)
  {
    mHeaders.AppendElement(aHeader);
  }

  void GetName(nsString& aRetName)
  {
    aRetName.Truncate();

    int length = mHeaders.Length();

    for (int i = 0; i < length; ++i) {
      if (mHeaders[i]->mId == ObexHeaderId::Name) {
        uint8_t* ptr = mHeaders[i]->mData.get();
        int nameLength = mHeaders[i]->mDataLength / 2;

        for (int j = 0; j < nameLength; ++j) {
          PRUnichar c = ((((uint32_t)ptr[j * 2]) << 8) | ptr[j * 2 + 1]);
          aRetName += c;
        }

        break;
      }
    }
  }

  void GetContentType(nsString& aRetContentType)
  {
    aRetContentType.Truncate();

    int length = mHeaders.Length();

    for (int i = 0; i < length; ++i) {
      if (mHeaders[i]->mId == ObexHeaderId::Type) {
        uint8_t* ptr = mHeaders[i]->mData.get();
        aRetContentType.AssignASCII((const char*)ptr);
        break;
      }
    }
  }

  
  void GetLength(uint32_t* aRetLength)
  {
    int length = mHeaders.Length();
    *aRetLength = 0;

    for (int i = 0; i < length; ++i) {
      if (mHeaders[i]->mId == ObexHeaderId::Length) {
        uint8_t* ptr = mHeaders[i]->mData.get();
        *aRetLength = ((uint32_t)ptr[0] << 24) |
                      ((uint32_t)ptr[1] << 16) |
                      ((uint32_t)ptr[2] << 8) |
                      ((uint32_t)ptr[3]);
        return;
      }
    }
  }

  void GetBodyLength(int* aRetBodyLength)
  {
    int length = mHeaders.Length();
    *aRetBodyLength = 0;

    for (int i = 0; i < length; ++i) {
      if (mHeaders[i]->mId == ObexHeaderId::Body ||
          mHeaders[i]->mId == ObexHeaderId::EndOfBody) {
        *aRetBodyLength = mHeaders[i]->mDataLength;
        return;
      }
    }
  }

  void GetBody(uint8_t** aRetBody)
  {
    int length = mHeaders.Length();
    *aRetBody = nullptr;

    for (int i = 0; i < length; ++i) {
      if (mHeaders[i]->mId == ObexHeaderId::Body ||
          mHeaders[i]->mId == ObexHeaderId::EndOfBody) {
        uint8_t* ptr = mHeaders[i]->mData.get();
        *aRetBody = new uint8_t[mHeaders[i]->mDataLength];
        memcpy(*aRetBody, ptr, mHeaders[i]->mDataLength);
        return;
      }
    }
  }

  bool Has(ObexHeaderId aId)
  {
    int length = mHeaders.Length();
    for (int i = 0; i < length; ++i) {
      if (mHeaders[i]->mId == aId) {
        return true;
      }
    }

    return false;
  }
};

int AppendHeaderName(uint8_t* aRetBuf, const char* aName, int aLength);
int AppendHeaderBody(uint8_t* aRetBuf, uint8_t* aData, int aLength);
int AppendHeaderLength(uint8_t* aRetBuf, int aObjectLength);
int AppendHeaderConnectionId(uint8_t* aRetBuf, int aConnectionId);
void SetObexPacketInfo(uint8_t* aRetBuf, uint8_t aOpcode, int aPacketLength);
void ParseHeaders(const uint8_t* aHeaderStart,
                  int aTotalLength,
                  ObexHeaderSet* aRetHanderSet);

END_BLUETOOTH_NAMESPACE

#endif
