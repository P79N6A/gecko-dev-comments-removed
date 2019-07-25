



































const Cc = Components.classes;
const Ci = Components.interfaces;

const PREF_BD_USEDOWNLOADDIR = "browser.download.useDownloadDir";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





function HelperAppLauncherDialog() { }

HelperAppLauncherDialog.prototype = {
  classDescription: "HelperApp Launcher Dialog",
  contractID: "@mozilla.org/helperapplauncherdialog;1",
  classID: Components.ID("{e9d277a0-268a-4ec2-bb8c-10fdf3e44611}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIHelperAppLauncherDialog]),

  show: function hald_show(aLauncher, aContext, aReason) {
    this._launcher = aLauncher;
    this._context  = aContext;

    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
    let browser = wm.getMostRecentWindow("navigator:browser");
    browser.HelperAppDialog.show(aLauncher);
  },

  promptForSaveToFile: function hald_promptForSaveToFile(aLauncher, aContext, aDefaultFile, aSuggestedFileExt, aForcePrompt) {
    let file = null;

    let prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

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

    
    let picker = Components.classes["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let windowTitle = "";
    let parent = aContext.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowInternal);
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
      aLocalFile.create(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0600);
    }
    catch (e) {
      dump("*** exception in validateLeafName: " + e + "\n");

      if (e.result == Components.results.NS_ERROR_FILE_ACCESS_DENIED)
        throw e;

      if (aLocalFile.leafName == "" || aLocalFile.isDirectory()) {
        aLocalFile.append("unnamed");
        if (aLocalFile.exists())
          aLocalFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0600);
      }
    }
  },

  isUsableDirectory: function hald_isUsableDirectory(aDirectory) {
    return aDirectory.exists() && aDirectory.isDirectory() && aDirectory.isWritable();
  }
};








var module = {
  firstTime: true,

  registerSelf: function(compMgr, fileSpec, location, type) {
    if (this.firstTime) {
      this.firstTime = false;
      throw Components.results.NS_ERROR_FACTORY_REGISTER_AGAIN;
    }
    compMgr = compMgr.QueryInterface(Ci.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(this.classID, this.classDescription, this.contractID, fileSpec, location, type);
  },

  getClassObject: function(compMgr, cid, iid) {
    if (!cid.equals(this.classID))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    if (!iid.equals(Ci.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return this.factory;
  },

  classDescription: "HelperApp Launcher Dialog",
  contractID: "@mozilla.org/helperapplauncherdialog;1",
  classID: Components.ID("{e9d277a0-268a-4ec2-bb8c-10fdf3e44611}"),

  factory: {
    createInstance: function(outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return (new HelperAppLauncherDialog()).QueryInterface(iid);
    }
  },

  canUnload: function(compMgr) {
    return true;
  }
};

function NSGetModule(compMgr, fileSpec) {
  return module;
}
