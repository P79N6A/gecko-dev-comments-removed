




































#ifndef mozilla_plugins_BrowserStreamChild_h
#define mozilla_plugins_BrowserStreamChild_h 1

#include "mozilla/plugins/PBrowserStreamProtocolChild.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;

class BrowserStreamChild : public PBrowserStreamProtocolChild
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

  virtual nsresult AnswerNPP_WriteReady(const int32_t& newlength,
                                        int32_t *size);
  virtual nsresult AnswerNPP_Write(const int32_t& offset,
                                   const Buffer& data,
                                   int32_t* consumed);

  virtual nsresult AnswerNPP_StreamAsFile(const nsCString& fname);

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
