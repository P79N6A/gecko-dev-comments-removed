








#ifndef WifiHotspotUtils_h
#define WifiHotspotUtils_h


struct wpa_ctrl;

class WifiHotspotUtils
{
public:
  static void* GetSharedLibrary();

  int32_t do_wifi_connect_to_hostapd();
  int32_t do_wifi_close_hostapd_connection();
  int32_t do_wifi_hostapd_command(const char *command,
                                  char *reply,
                                  size_t *reply_len);
  int32_t do_wifi_hostapd_get_stations();

private:
  struct wpa_ctrl * openConnection(const char *ifname);
  int32_t sendCommand(struct wpa_ctrl *ctrl, const char *cmd,
                      char *reply, size_t *reply_len);
};


#define DEFINE_DLFUNC(name, ret, args...) typedef ret (*FUNC##name)(args);


#define USE_DLFUNC(name)                                                      \
  FUNC##name name = (FUNC##name) dlsym(GetSharedLibrary(), #name);            \
  if (!name) {                                                                \
    MOZ_ASSUME_UNREACHABLE("Symbol not found in shared library : " #name);    \
  }

#endif 
