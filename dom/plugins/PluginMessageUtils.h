





































#ifndef DOM_PLUGINS_PLUGINMESSAGEUTILS_H
#define DOM_PLUGINS_PLUGINMESSAGEUTILS_H

#include "IPC/IPCMessageUtils.h"

#include "npapi.h"
#include "npruntime.h"
#include "nsAutoPtr.h"
#include "nsStringGlue.h"

namespace mozilla {



struct void_t { };
struct null_t { };


namespace plugins {
class Variant;
}

namespace ipc {

typedef intptr_t NPRemoteIdentifier;
typedef mozilla::plugins::Variant NPRemoteVariant;

} 

namespace plugins {




struct IPCByteRange
{
  int32_t offset;
  uint32_t length;
};  

typedef std::vector<IPCByteRange> IPCByteRanges;

typedef nsCString Buffer;




#define VARSTR(v_)  case v_: return #v_
inline const char* const
NPPVariableToString(NPPVariable aVar)
{
    switch (aVar) {
        VARSTR(NPPVpluginNameString);
        VARSTR(NPPVpluginDescriptionString);
        VARSTR(NPPVpluginWindowBool);
        VARSTR(NPPVpluginTransparentBool);
        VARSTR(NPPVjavaClass);
        VARSTR(NPPVpluginWindowSize);
        VARSTR(NPPVpluginTimerInterval);

        VARSTR(NPPVpluginScriptableInstance);
        VARSTR(NPPVpluginScriptableIID);

        VARSTR(NPPVjavascriptPushCallerBool);

        VARSTR(NPPVpluginKeepLibraryInMemory);
        VARSTR(NPPVpluginNeedsXEmbed);

        VARSTR(NPPVpluginScriptableNPObject);

        VARSTR(NPPVformValue);
  
        VARSTR(NPPVpluginUrlRequestsDisplayedBool);
  
        VARSTR(NPPVpluginWantsAllNetworkStreams);

#ifdef XP_MACOSX
        VARSTR(NPPVpluginDrawingModel);
        VARSTR(NPPVpluginEventModel);
#endif

    default: return "???";
    }
}

inline const char*
NPNVariableToString(NPNVariable aVar)
{
    switch(aVar) {
        VARSTR(NPNVxDisplay);
        VARSTR(NPNVxtAppContext);
        VARSTR(NPNVnetscapeWindow);
        VARSTR(NPNVjavascriptEnabledBool);
        VARSTR(NPNVasdEnabledBool);
        VARSTR(NPNVisOfflineBool);

        VARSTR(NPNVserviceManager);
        VARSTR(NPNVDOMElement);
        VARSTR(NPNVDOMWindow);
        VARSTR(NPNVToolkit);
        VARSTR(NPNVSupportsXEmbedBool);

        VARSTR(NPNVWindowNPObject);

        VARSTR(NPNVPluginElementNPObject);

        VARSTR(NPNVSupportsWindowless);

        VARSTR(NPNVprivateModeBool);

    default: return "???";
    }
}
#undef VARSTR


} 

} 


namespace {



nsCString
NullableString(const char* aString)
{
    if (!aString) {
        nsCString str;
        str.SetIsVoid(PR_TRUE);
        return str;
    }
    return nsCString(aString);
}

} 


#define NullableStringGet(__string)                     \
    ( __string.IsVoid() ? NULL : __string.get())


namespace IPC {

template <>
struct ParamTraits<NPRect>
{
  typedef NPRect paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.top);
    WriteParam(aMsg, aParam.left);
    WriteParam(aMsg, aParam.bottom);
    WriteParam(aMsg, aParam.right);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    uint16_t top, left, bottom, right;
    if (ReadParam(aMsg, aIter, &top) &&
        ReadParam(aMsg, aIter, &left) &&
        ReadParam(aMsg, aIter, &bottom) &&
        ReadParam(aMsg, aIter, &right)) {
      aResult->top = top;
      aResult->left = left;
      aResult->bottom = bottom;
      aResult->right = right;
      return true;
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"[%u, %u, %u, %u]", aParam.top, aParam.left,
                              aParam.bottom, aParam.right));
  }
};

template <>
struct ParamTraits<NPWindowType>
{
  typedef NPWindowType paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    aMsg->WriteInt16(int16(aParam));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    int16 result;
    if (aMsg->ReadInt16(aIter, &result)) {
      *aResult = paramType(result);
      return true;
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"%d", int16(aParam)));
  }
};

template <>
struct ParamTraits<NPWindow>
{
  typedef NPWindow paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    aMsg->WriteULong(reinterpret_cast<unsigned long>(aParam.window));
    WriteParam(aMsg, aParam.x);
    WriteParam(aMsg, aParam.y);
    WriteParam(aMsg, aParam.width);
    WriteParam(aMsg, aParam.height);
    WriteParam(aMsg, aParam.clipRect);
    
    
    
    WriteParam(aMsg, aParam.type);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    unsigned long window;
    int32_t x, y;
    uint32_t width, height;
    NPRect clipRect;
    NPWindowType type;
    if (!(aMsg->ReadULong(aIter, &window) &&
          ReadParam(aMsg, aIter, &x) &&
          ReadParam(aMsg, aIter, &y) &&
          ReadParam(aMsg, aIter, &width) &&
          ReadParam(aMsg, aIter, &height) &&
          ReadParam(aMsg, aIter, &clipRect) &&
          ReadParam(aMsg, aIter, &type)))
      return false;

    aResult->window = (void*)window;
    aResult->x = x;
    aResult->y = y;
    aResult->width = width;
    aResult->height = height;
    aResult->clipRect = clipRect;
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    aResult->ws_info = 0;     
#endif
    aResult->type = type;
    return true;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"[%u, %d, %d, %u, %u, %d",
                              (unsigned long)aParam.window,
                              aParam.x, aParam.y, aParam.width,
                              aParam.height, (long)aParam.type));
  }
};

