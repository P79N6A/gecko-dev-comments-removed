






let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/LoadContextInfo.jsm");
Cu.import("resource://gre/modules/FormHistory.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Downloads.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Accounts.jsm");

function dump(a) {
  Services.console.logStringMessage(a);
}

this.EXPORTED_SYMBOLS = ["Sanitizer"];

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

          
          var sss = Cc["@mozilla.org/ssservice;1"]
                      .getService(Ci.nsISiteSecurityService);
          sss.clearAll();

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
          .catch(e => Cu.reportError("Java-side history clearing failed: " + e))
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

    searchHistory: {
      clear: function ()
      {
        return Messaging.sendRequestForResult({ type: "Sanitize:ClearHistory", clearSearchHistory: true })
          .catch(e => Cu.reportError("Java-side search history clearing failed: " + e))
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
      clear: Task.async(function* () {
        let list = yield Downloads.getList(Downloads.ALL);
        let downloads = yield list.getAll();

        
        
        
        for (let download of downloads) {
          
          
          
          if (download.stopped && (!download.hasPartialData || download.error)) {
            
            
            yield list.remove(download);
            
            
            
            
            download.finalize(true).then(null, Cu.reportError);

            
            OS.File.remove(download.target.path).then(null, ex => {
              if (!(ex instanceof OS.File.Error && ex.becauseNoSuchFile)) {
                Cu.reportError(ex);
              }
            });
          }
        }
      }),

      get canClear()
      {
        return true;
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
    },

    syncedTabs: {
      clear: function ()
      {
        return Messaging.sendRequestForResult({ type: "Sanitize:ClearSyncedTabs" })
          .catch(e => Cu.reportError("Java-side synced tabs clearing failed: " + e));
      },

      canClear: function(aCallback)
      {
        Accounts.anySyncAccountsExist().then(aCallback)
          .catch(function(err) {
            Cu.reportError("Java-side synced tabs clearing failed: " + err)
            aCallback(false);
          });
      }
    }

  }
};

this.Sanitizer = new Sanitizer();
