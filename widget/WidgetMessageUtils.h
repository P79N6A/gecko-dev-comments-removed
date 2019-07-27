



#ifndef mozilla_WidgetMessageUtils_h
#define mozilla_WidgetMessageUtils_h

#include "ipc/IPCMessageUtils.h"
#include "mozilla/LookAndFeel.h"

namespace IPC {

template<>
struct ParamTraits<LookAndFeelInt>
{
  typedef LookAndFeelInt paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.id);
    WriteParam(aMsg, aParam.value);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    int32_t id, value;
    if (ReadParam(aMsg, aIter, &id) &&
        ReadParam(aMsg, aIter, &value)) {
      aResult->id = id;
      aResult->value = value;
      return true;
    }
    return false;
  }
};

} 

#endif 