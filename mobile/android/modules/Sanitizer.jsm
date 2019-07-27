




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/LoadContextInfo.jsm");
Cu.import("resource://gre/modules/FormHistory.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

function dump(a) {
  Services.console.logStringMessage(a);
}

this.EXPORTED_SYMBOLS = ["Sanitizer"];

let downloads = {
  dlmgr: Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager),

  iterate: function (aCallback) {
    let dlmgr = downloads.dlmgr;
    let dbConn = dlmgr.DBConnection;
    let stmt = dbConn.createStatement("SELECT id FROM moz_downloads WHERE " +
        "state = ? OR state = ? OR state = ? OR state = ? OR state = ? OR state = ?");
    stmt.bindByIndex(0, Ci.nsIDownloadManager.DOWNLOAD_FINISHED);
    stmt.bindByIndex(1, Ci.nsIDownloadManager.DOWNLOAD_FAILED);
    stmt.bindByIndex(2, Ci.nsIDownloadManager.DOWNLOAD_CANCELED);
    stmt.bindByIndex(3, Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_PARENTAL);
    stmt.bindByIndex(4, Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_POLICY);
    stmt.bindByIndex(5, Ci.nsIDownloadManager.DOWNLOAD_DIRTY);
    while (stmt.executeStep()) {
      aCallback(dlmgr.getDownload(stmt.row.id));
    }
    stmt.finalize();
  },

  get canClear() {
    return this.dlmgr.canCleanUp;
  }
};

function Sanitizer() {}
Sanitizer.prototype = {
  clearItem: function (aItemName)
  {
    let item = this.items[aItemName];
    let canClear = item.canClear;
    if (typeof canClear == "function") {
      canClear(function clearCallback(aCanClear) {
        if (aCanClear)
          item.clear();
      });
    } else if (canClear) {
      item.clear();
    }
  },

  items: {
    cache: {
      clear: function ()
      {
        return new Promise(function(resolve, reject) {
          var cache = Cc["@mozilla.org/netwerk/cache-storage-service;1"].getService(Ci.nsICacheStorageService);
          try {
            cache.clear();
          } catch(er) {}

          let imageCache = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                                                           .getImgCacheForDocument(null);
          try {
            imageCache.clearCache(false); 
          } catch(er) {}

          resolve();
        });
      },

      get canClear()
      {
        return true;
      }
    },

    cookies: {
      clear: function ()
      {
        return new Promise(function(resolve, reject) {
          Services.cookies.removeAll();
          resolve();
        });
      },

      get canClear()
      {
        return true;
      }
    },

    siteSettings: {
      clear: function ()
      {
        return new Promise(function(resolve, reject) {
          
          Services.perms.removeAll();

          
          Cc["@mozilla.org/content-pref/service;1"]
            .getService(Ci.nsIContentPrefService2)
            .removeAllDomains(null);

          
          
          var hosts = Services.logins.getAllDisabledHosts({})
          for (var host of hosts) {
            Services.logins.setLoginSavingEnabled(host, true);
          }

          resolve();
        });
      },

      get canClear()
      {
        return true;
      }
    },

    offlineApps: {
      clear: function ()
      {
        return new Promise(function(resolve, reject) {
          var cacheService = Cc["@mozilla.org/netwerk/cache-storage-service;1"].getService(Ci.nsICacheStorageService);
          var appCacheStorage = cacheService.appCacheStorage(LoadContextInfo.default, null);
          try {
            appCacheStorage.asyncEvictStorage(null);
          } catch(er) {}

          resolve();
        });
      },

      get canClear()
      {
          return true;
      }
    },

    history: {
      clear: function ()
      {
        return Messaging.sendRequestForResult({ type: "Sanitize:ClearHistory" })
          .catch() 
          .then(function() {
            try {
              Services.obs.notifyObservers(null, "browser:purge-session-history", "");
            }
            catch (e) { }

            try {
              var predictor = Cc["@mozilla.org/network/predictor;1"].getService(Ci.nsINetworkPredictor);
              predictor.reset();
            } catch (e) { }
          });
      },

      get canClear()
      {
        
        
        return true;
      }
    },

    formdata: {
      clear: function ()
      {
        return new Promise(function(resolve, reject) {
          FormHistory.update({ op: "remove" });
          resolve();
        });
      },

      canClear: function (aCallback)
      {
        let count = 0;
        let countDone = {
          handleResult: function(aResult) { count = aResult; },
          handleError: function(aError) { Cu.reportError(aError); },
          handleCompletion: function(aReason) { aCallback(aReason == 0 && count > 0); }
        };
        FormHistory.count({}, countDone);
      }
    },

    downloadFiles: {
      clear: function ()
      {
        return new Promise(function(resolve, reject) {
          downloads.iterate(function (dl) {
            
            let f = dl.targetFile;
            if (f.exists()) {
              f.remove(false);
            }

            
            dl.remove();
          });
          resolve();
        });
      },

      get canClear()
      {
        return downloads.canClear;
      }
    },

    passwords: {
      clear: function ()
      {
        return new Promise(function(resolve, reject) {
          Services.logins.removeAllLogins();
          resolve();
        });
      },

      get canClear()
      {
        let count = Services.logins.countLogins("", "", ""); 
        return (count > 0);
      }
    },

    sessions: {
      clear: function ()
      {
        return new Promise(function(resolve, reject) {
          
          var sdr = Cc["@mozilla.org/security/sdr;1"].getService(Ci.nsISecretDecoderRing);
          sdr.logoutAndTeardown();

          
          Services.obs.notifyObservers(null, "net:clear-active-logins", null);

          resolve();
        });
      },

      get canClear()
      {
        return true;
      }
    }
  }
};

this.Sanitizer = new Sanitizer();
