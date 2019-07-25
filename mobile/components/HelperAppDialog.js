



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const PREF_BD_USEDOWNLOADDIR = "browser.download.useDownloadDir";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");





function HelperAppLauncherDialog() { }

HelperAppLauncherDialog.prototype = {
  classID: Components.ID("{e9d277a0-268a-4ec2-bb8c-10fdf3e44611}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIHelperAppLauncherDialog]),

  show: function hald_show(aLauncher, aContext, aReason) {
    const NS_BINDING_ABORTED = 0x804b0002;
 
    let window = aContext.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowInternal);

    let bundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");

    let flags = (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_0) +
                (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_1);

    let title = bundle.GetStringFromName("helperApp.title");
    let message = bundle.GetStringFromName("helperApp.prompt");
    message += "\n  " + aLauncher.suggestedFileName;
    
    let type = aLauncher.MIMEInfo.description;
    if (type == "") {
      try {
        type = aLauncher.MIMEInfo.primaryExtension.toUpperCase();
      } catch (e) {
        type = aLauncher.MIMEInfo.MIMEType;
      }
    }
    message += "\n  " + type;

    let open = bundle.GetStringFromName("helperApp.open");
    let save = bundle.GetStringFromName("helperApp.save");
    let nothing = bundle.GetStringFromName("helperApp.nothing");

    
    if (aLauncher.MIMEInfo.hasDefaultHandler) {
      flags += (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_2);

      let choice = Services.prompt.confirmEx(window,
                                             title, message,
                                             flags, save, open, nothing,
                                             null, {});

      if (choice == 0) {
        aLauncher.saveToDisk(null, false);
      }
      else if (choice == 1) {
        aLauncher.MIMEInfo.preferredAction = Ci.nsIMIMEInfo.useSystemDefault;
        aLauncher.launchWithApplication(null, false);
      }
      else {
        try {
          aLauncher.cancel(NS_BINDING_ABORTED);
        } catch(e) {} 
      }
    } else {
      let choice = Services.prompt.confirmEx(window,
                                             title, message,
                                             flags, save, nothing, null,
                                             null, {});

      if (choice == 0) {
        aLauncher.saveToDisk(null, false);
      }
      else {
        try {
          aLauncher.cancel(NS_BINDING_ABORTED);
        } catch(e) {} 
      }
    }
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

const NSGetFactory = XPCOMUtils.generateNSGetFactory([HelperAppLauncherDialog]);
