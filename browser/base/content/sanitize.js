# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Firefox Sanitizer.
#
# The Initial Developer of the Original Code is
# Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <ben@mozilla.org>
#   Giorgio Maone <g.maone@informaction.com>
#   Johnathan Nightingale <johnath@mozilla.com>
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

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
        const Cc = Components.classes;
        const Ci = Components.interfaces;
        var cacheService = Cc["@mozilla.org/network/cache-service;1"].
                          getService(Ci.nsICacheService);
        try {
          
          
          cacheService.evictEntries(Ci.nsICache.STORE_ANYWHERE);
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
        const Ci = Components.interfaces;
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
        const Cc = Components.classes;
        const Ci = Components.interfaces;
        var cacheService = Cc["@mozilla.org/network/cache-service;1"].
                           getService(Ci.nsICacheService);
        try {
          
          
          cacheService.evictEntries(Ci.nsICache.STORE_OFFLINE);
        } catch(er) {}

        var storageManagerService = Cc["@mozilla.org/dom/storagemanager;1"].
                                    getService(Ci.nsIDOMStorageManager);
        storageManagerService.clearOfflineApps();
      },

      get canClear()
      {
          return true;
      }
    },

    history: {
      clear: function ()
      {
        var globalHistory = Components.classes["@mozilla.org/browser/global-history;2"]
                                      .getService(Components.interfaces.nsIBrowserHistory);
        if (this.range)
          globalHistory.removeVisitsByTimeframe(this.range[0], this.range[1]);
        else
          globalHistory.removeAllPages();
        
        try {
          var os = Components.classes["@mozilla.org/observer-service;1"]
                             .getService(Components.interfaces.nsIObserverService);
          os.notifyObservers(null, "browser:purge-session-history", "");
        }
        catch (e) { }
        
        
        var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefBranch2);
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
          var searchBar = windows.getNext().document.getElementById("searchbar");
          if (searchBar)
            searchBar.textbox.reset();
        }

        var formHistory = Components.classes["@mozilla.org/satchel/form-history;1"]
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
          var searchBar = windows.getNext().document.getElementById("searchbar");
          if (searchBar) {
            var transactionMgr = searchBar.textbox.editor.transactionManager;
            if (searchBar.value ||
                transactionMgr.numberOfUndoItems ||
                transactionMgr.numberOfRedoItems)
              return true;
          }
        }

        var formHistory = Components.classes["@mozilla.org/satchel/form-history;1"]
                                    .getService(Components.interfaces.nsIFormHistory2);
        return formHistory.hasEntries;
      }
    },
    
    downloads: {
      clear: function ()
      {
        var dlMgr = Components.classes["@mozilla.org/download-manager;1"]
                              .getService(Components.interfaces.nsIDownloadManager);

        var dlIDsToRemove = [];
        if (this.range) {
          
          dlMgr.removeDownloadsByTimeframe(this.range[0], this.range[1]);
          
          
          var dlsEnum = dlMgr.activeDownloads;
          while(dlsEnum.hasMoreElements()) {
            var dl = dlsEnum.next();
            if(dl.startTime >= this.range[0])
              dlIDsToRemove.push(dl.id);
          }
        }
        else {
          
          dlMgr.cleanUp();
          
          
          var dlsEnum = dlMgr.activeDownloads;
          while(dlsEnum.hasMoreElements()) {
            dlIDsToRemove.push(dlsEnum.next().id);
          }
        }
        
        
        dlIDsToRemove.forEach(function(id) {
          dlMgr.removeDownload(id);
        });
      },

      get canClear()
      {
        var dlMgr = Components.classes["@mozilla.org/download-manager;1"]
                              .getService(Components.interfaces.nsIDownloadManager);
        return dlMgr.canCleanUp;
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
                            .getService(Components.interfaces.nsIContentPrefService);
        cps.removeGroupedPrefs();
        
        
        
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
                "chrome:
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


