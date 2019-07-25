














































































































var EXPORTED_SYMBOLS = [ "XPCOMUtils" ];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

var XPCOMUtils = {
  





  generateQI: function XPCU_generateQI(interfaces) {
    
    return makeQI([Ci[i].name for each (i in interfaces) if (Ci[i])]);
  },

  


  generateNSGetFactory: function XPCU_generateNSGetFactory(componentsArray) {
    let classes = {};
    for each (let component in componentsArray) {
        if (!(component.prototype.classID instanceof Components.ID))
          throw Error("In generateNSGetFactory, classID missing or incorrect for component " + component);

        classes[component.prototype.classID] = this._getFactory(component);
    }
    return function NSGetFactory(cid) {
      let cidstring = cid.toString();
      if (cidstring in classes)
        return classes[cidstring];
      throw Cr.NS_ERROR_FACTORY_NOT_REGISTERED;
    }
  },

  










  defineLazyGetter: function XPCU_defineLazyGetter(aObject, aName, aLambda)
  {
    aObject.__defineGetter__(aName, function() {
      delete aObject[aName];
      return aObject[aName] = aLambda.apply(aObject);
    });
  },

  












  defineLazyServiceGetter: function XPCU_defineLazyServiceGetter(aObject, aName,
                                                                 aContract,
                                                                 aInterfaceName)
  {
    this.defineLazyGetter(aObject, aName, function XPCU_serviceLambda() {
      return Cc[aContract].getService(Ci[aInterfaceName]);
    });
  },

  













  defineLazyModuleGetter: function XPCU_defineLazyModuleGetter(aObject, aName,
                                                               aResource,
                                                               aSymbol)
  {
    this.defineLazyGetter(aObject, aName, function XPCU_moduleLambda() {
      var temp = {};
      Cu.import(aResource, temp);
      return temp[aSymbol || aName];
    });
  },

  


  get categoryManager() {
    return Components.classes["@mozilla.org/categorymanager;1"]
           .getService(Ci.nsICategoryManager);
  },

  




  IterSimpleEnumerator: function XPCU_IterSimpleEnumerator(e, i)
  {
    while (e.hasMoreElements())
      yield e.getNext().QueryInterface(i);
  },

  




  IterStringEnumerator: function XPCU_IterStringEnumerator(e)
  {
    while (e.hasMore())
      yield e.getNext();
  },

  


  _getFactory: function XPCOMUtils__getFactory(component) {
    var factory = component.prototype._xpcom_factory;
    if (!factory) {
      factory = {
        createInstance: function(outer, iid) {
          if (outer)
            throw Cr.NS_ERROR_NO_AGGREGATION;
          return (new component()).QueryInterface(iid);
        }
      }
    }
    return factory;
  },

  



  importRelative: function XPCOMUtils__importRelative(that, path) {
    if (!("__URI__" in that))
      throw Error("importRelative may only be used from a JSM, and its first argument "+
                  "must be that JSM's global object (hint: use this)");
    let uri = that.__URI__;
    let i = uri.lastIndexOf("/");
    Components.utils.import(uri.substring(0, i+1) + path, that);
  },
};




function makeQI(interfaceNames) {
  return function XPCOMUtils_QueryInterface(iid) {
    if (iid.equals(Ci.nsISupports))
      return this;
    for each(let interfaceName in interfaceNames) {
      if (Ci[interfaceName].equals(iid))
        return this;
    }

    throw Cr.NS_ERROR_NO_INTERFACE;
  };
}

