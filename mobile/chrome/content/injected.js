




































if (window && window.navigator) {
  window.navigator.mozApps = {
    install: function(aParam) {
      return OpenWebapps_install(window.location, aParam.url, aParam.install_data, aParam.onsuccess, aParam.onerror);
    },
    
    amInstalled: function(aSuccessCallback, aErrorCallback) {
      return OpenWebapps_amInstalled(window.location, aSuccessCallback, aErrorCallback);
    },
    
    getInstalledBy: function(aSuccessCallback, aErrorCallback) {
      return OpenWebapps_getInstalledBy(window.location, aSuccessCallback, aErrorCallback);
    }
  }
  
  window.navigator.mozApps.mgmt = {
    launch: function(aOrigin, aSuccessCallback, aErrorCallback) {
      return OpenWebappsMgmt_launch(aOrigin, aSuccessCallback, aErrorCallback);
    },
    
    list: function(aSuccessCallback, aErrorCallback) {
      return OpenWebappsMgmt_list(window.location, aSuccessCallback, aErrorCallback);
    },
    
    uninstall: function(aOrigin, aSuccessCallback, aErrorCallback) {
      return OpenWebappsMgmt_uninstall(window.location, aOrigin, aSuccessCallback, aErrorCallback);
    }
  }
}
