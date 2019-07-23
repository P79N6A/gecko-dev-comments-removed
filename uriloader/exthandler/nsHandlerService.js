



































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;


const CLASS_MIMEINFO        = "mimetype";
const CLASS_PROTOCOLINFO    = "scheme";



const NC_NS                 = "http://home.netscape.com/NC-rdf#";



const NC_MIME_TYPES         = NC_NS + "MIME-types";
const NC_PROTOCOL_SCHEMES   = NC_NS + "Protocol-Schemes";




const NC_VALUE              = NC_NS + "value";


const NC_HANDLER_INFO       = NC_NS + "handlerProp";




const NC_SAVE_TO_DISK       = NC_NS + "saveToDisk";
const NC_HANDLE_INTERNALLY  = NC_NS + "handleInternal";
const NC_USE_SYSTEM_DEFAULT = NC_NS + "useSystemDefault";


const NC_ALWAYS_ASK         = NC_NS + "alwaysAsk";


const NC_PREFERRED_APP      = NC_NS + "externalApplication";




const NC_PRETTY_NAME        = NC_NS + "prettyName";


const NC_PATH               = NC_NS + "path";


const NC_URI_TEMPLATE       = NC_NS + "uriTemplate";


Cu.import("resource://gre/modules/XPCOMUtils.jsm");


function HandlerService() {}

