









#ifndef WEBRTC_BASE_WINFIREWALL_H_
#define WEBRTC_BASE_WINFIREWALL_H_

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef long HRESULT;  
#endif 

struct INetFwMgr;
struct INetFwPolicy;
struct INetFwProfile;

namespace rtc {





class WinFirewall {
 public:
  WinFirewall();
  ~WinFirewall();

  bool Initialize(HRESULT* result);
  void Shutdown();

  bool Enabled() const;
  bool QueryAuthorized(const char* filename, bool* authorized) const;
  bool QueryAuthorizedW(const wchar_t* filename, bool* authorized) const;

  bool AddApplication(const char* filename, const char* friendly_name,
                      bool authorized, HRESULT* result);
  bool AddApplicationW(const wchar_t* filename, const wchar_t* friendly_name,
                       bool authorized, HRESULT* result);

 private:
  INetFwMgr* mgr_;
  INetFwPolicy* policy_;
  INetFwProfile* profile_;
};



}  

#endif  
