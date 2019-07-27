















var EXPORTED_SYMBOLS = ['SpecialStorageUtils'];

Components.utils.import('resource://gre/modules/Services.jsm');

var SpecialStorageUtils = {
  createWrappedSpecialStorage: function (sandbox, swfUrl, privateBrowsing) {
    
    var uri = Services.io.newURI(swfUrl, null, null);
    var principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                              .getService(Components.interfaces.nsIScriptSecurityManager)
                              .getNoAppCodebasePrincipal(uri);
    var dsm = Components.classes["@mozilla.org/dom/localStorage-manager;1"]
                                .getService(Components.interfaces.nsIDOMStorageManager);
    var storage = dsm.createStorage(null, principal, privateBrowsing);

    
    
    
    var wrapper = Components.utils.cloneInto({
      getItem: function (key) {
        return storage.getItem(key);
      },
      setItem: function (key, value) {
        storage.setItem(key, value);
      },
      removeItem: function (key) {
        storage.removeItem(key);
      }
    }, sandbox, {cloneFunctions:true});
    return wrapper;
  }
};
