




const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

const PREF_BD_USEDOWNLOADDIR = "browser.download.useDownloadDir";
const URI_GENERIC_ICON_DOWNLOAD = "drawable://alert_download";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Prompt.jsm");
Cu.import("resource://gre/modules/HelperApps.jsm");





function HelperAppLauncherDialog() { }

HelperAppLauncherDialog.prototype = {
  classID: Components.ID("{e9d277a0-268a-4ec2-bb8c-10fdf3e44611}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIHelperAppLauncherDialog]),

  show: function hald_show(aLauncher, aContext, aReason) {
    let bundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");

    let defaultHandler = new Object();
    let apps = HelperApps.getAppsForUri(aLauncher.source, {
      mimeType: aLauncher.MIMEInfo.MIMEType,
    });

    
    apps.unshift({
      name: bundle.GetStringFromName("helperapps.saveToDisk"),
      packageName: "org.mozilla.gecko.Download",
      iconUri: "drawable://icon",
      launch: function() {
        
        aLauncher.MIMEInfo.preferredAction = Ci.nsIMIMEInfo.saveToDisk;
        aLauncher.saveToDisk(null, false);
        return true;
      }
    });

    
    
    let preferredApp = this._getPreferredApp(aLauncher);
    if (preferredApp) {
      let pref = apps.filter(function(app) {
        return app.packageName === preferredApp;
      });

      if (pref.length > 0) {
        pref[0].launch(aLauncher.source);
        return;
      }
    }

    let callback = function(app) {
      aLauncher.MIMEInfo.preferredAction = Ci.nsIMIMEInfo.useHelperApp;
      if (!app.launch(aLauncher.source)) {
        aLauncher.cancel(Cr.NS_BINDING_ABORTED);
      }
    }

    if (apps.length > 1) {
      HelperApps.prompt(apps, {
        title: bundle.GetStringFromName("helperapps.pick"),
        buttons: [
          bundle.GetStringFromName("helperapps.alwaysUse"),
          bundle.GetStringFromName("helperapps.useJustOnce")
        ]
      }, (data) => {
        if (data.button < 0)
          return;

        callback(apps[data.icongrid0]);

        if (data.button == 0)
          this._setPreferredApp(aLauncher, apps[data.icongrid0]);
      });
    } else {
      callback(apps[0]);
    }
  },

  _getPrefName: function getPrefName(mimetype) {
    return "browser.download.preferred." + mimetype.replace("\\", ".");
  },

  _getMimeTypeFromLauncher: function getMimeTypeFromLauncher(launcher) {
    let mime = launcher.MIMEInfo.MIMEType;
    if (!mime)
      mime = ContentAreaUtils.getMIMETypeForURI(launcher.source) || "";
    return mime;
  },

  _getPreferredApp: function getPreferredApp(launcher) {
    let mime = this._getMimeTypeFromLauncher(launcher);
    if (!mime)
      return;

    try {
      return Services.prefs.getCharPref(this._getPrefName(mime));
    } catch(ex) {
      Services.console.logStringMessage("Error getting pref for " + mime + " " + ex);
    }
    return null;
  },

  _setPreferredApp: function setPreferredApp(launcher, app) {
    let mime = this._getMimeTypeFromLauncher(launcher);
    if (!mime)
      return;

    if (app)
      Services.prefs.setCharPref(this._getPrefName(mime), app.packageName);
    else
      Services.prefs.clearUserPref(this._getPrefName(mime));
  },

  promptForSaveToFile: function hald_promptForSaveToFile(aLauncher, aContext, aDefaultFile, aSuggestedFileExt, aForcePrompt) {
    
    let dnldMgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
    let defaultFolder = dnldMgr.userDownloadsDirectory;

    try {
      file = this.validateLeafName(defaultFolder, aDefaultFile, aSuggestedFileExt);
    } catch (e) { }

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
      
      
      
      
      let collisionCount = 0;
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