template <>
struct ParamTraits<NPString>
{
  typedef NPString paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.UTF8Length);
    aMsg->WriteBytes(aParam.UTF8Characters,
                     aParam.UTF8Length * sizeof(NPUTF8));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    if (ReadParam(aMsg, aIter, &aResult->UTF8Length)) {
      int byteCount = aResult->UTF8Length * sizeof(NPUTF8);
      if (!byteCount) {
        aResult->UTF8Characters = "\0";
        return true;
      }

      const char* messageBuffer = nsnull;
      nsAutoArrayPtr<char> newBuffer(new char[byteCount]);
      if (newBuffer && aMsg->ReadBytes(aIter, &messageBuffer, byteCount )) {
        memcpy((void*)messageBuffer, newBuffer.get(), byteCount);
        aResult->UTF8Characters = newBuffer.forget();
        return true;
      }
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"%s", aParam.UTF8Characters));
  }
};

template <>
struct ParamTraits<NPVariant>
{
  typedef NPVariant paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    if (NPVARIANT_IS_VOID(aParam)) {
      aMsg->WriteInt(0);
      return;
    }

    if (NPVARIANT_IS_NULL(aParam)) {
      aMsg->WriteInt(1);
      return;
    }

    if (NPVARIANT_IS_BOOLEAN(aParam)) {
      aMsg->WriteInt(2);
      WriteParam(aMsg, NPVARIANT_TO_BOOLEAN(aParam));
      return;
    }

    if (NPVARIANT_IS_INT32(aParam)) {
      aMsg->WriteInt(3);
      WriteParam(aMsg, NPVARIANT_TO_INT32(aParam));
      return;
    }

    if (NPVARIANT_IS_DOUBLE(aParam)) {
      aMsg->WriteInt(4);
      WriteParam(aMsg, NPVARIANT_TO_DOUBLE(aParam));
      return;
    }

    if (NPVARIANT_IS_STRING(aParam)) {
      aMsg->WriteInt(5);
      WriteParam(aMsg, NPVARIANT_TO_STRING(aParam));
      return;
    }

    NS_ERROR("Unsupported type!");
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    int type;
    if (!aMsg->ReadInt(aIter, &type)) {
      return false;
    }

    switch (type) {
      case 0:
        VOID_TO_NPVARIANT(*aResult);
        return true;

      case 1:
        NULL_TO_NPVARIANT(*aResult);
        return true;

      case 2: {
        bool value;
        if (ReadParam(aMsg, aIter, &value)) {
          BOOLEAN_TO_NPVARIANT(value, *aResult);
          return true;
        }
      } break;

      case 3: {
        int32 value;
        if (ReadParam(aMsg, aIter, &value)) {
          INT32_TO_NPVARIANT(value, *aResult);
          return true;
        }
      } break;

      case 4: {
        double value;
        if (ReadParam(aMsg, aIter, &value)) {
          DOUBLE_TO_NPVARIANT(value, *aResult);
          return true;
        }
      } break;

      case 5: {
        NPString value;
        if (ReadParam(aMsg, aIter, &value)) {
          STRINGN_TO_NPVARIANT(value.UTF8Characters, value.UTF8Length,
                               *aResult);
          return true;
        }
      } break;

      default:
        NS_ERROR("Unsupported type!");
    }

    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    if (NPVARIANT_IS_VOID(aParam)) {
      aLog->append(L"[void]");
      return;
    }

    if (NPVARIANT_IS_NULL(aParam)) {
      aLog->append(L"[null]");
      return;
    }

    if (NPVARIANT_IS_BOOLEAN(aParam)) {
      LogParam(NPVARIANT_TO_BOOLEAN(aParam), aLog);
      return;
    }

    if (NPVARIANT_IS_INT32(aParam)) {
      LogParam(NPVARIANT_TO_INT32(aParam), aLog);
      return;
    }

    if (NPVARIANT_IS_DOUBLE(aParam)) {
      LogParam(NPVARIANT_TO_DOUBLE(aParam), aLog);
      return;
    }

    if (NPVARIANT_IS_STRING(aParam)) {
      LogParam(NPVARIANT_TO_STRING(aParam), aLog);
      return;
    }

    NS_ERROR("Unsupported type!");
  }
};

template<>
struct ParamTraits<mozilla::void_t>
{
  typedef mozilla::void_t paramType;
  static void Write(Message* aMsg, const paramType& aParam) { }
  static bool
  Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    *aResult = paramType();
    return true;
  }
};

template<>
struct ParamTraits<mozilla::null_t>
{
  typedef mozilla::null_t paramType;
  static void Write(Message* aMsg, const paramType& aParam) { }
  static bool
  Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    *aResult = paramType();
    return true;
  }
};

template <>
struct ParamTraits<mozilla::plugins::IPCByteRange>
{
  typedef mozilla::plugins::IPCByteRange paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.offset);
    WriteParam(aMsg, aParam.length);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    paramType p;
    if (ReadParam(aMsg, aIter, &p.offset) &&
        ReadParam(aMsg, aIter, &p.length)) {
      *aResult = p;
      return true;
    }
    return false;
  }
};


} 








#if defined(XP_MACOSX)
#  include "mozilla/plugins/NPEventOSX.h"
#elif defined(XP_WIN)
#  include "mozilla/plugins/NPEventWindows.h"
#elif defined(XP_OS2)
#  error Sorry, OS/2 is not supported
#elif defined(XP_UNIX) && defined(MOZ_X11)
#  include "mozilla/plugins/NPEventX11.h"
#else
#  error Unsupported platform
#endif


#endif 
