



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const PREF_BD_USEDOWNLOADDIR = "browser.download.useDownloadDir";
const URI_GENERIC_ICON_DOWNLOAD = "chrome://browser/skin/images/alert-downloads-30.png";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DownloadUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "ContentUtil", function() {
  Cu.import("resource:///modules/ContentUtil.jsm");
  return ContentUtil;
});





function HelperAppLauncherDialog() { }

HelperAppLauncherDialog.prototype = {
  classID: Components.ID("{e9d277a0-268a-4ec2-bb8c-10fdf3e44611}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIHelperAppLauncherDialog]),

  show: function hald_show(aLauncher, aContext, aReason) {
    
    
    
    if (aLauncher.MIMEInfo.hasDefaultHandler && !aLauncher.targetFileIsExecutable) {
      aLauncher.MIMEInfo.preferredAction = Ci.nsIMIMEInfo.useSystemDefault;
      aLauncher.launchWithApplication(null, false);
    } else {
      let wasClicked = false;
      this._showDownloadInfobar(aLauncher);
    }
  },

  _getDownloadSize: function dv__getDownloadSize (aSize) {
    let displaySize = DownloadUtils.convertByteUnits(aSize);
    
    if (aSize > 0)
      return displaySize.join("");
    else {
      let browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
      return browserBundle.GetStringFromName("downloadsUnknownSize");
    }
  },

  _getChromeWindow: function (aWindow) {
      let chromeWin = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIWebNavigation)
                            .QueryInterface(Ci.nsIDocShellTreeItem)
                            .rootTreeItem
                            .QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindow)
                            .QueryInterface(Ci.nsIDOMChromeWindow);
     return chromeWin;
  },

  _showDownloadInfobar: function do_showDownloadInfobar(aLauncher) {
    Services.obs.notifyObservers(null, "dl-request", "");
    let browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");

    let runButtonText =
              browserBundle.GetStringFromName("downloadRun");
    let saveButtonText =
              browserBundle.GetStringFromName("downloadSave");
    let cancelButtonText =
              browserBundle.GetStringFromName("downloadCancel");

    let buttons = [
      {
        isDefault: true,
        label: runButtonText,
        accessKey: "",
        callback: function() {
          aLauncher.saveToDisk(null, false);
          Services.obs.notifyObservers(aLauncher.targetFile, "dl-run", "true");
        }
      },
      {
        label: saveButtonText,
        accessKey: "",
        callback: function() {
          aLauncher.saveToDisk(null, false);
          Services.obs.notifyObservers(aLauncher.targetFile, "dl-run", "false");
        }
      },
      {
        label: cancelButtonText,
        accessKey: "",
        callback: function() { aLauncher.cancel(Cr.NS_BINDING_ABORTED); }
      }
    ];

    let window = Services.wm.getMostRecentWindow("navigator:browser");
    let chromeWin = this._getChromeWindow(window).wrappedJSObject;
    let notificationBox = chromeWin.Browser.getNotificationBox();
    let document = notificationBox.ownerDocument;
    downloadSize = this._getDownloadSize(aLauncher.contentLength);

    let msg = browserBundle.GetStringFromName("alertDownloadSave");

    let fragment =  ContentUtil.populateFragmentFromString(
                      document.createDocumentFragment(),
                      msg,
                      {
                        text: aLauncher.suggestedFileName,
                        className: "download-filename-text"
                      },
                      {
                        text: aLauncher.suggestedFileName,
                        className: "download-size-text"
                      },
                      {
                        text: aLauncher.source.host,
                        className: "download-host-text"
                      }
                    );
    notificationBox.notificationsHidden = false;
    let newBar = notificationBox.appendNotification("",
                                                    "save-download",
                                                    URI_GENERIC_ICON_DOWNLOAD,
                                                    notificationBox.PRIORITY_WARNING_HIGH,
                                                    buttons);
    let messageContainer = document.getAnonymousElementByAttribute(newBar, "anonid", "messageText");
    messageContainer.appendChild(fragment);
  },

  promptForSaveToFile: function hald_promptForSaveToFile(aLauncher, aContext, aDefaultFile, aSuggestedFileExt, aForcePrompt) {
    let file = null;
    let prefs = Services.prefs;

    if (!aForcePrompt) {
      
      
      let autodownload = true;
      try {
        autodownload = prefs.getBoolPref(PREF_BD_USEDOWNLOADDIR);
      } catch (e) { }

      if (autodownload) {
        
        let dnldMgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
        let defaultFolder = dnldMgr.userDownloadsDirectory;

        try {
          file = this.validateLeafName(defaultFolder, aDefaultFile, aSuggestedFileExt);
        }
        catch (e) {
        }

        
        if (file)
          return file;
      }
    }

    
    let picker = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let windowTitle = "";
    let parent = aContext.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
    picker.init(parent, windowTitle, Ci.nsIFilePicker.modeSave);
    picker.defaultString = aDefaultFile;

    if (aSuggestedFileExt) {
      
      picker.defaultExtension = aSuggestedFileExt.substring(1);
    }
    else {
      try {
        picker.defaultExtension = aLauncher.MIMEInfo.primaryExtension;
      }
      catch (e) { }
    }

    var wildCardExtension = "*";
    if (aSuggestedFileExt) {
      wildCardExtension += aSuggestedFileExt;
      picker.appendFilter(aLauncher.MIMEInfo.description, wildCardExtension);
    }

    picker.appendFilters(Ci.nsIFilePicker.filterAll);

    
    
    
    var dnldMgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
    picker.displayDirectory = dnldMgr.userDownloadsDirectory;

    
    try {
      let lastDir = prefs.getComplexValue("browser.download.lastDir", Ci.nsILocalFile);
      if (isUsableDirectory(lastDir))
        picker.displayDirectory = lastDir;
    }
    catch (e) { }

    if (picker.show() == Ci.nsIFilePicker.returnCancel) {
      
      return null;
    }

    
    
    
    file = picker.file;

    if (file) {
      try {
        
        
        
        if (file.exists())
          file.remove(false);
      }
      catch (e) { }
      var newDir = file.parent.QueryInterface(Ci.nsILocalFile);
      prefs.setComplexValue("browser.download.lastDir", Ci.nsILocalFile, newDir);
      file = this.validateLeafName(newDir, file.leafName, null);
    }
    return file;
  },

  validateLeafName: function hald_validateLeafName(aLocalFile, aLeafName, aFileExt) {
    if (!(aLocalFile && this.isUsableDirectory(aLocalFile)))
      return null;

    
    
    aLeafName = aLeafName.replace(/^\.+/, "");

    if (aLeafName == "")
      aLeafName = "unnamed" + (aFileExt ? "." + aFileExt : "");
    aLocalFile.append(aLeafName);

    this.makeFileUnique(aLocalFile);
    return aLocalFile;
  },

  makeFileUnique: function hald_makeFileUnique(aLocalFile) {
    try {
      
      
      
      
      var collisionCount = 0;
      while (aLocalFile.exists()) {
        collisionCount++;
        if (collisionCount == 1) {
          
          
          if (aLocalFile.leafName.match(/\.[^\.]{1,3}\.(gz|bz2|Z)$/i))
            aLocalFile.leafName = aLocalFile.leafName.replace(/\.[^\.]{1,3}\.(gz|bz2|Z)$/i, "(2)$&");
          else
            aLocalFile.leafName = aLocalFile.leafName.replace(/(\.[^\.]*)?$/, "(2)$&");
        }
        else {
          
          aLocalFile.leafName = aLocalFile.leafName.replace(/^(.*\()\d+\)/, "$1" + (collisionCount+1) + ")");
        }
      }
      aLocalFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);
    }
    catch (e) {
      dump("*** exception in validateLeafName: " + e + "\n");

      if (e.result == Cr.NS_ERROR_FILE_ACCESS_DENIED)
        throw e;

      if (aLocalFile.leafName == "" || aLocalFile.isDirectory()) {
        aLocalFile.append("unnamed");
        if (aLocalFile.exists())
          aLocalFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);
      }
    }
  },

  isUsableDirectory: function hald_isUsableDirectory(aDirectory) {
    return aDirectory.exists() && aDirectory.isDirectory() && aDirectory.isWritable();
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([HelperAppLauncherDialog]);
