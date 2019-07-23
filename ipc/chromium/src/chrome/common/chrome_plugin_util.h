



#ifndef CHROME_COMMON_CHROME_PLUGIN_UTIL_H_
#define CHROME_COMMON_CHROME_PLUGIN_UTIL_H_

#include <string>

#include "base/basictypes.h"
#include "base/non_thread_safe.h"
#include "base/ref_counted.h"
#include "chrome/common/chrome_plugin_api.h"
#include "chrome/common/notification_observer.h"

class ChromePluginLib;
class MessageLoop;
namespace net {
class HttpResponseHeaders;
}




struct ScopableCPRequest : public CPRequest {
  template<class T>
  static T GetData(CPRequest* request) {
    return static_cast<T>(static_cast<ScopableCPRequest*>(request)->data);
  }

  ScopableCPRequest(const char* url, const char* method,
                    CPBrowsingContext context);
  ~ScopableCPRequest();

  void* data;
};




class PluginHelper : public NotificationObserver, public NonThreadSafe {
 public:
  static void DestroyAllHelpersForPlugin(ChromePluginLib* plugin);

  PluginHelper(ChromePluginLib* plugin);
  virtual ~PluginHelper();

  
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 protected:
  scoped_refptr<ChromePluginLib> plugin_;

  DISALLOW_COPY_AND_ASSIGN(PluginHelper);
};


class PluginResponseUtils {
public:
  
  
  static uint32 CPLoadFlagsToNetFlags(uint32 flags);

  
  static int GetResponseInfo(
      const net::HttpResponseHeaders* response_headers,
      CPResponseInfoType type, void* buf, size_t buf_size);
};


inline char* CPB_StringDup(CPB_AllocFunc alloc, const std::string& str) {
  char* cstr = static_cast<char*>(alloc(static_cast<uint32>(str.length() + 1)));
  memcpy(cstr, str.c_str(), str.length() + 1);  
  return cstr;
}

CPError CPB_GetCommandLineArgumentsCommon(const char* url,
                                          std::string* arguments);

void* STDCALL CPB_Alloc(uint32 size);
void STDCALL CPB_Free(void* memory);

#endif  
