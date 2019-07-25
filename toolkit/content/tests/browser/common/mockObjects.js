




















































function MockObjectRegisterer(aContractID, aReplacementCtor) {
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
          throw Components.results.NS_ERROR_NO_AGGREGATION;
        return new providedConstructor().QueryInterface(aIid);
      }
    };

    var retVal = SpecialPowers.swapFactoryRegistration(this._cid, this._contractID, this._mockFactory, this._originalFactory);
    if ('error' in retVal) {
      throw new Exception("ERROR: " + retVal.error);
    } else {
      this._cid = retVal.cid;
      this._originalFactory = retVal.originalFactory;
    }
  },

  


  unregister: function MOR_unregister() {
    if (!this._originalFactory)
      throw new Exception("Invalid object state when calling unregister()");

    
    SpecialPowers.swapFactoryRegistration(this._cid, this._contractID, this._mockFactory, this._originalFactory);

    
    this._cid = null;
    this._originalFactory = null;
    this._mockFactory = null;
  },

  

  


  _originalFactory: null,

  


  _cid: null,

  


  _mockFactory: null
}
