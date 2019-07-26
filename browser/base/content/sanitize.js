# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");

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
  
  prefDomain: "",
  
  getNameFromPreference: function (aPreferenceName)
  {
    return aPreferenceName.substr(this.prefDomain.length);
  },
  
  





  sanitize: function ()
  {
    var psvc = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefService);
    var branch = psvc.getBranch(this.prefDomain);
    var errors = null;

    
    if (this.ignoreTimespan)
      var range = null;  
    else
      range = this.range || Sanitizer.getClearRange();
      
    for (var itemName in this.items) {
      var item = this.items[itemName];
      item.range = range;
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
  
  
  
  
  
  
  ignoreTimespan : true,
  range : null,
  
  items: {
    cache: {
      clear: function ()
      {
        var cacheService = Cc["@mozilla.org/network/cache-service;1"].
                          getService(Ci.nsICacheService);
        try {
          
          
          cacheService.evictEntries(Ci.nsICache.STORE_ANYWHERE);
        } catch(er) {}

        var imageCache = Cc["@mozilla.org/image/tools;1"].
                         getService(Ci.imgITools).getImgCacheForDocument(null);
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
        var cookieMgr = Components.classes["@mozilla.org/cookiemanager;1"]
                                  .getService(Ci.nsICookieManager);
        if (this.range) {
          
          var cookiesEnum = cookieMgr.enumerator;
          while (cookiesEnum.hasMoreElements()) {
            var cookie = cookiesEnum.getNext().QueryInterface(Ci.nsICookie2);
            
            if (cookie.creationTime > this.range[0])
              
              cookieMgr.remove(cookie.host, cookie.name, cookie.path, false);
          }
        }
        else {
          
          cookieMgr.removeAll();
        }

        
        const phInterface = Ci.nsIPluginHost;
        const FLAG_CLEAR_ALL = phInterface.FLAG_CLEAR_ALL;
        let ph = Cc["@mozilla.org/plugin/host;1"].getService(phInterface);

        
        
        
        
        let age = this.range ? (Date.now() / 1000 - this.range[0] / 1000000)
                             : -1;
        if (!this.range || age >= 0) {
          let tags = ph.getPluginTags();
          for (let i = 0; i < tags.length; i++) {
            try {
              ph.clearSiteData(tags[i], null, FLAG_CLEAR_ALL, age);
            } catch (e) {
              
              if (e.result == Components.results.
                    NS_ERROR_PLUGIN_TIME_RANGE_NOT_SUPPORTED) {
                try {
                  ph.clearSiteData(tags[i], null, FLAG_CLEAR_ALL, -1);
                } catch (e) {
                  
                }
              }
            }
          }
        }

        
        var psvc = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefService);
        try {
            var branch = psvc.getBranch("geo.wifi.access_token.");
            branch.deleteBranch("");
        } catch (e) {}

      },

      get canClear()
      {
        return true;
      }
    },

    offlineApps: {
      clear: function ()
      {
        Components.utils.import("resource:///modules/offlineAppCache.jsm");
        OfflineAppCacheHelper.clear();
      },

      get canClear()
      {
        return true;
      }
    },

    history: {
      clear: function ()
      {
        if (this.range)
          PlacesUtils.history.removeVisitsByTimeframe(this.range[0], this.range[1]);
        else
          PlacesUtils.history.removeAllPages();
        
        try {
          var os = Components.classes["@mozilla.org/observer-service;1"]
                             .getService(Components.interfaces.nsIObserverService);
          os.notifyObservers(null, "browser:purge-session-history", "");
        }
        catch (e) { }
        
        
        var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefBranch);
        try {
          prefs.clearUserPref("general.open_location.last_url");
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
        
        var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1']
                                      .getService(Components.interfaces.nsIWindowMediator);
        var windows = windowManager.getEnumerator("navigator:browser");
        while (windows.hasMoreElements()) {
          let currentDocument = windows.getNext().document;
          let searchBar = currentDocument.getElementById("searchbar");
          if (searchBar)
            searchBar.textbox.reset();
          let findBar = currentDocument.getElementById("FindToolbar");
          if (findBar)
            findBar.clear();
        }

        let formHistory = Components.classes["@mozilla.org/satchel/form-history;1"]
                                    .getService(Components.interfaces.nsIFormHistory2);
        if (this.range)
          formHistory.removeEntriesByTimeframe(this.range[0], this.range[1]);
        else
          formHistory.removeAllEntries();
      },

      get canClear()
      {
        var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1']
                                      .getService(Components.interfaces.nsIWindowMediator);
        var windows = windowManager.getEnumerator("navigator:browser");
        while (windows.hasMoreElements()) {
          let currentDocument = windows.getNext().document;
          let searchBar = currentDocument.getElementById("searchbar");
          if (searchBar) {
            let transactionMgr = searchBar.textbox.editor.transactionManager;
            if (searchBar.value ||
                transactionMgr.numberOfUndoItems ||
                transactionMgr.numberOfRedoItems)
              return true;
          }
          let findBar = currentDocument.getElementById("FindToolbar");
          if (findBar && findBar.canClear)
            return true;
        }

        let formHistory = Components.classes["@mozilla.org/satchel/form-history;1"]
                                    .getService(Components.interfaces.nsIFormHistory2);
        return formHistory.hasEntries;
      }
    },
    
    downloads: {
      clear: function ()
      {
        var dlMgr = Components.classes["@mozilla.org/download-manager;1"]
                              .getService(Components.interfaces.nsIDownloadManager);

        var dlsToRemove = [];
        if (this.range) {
          
          dlMgr.removeDownloadsByTimeframe(this.range[0], this.range[1]);

          
          for (let dlsEnum of [dlMgr.activeDownloads, dlMgr.activePrivateDownloads]) {
            while (dlsEnum.hasMoreElements()) {
              var dl = dlsEnum.next();
              if (dl.startTime >= this.range[0])
                dlsToRemove.push(dl);
            }
          }
        }
        else {
          
          dlMgr.cleanUp();
          dlMgr.cleanUpPrivate();
          
          
          for (let dlsEnum of [dlMgr.activeDownloads, dlMgr.activePrivateDownloads]) {
            while (dlsEnum.hasMoreElements()) {
              dlsToRemove.push(dlsEnum.next());
            }
          }
        }

        
        dlsToRemove.forEach(function (dl) {
          dl.remove();
        });
      },

      get canClear()
      {
        var dlMgr = Components.classes["@mozilla.org/download-manager;1"]
                              .getService(Components.interfaces.nsIDownloadManager);
        return dlMgr.canCleanUp || dlMgr.canCleanUpPrivate;
      }
    },
    
    passwords: {
      clear: function ()
      {
        var pwmgr = Components.classes["@mozilla.org/login-manager;1"]
                              .getService(Components.interfaces.nsILoginManager);
        
        pwmgr.removeAllLogins();
      },
      
      get canClear()
      {
        var pwmgr = Components.classes["@mozilla.org/login-manager;1"]
                              .getService(Components.interfaces.nsILoginManager);
        var count = pwmgr.countLogins("", "", ""); 
        return (count > 0);
      }
    },
    
    sessions: {
      clear: function ()
      {
        
        var sdr = Components.classes["@mozilla.org/security/sdr;1"]
                            .getService(Components.interfaces.nsISecretDecoderRing);
        sdr.logoutAndTeardown();

        
        var os = Components.classes["@mozilla.org/observer-service;1"]
                           .getService(Components.interfaces.nsIObserverService);
        os.notifyObservers(null, "net:clear-active-logins", null);
      },
      
      get canClear()
      {
        return true;
      }
    },
    
    siteSettings: {
      clear: function ()
      {
        
        var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                           .getService(Components.interfaces.nsIPermissionManager);
        pm.removeAll();
        
        
        var cps = Components.classes["@mozilla.org/content-pref/service;1"]
                            .getService(Components.interfaces.nsIContentPrefService2);
        cps.removeAllDomains(null);
        
        
        
        var pwmgr = Components.classes["@mozilla.org/login-manager;1"]
                              .getService(Components.interfaces.nsILoginManager);
        var hosts = pwmgr.getAllDisabledHosts();
        for each (var host in hosts) {
          pwmgr.setLoginSavingEnabled(host, true);
        }
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



Sanitizer.TIMESPAN_EVERYTHING = 0;
Sanitizer.TIMESPAN_HOUR       = 1;
Sanitizer.TIMESPAN_2HOURS     = 2;
Sanitizer.TIMESPAN_4HOURS     = 3;
Sanitizer.TIMESPAN_TODAY      = 4;





Sanitizer.getClearRange = function (ts) {
  if (ts === undefined)
    ts = Sanitizer.prefs.getIntPref("timeSpan");
  if (ts === Sanitizer.TIMESPAN_EVERYTHING)
    return null;
  
  
  var endDate = Date.now() * 1000;
  switch (ts) {
    case Sanitizer.TIMESPAN_HOUR :
      var startDate = endDate - 3600000000; 
      break;
    case Sanitizer.TIMESPAN_2HOURS :
      startDate = endDate - 7200000000; 
      break;
    case Sanitizer.TIMESPAN_4HOURS :
      startDate = endDate - 14400000000; 
      break;
    case Sanitizer.TIMESPAN_TODAY :
      var d = new Date();  
      d.setHours(0);      
      d.setMinutes(0);
      d.setSeconds(0);
      startDate = d.valueOf() * 1000; 
      break;
    default:
      throw "Invalid time span for clear private data: " + ts;
  }
  return [startDate, endDate];
};

Sanitizer._prefs = null;
Sanitizer.__defineGetter__("prefs", function() 
{
  return Sanitizer._prefs ? Sanitizer._prefs
    : Sanitizer._prefs = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefService)
                         .getBranch(Sanitizer.prefDomain);
});


Sanitizer.showUI = function(aParentWindow) 
{
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService(Components.interfaces.nsIWindowWatcher);
#ifdef XP_MACOSX
  ww.openWindow(null, 
#else
  ww.openWindow(aParentWindow,
#endif
                "chrome://browser/content/sanitize.xul",
                "Sanitize",
                "chrome,titlebar,dialog,centerscreen,modal",
                null);
};





Sanitizer.sanitize = function(aParentWindow) 
{
  Sanitizer.showUI(aParentWindow);
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
    
    var s = new Sanitizer();
    s.prefDomain = "privacy.clearOnShutdown.";
    s.sanitize() || 
      prefs.setBoolPref(Sanitizer.prefDidShutdown, true);
  }
};


