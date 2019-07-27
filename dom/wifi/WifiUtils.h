








#ifndef WifiUtils_h
#define WifiUtils_h

#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/WifiOptionsBinding.h"
#include "WifiHotspotUtils.h"


struct CommandOptions
{
public:
  CommandOptions(const mozilla::dom::WifiCommandOptions& aOther) {

#define COPY_OPT_FIELD(prop, defaultValue)            \
    if (aOther.prop.WasPassed()) {                    \
      prop = aOther.prop.Value();                     \
    } else {                                          \
      prop = defaultValue;                            \
    }

#define COPY_FIELD(prop) prop = aOther.prop;
    COPY_FIELD(mId)
    COPY_FIELD(mCmd)
    COPY_OPT_FIELD(mRequest, EmptyString())

#undef COPY_OPT_FIELD
#undef COPY_FIELD
  }

  
  nsString mCmd;
  int32_t mId;
  nsString mRequest;
};




class WpaSupplicantImpl
{
public:
  
  virtual ~WpaSupplicantImpl() {}

  virtual int32_t
  do_wifi_wait_for_event(const char *iface, char *buf, size_t len) = 0; 

  virtual int32_t
  do_wifi_command(const char* iface, const char* cmd, char* buff, size_t* len) = 0; 

  virtual int32_t
  do_wifi_load_driver() = 0;

  virtual int32_t
  do_wifi_unload_driver() = 0;

  virtual int32_t
  do_wifi_start_supplicant(int32_t) = 0; 

  virtual int32_t
  do_wifi_stop_supplicant(int32_t) = 0; 

  virtual int32_t
  do_wifi_connect_to_supplicant(const char* iface) = 0; 

  virtual void
  do_wifi_close_supplicant_connection(const char* iface) = 0; 
};


class WpaSupplicant MOZ_FINAL
{
public:
  WpaSupplicant();

  
  
  
  void WaitForEvent(nsAString& aEvent, const nsCString& aInterface);
  bool ExecuteCommand(CommandOptions aOptions,
                      mozilla::dom::WifiResultOptions& result,
                      const nsCString& aInterface);

private:
  nsAutoPtr<WpaSupplicantImpl> mImpl;
  nsAutoPtr<WifiHotspotUtils> mWifiHotspotUtils;

  uint32_t mSdkVersion;

protected:
  void CheckBuffer(char* buffer, int32_t length, nsAString& aEvent);
  uint32_t MakeMask(uint32_t len);
};

#endif 
