




































var PluginInstallService = {
  
  init: function () 
  {
  },

  pluginPidArray: null,

  startPluginInstallation: function (aPluginXPIUrlsArray,
                                     aPluginHashes,
                                     aPluginPidArray) {
     this.pluginPidArray = aPluginPidArray;

     var xpiManager = Components.classes["@mozilla.org/xpinstall/install-manager;1"]
                                .createInstance(Components.interfaces.nsIXPInstallManager);
     xpiManager.initManagerWithHashes(aPluginXPIUrlsArray, aPluginHashes,
                                      aPluginXPIUrlsArray.length, this);
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
