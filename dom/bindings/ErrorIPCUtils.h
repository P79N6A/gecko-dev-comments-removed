





#include "ipc/IPCMessageUtils.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/Assertions.h"
#include "mozilla/Move.h"

#ifndef IPC_ErrorIPCUtils_h
#define IPC_ErrorIPCUtils_h

namespace IPC {

template<>
struct ParamTraits<mozilla::dom::ErrNum> :
  public ContiguousEnumSerializer<mozilla::dom::ErrNum,
                                  mozilla::dom::ErrNum(0),
                                  mozilla::dom::ErrNum(mozilla::dom::Err_Limit)> {};

template<>
struct ParamTraits<mozilla::ErrorResult>
{
  typedef mozilla::ErrorResult paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    
    
    
    
    MOZ_ASSERT_IF(aParam.IsJSException(), aParam.mMightHaveUnreportedJSException);
    if (aParam.IsJSException()
#ifdef DEBUG
        || aParam.mMightHaveUnreportedJSException
#endif
        ) {
      MOZ_CRASH("Cannot encode an ErrorResult representing a Javascript exception");
    }

    WriteParam(aMsg, aParam.mResult);
    WriteParam(aMsg, aParam.IsErrorWithMessage());
    if (aParam.IsErrorWithMessage()) {
      aParam.SerializeMessage(aMsg);
    }
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    paramType readValue;
    if (!ReadParam(aMsg, aIter, &readValue.mResult)) {
      return false;
    }
    bool hasMessage = false;
    if (!ReadParam(aMsg, aIter, &hasMessage)) {
      return false;
    }
    if (hasMessage && !readValue.DeserializeMessage(aMsg, aIter)) {
      return false;
    }
    *aResult = Move(readValue);
    return true;
  }
};

}

#endif
