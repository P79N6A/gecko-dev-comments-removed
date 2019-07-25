


































 
let EXPORTED_SYMBOLS = ["OfflineAppCacheHelper"];

const Cc = Components.classes;
const Ci = Components.interfaces;

let OfflineAppCacheHelper = {
  clear: function() {
    var cacheService = Cc["@mozilla.org/network/cache-service;1"].
                       getService(Ci.nsICacheService);
    try {
      cacheService.evictEntries(Ci.nsICache.STORE_OFFLINE);
    } catch(er) {}

    var storageManagerService = Cc["@mozilla.org/dom/storagemanager;1"].
                                getService(Ci.nsIDOMStorageManager);
    storageManagerService.clearOfflineApps();
  }
};
