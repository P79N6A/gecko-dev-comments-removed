



































#ifndef __IPC_GLUE_IPCMESSAGEUTILS_H__
#define __IPC_GLUE_IPCMESSAGEUTILS_H__

#include "chrome/common/ipc_message_utils.h"

#include "prtypes.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

#include "IPCMessageStart.h"

COMPILE_ASSERT(LastMsgIndex <= 16, need_to_update_IPC_MESSAGE_MACRO);

namespace IPC {

template <>
struct ParamTraits<nsACString>
{
  typedef nsACString paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    PRUint32 length = aParam.Length();
    WriteParam(aMsg, length);
    aMsg->WriteBytes(aParam.BeginReading(), length);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    PRUint32 length;
    if (ReadParam(aMsg, aIter, &length)) {
      const char* buf;
      if (aMsg->ReadBytes(aIter, &buf, length)) {
        aResult->Assign(buf, length);
        return true;
      }
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(UTF8ToWide(aParam.BeginReading()));
  }
};

template <>
struct ParamTraits<nsAString>
{
  typedef nsAString paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    PRUint32 length = aParam.Length();
    WriteParam(aMsg, length);
    aMsg->WriteBytes(aParam.BeginReading(), length * sizeof(PRUnichar));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    PRUint32 length;
    if (ReadParam(aMsg, aIter, &length)) {
      const PRUnichar* buf;
      if (aMsg->ReadBytes(aIter, reinterpret_cast<const char**>(&buf),
                       length * sizeof(PRUnichar))) {
        aResult->Assign(buf, length);
        return true;
      }
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
#ifdef WCHAR_T_IS_UTF16
    aLog->append(reinterpret_cast<const wchar_t*>(aParam.BeginReading()));
#else
    PRUint32 length = aParam.Length();
    for (PRUint32 index = 0; index < length; index++) {
      aLog->push_back(std::wstring::value_type(aParam[index]));
    }
#endif
  }
};

template <>
struct ParamTraits<nsCString> : ParamTraits<nsACString>
{
  typedef nsCString paramType;
};

template <>
struct ParamTraits<nsString> : ParamTraits<nsAString>
{
  typedef nsString paramType;
};

template <typename E>
struct ParamTraits<nsTArray<E> >
{
  typedef nsTArray<E> paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    PRUint32 length = aParam.Length();
    WriteParam(aMsg, length);
    for (PRUint32 index = 0; index < length; index++) {
      WriteParam(aMsg, aParam[index]);
    }
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    PRUint32 length;
    if (!ReadParam(aMsg, aIter, &length)) {
      return false;
    }

    
    
    
    
    
    
    if (aMsg->IteratorHasRoomFor(*aIter, length * sizeof(E))) {
      if (!aResult->SetLength(length)) {
        return false;
      }
      for (PRUint32 index = 0; index < length; index++) {
        E& element = aResult->ElementAt(index);
        if (!ReadParam(aMsg, aIter, &element)) {
          return false;
        }
      }
    }
    else {
      for (PRUint32 index = 0; index < length; index++) {
        E* element = aResult->AppendElement();
        if (!(element && ReadParam(aMsg, aIter, element))) {
          return false;
        }
      }
    }

    return true;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    for (PRUint32 index = 0; index < aParam.Length(); index++) {
      if (index) {
        aLog->append(L" ");
      }
      LogParam(aParam[index], aLog);
    }
  }
};

} 

#endif 
