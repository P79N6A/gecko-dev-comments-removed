





































#ifndef __IPC_GLUE_IPCMESSAGEUTILS_H__
#define __IPC_GLUE_IPCMESSAGEUTILS_H__

#include "chrome/common/ipc_message_utils.h"

#include "prtypes.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "gfxMatrix.h"

#ifdef _MSC_VER
#pragma warning( disable : 4800 )
#endif

namespace IPC {

template<>
struct ParamTraits<PRInt8>
{
  typedef PRInt8 paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    aMsg->WriteBytes(&aParam, sizeof(aParam));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    const char* outp;
    if (!aMsg->ReadBytes(aIter, &outp, sizeof(*aResult)))
      return false;

    *aResult = *reinterpret_cast<const paramType*>(outp);
    return true;
  }
};

template<>
struct ParamTraits<PRUint8>
{
  typedef PRUint8 paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    aMsg->WriteBytes(&aParam, sizeof(aParam));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    const char* outp;
    if (!aMsg->ReadBytes(aIter, &outp, sizeof(*aResult)))
      return false;

    *aResult = *reinterpret_cast<const paramType*>(outp);
    return true;
  }
};

template <>
struct ParamTraits<nsACString>
{
  typedef nsACString paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    bool isVoid = aParam.IsVoid();
    aMsg->WriteBool(isVoid);

    if (isVoid)
      
      return;

    PRUint32 length = aParam.Length();
    WriteParam(aMsg, length);
    aMsg->WriteBytes(aParam.BeginReading(), length);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    bool isVoid;
    if (!aMsg->ReadBool(aIter, &isVoid))
      return false;

    if (isVoid) {
      aResult->SetIsVoid(PR_TRUE);
      return true;
    }

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
    if (aParam.IsVoid())
      aLog->append(L"(NULL)");
    else
      aLog->append(UTF8ToWide(aParam.BeginReading()));
  }
};

template <>
struct ParamTraits<nsAString>
{
  typedef nsAString paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    bool isVoid = aParam.IsVoid();
    aMsg->WriteBool(isVoid);

    if (isVoid)
      
      return;

    PRUint32 length = aParam.Length();
    WriteParam(aMsg, length);
    aMsg->WriteBytes(aParam.BeginReading(), length * sizeof(PRUnichar));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    bool isVoid;
    if (!aMsg->ReadBool(aIter, &isVoid))
      return false;

    if (isVoid) {
      aResult->SetIsVoid(PR_TRUE);
      return true;
    }

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
    if (aParam.IsVoid())
      aLog->append(L"(NULL)");
    else {
#ifdef WCHAR_T_IS_UTF16
      aLog->append(reinterpret_cast<const wchar_t*>(aParam.BeginReading()));
#else
      PRUint32 length = aParam.Length();
      for (PRUint32 index = 0; index < length; index++) {
        aLog->push_back(std::wstring::value_type(aParam[index]));
      }
#endif
    }
  }
};

template <>
struct ParamTraits<nsCString> : ParamTraits<nsACString>
{
  typedef nsCString paramType;
};

#ifdef MOZILLA_INTERNAL_API

template<>
struct ParamTraits<nsCAutoString> : ParamTraits<nsCString>
{
  typedef nsCAutoString paramType;
};

#endif  

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

    for (PRUint32 index = 0; index < length; index++) {
      E* element = aResult->AppendElement();
      if (!(element && ReadParam(aMsg, aIter, element))) {
        return false;
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

template<>
struct ParamTraits<float>
{
  typedef float paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    aMsg->WriteBytes(&aParam, sizeof(paramType));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    const char* outFloat;
    if (!aMsg->ReadBytes(aIter, &outFloat, sizeof(float)))
      return false;
    *aResult = *reinterpret_cast<const float*>(outFloat);
    return true;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"%g", aParam));
  }
};

template<>
struct ParamTraits<gfxMatrix>
{
  typedef gfxMatrix paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.xx);
    WriteParam(aMsg, aParam.xy);
    WriteParam(aMsg, aParam.yx);
    WriteParam(aMsg, aParam.yy);
    WriteParam(aMsg, aParam.x0);
    WriteParam(aMsg, aParam.y0);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    if (ReadParam(aMsg, aIter, &aResult->xx) &&
        ReadParam(aMsg, aIter, &aResult->xy) &&
        ReadParam(aMsg, aIter, &aResult->yx) &&
        ReadParam(aMsg, aIter, &aResult->yy) &&
        ReadParam(aMsg, aIter, &aResult->x0) &&
        ReadParam(aMsg, aIter, &aResult->y0))
      return true;

    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"[[%g %g] [%g %g] [%g %g]]", aParam.xx, aParam.xy, aParam.yx, aParam.yy,
	  						    aParam.x0, aParam.y0));
  }
};

} 

#endif 
