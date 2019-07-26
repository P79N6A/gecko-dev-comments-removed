




function Sanitizer() {}
Sanitizer.prototype = {
  
  clearItem: function (aItemName)
  {
    if (this.items[aItemName].canClear)
      this.items[aItemName].clear();
  },

  canClearItem: function (aItemName)
  {
    return this.items[aItemName].canClear;
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
      var item = this.items[itemName];
      if ("clear" in item && item.canClear && branch.getBoolPref(itemName)) {
        
        
        
        
        
        try {
          item.clear();
        } catch(er) {
          if (!errors)
            errors = {};
          errors[itemName] = er;
          dump("Error sanitizing " + itemName + ": " + er + "\n");
        }
      }
    }
    return errors;
  },

  items: {
    cache: {
      clear: function ()
      {
        var cacheService = Cc["@mozilla.org/network/cache-service;1"].getService(Ci.nsICacheService);
        try {
          cacheService.evictEntries(Ci.nsICache.STORE_ANYWHERE);
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

        
        try {
          var branch = Services.prefs.getBranch("geo.wifi.access_token.");
          branch.deleteBranch("");
        } catch (e) {dump(e);}
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

        
        Services.contentPrefs.removeGroupedPrefs(null);

        
        
        var hosts = Services.logins.getAllDisabledHosts({})
        for each (var host in hosts) {
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
        var cacheService = Cc["@mozilla.org/network/cache-service;1"].getService(Ci.nsICacheService);
        try {
          cacheService.evictEntries(Ci.nsICache.STORE_OFFLINE);
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
          Services.prefs.clearUserPref("general.open_location.last_url");
        }
        catch (e) { }
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

        var formHistory = Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2);
        formHistory.removeAllEntries();
      },

      get canClear()
      {
        var formHistory = Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2);
        return formHistory.hasEntries;
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

        
        var os = Components.classes["@mozilla.org/observer-service;1"]
                           .getService(Components.interfaces.nsIObserverService);
        os.notifyObservers(null, "net:clear-active-logins", null);
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


