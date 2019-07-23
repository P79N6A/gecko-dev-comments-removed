




#ifndef CHROME_COMMON_SECURITY_FILTER_PEER_H__
#define CHROME_COMMON_SECURITY_FILTER_PEER_H__

#include "chrome/common/filter_policy.h"
#include "webkit/glue/resource_loader_bridge.h"








class SecurityFilterPeer : public webkit_glue::ResourceLoaderBridge::Peer {
 public:
  virtual ~SecurityFilterPeer();

  static SecurityFilterPeer* CreateSecurityFilterPeer(
      webkit_glue::ResourceLoaderBridge* resource_loader_bridge,
      webkit_glue::ResourceLoaderBridge::Peer* peer,
      ResourceType::Type resource_type,
      const std::string& mime_type,
      FilterPolicy::Type filter_policy,
      int os_error);

  static SecurityFilterPeer* CreateSecurityFilterPeerForDeniedRequest(
      ResourceType::Type resource_type,
      webkit_glue::ResourceLoaderBridge::Peer* peer,
      int os_error);

  static SecurityFilterPeer* CreateSecurityFilterPeerForFrame(
      webkit_glue::ResourceLoaderBridge::Peer* peer,
      int os_error);

  
  virtual void OnUploadProgress(uint64 position, uint64 size);
  virtual void OnReceivedRedirect(const GURL& new_url);
  virtual void OnReceivedResponse(
      const webkit_glue::ResourceLoaderBridge::ResponseInfo& info,
      bool content_filtered);
  virtual void OnReceivedData(const char* data, int len);
  virtual void OnCompletedRequest(const URLRequestStatus& status,
                                  const std::string& security_info);
  virtual std::string GetURLForDebugging();

 protected:
   SecurityFilterPeer(webkit_glue::ResourceLoaderBridge* resource_loader_bridge,
                      webkit_glue::ResourceLoaderBridge::Peer* peer);

   webkit_glue::ResourceLoaderBridge::Peer* original_peer_;
   webkit_glue::ResourceLoaderBridge* resource_loader_bridge_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SecurityFilterPeer);
};



class BufferedPeer : public SecurityFilterPeer {
 public:
  BufferedPeer(webkit_glue::ResourceLoaderBridge* resource_loader_bridge,
               webkit_glue::ResourceLoaderBridge::Peer* peer,
               const std::string& mime_type);
  virtual ~BufferedPeer();

  
  virtual void OnReceivedResponse(
      const webkit_glue::ResourceLoaderBridge::ResponseInfo& info,
      bool content_filtered);
  virtual void OnReceivedData(const char* data, int len);
  virtual void OnCompletedRequest(const URLRequestStatus& status,
                                  const std::string& security_info);

 protected:
  
  
  
  
  virtual bool DataReady() = 0;

  webkit_glue::ResourceLoaderBridge::ResponseInfo response_info_;
  std::string data_;

 private:
  std::string mime_type_;
  DISALLOW_EVIL_CONSTRUCTORS(BufferedPeer);
};









class ReplaceContentPeer : public SecurityFilterPeer {
 public:
  ReplaceContentPeer(webkit_glue::ResourceLoaderBridge* resource_loader_bridge,
                     webkit_glue::ResourceLoaderBridge::Peer* peer,
                     const std::string& mime_type,
                     const std::string& data);
  virtual ~ReplaceContentPeer();

  
  virtual void OnReceivedResponse(
      const webkit_glue::ResourceLoaderBridge::ResponseInfo& info,
      bool content_filtered);
  void OnReceivedData(const char* data, int len);
  void OnCompletedRequest(const URLRequestStatus& status,
                          const std::string& security_info);
 private:
   webkit_glue::ResourceLoaderBridge::ResponseInfo response_info_;
   std::string mime_type_;
   std::string data_;
   DISALLOW_EVIL_CONSTRUCTORS(ReplaceContentPeer);
};



class ImageFilterPeer : public BufferedPeer {
 public:
  ImageFilterPeer(webkit_glue::ResourceLoaderBridge* resource_loader_bridge,
                  webkit_glue::ResourceLoaderBridge::Peer* peer);
  virtual ~ImageFilterPeer();

 protected:
   virtual bool DataReady();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ImageFilterPeer);
};

#endif  