HandlerService.prototype = {
  
  

  classDescription: "Handler Service",
  classID:          Components.ID("{32314cc8-22f7-4f7f-a645-1a45453ba6a6}"),
  contractID:       "@mozilla.org/uriloader/handler-service;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIHandlerService]),


  
  

  enumerate: function HS_enumerate() {
    var handlers = Cc["@mozilla.org/array;1"].
                   createInstance(Ci.nsIMutableArray);
    this._appendHandlers(handlers, CLASS_MIMEINFO);
    this._appendHandlers(handlers, CLASS_PROTOCOLINFO);
    return handlers.enumerate();
  },

  store: function HS_store(aHandlerInfo) {
    
    
    
    

    this._ensureRecordsForType(aHandlerInfo);
    this._storePreferredAction(aHandlerInfo);
    this._storePreferredHandler(aHandlerInfo);
    this._storeAlwaysAsk(aHandlerInfo);

    
    
    if (this._ds instanceof Ci.nsIRDFRemoteDataSource)
      this._ds.Flush();
  },

  remove: function HS_remove(aHandlerInfo) {
    var preferredHandlerID = this._getPreferredHandlerID(aHandlerInfo);
    this._removeAssertions(preferredHandlerID);

    var infoID = this._getInfoID(aHandlerInfo);
    this._removeAssertions(infoID);

    var typeID = this._getTypeID(aHandlerInfo);
    this._removeAssertions(typeID);

    
    
    var typeList = this._ensureAndGetTypeList(this._getClass(aHandlerInfo));
    var type = this._rdf.GetResource(typeID);
    var typeIndex = typeList.IndexOf(type);
    if (typeIndex != -1)
      typeList.RemoveElementAt(typeIndex, true);

    
    
    
    
    if (this._ds instanceof Ci.nsIRDFRemoteDataSource)
      this._ds.Flush();
  },


  
  

  _storePreferredAction: function HS__storePreferredAction(aHandlerInfo) {
    var infoID = this._getInfoID(aHandlerInfo);

    switch(aHandlerInfo.preferredAction) {
      case Ci.nsIHandlerInfo.saveToDisk:
        this._setLiteral(infoID, NC_SAVE_TO_DISK, "true");
        this._removeValue(infoID, NC_HANDLE_INTERNALLY);
        this._removeValue(infoID, NC_USE_SYSTEM_DEFAULT);
        break;

      case Ci.nsIHandlerInfo.handleInternally:
        this._setLiteral(infoID, NC_HANDLE_INTERNALLY, "true");
        this._removeValue(infoID, NC_SAVE_TO_DISK);
        this._removeValue(infoID, NC_USE_SYSTEM_DEFAULT);
        break;

      case Ci.nsIHandlerInfo.useSystemDefault:
        this._setLiteral(infoID, NC_USE_SYSTEM_DEFAULT, "true");
        this._removeValue(infoID, NC_SAVE_TO_DISK);
        this._removeValue(infoID, NC_HANDLE_INTERNALLY);
        break;

      
      
      
      
      case Ci.nsIHandlerInfo.useHelperApp:
      default:
        this._removeValue(infoID, NC_SAVE_TO_DISK);
        this._removeValue(infoID, NC_HANDLE_INTERNALLY);
        this._removeValue(infoID, NC_USE_SYSTEM_DEFAULT);
        break;
    }
  },

  _storePreferredHandler: function HS__storePreferredHandler(aHandlerInfo) {
    var infoID = this._getInfoID(aHandlerInfo);
    var handlerID = this._getPreferredHandlerID(aHandlerInfo);
    var handler = aHandlerInfo.preferredApplicationHandler;

    if (handler) {
      
      
      
      this._setLiteral(handlerID, NC_PRETTY_NAME, handler.name);
      if (handler instanceof Ci.nsILocalHandlerApp) {
        this._setLiteral(handlerID, NC_PATH, handler.executable.path);
        this._removeValue(handlerID, NC_URI_TEMPLATE);
      }
      else {
        handler.QueryInterface(Ci.nsIWebHandlerApp);
        this._setLiteral(handlerID, NC_URI_TEMPLATE, handler.uriTemplate);
        this._removeValue(handlerID, NC_PATH);
      }

      
      
      
      this._setResource(infoID, NC_PREFERRED_APP, handlerID);
    }
    else {
      
      
      this._removeValue(handlerID, NC_PRETTY_NAME);
      this._removeValue(handlerID, NC_PATH);
      this._removeValue(handlerID, NC_URI_TEMPLATE);
      this._removeValue(infoID, NC_PREFERRED_APP);
    }
  },

  _storeAlwaysAsk: function HS__storeAlwaysAsk(aHandlerInfo) {
    var infoID = this._getInfoID(aHandlerInfo);
    this._setLiteral(infoID,
                     NC_ALWAYS_ASK,
                     aHandlerInfo.alwaysAskBeforeHandling ? "true" : "false");
  },


  
  

  
  __mimeSvc: null,
  get _mimeSvc() {
    if (!this.__mimeSvc)
      this.__mimeSvc =
        Cc["@mozilla.org/uriloader/external-helper-app-service;1"].
        getService(Ci.nsIMIMEService);
    return this.__mimeSvc;
  },

  
  __protocolSvc: null,
  get _protocolSvc() {
    if (!this.__protocolSvc)
      this.__protocolSvc =
        Cc["@mozilla.org/uriloader/external-protocol-service;1"].
        getService(Ci.nsIExternalProtocolService);
    return this.__protocolSvc;
  },

  
  __rdf: null,
  get _rdf() {
    if (!this.__rdf)
      this.__rdf = Cc["@mozilla.org/rdf/rdf-service;1"].
                   getService(Ci.nsIRDFService);
    return this.__rdf;
  },

  
  __containerUtils: null,
  get _containerUtils() {
    if (!this.__containerUtils)
      this.__containerUtils = Cc["@mozilla.org/rdf/container-utils;1"].
                              getService(Ci.nsIRDFContainerUtils);
    return this.__containerUtils;
  },

  
  __ds: null,
  get _ds() {
    if (!this.__ds) {
      var fileLocator = Cc["@mozilla.org/file/directory_service;1"].
                        getService(Ci.nsIProperties);
      var file = fileLocator.get("UMimTyp", Ci.nsIFile);
      
      var ioService = Cc["@mozilla.org/network/io-service;1"].
                      getService(Ci.nsIIOService);
      var fileHandler = ioService.getProtocolHandler("file").
                        QueryInterface(Ci.nsIFileProtocolHandler);
      this.__ds =
        this._rdf.GetDataSourceBlocking(fileHandler.getURLSpecFromFile(file));
    }

    return this.__ds;
  },


  
  

  







  _getClass: function HS__getClass(aHandlerInfo) {
    if (aHandlerInfo instanceof Ci.nsIMIMEInfo)
      return CLASS_MIMEINFO;
    else
      return CLASS_PROTOCOLINFO;
  },

  











  _getTypeID: function HS__getTypeID(aHandlerInfo) {
    return "urn:" + this._getClass(aHandlerInfo) + ":" + aHandlerInfo.type;
  },

  














  _getInfoID: function HS__getInfoID(aHandlerInfo) {
    return "urn:" + this._getClass(aHandlerInfo) + ":handler:" +
           aHandlerInfo.type;
  },

  



















  _getPreferredHandlerID: function HS__getPreferredHandlerID(aHandlerInfo) {
    return "urn:" + this._getClass(aHandlerInfo) + ":externalApplication:" +
           aHandlerInfo.type;
  },

  











  _ensureAndGetTypeList: function HS__ensureAndGetTypeList(aClass) {
    
    
    

    var source = this._rdf.GetResource("urn:" + aClass + "s");
    var property =
      this._rdf.GetResource(aClass == CLASS_MIMEINFO ? NC_MIME_TYPES
                                                     : NC_PROTOCOL_SCHEMES);
    var target = this._rdf.GetResource("urn:" + aClass + "s:root");

    
    if (!this._ds.HasAssertion(source, property, target, true))
      this._ds.Assert(source, property, target, true);

    
    if (!this._containerUtils.IsContainer(this._ds, target))
      this._containerUtils.MakeSeq(this._ds, target);

    
    var typeList = Cc["@mozilla.org/rdf/container;1"].
                   createInstance(Ci.nsIRDFContainer);
    typeList.Init(this._ds, target);

    return typeList;
  },

  











  _ensureRecordsForType: function HS__ensureRecordsForType(aHandlerInfo) {
    
    var typeList = this._ensureAndGetTypeList(this._getClass(aHandlerInfo));

    
    
    var typeID = this._getTypeID(aHandlerInfo);
    var type = this._rdf.GetResource(typeID);
    if (typeList.IndexOf(type) != -1)
      return;

    
    typeList.AppendElement(type);
    this._setLiteral(typeID, NC_VALUE, aHandlerInfo.type);
    
    
    var infoID = this._getInfoID(aHandlerInfo);
    this._setLiteral(infoID, NC_ALWAYS_ASK, "false");
    this._setResource(typeID, NC_HANDLER_INFO, infoID);
    
    
    
    
    
    
    
    
    
    var preferredHandlerID = this._getPreferredHandlerID(aHandlerInfo);
    this._setLiteral(preferredHandlerID, NC_PATH, "");
    this._setResource(infoID, NC_PREFERRED_APP, preferredHandlerID);
  },

  






  _appendHandlers: function HS__appendHandlers(aHandlers, aClass) {
    var typeList = this._ensureAndGetTypeList(aClass);
    var enumerator = typeList.GetElements();

    while (enumerator.hasMoreElements()) {
      var element = enumerator.getNext();
      
      
      
      
      
      if (!(element instanceof Ci.nsIRDFResource))
        continue;

      
      
      var type = this._getValue(element.ValueUTF8, NC_VALUE);
      if (!type)
        continue;

      var handler;
      if (typeList.Resource.Value == "urn:mimetypes:root")
        handler = this._mimeSvc.getFromTypeAndExtension(type, null);
      else
        handler = this._protocolSvc.getProtocolHandlerInfo(type);

      aHandlers.appendElement(handler, false);
    }
  },

  






  _getValue: function HS__getValue(sourceURI, propertyURI) {
    var source = this._rdf.GetResource(sourceURI);
    var property = this._rdf.GetResource(propertyURI);

    var target = this._ds.GetTarget(source, property, true);

    if (!target)
      return null;
    
    if (target instanceof Ci.nsIRDFResource)
      return target.ValueUTF8;

    if (target instanceof Ci.nsIRDFLiteral)
      return target.Value;

    return null;
  },

  






  _setLiteral: function HS__setLiteral(sourceURI, propertyURI, value) {
    var source = this._rdf.GetResource(sourceURI);
    var property = this._rdf.GetResource(propertyURI);
    var target = this._rdf.GetLiteral(value);
    
    this._setTarget(source, property, target);
  },

  






  _setResource: function HS__setResource(sourceURI, propertyURI, resourceURI) {
    var source = this._rdf.GetResource(sourceURI);
    var property = this._rdf.GetResource(propertyURI);
    var target = this._rdf.GetResource(resourceURI);
    
    this._setTarget(source, property, target);
  },

  








  _setTarget: function HS__setTarget(source, property, target) {
    if (this._ds.hasArcOut(source, property)) {
      var oldTarget = this._ds.GetTarget(source, property, true);
      this._ds.Change(source, property, oldTarget, target);
    }
    else
      this._ds.Assert(source, property, target, true);
  },

  





  _removeValue: function HS__removeValue(sourceURI, propertyURI) {
    var source = this._rdf.GetResource(sourceURI);
    var property = this._rdf.GetResource(propertyURI);

    if (this._ds.hasArcOut(source, property)) {
      var target = this._ds.GetTarget(source, property, true);
      this._ds.Unassert(source, property, target);
    }
  },

 








  _removeAssertions: function HS__removeAssertions(sourceURI) {
    var source = this._rdf.GetResource(sourceURI);
    var properties = this._ds.ArcLabelsOut(source);
 
    while (properties.hasMoreElements()) {
      var property = properties.getNext();
      var target = this._ds.GetTarget(source, property, true);
      this._ds.Unassert(source, property, target);
    }
  },


  
  

  
  

  






  _getAppPref: function _getAppPref(aPrefName, aDefaultValue) {
    try {
      var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch);
      switch (prefBranch.getPrefType(aPrefName)) {
        case prefBranch.PREF_STRING:
          return prefBranch.getCharPref(aPrefName);

        case prefBranch.PREF_INT:
          return prefBranch.getIntPref(aPrefName);

        case prefBranch.PREF_BOOL:
          return prefBranch.getBoolPref(aPrefName);
      }
    }
    catch (ex) {  }
    
    return aDefaultValue;
  },

  
  __consoleSvc: null,
  get _consoleSvc() {
    if (!this.__consoleSvc)
      this.__consoleSvc = Cc["@mozilla.org/consoleservice;1"].
                          getService(Ci.nsIConsoleService);
    return this.__consoleSvc;
  },

  _log: function _log(aMessage) {
    if (!this._getAppPref("browser.contentHandling.log", false))
      return;

    aMessage = "*** HandlerService: " + aMessage;
    dump(aMessage + "\n");
    this._consoleSvc.logStringMessage(aMessage);
  }
};





function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([HandlerService]);
}
