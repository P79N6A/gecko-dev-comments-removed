




















































function MockObjectRegisterer(aContractID, aReplacementCtor) {
  this._contractID = aContractID;
  this._replacementCtor = aReplacementCtor;
}

MockObjectRegisterer.prototype = {
  






  register: function MOR_register() {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    if (this._originalFactory)
      throw new Exception("Invalid object state when calling register()");

    
    var providedConstructor = this._replacementCtor;
    this._mockFactory = {
      createInstance: function MF_createInstance(aOuter, aIid) {
        if (aOuter != null)
          throw Components.results.NS_ERROR_NO_AGGREGATION;
        return new providedConstructor().QueryInterface(aIid);
      }
    };

    var componentRegistrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);

    
    this._cid = componentRegistrar.contractIDToCID(this._contractID);

    
    this._originalFactory = Components.manager.getClassObject(Components.classes[this._contractID],
                                                              Components.interfaces.nsIFactory);

    componentRegistrar.unregisterFactory(this._cid, this._originalFactory);

    componentRegistrar.registerFactory(this._cid,
                                       "",
                                       this._contractID,
                                       this._mockFactory);
  },

  


  unregister: function MOR_unregister() {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    if (!this._originalFactory)
      throw new Exception("Invalid object state when calling unregister()");

    
    var componentRegistrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    componentRegistrar.unregisterFactory(this._cid,
                                         this._mockFactory);

    
    componentRegistrar.registerFactory(this._cid,
                                       "",
                                       this._contractID,
                                       this._originalFactory);

    
    this._cid = null;
    this._originalFactory = null;
    this._mockFactory = null;
  },

  

  


  _originalFactory: null,

  


  _cid: null,

  


  _mockFactory: null
}
