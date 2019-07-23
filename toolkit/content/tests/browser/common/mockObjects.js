



















































function MockObjectRegisterer(aContractID, aReplacementCtor)
{
  this._contractID = aContractID;
  this._replacementCtor = aReplacementCtor;
}

MockObjectRegisterer.prototype = {
  






  register: function MOR_register() {
    if (this._originalFactory)
      throw new Exception("Invalid object state when calling register()");

    
    var providedConstructor = this._replacementCtor;
    this._mockFactory = {
      createInstance: function MF_createInstance(aOuter, aIid) {
        if (aOuter != null)
          throw Cr.NS_ERROR_NO_AGGREGATION;
        return new providedConstructor().QueryInterface(aIid);
      }
    };

    
    this._originalFactory = Cm.getClassObjectByContractID(this._contractID,
                                                          Ci.nsIFactory);

    
    var classInfo = this._originalFactory.QueryInterface(Ci.nsIClassInfo);
    var componentRegistrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    componentRegistrar.registerFactory(classInfo.classID,
                                       "Mock " + classInfo.classDescription,
                                       this._contractID,
                                       this._mockFactory);
  },

  


  unregister: function MOR_unregister() {
    if (!this._originalFactory)
      throw new Exception("Invalid object state when calling unregister()");

    
    var classInfo = this._originalFactory.QueryInterface(Ci.nsIClassInfo);
    var componentRegistrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    componentRegistrar.unregisterFactory(classInfo.classID,
                                         this._mockFactory);

    
    componentRegistrar.registerFactory(classInfo.classID,
                                       classInfo.classDescription,
                                       this._contractID,
                                       this._originalFactory);

    
    this._originalFactory = null;
    this._mockFactory = null;
  },

  

  



  _originalFactory: null,

  


  _mockFactory: null
}
