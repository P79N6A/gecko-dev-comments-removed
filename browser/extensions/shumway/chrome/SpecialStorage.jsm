















var EXPORTED_SYMBOLS = ['SpecialStorageUtils'];

Components.utils.import('resource://gre/modules/Services.jsm');
Components.utils.import('resource://gre/modules/Services.jsm');

var SpecialStorageUtils = {
  createWrappedSpecialStorage: function (sandbox, swfUrl, privateBrowsing) {
    function genPropDesc(value) {
      return {
        enumerable: true, configurable: true, writable: true, value: value
      };
    }

    
    var uri = Services.io.newURI(swfUrl, null, null);
    var principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                              .getService(Components.interfaces.nsIScriptSecurityManager)
                              .getNoAppCodebasePrincipal(uri);
    var dsm = Components.classes["@mozilla.org/dom/localStorage-manager;1"]
                                .getService(Components.interfaces.nsIDOMStorageManager);
    var storage = dsm.createStorage(null, principal, privateBrowsing);

    
    
    
    var wrapper = Components.utils.createObjectIn(sandbox);
    Object.defineProperties(wrapper, {
      getItem: genPropDesc(function (key) {
        return storage.getItem(key);
      }),
      setItem: genPropDesc(function (key, value) {
        storage.setItem(key, value);
      }),
      removeItem: genPropDesc(function (key) {
        storage.removeItem(key);
      })
    });
    Components.utils.makeObjectPropsNormal(wrapper);
    return wrapper;
  }
};
