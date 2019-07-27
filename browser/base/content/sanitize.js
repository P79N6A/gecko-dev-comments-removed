# -*- indent-tabs-mode: nil; js-indent-level: 4 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FormHistory",
                                  "resource://gre/modules/FormHistory.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
                                  "resource://gre/modules/TelemetryStopwatch.jsm");

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
    if (typeof canClear == "function") {
      canClear(aCallback, aArg);
      return false;
    }

    aCallback(aItemName, canClear, aArg);
    return canClear;
  },

  prefDomain: "",

  getNameFromPreference: function (aPreferenceName)
  {
    return aPreferenceName.substr(this.prefDomain.length);
  },

  








  sanitize: function (aItemsToClear)
  {
    var deferred = Promise.defer();
    var seenError = false;
    if (Array.isArray(aItemsToClear)) {
      var itemsToClear = [...aItemsToClear];
    } else {
      let branch = Services.prefs.getBranch(this.prefDomain);
      itemsToClear = Object.keys(this.items).filter(itemName => branch.getBoolPref(itemName));
    }

    
    
    
    let openWindowsIndex = itemsToClear.indexOf("openWindows");
    if (openWindowsIndex != -1) {
      itemsToClear.splice(openWindowsIndex, 1);
      let item = this.items.openWindows;

      let ok = item.clear(() => {
        try {
          let clearedPromise = this.sanitize(itemsToClear);
          clearedPromise.then(deferred.resolve, deferred.reject);
        } catch(e) {
          let error = "Sanitizer threw after closing windows: " + e;
          Cu.reportError(error);
          deferred.reject(error);
        }
      });
      
      if (!ok) {
        deferred.reject("Sanitizer canceled closing windows");
      }

      return deferred.promise;
    }

    TelemetryStopwatch.start("FX_SANITIZE_TOTAL");

    
    if (this.ignoreTimespan)
      var range = null;  
    else
      range = this.range || Sanitizer.getClearRange();

    let itemCount = Object.keys(itemsToClear).length;
    let onItemComplete = function() {
      if (!--itemCount) {
        TelemetryStopwatch.finish("FX_SANITIZE_TOTAL");
        seenError ? deferred.reject() : deferred.resolve();
      }
    };
    for (let itemName of itemsToClear) {
      let item = this.items[itemName];
      item.range = range;
      if ("clear" in item) {
        let clearCallback = (itemName, aCanClear) => {
          
          
          
          
          
          let item = this.items[itemName];
          try {
            if (aCanClear)
              item.clear();
          } catch(er) {
            seenError = true;
            Components.utils.reportError("Error sanitizing " + itemName +
                                         ": " + er + "\n");
          }
          onItemComplete();
        };
        this.canClearItem(itemName, clearCallback);
      } else {
        onItemComplete();
      }
    }

    return deferred.promise;
  },

  
  
  
  
  
  ignoreTimespan : true,
  range : null,

  items: {
    cache: {
      clear: function ()
      {
        TelemetryStopwatch.start("FX_SANITIZE_CACHE");

        var cache = Cc["@mozilla.org/netwerk/cache-storage-service;1"].
                    getService(Ci.nsICacheStorageService);
        try {
          
          
          cache.clear();
        } catch(er) {}

        var imageCache = Cc["@mozilla.org/image/tools;1"].
                         getService(Ci.imgITools).getImgCacheForDocument(null);
        try {
          imageCache.clearCache(false); 
        } catch(er) {}

        TelemetryStopwatch.finish("FX_SANITIZE_CACHE");
      },

      get canClear()
      {
        return true;
      }
    },

    cookies: {
      clear: function ()
      {
        TelemetryStopwatch.start("FX_SANITIZE_COOKIES");

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

        
        let mediaMgr = Components.classes["@mozilla.org/mediaManagerService;1"]
                                 .getService(Ci.nsIMediaManagerService);
        mediaMgr.sanitizeDeviceIds(this.range && this.range[0]);

        
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

        TelemetryStopwatch.finish("FX_SANITIZE_COOKIES");
      },

      get canClear()
      {
        return true;
      }
    },

    offlineApps: {
      clear: function ()
      {
        TelemetryStopwatch.start("FX_SANITIZE_OFFLINEAPPS");
        Components.utils.import("resource:///modules/offlineAppCache.jsm");
        OfflineAppCacheHelper.clear();
        TelemetryStopwatch.finish("FX_SANITIZE_OFFLINEAPPS");
      },

      get canClear()
      {
        return true;
      }
    },

    history: {
      clear: function ()
      {
        TelemetryStopwatch.start("FX_SANITIZE_HISTORY");

        if (this.range)
          PlacesUtils.history.removeVisitsByTimeframe(this.range[0], this.range[1]);
        else
          PlacesUtils.history.removeAllPages();

        try {
          var os = Components.classes["@mozilla.org/observer-service;1"]
                             .getService(Components.interfaces.nsIObserverService);
          let clearStartingTime = this.range ? String(this.range[0]) : "";
          os.notifyObservers(null, "browser:purge-session-history", clearStartingTime);
        }
        catch (e) { }

        try {
          var predictor = Components.classes["@mozilla.org/network/predictor;1"]
                                    .getService(Components.interfaces.nsINetworkPredictor);
          predictor.reset();
        } catch (e) { }

        TelemetryStopwatch.finish("FX_SANITIZE_HISTORY");
      },

      get canClear()
      {
        
        
        return true;
      }
    },

    formdata: {
      clear: function ()
      {
        TelemetryStopwatch.start("FX_SANITIZE_FORMDATA");

        
        var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1']
                                      .getService(Components.interfaces.nsIWindowMediator);
        var windows = windowManager.getEnumerator("navigator:browser");
        while (windows.hasMoreElements()) {
          let currentWindow = windows.getNext();
          let currentDocument = currentWindow.document;
          let searchBar = currentDocument.getElementById("searchbar");
          if (searchBar)
            searchBar.textbox.reset();
          let tabBrowser = currentWindow.gBrowser;
          for (let tab of tabBrowser.tabs) {
            if (tabBrowser.isFindBarInitialized(tab))
              tabBrowser.getFindBar(tab).clear();
          }
          
          tabBrowser._lastFindValue = "";
        }

        let change = { op: "remove" };
        if (this.range) {
          [ change.firstUsedStart, change.firstUsedEnd ] = this.range;
        }
        FormHistory.update(change);

        TelemetryStopwatch.finish("FX_SANITIZE_FORMDATA");
      },

      canClear : function(aCallback, aArg)
      {
        var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1']
                                      .getService(Components.interfaces.nsIWindowMediator);
        var windows = windowManager.getEnumerator("navigator:browser");
        while (windows.hasMoreElements()) {
          let currentWindow = windows.getNext();
          let currentDocument = currentWindow.document;
          let searchBar = currentDocument.getElementById("searchbar");
          if (searchBar) {
            let transactionMgr = searchBar.textbox.editor.transactionManager;
            if (searchBar.value ||
                transactionMgr.numberOfUndoItems ||
                transactionMgr.numberOfRedoItems) {
              aCallback("formdata", true, aArg);
              return false;
            }
          }
          let tabBrowser = currentWindow.gBrowser;
          let findBarCanClear = Array.some(tabBrowser.tabs, function (aTab) {
            return tabBrowser.isFindBarInitialized(aTab) &&
                   tabBrowser.getFindBar(aTab).canClear;
          });
          if (findBarCanClear) {
            aCallback("formdata", true, aArg);
            return false;
          }
        }

        let count = 0;
        let countDone = {
          handleResult : function(aResult) count = aResult,
          handleError : function(aError) Components.utils.reportError(aError),
          handleCompletion :
            function(aReason) { aCallback("formdata", aReason == 0 && count > 0, aArg); }
        };
        FormHistory.count({}, countDone);
        return false;
      }
    },

    downloads: {
      clear: function ()
      {
        TelemetryStopwatch.start("FX_SANITIZE_DOWNLOADS");
        Task.spawn(function () {
          let filterByTime = null;
          if (this.range) {
            
            let rangeBeginMs = this.range[0] / 1000;
            let rangeEndMs = this.range[1] / 1000;
            filterByTime = download => download.startTime >= rangeBeginMs &&
                                       download.startTime <= rangeEndMs;
          }

          
          let list = yield Downloads.getList(Downloads.ALL);
          list.removeFinished(filterByTime);
          TelemetryStopwatch.finish("FX_SANITIZE_DOWNLOADS");
        }.bind(this)).then(null, error => {
          TelemetryStopwatch.finish("FX_SANITIZE_DOWNLOADS");
          Components.utils.reportError(error);
        });
      },

      canClear : function(aCallback, aArg)
      {
        aCallback("downloads", true, aArg);
        return false;
      }
    },

    passwords: {
      clear: function ()
      {
        TelemetryStopwatch.start("FX_SANITIZE_PASSWORDS");
        var pwmgr = Components.classes["@mozilla.org/login-manager;1"]
                              .getService(Components.interfaces.nsILoginManager);
        
        pwmgr.removeAllLogins();
        TelemetryStopwatch.finish("FX_SANITIZE_PASSWORDS");
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
        TelemetryStopwatch.start("FX_SANITIZE_SESSIONS");

        
        var sdr = Components.classes["@mozilla.org/security/sdr;1"]
                            .getService(Components.interfaces.nsISecretDecoderRing);
        sdr.logoutAndTeardown();

        
        var os = Components.classes["@mozilla.org/observer-service;1"]
                           .getService(Components.interfaces.nsIObserverService);
        os.notifyObservers(null, "net:clear-active-logins", null);

        TelemetryStopwatch.finish("FX_SANITIZE_SESSIONS");
      },

      get canClear()
      {
        return true;
      }
    },

    siteSettings: {
      clear: function ()
      {
        TelemetryStopwatch.start("FX_SANITIZE_SITESETTINGS");

        
        
        
        let startDateMS = this.range == null ? null : this.range[0] / 1000;
        var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                           .getService(Components.interfaces.nsIPermissionManager);
        if (startDateMS == null) {
          pm.removeAll();
        } else {
          pm.removeAllSince(startDateMS);
        }

        
        var cps = Components.classes["@mozilla.org/content-pref/service;1"]
                            .getService(Components.interfaces.nsIContentPrefService2);
        if (startDateMS == null) {
          cps.removeAllDomains(null);
        } else {
          cps.removeAllDomainsSince(startDateMS, null);
        }

        
        
        
        
        
        var pwmgr = Components.classes["@mozilla.org/login-manager;1"]
                              .getService(Components.interfaces.nsILoginManager);
        var hosts = pwmgr.getAllDisabledHosts();
        for each (var host in hosts) {
          pwmgr.setLoginSavingEnabled(host, true);
        }

        
        
        var sss = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);
        sss.clearAll();

        
        var push = Cc["@mozilla.org/push/NotificationService;1"]
                    .getService(Ci.nsIPushNotificationService);
        push.clearAll();

        TelemetryStopwatch.finish("FX_SANITIZE_SITESETTINGS");
      },

      get canClear()
      {
        return true;
      }
    },
    openWindows: {
      privateStateForNewWindow: "non-private",
      _canCloseWindow: function(aWindow) {
        
        if (!aWindow.gMultiProcessBrowser) {
          
          
          
          
          for (let browser of aWindow.gBrowser.browsers) {
            let ds = browser.docShell;
            
            
            
            
            if (ds.contentViewer && !ds.contentViewer.permitUnload(true)) {
              return false;
            }
          }
        }
        return true;
      },
      _resetAllWindowClosures: function(aWindowList) {
        for (let win of aWindowList) {
          win.getInterface(Ci.nsIDocShell).contentViewer.resetCloseWindow();
        }
      },
      clear: function(aCallback)
      {
        
        

        if (!aCallback) {
          throw "Sanitizer's openWindows clear() requires a callback.";
        }

        
        
        let existingWindow = Services.appShell.hiddenDOMWindow;
        let startDate = existingWindow.performance.now();

        
        let windowEnumerator = Services.wm.getEnumerator("navigator:browser");
        let windowList = [];
        while (windowEnumerator.hasMoreElements()) {
          let someWin = windowEnumerator.getNext();
          windowList.push(someWin);
          
          if (!this._canCloseWindow(someWin)) {
            this._resetAllWindowClosures(windowList);
            return false;
          }

          
          
          
          
          if (existingWindow.performance.now() > (startDate + 60 * 1000)) {
            this._resetAllWindowClosures(windowList);
            return false;
          }
        }

        

        TelemetryStopwatch.start("FX_SANITIZE_OPENWINDOWS");

        
        
        let handler = Cc["@mozilla.org/browser/clh;1"].getService(Ci.nsIBrowserHandler);
        let defaultArgs = handler.defaultArgs;
        let features = "chrome,all,dialog=no," + this.privateStateForNewWindow;
        let newWindow = existingWindow.openDialog("chrome://browser/content/", "_blank",
                                                  features, defaultArgs);
#ifdef XP_MACOSX
        function onFullScreen(e) {
          newWindow.removeEventListener("fullscreen", onFullScreen);
          let docEl = newWindow.document.documentElement;
          let sizemode = docEl.getAttribute("sizemode");
          if (!newWindow.fullScreen && sizemode == "fullscreen") {
            docEl.setAttribute("sizemode", "normal");
            e.preventDefault();
            e.stopPropagation();
            return false;
          }
        }
        newWindow.addEventListener("fullscreen", onFullScreen);
#endif

        
        
        
        
        
        
        let newWindowOpened = false;
        function onWindowOpened(subject, topic, data) {
          if (subject != newWindow)
            return;

          Services.obs.removeObserver(onWindowOpened, "browser-delayed-startup-finished");
#ifdef XP_MACOSX
          newWindow.removeEventListener("fullscreen", onFullScreen);
#endif
          newWindowOpened = true;
          
          if (numWindowsClosing == 0) {
            TelemetryStopwatch.finish("FX_SANITIZE_OPENWINDOWS");
            aCallback();
          }
        }

        let numWindowsClosing = windowList.length;
        function onWindowClosed() {
          numWindowsClosing--;
          if (numWindowsClosing == 0) {
            Services.obs.removeObserver(onWindowClosed, "xul-window-destroyed");
            
            if (newWindowOpened) {
              TelemetryStopwatch.finish("FX_SANITIZE_OPENWINDOWS");
              aCallback();
            }
          }
        }

        Services.obs.addObserver(onWindowOpened, "browser-delayed-startup-finished", false);
        Services.obs.addObserver(onWindowClosed, "xul-window-destroyed", false);

        
        while (windowList.length) {
          windowList.pop().close();
        }
        newWindow.focus();
        return true;
      },

      get canClear()
      {
        return true;
      }
    },
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
Sanitizer.TIMESPAN_5MIN       = 5;
Sanitizer.TIMESPAN_24HOURS    = 6;





Sanitizer.getClearRange = function (ts) {
  if (ts === undefined)
    ts = Sanitizer.prefs.getIntPref("timeSpan");
  if (ts === Sanitizer.TIMESPAN_EVERYTHING)
    return null;

  
  var endDate = Date.now() * 1000;
  switch (ts) {
    case Sanitizer.TIMESPAN_5MIN :
      var startDate = endDate - 300000000; 
      break;
    case Sanitizer.TIMESPAN_HOUR :
      startDate = endDate - 3600000000; 
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
    case Sanitizer.TIMESPAN_24HOURS :
      startDate = endDate - 86400000000; 
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
    s.sanitize().then(function() {
      prefs.setBoolPref(Sanitizer.prefDidShutdown, true);
    });
  }
};
