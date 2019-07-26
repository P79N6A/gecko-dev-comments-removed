










"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadIntegration",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "env",
                                   "@mozilla.org/process/environment;1",
                                   "nsIEnvironment");
XPCOMUtils.defineLazyGetter(this, "gStringBundle", function() {
  return Services.strings.
    createBundle("chrome://mozapps/locale/downloads/downloads.properties");
});








this.DownloadIntegration = {
  
  testMode: false,

  





  getSystemDownloadsDirectory: function DI_getSystemDownloadsDirectory() {
    return Task.spawn(function() {
      if (this._downloadsDirectory) {
        
        
        
        yield;
        throw new Task.Result(this._downloadsDirectory);
      }

      let directory = null;
#ifdef XP_MACOSX
      directory = this._getDirectory("DfltDwnld");
#elifdef XP_WIN
      
      
      let version = parseFloat(Services.sysinfo.getProperty("version"));
      if (version < 6) {
        directory = yield this._createDownloadsDirectory("Pers");
      } else {
        directory = this._getDirectory("DfltDwnld");
      }
#elifdef XP_UNIX
#ifdef MOZ_PLATFORM_MAEMO
      
      
      
      
      
      directory = this._getDirectory("XDGDocs");
#elifdef ANDROID
      
      
      let directoryPath = env.get("DOWNLOADS_DIRECTORY");
      if (!directoryPath) {
        throw new Components.Exception("DOWNLOADS_DIRECTORY is not set.",
                                       Cr.NS_ERROR_FILE_UNRECOGNIZED_PATH);
      }
      directory = new FileUtils.File(directoryPath);
#else
      
      
      try {
        directory = this._getDirectory("DfltDwnld");
      } catch(e) {
        directory = yield this._createDownloadsDirectory("Home");
      }
#endif
#else
      directory = yield this._createDownloadsDirectory("Home");
#endif
      this._downloadsDirectory = directory;
      throw new Task.Result(this._downloadsDirectory);
    }.bind(this));
  },
  _downloadsDirectory: null,

  





  getUserDownloadsDirectory: function DI_getUserDownloadsDirectory() {
    return Task.spawn(function() {
      let directory = null;
      let prefValue = 1;

      try {
        prefValue = Services.prefs.getIntPref("browser.download.folderList");
      } catch(e) {}

      switch(prefValue) {
        case 0: 
          directory = this._getDirectory("Desk");
          break;
        case 1: 
          directory = yield this.getSystemDownloadsDirectory();
          break;
        case 2: 
          try {
            directory = Services.prefs.getComplexValue("browser.download.dir",
                                                       Ci.nsIFile);
            yield OS.File.makeDir(directory.path, { ignoreExisting: true });

          } catch(ex) {
            
            directory = yield this.getSystemDownloadsDirectory();
          }
          break;
        default:
          directory = yield this.getSystemDownloadsDirectory();
      }
      throw new Task.Result(directory);
    }.bind(this));
  },

  





  getTemporaryDownloadsDirectory: function DI_getTemporaryDownloadsDirectory() {
    return Task.spawn(function() {
      let directory = null;
#ifdef XP_MACOSX
      directory = yield this.getUserDownloadsDirectory();
#elifdef ANDROID
      directory = yield this.getSystemDownloadsDirectory();
#else
      
      
      if (this._isImmersiveProcess()) {
        directory = yield this.getSystemDownloadsDirectory();
      } else {
        directory = this._getDirectory("TmpD");
      }
#endif
      throw new Task.Result(directory);
    }.bind(this));
  },

  


  _isImmersiveProcess: function() {
    
    return false;
  },

  






  _createDownloadsDirectory: function DI_createDownloadsDirectory(aName) {
    let directory = this._getDirectory(aName);
    directory.append(gStringBundle.GetStringFromName("downloadsFolder"));

    
    return OS.File.makeDir(directory.path, { ignoreExisting: true }).
             then(function() {
               return directory;
             });
  },

  





  _getDirectory: function DI_getDirectory(aName) {
    return Services.dirsvc.get(this.testMode ? "TmpD" : aName, Ci.nsIFile);
  }
};
