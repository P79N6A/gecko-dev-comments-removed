




































#ifndef mozilla_plugins_BrowserStreamChild_h
#define mozilla_plugins_BrowserStreamChild_h 1

#include "mozilla/plugins/PBrowserStreamChild.h"
#include "mozilla/plugins/AStream.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;

class BrowserStreamChild : public PBrowserStreamChild, public AStream
{
public:
  BrowserStreamChild(PluginInstanceChild* instance,
                     const nsCString& url,
                     const uint32_t& length,
                     const uint32_t& lastmodified,
                     const nsCString& headers,
                     const nsCString& mimeType,
                     const bool& seekable,
                     NPError* rv,
                     uint16_t* stype);
  virtual ~BrowserStreamChild() { }

  NS_OVERRIDE virtual bool IsBrowserStream() { return true; }

  virtual bool AnswerNPP_WriteReady(const int32_t& newlength,
                                        int32_t *size);
  virtual bool AnswerNPP_Write(const int32_t& offset,
                                   const Buffer& data,
                                   int32_t* consumed);

  virtual bool AnswerNPP_StreamAsFile(const nsCString& fname);

  void EnsureCorrectInstance(PluginInstanceChild* i)
  {
    if (i != mInstance)
      NS_RUNTIMEABORT("Incorrect stream instance");
  }

  void NPP_DestroyStream(NPError reason);

private:
  PluginInstanceChild* mInstance;
  NPStream mStream;
  bool mClosed;
  nsCString mURL;
  nsCString mHeaders;
};

} 
} 

#endif 
