




















































netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

if (Cc === undefined) { 
  var Cc = Components.classes;
}

if (Ci === undefined) {
  var Ci = Components.interfaces;
}

if (Cu === undefined) {
  var Cu = Components.utils;
}

if (Cr === undefined) {
  var Cr = Components.results;
}

if (Cm === undefined) {
  var Cm = Components.manager;
}

function MockObjectRegisterer(aContractID, aReplacementCtor)
{
  this._contractID = aContractID;
  this._replacementCtor = aReplacementCtor;
}

MockObjectRegisterer.prototype = {
  






  register: function MOR_register() {
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    if (this._originalCID)
      throw new Exception("Invalid object state when calling register()");

    
    var providedConstructor = this._replacementCtor;
    this._mockFactory = {
      createInstance: function MF_createInstance(aOuter, aIid) {
        if (aOuter != null)
          throw Cr.NS_ERROR_NO_AGGREGATION;
        return new providedConstructor().QueryInterface(aIid);
      }
    };

    this._cid = Components.classes["@mozilla.org/uuid-generator;1"].
      getService(Components.interfaces.nsIUUIDGenerator).generateUUID();

    
    var componentRegistrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    this._originalCID = componentRegistrar.contractIDToCID(this._contractID);

    
    componentRegistrar.registerFactory(this._cid,
                                       "",
                                       this._contractID,
                                       this._mockFactory);
  },

  


  unregister: function MOR_unregister() {
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    if (!this._originalCID)
      throw new Exception("Invalid object state when calling unregister()");

    
    var componentRegistrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    componentRegistrar.unregisterFactory(this._cid,
                                         this._mockFactory);

    
    componentRegistrar.registerFactory(this._originalCID,
                                       "",
                                       this._contractID,
                                       null);

    
    this._cid = null;
    this._originalCID = null;
    this._mockFactory = null;
  },

  

  


  _originalCID: null,

  


  _cid: null,

  


  _mockFactory: null
}
