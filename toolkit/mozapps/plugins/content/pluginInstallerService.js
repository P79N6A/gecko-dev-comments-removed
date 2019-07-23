




































const nsIXPIProgressDialog = Components.interfaces.nsIXPIProgressDialog;

function getLocalizedError(key)
{
  return document.getElementById("xpinstallStrings").getString(key);
}

function binaryToHex(input)
{
  return [('0' + input.charCodeAt(i).toString(16)).slice(-2)
          for (i in input)].join('');
}

function verifyHash(aFile, aHash)
{
  try {
    var [, method, hash] = /^([A-Za-z0-9]+):(.*)$/.exec(aHash);

    var fis = Components.classes['@mozilla.org/network/file-input-stream;1'].
      createInstance(Components.interfaces.nsIFileInputStream);
    fis.init(aFile, -1, -1, 0);

    var hasher = Components.classes['@mozilla.org/security/hash;1'].
      createInstance(Components.interfaces.nsICryptoHash);
    hasher.initWithString(method);
    hasher.updateFromStream(fis, -1);
    dlhash = binaryToHex(hasher.finish(false));
    return dlhash == hash;
  }
  catch (e) {
    Components.utils.reportError(e);
    return false;
  }
}

function InstallerObserver(aPlugin)
{
  this._plugin = aPlugin;
  this._init();
}

InstallerObserver.prototype = {
  _init: function()
  {
    try {
      var ios = Components.classes["@mozilla.org/network/io-service;1"].
        getService(Components.interfaces.nsIIOService);
      var uri = ios.newURI(this._plugin.InstallerLocation, null, null);
      uri.QueryInterface(Components.interfaces.nsIURL);

      var leafName = uri.fileName;
      if (leafName.indexOf('.') == -1)
        throw "Filename needs to contain a dot for platform-native launching to work correctly.";

      var dirs = Components.classes["@mozilla.org/file/directory_service;1"].
        getService(Components.interfaces.nsIProperties);

      var resultFile = dirs.get("TmpD", Components.interfaces.nsIFile);
      resultFile.append(leafName);
      resultFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE,
                              0x770);

      var channel = ios.newChannelFromURI(uri);
      this._downloader =
        Components.classes["@mozilla.org/network/downloader;1"].
          createInstance(Components.interfaces.nsIDownloader);
      this._downloader.init(this, resultFile);
      channel.notificationCallbacks = this;

      this._fireNotification(nsIXPIProgressDialog.DOWNLOAD_START, null);

      channel.asyncOpen(this._downloader, null);
    }
    catch (e) {
      this._fireNotification(nsIXPIProgressDialog.INSTALL_DONE,
                             getLocalizedError("error-228"));
      if (resultFile && resultFile.exists())
        resultfile.remove(false);
    }
  },

  


  _fireNotification: function(aStatus, aErrorMsg)
  {
    gPluginInstaller.pluginInstallationProgress(this._plugin.pid,
                                                aStatus, aErrorMsg);

    if (aStatus == nsIXPIProgressDialog.INSTALL_DONE) {
      --PluginInstallService._installersPending;
      PluginInstallService._fireFinishedNotification();
    }
  },

  QueryInterface: function(iid)
  {
    if (iid.equals(Components.interfaces.nsISupports) ||
        iid.equals(Components.interfaces.nsIInterfaceRequestor) ||
        iid.equals(Components.interfaces.nsIDownloadObserver) ||
        iid.equals(Components.interfaces.nsIProgressEventSink))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  getInterface: function(iid)
  {
    if (iid.equals(Components.interfaces.nsIProgressEventSink))
      return this;

    return null;
  },

  onDownloadComplete: function(downloader, request, ctxt, status, result)
  {
    if (!Components.isSuccessCode(status)) {
      
      this._fireNotification(nsIXPIProgressDialog.INSTALL_DONE,
                             getLocalizedError("error-228"));
      result.remove(false);
      return;
    }

    this._fireNotification(nsIXPIProgressDialog.DOWNLOAD_DONE);

    if (this._plugin.InstallerHash &&
        !verifyHash(result, this._plugin.InstallerHash)) {
      
      this._fireNotification(nsIXPIProgressDialog.INSTALL_DONE,
                             getLocalizedError("error-261"));
      result.remove(false);
      return;
    }

    this._fireNotification(nsIXPIProgressDialog.INSTALL_START);

    result.QueryInterface(Components.interfaces.nsILocalFile);
    try {
      result.launch();
      this._fireNotification(nsIXPIProgressDialog.INSTALL_DONE, null);
      
      
    }
    catch (e) {
      Components.utils.reportError(e);
      this._fireNotification(nsIXPIProgressDialog.INSTALL_DONE,
                             getLocalizedError("error-207"));
      result.remove(false);
    }
  },

  onProgress: function(aRequest, aContext, aProgress, aProgressMax)
  {
    gPluginInstaller.pluginInstallationProgressMeter(this._plugin.pid,
                                                     aProgress,
                                                     aProgressMax);
  },

  onStatus: function(aRequest, aContext, aStatus, aStatusArg)
  {
    
  }
};

var PluginInstallService = {

  








  startPluginInstallation: function (aInstallerPlugins,
                                     aXPIPlugins)
  {
    this._installerPlugins = [new InstallerObserver(plugin)
                              for each (plugin in aInstallerPlugins)];
    this._installersPending = this._installerPlugins.length;

    this._xpiPlugins = aXPIPlugins;

    if (this._xpiPlugins.length > 0) {
      this._xpisDone = false;

      var xpiManager = Components.classes["@mozilla.org/xpinstall/install-manager;1"]
                                 .createInstance(Components.interfaces.nsIXPInstallManager);
      xpiManager.initManagerWithHashes(
        [plugin.XPILocation for each (plugin in this._xpiPlugins)],
        [plugin.XPIHash for each (plugin in this._xpiPlugins)],
        this._xpiPlugins.length, this);
    }
    else {
      this._xpisDone = true;
    }
  },

  _fireFinishedNotification: function()
  {
    if (this._installersPending == 0 && this._xpisDone)
      gPluginInstaller.
        pluginInstallationProgress(null, nsIXPIProgressDialog.DIALOG_CLOSE,
                                   null);
  },

  
  onStateChange: function (aIndex, aState, aValue)
  {
    
    var pid = this._xpiPlugins[aIndex].pid;
    var errorMsg;

    if (aState == nsIXPIProgressDialog.INSTALL_DONE) {
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

    if (aState == nsIXPIProgressDialog.DIALOG_CLOSE) {
      this._xpisDone = true;
      this._fireFinishedNotification();
    }
    else {
      gPluginInstaller.pluginInstallationProgress(pid, aState, errorMsg);
    }
  },

  onProgress: function (aIndex, aValue, aMaxValue)
  {
    
    var pid = this._xpiPlugins[aIndex].pid;
    gPluginInstaller.pluginInstallationProgressMeter(pid, aValue, aMaxValue);
  }
}
