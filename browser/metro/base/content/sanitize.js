




XPCOMUtils.defineLazyModuleGetter(this, "LoadContextInfo",
                                  "resource://gre/modules/LoadContextInfo.jsm");
function Sanitizer() {}

Sanitizer.prototype = {
  
  clearItem: function (aItemName)
  {
    if (this.items[aItemName].canClear)
      this.items[aItemName].clear();
  },

  canClearItem: function (aItemName, aCallback, aArg)
  {
    let canClear = this.items[aItemName].canClear;
    if (typeof canClear == "function"){
      canClear(aCallback, aArg);
    } else {
      aCallback(aItemName, canClear, aArg);
    }
  },

  _prefDomain: "privacy.item.",
  getNameFromPreference: function (aPreferenceName)
  {
    return aPreferenceName.substr(this._prefDomain.length);
  },

  





  sanitize: function ()
  {
    var branch = Services.prefs.getBranch(this._prefDomain);
    var errors = null;
    for (var itemName in this.items) {
      if ("clear" in item && branch.getBoolPref(itemName)) {
        
        
        
        
        
        let clearCallback = (itemName, aCanClear) => {
          let item = this.items[itemName];
          try{
            if (aCanClear){
              item.clear();
            }
          } catch(er){
            if (!errors){
              errors = {};
            }
            errors[itemName] = er;
            dump("Error sanitizing " + itemName + ":" + er + "\n");
          }
        }
        this.canClearItem(itemName, clearCallback);
      }
    }
    return errors;
  },

  items: {
    
    
    
    syncAccount: {
      clear: function ()
      {
        Sync.disconnect();
      },

      get canClear()
      {
        return (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED);
      }
    },

    cache: {
      clear: function ()
      {
        var cache = Cc["@mozilla.org/netwerk/cache-storage-service;1"].getService(Ci.nsICacheStorageService);
        try {
          cache.clear();
        } catch(er) {}

        let imageCache = Cc["@mozilla.org/image/cache;1"].getService(Ci.imgICache);
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
        var cookieMgr = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager);
        cookieMgr.removeAll();
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

        
        var cps = Cc["@mozilla.org/content-pref/service;1"].getService(Ci.nsIContentPrefService2);
        cps.removeAllDomains(null);

        
        
        var pwmgr = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
        var hosts = pwmgr.getAllDisabledHosts({})
        for each (var host in hosts) {
          pwmgr.setLoginSavingEnabled(host, true);
        }

        
        var sss = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);
        sss.clearAll();
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
        try {
          Services.obs.notifyObservers(null, "browser:purge-session-history", "");
        }
        catch (e) {
          Components.utils.reportError("Failed to notify observers of "
                                     + "browser:purge-session-history: "
                                     + e);
        }
      },

      get canClear()
      {
        
        
        return true;
      }
    },

    formdata: {
      clear: function ()
      {
        
        var windows = Services.wm.getEnumerator("navigator:browser");
        while (windows.hasMoreElements()) {
          var searchBar = windows.getNext().document.getElementById("searchbar");
          if (searchBar) {
            searchBar.value = "";
            searchBar.textbox.editor.transactionManager.clear();
          }
        }
        FormHistory.update({op : "remove"});
      },

      canClear : function(aCallback, aArg)
      {
        let count = 0;
        let countDone = {
          handleResult : function(aResult) { count = aResult; },
          handleError : function(aError) { Components.utils.reportError(aError); },
          handleCompletion : function(aReason) { aCallback("formdata", aReason == 0 && count > 0, aArg); }
        };
        FormHistory.count({}, countDone);
      }
    },

    downloads: {
      clear: function ()
      {
        var dlMgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
        dlMgr.cleanUp();
      },

      get canClear()
      {
        var dlMgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
        return dlMgr.canCleanUp;
      }
    },

    passwords: {
      clear: function ()
      {
        var pwmgr = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
        pwmgr.removeAllLogins();
      },

      get canClear()
      {
        var pwmgr = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
        var count = pwmgr.countLogins("", "", ""); 
        return (count > 0);
      }
    },

    sessions: {
      clear: function ()
      {
        
        var sdr = Cc["@mozilla.org/security/sdr;1"].getService(Ci.nsISecretDecoderRing);
        sdr.logoutAndTeardown();

        
        var authMgr = Cc['@mozilla.org/network/http-auth-manager;1'].getService(Ci.nsIHttpAuthManager);
        authMgr.clearAll();
      },

      get canClear()
      {
        return true;
      }
    }
  }
};



Sanitizer.prefDomain          = "privacy.sanitize.";
Sanitizer.prefShutdown        = "sanitizeOnShutdown";
Sanitizer.prefDidShutdown     = "didShutdownSanitize";

Sanitizer._prefs = null;
Sanitizer.__defineGetter__("prefs", function() 
{
  return Sanitizer._prefs ? Sanitizer._prefs
    : Sanitizer._prefs = Cc["@mozilla.org/preferences-service;1"]
                         .getService(Ci.nsIPrefService)
                         .getBranch(Sanitizer.prefDomain);
});








Sanitizer.sanitize = function() 
{
  return new Sanitizer().sanitize();
};

Sanitizer.onStartup = function() 
{
  
  Sanitizer._checkAndSanitize();
};

Sanitizer.onShutdown = function() 
{
  
  Sanitizer._checkAndSanitize();
};


Sanitizer._checkAndSanitize = function() 
{
  const prefs = Sanitizer.prefs;
  if (prefs.getBoolPref(Sanitizer.prefShutdown) && 
      !prefs.prefHasUserValue(Sanitizer.prefDidShutdown)) {
    
    Sanitizer.sanitize() || 
      prefs.setBoolPref(Sanitizer.prefDidShutdown, true);
  }
};


