





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

var HandlerServiceTest = {
  
  

  __dirSvc: null,
  get _dirSvc() {
    if (!this.__dirSvc)
      this.__dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                      getService(Ci.nsIProperties).
                      QueryInterface(Ci.nsIDirectoryService);
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


  
  
  
  init: function HandlerServiceTest_init() {
    
    
    try        { this._dirSvc.get("UMimTyp", Ci.nsIFile) }
    catch (ex) { this._dirSvc.registerProvider(this) }

    
    
    
    this._deleteDatasourceFile();

    
    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    prefBranch.setBoolPref("browser.contentHandling.log", true);
  },

  destroy: function HandlerServiceTest_destroy() {
    
    
    this._deleteDatasourceFile();
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


  
  

  


  _deleteDatasourceFile: function HandlerServiceTest__deleteDatasourceFile() {
    var file = this._dirSvc.get("UMimTyp", Ci.nsIFile);
    if (file.exists())
      file.remove(false);
  },

  







  getDatasourceContents: function HandlerServiceTest_getDatasourceContents() {
    var rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
  
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    var fileHandler = ioService.getProtocolHandler("file").
                      QueryInterface(Ci.nsIFileProtocolHandler);
    var fileURL = fileHandler.getURLSpecFromFile(this.getDatasourceFile());
    var ds = rdf.GetDataSourceBlocking(fileURL);
  
    var outputStream = {
      data: "",
      close: function() {},
      flush: function() {},
      write: function (buffer,count) {
        this.data += buffer;
        return count;
      },
      writeFrom: function (stream,count) {},
      isNonBlocking: false
    };
  
    ds.QueryInterface(Components.interfaces.nsIRDFXMLSource);
    ds.Serialize(outputStream);
  
    return outputStream.data;
  },

  


  log: function HandlerServiceTest_log(message) {
    message = "*** HandlerServiceTest: " + message;
    this._consoleSvc.logStringMessage(message);
    print(message);
  }

};

HandlerServiceTest.init();
