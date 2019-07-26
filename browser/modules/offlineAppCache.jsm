


 
this.EXPORTED_SYMBOLS = ["OfflineAppCacheHelper"];

Components.utils.import('resource://gre/modules/LoadContextInfo.jsm');

const Cc = Components.classes;
const Ci = Components.interfaces;

this.OfflineAppCacheHelper = {
  clear: function() {
    var cacheService = Cc["@mozilla.org/netwerk/cache-storage-service;1"].getService(Ci.nsICacheStorageService);
    var appCacheStorage = cacheService.appCacheStorage(LoadContextInfo.default, null);
    try {
      appCacheStorage.asyncEvictStorage(null);
    } catch(er) {}
  }
};
