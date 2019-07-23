





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

var HandlerServiceTest = {
  
  

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


  
  
  
  interfaces: [Ci.nsIDirectoryServiceProvider, Ci.nsISupports],

  QueryInterface: function HandlerServiceTest_QueryInterface(iid) {
    if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },


  
  

  getFile: function HandlerServiceTest_getFile(property, persistent) {
    this.log("getFile: requesting " + property);

    persistent.value = true;

    if (property == "UMimTyp") {
      var datasourceFile = this._dirSvc.get("CurProcD", Ci.nsIFile);
      datasourceFile.append("mimeTypes.rdf");
      return datasourceFile;
    }

    
    
    
    this.log("the following NS_ERROR_FAILURE exception in " +
             "nsIDirectoryServiceProvider::getFile is expected, " +
             "as we don't provide the '" + property + "' file");
    throw Cr.NS_ERROR_FAILURE;
  },


  
  

  



  getDatasourceFile: function HandlerServiceTest_getDatasourceFile() {
    var datasourceFile;

    try {
      datasourceFile = this._dirSvc.get("UMimTyp", Ci.nsIFile);
    }
    catch (e) {}

    if (!datasourceFile) {
      this._dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(this);
      datasourceFile = this._dirSvc.get("UMimTyp", Ci.nsIFile);
    }

    return datasourceFile;
  },

  deleteDatasourceFile: function HandlerServiceTest_deleteDatasourceFile() {
    try {
      var file = this.getDatasourceFile();
      if (file.exists())
        file.remove(false);
    }
    catch(ex) {
      Cu.reportError(ex);
    }
  },

  


  log: function HandlerServiceTest_log(message) {
    message = "*** HandlerServiceTest: " + message;
    this._consoleSvc.logStringMessage(message);
    print(message);
  }

};

HandlerServiceTest.getDatasourceFile();




var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                 getService(Ci.nsIPrefBranch);
prefBranch.setBoolPref("browser.contentHandling.log", true);
