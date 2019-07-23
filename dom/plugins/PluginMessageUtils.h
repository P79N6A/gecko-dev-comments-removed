





































#ifndef DOM_PLUGINS_PLUGINMESSAGEUTILS_H
#define DOM_PLUGINS_PLUGINMESSAGEUTILS_H

#include "IPC/IPCMessageUtils.h"

#include "npapi.h"

namespace IPC {

#if 0
  void* window;  
                 
                 
  int32_t  x;      
  int32_t  y;      
  uint32_t width;  
  uint32_t height;
  NPRect   clipRect; 
                     
#if defined(XP_UNIX) && !defined(XP_MACOSX)
  void * ws_info; 
#endif 
  NPWindowType type; 
#endif

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
    NPWindowType type;
    if (aMsg->ReadULong(aIter, &window) &&
        ReadParam(aMsg, aIter, &x) &&
        ReadParam(aMsg, aIter, &y) &&
        ReadParam(aMsg, aIter, &width) &&
        ReadParam(aMsg, aIter, &height) &&
        ReadParam(aMsg, aIter, &type)) {
      aResult->window = (void*)window;
      aResult->x = x;
      aResult->y = y;
      aResult->width = width;
      aResult->height = height;
#if defined(XP_UNIX) && !defined(XP_MACOSX)
      aResult->ws_info = 0;     
#endif
      aResult->type = type;
      return true;
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"[%u, %d, %d, %u, %u, %d",
                              (unsigned long)aParam.window,
                              aParam.x, aParam.y, aParam.width,
                              aParam.height, (long)aParam.type));
  }
};

} 

#endif 
