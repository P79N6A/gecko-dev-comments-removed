





#ifndef SerializedLoadContext_h
#define SerializedLoadContext_h

#include "base/basictypes.h"
#include "IPC/IPCMessageUtils.h"
#include "nsILoadContext.h"







class nsIChannel;
class nsIWebSocketChannel;

namespace IPC {

class SerializedLoadContext
{
public:
  SerializedLoadContext()
  {
    Init(nullptr);
  }

  SerializedLoadContext(nsILoadContext* aLoadContext);
  SerializedLoadContext(nsIChannel* aChannel);
  SerializedLoadContext(nsIWebSocketChannel* aChannel);

  void Init(nsILoadContext* aLoadContext);

  bool IsNotNull() const 
  {
    return mIsNotNull;
  }

  
  bool          mIsNotNull;
  bool          mIsContent;
  bool          mUsePrivateBrowsing;
  bool          mIsInBrowserElement;
  PRUint32      mAppId;
};


template<>
struct ParamTraits<SerializedLoadContext>
{
  typedef SerializedLoadContext paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mIsNotNull);
    WriteParam(aMsg, aParam.mIsContent);
    WriteParam(aMsg, aParam.mUsePrivateBrowsing);
    WriteParam(aMsg, aParam.mAppId);
    WriteParam(aMsg, aParam.mIsInBrowserElement);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    if (!ReadParam(aMsg, aIter, &aResult->mIsNotNull) ||
        !ReadParam(aMsg, aIter, &aResult->mIsContent)  ||
        !ReadParam(aMsg, aIter, &aResult->mUsePrivateBrowsing)  ||
        !ReadParam(aMsg, aIter, &aResult->mAppId)  ||
        !ReadParam(aMsg, aIter, &aResult->mIsInBrowserElement)) {
      return false;
    }

    return true;
  }
};

} 

#endif 

