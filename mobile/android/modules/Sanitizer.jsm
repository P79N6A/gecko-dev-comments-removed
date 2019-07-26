




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
    stmt.bindInt32Parameter(0, Ci.nsIDownloadManager.DOWNLOAD_FINISHED);
    stmt.bindInt32Parameter(1, Ci.nsIDownloadManager.DOWNLOAD_FAILED);
    stmt.bindInt32Parameter(2, Ci.nsIDownloadManager.DOWNLOAD_CANCELED);
    stmt.bindInt32Parameter(3, Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_PARENTAL);
    stmt.bindInt32Parameter(4, Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_POLICY);
    stmt.bindInt32Parameter(5, Ci.nsIDownloadManager.DOWNLOAD_DIRTY);
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
        var cache = Cc["@mozilla.org/netwerk/cache-storage-service;1"].getService(Ci.nsICacheStorageService);
        try {
          cache.clear();
        } catch(er) {}

        let imageCache = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                                                         .getImgCacheForDocument(null);
        try {
          imageCache.clearCache(false); 
        } catch(er) {}
      },

      get canClear()
      {
        return true;
      }
    },

    cookies: {
      clear: function ()
      {
        Services.cookies.removeAll();
      },

      get canClear()
      {
        return true;
      }
    },

    siteSettings: {
      clear: function ()
      {
        
        Services.perms.removeAll();

        
        Cc["@mozilla.org/content-pref/service;1"]
          .getService(Ci.nsIContentPrefService2)
          .removeAllDomains(null);

        
        
        var hosts = Services.logins.getAllDisabledHosts({})
        for (var host of hosts) {
          Services.logins.setLoginSavingEnabled(host, true);
        }
      },

      get canClear()
      {
        return true;
      }
    },

    offlineApps: {
      clear: function ()
      {
        var cacheService = Cc["@mozilla.org/netwerk/cache-storage-service;1"].getService(Ci.nsICacheStorageService);
        var appCacheStorage = cacheService.appCacheStorage(LoadContextInfo.default, null);
        try {
          appCacheStorage.asyncEvictStorage(null);
        } catch(er) {}
      },

      get canClear()
      {
          return true;
      }
    },

    history: {
      clear: function ()
      {
        sendMessageToJava({ type: "Sanitize:ClearHistory" });

        try {
          Services.obs.notifyObservers(null, "browser:purge-session-history", "");
        }
        catch (e) { }

        try {
          var predictor = Cc["@mozilla.org/network/predictor;1"].getService(Ci.nsINetworkPredictor);
          predictor.reset();
        } catch (e) { }
      },

      get canClear()
      {
        
        
        return true;
      }
    },

    formdata: {
      clear: function ()
      {
        FormHistory.update({ op: "remove" });
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
        downloads.iterate(function (dl) {
          
          let f = dl.targetFile;
          if (f.exists()) {
            f.remove(false);
          }

          
          dl.remove();
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
        Services.logins.removeAllLogins();
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
        
        var sdr = Cc["@mozilla.org/security/sdr;1"].getService(Ci.nsISecretDecoderRing);
        sdr.logoutAndTeardown();

        
        Services.obs.notifyObservers(null, "net:clear-active-logins", null);
      },

      get canClear()
      {
        return true;
      }
    }
  }
};

this.Sanitizer = new Sanitizer();
