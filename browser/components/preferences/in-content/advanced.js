




Components.utils.import("resource://gre/modules/DownloadUtils.jsm");

var gAdvancedPane = {
  _inited: false,

  


  init: function ()
  {
    this._inited = true;
    var advancedPrefs = document.getElementById("advancedPrefs");

    var preference = document.getElementById("browser.preferences.advanced.selectedTabIndex");
    if (preference.value !== null)
        advancedPrefs.selectedIndex = preference.value;

#ifdef HAVE_SHELL_SERVICE
    this.updateSetDefaultBrowser();
#endif
#ifdef MOZ_UPDATER
    this.updateReadPrefs();
#endif
    this.updateOfflineApps();
#ifdef MOZ_CRASHREPORTER
    this.initSubmitCrashes();
#endif
    this.updateActualCacheSize("disk");
    this.updateActualCacheSize("offline");
  },

  



  tabSelectionChanged: function ()
  {
    if (!this._inited)
      return;
    var advancedPrefs = document.getElementById("advancedPrefs");
    var preference = document.getElementById("browser.preferences.advanced.selectedTabIndex");
    preference.valueFromPreferences = advancedPrefs.selectedIndex;
  },

  

  

























  



  _storedSpellCheck: 0,

  




  readCheckSpelling: function ()
  {
    var pref = document.getElementById("layout.spellcheckDefault");
    this._storedSpellCheck = pref.value;

    return (pref.value != 0);
  },

  




  writeCheckSpelling: function ()
  {
    var checkbox = document.getElementById("checkSpelling");
    return checkbox.checked ? (this._storedSpellCheck == 2 ? 2 : 1) : 0;
  },

  


  initSubmitCrashes: function ()
  {
    var checkbox = document.getElementById("submitCrashesBox");
    try {
      var cr = Components.classes["@mozilla.org/toolkit/crash-reporter;1"].
               getService(Components.interfaces.nsICrashReporter);
      checkbox.checked = cr.submitReports;
    } catch (e) {
      checkbox.style.display = "none";
    }
  },

  


  updateSubmitCrashes: function ()
  {
    var checkbox = document.getElementById("submitCrashesBox");
    try {
      var cr = Components.classes["@mozilla.org/toolkit/crash-reporter;1"].
               getService(Components.interfaces.nsICrashReporter);
      cr.submitReports = checkbox.checked;
    } catch (e) { }
  },

  



  updateHardwareAcceleration: function()
  {
#ifdef XP_WIN
    var fromPref = document.getElementById("layers.acceleration.disabled");
    var toPref = document.getElementById("gfx.direct2d.disabled");
    toPref.value = fromPref.value;
#endif
  },

  

  







  


  showConnections: function ()
  {
    openDialog("chrome://browser/content/preferences/connection.xul",
               "mozilla:connectionmanager",
               "model=yes",
               null);
  },

  
  updateActualCacheSize: function (device)
  {
    var visitor = {
      visitDevice: function (deviceID, deviceInfo)
      {
        if (deviceID == device) {
          var actualSizeLabel = document.getElementById(device == "disk" ?
                                                        "actualDiskCacheSize" :
                                                        "actualAppCacheSize");
          var sizeStrings = DownloadUtils.convertByteUnits(deviceInfo.totalSize);
          var prefStrBundle = document.getElementById("bundlePreferences");
          var sizeStr = prefStrBundle.getFormattedString(device == "disk" ?
                                                         "actualDiskCacheSize" :
                                                         "actualAppCacheSize",
                                                         sizeStrings);
          actualSizeLabel.value = sizeStr;
        }
        
        return false;
      },

      visitEntry: function (deviceID, entryInfo)
      {
        
        return false;
      }
    };

    var cacheService =
      Components.classes["@mozilla.org/network/cache-service;1"]
                .getService(Components.interfaces.nsICacheService);
    cacheService.visitEntries(visitor);
  },

  updateCacheSizeUI: function (smartSizeEnabled)
  {
    document.getElementById("useCacheBefore").disabled = smartSizeEnabled;
    document.getElementById("cacheSize").disabled = smartSizeEnabled;
    document.getElementById("useCacheAfter").disabled = smartSizeEnabled;
  },

  readSmartSizeEnabled: function ()
  {
    
    
    var disabled = document.getElementById("browser.cache.disk.smart_size.enabled").value;
    this.updateCacheSizeUI(!disabled);
  },

  



  readCacheSize: function ()
  {
    var preference = document.getElementById("browser.cache.disk.capacity");
    return preference.value / 1024;
  },

  



  writeCacheSize: function ()
  {
    var cacheSize = document.getElementById("cacheSize");
    var intValue = parseInt(cacheSize.value, 10);
    return isNaN(intValue) ? 0 : intValue * 1024;
  },

  


  clearCache: function ()
  {
    var cacheService = Components.classes["@mozilla.org/network/cache-service;1"]
                                 .getService(Components.interfaces.nsICacheService);
    try {
      cacheService.evictEntries(Components.interfaces.nsICache.STORE_ANYWHERE);
    } catch(ex) {}
    this.updateActualCacheSize("disk");
  },

  


  clearOfflineAppCache: function ()
  {
    Components.utils.import("resource:///modules/offlineAppCache.jsm");
    OfflineAppCacheHelper.clear();

    this.updateActualCacheSize("offline");
    this.updateOfflineApps();
  },

  readOfflineNotify: function()
  {
    var pref = document.getElementById("browser.offline-apps.notify");
    var button = document.getElementById("offlineNotifyExceptions");
    button.disabled = !pref.value;
    return pref.value;
  },

  showOfflineExceptions: function()
  {
    var bundlePreferences = document.getElementById("bundlePreferences");
    var params = { blockVisible     : false,
                   sessionVisible   : false,
                   allowVisible     : false,
                   prefilledHost    : "",
                   permissionType   : "offline-app",
                   manageCapability : Components.interfaces.nsIPermissionManager.DENY_ACTION,
                   windowTitle      : bundlePreferences.getString("offlinepermissionstitle"),
                   introText        : bundlePreferences.getString("offlinepermissionstext") };
    openDialog("chrome://browser/content/preferences/permissions.xul",
               "Browser:Permissions",
               "model=yes",
               params);
  },

  
  _getOfflineAppUsage: function (host, groups)
  {
    var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                       getService(Components.interfaces.nsIApplicationCacheService);
    if (!groups)
      groups = cacheService.getGroups();

    var ios = Components.classes["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);

    var usage = 0;
    for (var i = 0; i < groups.length; i++) {
      var uri = ios.newURI(groups[i], null, null);
      if (uri.asciiHost == host) {
        var cache = cacheService.getActiveCache(groups[i]);
        usage += cache.usage;
      }
    }

    var storageManager = Components.classes["@mozilla.org/dom/storagemanager;1"].
                         getService(Components.interfaces.nsIDOMStorageManager);
    usage += storageManager.getUsage(host);

    return usage;
  },

  


  updateOfflineApps: function ()
  {
    var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                       .getService(Components.interfaces.nsIPermissionManager);

    var list = document.getElementById("offlineAppsList");
    while (list.firstChild) {
      list.removeChild(list.firstChild);
    }

    var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                       getService(Components.interfaces.nsIApplicationCacheService);
    var groups = cacheService.getGroups();

    var bundle = document.getElementById("bundlePreferences");

    var enumerator = pm.enumerator;
    while (enumerator.hasMoreElements()) {
      var perm = enumerator.getNext().QueryInterface(Components.interfaces.nsIPermission);
      if (perm.type == "offline-app" &&
          perm.capability != Components.interfaces.nsIPermissionManager.DEFAULT_ACTION &&
          perm.capability != Components.interfaces.nsIPermissionManager.DENY_ACTION) {
        var row = document.createElement("listitem");
        row.id = "";
        row.className = "offlineapp";
        row.setAttribute("host", perm.host);
        var converted = DownloadUtils.
                        convertByteUnits(this._getOfflineAppUsage(perm.host, groups));
        row.setAttribute("usage",
                         bundle.getFormattedString("offlineAppUsage",
                                                   converted));
        list.appendChild(row);
      }
    }
  },

  offlineAppSelected: function()
  {
    var removeButton = document.getElementById("offlineAppsListRemove");
    var list = document.getElementById("offlineAppsList");
    if (list.selectedItem) {
      removeButton.setAttribute("disabled", "false");
    } else {
      removeButton.setAttribute("disabled", "true");
    }
  },

  removeOfflineApp: function()
  {
    var list = document.getElementById("offlineAppsList");
    var item = list.selectedItem;
    var host = item.getAttribute("host");

    var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                            .getService(Components.interfaces.nsIPromptService);
    var flags = prompts.BUTTON_TITLE_IS_STRING * prompts.BUTTON_POS_0 +
                prompts.BUTTON_TITLE_CANCEL * prompts.BUTTON_POS_1;

    var bundle = document.getElementById("bundlePreferences");
    var title = bundle.getString("offlineAppRemoveTitle");
    var prompt = bundle.getFormattedString("offlineAppRemovePrompt", [host]);
    var confirm = bundle.getString("offlineAppRemoveConfirm");
    var result = prompts.confirmEx(window, title, prompt, flags, confirm,
                                   null, null, null, {});
    if (result != 0)
      return;

    
    var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                       getService(Components.interfaces.nsIApplicationCacheService);
    var ios = Components.classes["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);
    var groups = cacheService.getGroups();
    for (var i = 0; i < groups.length; i++) {
        var uri = ios.newURI(groups[i], null, null);
        if (uri.asciiHost == host) {
            var cache = cacheService.getActiveCache(groups[i]);
            cache.discard();
        }
    }

    
    
    var obs = Components.classes["@mozilla.org/observer-service;1"]
                        .getService(Components.interfaces.nsIObserverService);
    obs.notifyObservers(null, "offline-app-removed", host);

    
    var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                       .getService(Components.interfaces.nsIPermissionManager);
    pm.remove(host, "offline-app",
              Components.interfaces.nsIPermissionManager.ALLOW_ACTION);
    pm.remove(host, "offline-app",
              Components.interfaces.nsIOfflineCacheUpdateService.ALLOW_NO_WARN);

    list.removeChild(item);
    gAdvancedPane.offlineAppSelected();
    this.updateActualCacheSize("offline");
  },

  

  





















#ifdef MOZ_UPDATER
  


























  updateReadPrefs: function ()
  {
    var enabledPref = document.getElementById("app.update.enabled");
    var autoPref = document.getElementById("app.update.auto");
    var radiogroup = document.getElementById("updateRadioGroup");

    if (!enabledPref.value)   
      radiogroup.value="manual"     
    else if (autoPref.value)  
      radiogroup.value="auto";      
    else                      
      radiogroup.value="checkOnly"; 

    var canCheck = Components.classes["@mozilla.org/updates/update-service;1"].
                     getService(Components.interfaces.nsIApplicationUpdateService).
                     canCheckForUpdates;
    
    
    
    radiogroup.disabled = !canCheck || enabledPref.locked || autoPref.locked;

    var modePref = document.getElementById("app.update.mode");
    var warnIncompatible = document.getElementById("warnIncompatible");
    
    warnIncompatible.disabled = radiogroup.disabled || modePref.locked ||
                                !enabledPref.value || !autoPref.value;

#ifdef MOZ_MAINTENANCE_SERVICE
    
    
    var installed;
    try {
      var wrk = Components.classes["@mozilla.org/windows-registry-key;1"]
                .createInstance(Components.interfaces.nsIWindowsRegKey);
      wrk.open(wrk.ROOT_KEY_LOCAL_MACHINE,
               "SOFTWARE\\Mozilla\\MaintenanceService",
               wrk.ACCESS_READ | wrk.WOW64_64);
      installed = wrk.readIntValue("Installed");
      wrk.close();
    } catch(e) {
    }
    if (installed != 1) {
      document.getElementById("useService").hidden = true;
    }
#endif
  },

  



  updateWritePrefs: function ()
  {
    var enabledPref = document.getElementById("app.update.enabled");
    var autoPref = document.getElementById("app.update.auto");
    var radiogroup = document.getElementById("updateRadioGroup");
    switch (radiogroup.value) {
      case "auto":      
        enabledPref.value = true;
        autoPref.value = true;
        break;
      case "checkOnly": 
        enabledPref.value = true;
        autoPref.value = false;
        break;
      case "manual":    
        enabledPref.value = false;
        autoPref.value = false;
    }

    var warnIncompatible = document.getElementById("warnIncompatible");
    var modePref = document.getElementById("app.update.mode");
    warnIncompatible.disabled = enabledPref.locked || !enabledPref.value ||
                                autoPref.locked || !autoPref.value ||
                                modePref.locked;
  },

  






  _modePreference: -1,

  












  readAddonWarn: function ()
  {
    var preference = document.getElementById("app.update.mode");
    var warn = preference.value != 0;
    gAdvancedPane._modePreference = warn ? preference.value : 1;
    return warn;
  },

  




  writeAddonWarn: function ()
  {
    var warnIncompatible = document.getElementById("warnIncompatible");
    return !warnIncompatible.checked ? 0 : gAdvancedPane._modePreference;
  },

  


  showUpdates: function ()
  {
    var prompter = Components.classes["@mozilla.org/updates/update-prompt;1"]
                             .createInstance(Components.interfaces.nsIUpdatePrompt);
    prompter.showUpdateHistory(window);
  },
#endif

  

  















  


  showCertificates: function ()
  {
    openDialog("chrome://pippki/content/certManager.xul",
               "mozilla:certmanager",
               "model=yes", null);
  },

  


  showCRLs: function ()
  {
    openDialog("chrome://pippki/content/crlManager.xul",
               "mozilla:crlmanager",
               "model=yes", null);
  },

  


  showOCSP: function ()
  {
    openDialog("chrome://mozapps/content/preferences/ocsp.xul",
               "mozilla:crlmanager",
               "model=yes", null);
  },

  


  showSecurityDevices: function ()
  {
    openDialog("chrome://pippki/content/device_manager.xul",
               "mozilla:devicemanager",
               "model=yes", null);
  }
#ifdef HAVE_SHELL_SERVICE
  ,

  

  







  



  updateSetDefaultBrowser: function()
  {
    var shellSvc = Components.classes["@mozilla.org/browser/shell-service;1"]
                             .getService(Components.interfaces.nsIShellService);
    let selectedIndex = shellSvc.isDefaultBrowser(false) ? 1 : 0;
    document.getElementById("setDefaultPane").selectedIndex = selectedIndex;
  },

  


  setDefaultBrowser: function()
  {
    var shellSvc = Components.classes["@mozilla.org/browser/shell-service;1"]
                             .getService(Components.interfaces.nsIShellService);
    shellSvc.setDefaultBrowser(true, false);
    document.getElementById("setDefaultPane").selectedIndex = 1;
  }
#endif
};
