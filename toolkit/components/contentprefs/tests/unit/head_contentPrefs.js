





const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/ContentPrefInstance.jsm');

const CONTENT_PREFS_DB_FILENAME = "content-prefs.sqlite";
const CONTENT_PREFS_BACKUP_DB_FILENAME = "content-prefs.sqlite.corrupt";

var ContentPrefTest = {
  
  

  __dirSvc: null,
  get _dirSvc() {
    if (!this.__dirSvc)
      this.__dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                      getService(Ci.nsIProperties);
    return this.__dirSvc;
  },

  __consoleSvc: null,
  get _consoleSvc() {
    if (!this.__consoleSvc)
      this.__consoleSvc = Cc["@mozilla.org/consoleservice;1"].
                          getService(Ci.nsIConsoleService);
    return this.__consoleSvc;
  },

  __ioSvc: null,
  get _ioSvc() {
    if (!this.__ioSvc)
      this.__ioSvc = Cc["@mozilla.org/network/io-service;1"].
                     getService(Ci.nsIIOService);
    return this.__ioSvc;
  },


  
  
  
  interfaces: [Ci.nsIDirectoryServiceProvider, Ci.nsISupports],

  QueryInterface: function ContentPrefTest_QueryInterface(iid) {
    if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },


  
  

  getFile: function ContentPrefTest_getFile(property, persistent) {
    persistent.value = true;

    if (property == "ProfD")
      return this._dirSvc.get("CurProcD", Ci.nsIFile);

    
    
    
    throw Cr.NS_ERROR_FAILURE;
  },


  
  

  getURI: function ContentPrefTest_getURI(spec) {
    return this._ioSvc.newURI(spec, null, null);
  },

  


  getProfileDir: function ContentPrefTest_getProfileDir() {
    
    if (runningInParent) {
      return do_get_profile();
    }
    
    
    let env = Components.classes["@mozilla.org/process/environment;1"]
                        .getService(Components.interfaces.nsIEnvironment);
    
    let profd = env.get("XPCSHELL_TEST_PROFILE_DIR");
    let file = Components.classes["@mozilla.org/file/local;1"]
                         .createInstance(Components.interfaces.nsILocalFile);
    file.initWithPath(profd);
    return file;
  },

  




  deleteDatabase: function ContentPrefTest_deleteDatabase() {
    var file = this.getProfileDir();
    file.append(CONTENT_PREFS_DB_FILENAME);
    if (file.exists())
      try { file.remove(false); } catch(e) {  }
    return file;
  },

  



  deleteBackupDatabase: function ContentPrefTest_deleteBackupDatabase() {
    var file = this.getProfileDir();
    file.append(CONTENT_PREFS_BACKUP_DB_FILENAME);
    if (file.exists())
      file.remove(false);
    return file;
  },

  


  log: function ContentPrefTest_log(message) {
    message = "*** ContentPrefTest: " + message;
    this._consoleSvc.logStringMessage(message);
    print(message);
  }

};

let gInPrivateBrowsing = false;
function enterPBMode() {
  gInPrivateBrowsing = true;
}
function exitPBMode() {
  gInPrivateBrowsing = false;
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
}

ContentPrefTest.deleteDatabase();

function inChildProcess() {
  var appInfo = Cc["@mozilla.org/xre/app-info;1"];
  if (!appInfo || appInfo.getService(Ci.nsIXULRuntime).processType ==
      Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
    return false;
  }
  return true;
}




if (!inChildProcess()) {
  var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.preferences.content.log", true);
}

do_register_cleanup(function tail_contentPrefs() {
  ContentPrefTest.deleteDatabase();
  ContentPrefTest.__dirSvc = null;
});
