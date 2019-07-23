




































var PluginInstallService = {
  
  init: function () 
  {
  },

  pluginPidArray: null,

  startPluginInsallation: function (aPluginXPIUrlsArray, aPluginPidArray) {
     this.pluginPidArray = aPluginPidArray;

     var xpiManager = Components.classes["@mozilla.org/xpinstall/install-manager;1"]
                                .createInstance(Components.interfaces.nsIXPInstallManager);
     xpiManager.initManagerFromChrome(aPluginXPIUrlsArray, aPluginXPIUrlsArray.length, this);
  },

  
  onStateChange: function (aIndex, aState, aValue)
  {
    
    var pid = this.pluginPidArray[aIndex];
    var errorMsg;

    if (aState == Components.interfaces.nsIXPIProgressDialog.INSTALL_DONE) {
      if (aValue != 0) {
        var xpinstallStrings = document.getElementById("xpinstallStrings");
        try {
          errorMsg = xpinstallStrings.getString("error" + aValue);
        }
        catch (e) {
          errorMsg = xpinstallStrings.getFormattedString("unknown.error", [aValue]);
        }
      }
    }

    gPluginInstaller.pluginInstallationProgress(pid, aState, errorMsg);

  },

  onProgress: function (aIndex, aValue, aMaxValue)
  {
    
    var pid = this.pluginPidArray[aIndex];

    gPluginInstaller.pluginInstallationProgressMeter(pid, aValue, aMaxValue);
  }
}
