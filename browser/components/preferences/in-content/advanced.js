




Components.utils.import("resource://gre/modules/DownloadUtils.jsm");
Components.utils.import("resource://gre/modules/LoadContextInfo.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

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
#ifdef XP_WIN
    
    
    
    
    
    window.setInterval(this.updateSetDefaultBrowser, 1000);

#ifdef MOZ_METRO
    
    
    let version = Components.classes["@mozilla.org/system-info;1"].
                  getService(Components.interfaces.nsIPropertyBag2).
                  getProperty("version");
    let preWin8 = parseFloat(version) < 6.2;
    this._showingWin8Prefs = !preWin8;
    if (preWin8) {
      ["autoMetro", "autoMetroIndent"].forEach(
        function(id) document.getElementById(id).collapsed = true
      );
    } else {
      let brandShortName =
        document.getElementById("bundleBrand").getString("brandShortName");
      let bundlePrefs = document.getElementById("bundlePreferences");
      let autoDesktop = document.getElementById("autoDesktop");
      autoDesktop.label =
        bundlePrefs.getFormattedString("updateAutoDesktop.label",
                                       [brandShortName]);
      autoDesktop.accessKey =
        bundlePrefs.getString("updateAutoDesktop.accessKey");
    }
#endif
#endif
#endif
#ifdef MOZ_UPDATER
    this.updateReadPrefs();
#endif
    this.updateOfflineApps();
#ifdef MOZ_CRASHREPORTER
    this.initSubmitCrashes();
#endif
    this.initTelemetry();
#ifdef MOZ_SERVICES_HEALTHREPORT
    this.initSubmitHealthReport();
#endif
    this.updateActualCacheSize();
    this.updateActualAppCacheSize();

    
    Services.obs.notifyObservers(window, "advanced-pane-loaded", null);
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


  



  updateHardwareAcceleration: function()
  {
#ifdef XP_WIN
    var fromPref = document.getElementById("layers.acceleration.disabled");
    var toPref = document.getElementById("gfx.direct2d.disabled");
    toPref.value = fromPref.value;
#endif
  },

  

  


  _setupLearnMoreLink: function (pref, element) {
    
    let url = Services.prefs.getCharPref(pref);
    let el = document.getElementById(element);

    if (url) {
      el.setAttribute("href", url);
    } else {
      el.setAttribute("hidden", "true");
    }
  },

  


  initSubmitCrashes: function ()
  {
    this._setupLearnMoreLink("toolkit.crashreporter.infoURL",
                             "crashReporterLearnMore");

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

  




  initTelemetry: function ()
  {
#ifdef MOZ_TELEMETRY_REPORTING
    this._setupLearnMoreLink("toolkit.telemetry.infoURL", "telemetryLearnMore");
#endif
  },

#ifdef MOZ_SERVICES_HEALTHREPORT
  


  initSubmitHealthReport: function () {
    this._setupLearnMoreLink("datareporting.healthreport.infoURL", "FHRLearnMore");

    let policy = Components.classes["@mozilla.org/datareporting/service;1"]
                                   .getService(Components.interfaces.nsISupports)
                                   .wrappedJSObject
                                   .policy;

    let checkbox = document.getElementById("submitHealthReportBox");

    if (!policy || policy.healthReportUploadLocked) {
      checkbox.setAttribute("disabled", "true");
      return;
    }

    checkbox.checked = policy.healthReportUploadEnabled;
  },

  


  updateSubmitHealthReport: function () {
    let policy = Components.classes["@mozilla.org/datareporting/service;1"]
                                   .getService(Components.interfaces.nsISupports)
                                   .wrappedJSObject
                                   .policy;

    if (!policy) {
      return;
    }

    let checkbox = document.getElementById("submitHealthReportBox");
    policy.recordHealthReportUploadEnabled(checkbox.checked,
                                           "Checkbox from preferences pane");
  },
#endif

  

  







  


  showConnections: function ()
  {
    openDialog("chrome://browser/content/preferences/connection.xul",
               "mozilla:connectionmanager",
               "modal=yes",
               null);
  },

  
  updateActualCacheSize: function ()
  {
    var actualSizeLabel = document.getElementById("actualDiskCacheSize");
    var prefStrBundle = document.getElementById("bundlePreferences");

    
    this.observer = {
      onNetworkCacheDiskConsumption: function(consumption) {
        var size = DownloadUtils.convertByteUnits(consumption);
        actualSizeLabel.value = prefStrBundle.getFormattedString("actualDiskCacheSize", size);
      },

      QueryInterface: XPCOMUtils.generateQI([
        Components.interfaces.nsICacheStorageConsumptionObserver,
        Components.interfaces.nsISupportsWeakReference
      ])
    };

    actualSizeLabel.textContent = prefStrBundle.getString("actualDiskCacheSizeCalculated");

    var cacheService =
      Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
                .getService(Components.interfaces.nsICacheStorageService);
    cacheService.asyncGetDiskConsumption(this.observer);
  },

  
  updateActualAppCacheSize: function ()
  {
    var visitor = {
      onCacheStorageInfo: function (aEntryCount, aConsumption, aCapacity, aDiskDirectory)
      {
        var actualSizeLabel = document.getElementById("actualAppCacheSize");
        var sizeStrings = DownloadUtils.convertByteUnits(aConsumption);
        var prefStrBundle = document.getElementById("bundlePreferences");
        var sizeStr = prefStrBundle.getFormattedString("actualAppCacheSize", sizeStrings);
        actualSizeLabel.value = sizeStr;
      }
    };

    var cacheService =
      Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
                .getService(Components.interfaces.nsICacheStorageService);
    var storage = cacheService.appCacheStorage(LoadContextInfo.default, null);
    storage.asyncVisitStorage(visitor, false);
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
    var cache = Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
                                 .getService(Components.interfaces.nsICacheStorageService);
    try {
      cache.clear();
    } catch(ex) {}
    this.updateActualCacheSize();
  },

  


  clearOfflineAppCache: function ()
  {
    Components.utils.import("resource:///modules/offlineAppCache.jsm");
    OfflineAppCacheHelper.clear();

    this.updateActualAppCacheSize();
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
               "modal=yes",
               params);
  },

  
  _getOfflineAppUsage: function (host, groups)
  {
    var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                       getService(Components.interfaces.nsIApplicationCacheService);
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

    var groups;
    try {
      var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                         getService(Components.interfaces.nsIApplicationCacheService);
      groups = cacheService.getGroups();
    } catch (e) {
      return;
    }

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

    
    try {
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
    } catch (e) {}

    
    var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                       .getService(Components.interfaces.nsIPermissionManager);
    pm.remove(host, "offline-app",
              Components.interfaces.nsIPermissionManager.ALLOW_ACTION);
    pm.remove(host, "offline-app",
              Components.interfaces.nsIOfflineCacheUpdateService.ALLOW_NO_WARN);

    list.removeChild(item);
    gAdvancedPane.offlineAppSelected();
    this.updateActualAppCacheSize();
  },

  

  





















#ifdef MOZ_UPDATER
  


























  updateReadPrefs: function ()
  {
    var enabledPref = document.getElementById("app.update.enabled");
    var autoPref = document.getElementById("app.update.auto");
#ifdef XP_WIN
#ifdef MOZ_METRO
    var metroEnabledPref = document.getElementById("app.update.metro.enabled");
#endif
#endif
    var radiogroup = document.getElementById("updateRadioGroup");

    if (!enabledPref.value)   
      radiogroup.value="manual";    
#ifdef XP_WIN
#ifdef MOZ_METRO
    
    else if (metroEnabledPref.value && this._showingWin8Prefs)
      radiogroup.value="autoMetro"; 
#endif
#endif
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
#ifdef XP_WIN
#ifdef MOZ_METRO
    if (this._showingWin8Prefs) {
      warnIncompatible.disabled |= metroEnabledPref.value;
      warnIncompatible.checked |= metroEnabledPref.value;
    }
#endif
#endif

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
    var modePref = document.getElementById("app.update.mode");
#ifdef XP_WIN
#ifdef MOZ_METRO
    var metroEnabledPref = document.getElementById("app.update.metro.enabled");
    
    if (this._showingWin8Prefs) {
      metroEnabledPref.value = false;
    }
#endif
#endif
    var radiogroup = document.getElementById("updateRadioGroup");
    switch (radiogroup.value) {
      case "auto":      
        enabledPref.value = true;
        autoPref.value = true;
        break;
#ifdef XP_WIN
#ifdef MOZ_METRO
      case "autoMetro": 
        enabledPref.value = true;
        autoPref.value = true;
        metroEnabledPref.value = true;
        modePref.value = 1;
        break;
#endif
#endif
      case "checkOnly": 
        enabledPref.value = true;
        autoPref.value = false;
        break;
      case "manual":    
        enabledPref.value = false;
        autoPref.value = false;
    }

    var warnIncompatible = document.getElementById("warnIncompatible");
    warnIncompatible.disabled = enabledPref.locked || !enabledPref.value ||
                                autoPref.locked || !autoPref.value ||
                                modePref.locked;
#ifdef XP_WIN
#ifdef MOZ_METRO
    if (this._showingWin8Prefs) {
      warnIncompatible.disabled |= metroEnabledPref.value;
      warnIncompatible.checked |= metroEnabledPref.value;
    }
#endif
#endif
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
               "modal=yes", null);
  },

  


  showOCSP: function ()
  {
    openDialog("chrome://mozapps/content/preferences/ocsp.xul",
               "mozilla:crlmanager",
               "modal=yes", null);
  },

  


  showSecurityDevices: function ()
  {
    openDialog("chrome://pippki/content/device_manager.xul",
               "mozilla:devicemanager",
               "modal=yes", null);
  }
#ifdef HAVE_SHELL_SERVICE
  ,

  

  







  



  updateSetDefaultBrowser: function()
  {
    let shellSvc = getShellService();
    let setDefaultPane = document.getElementById("setDefaultPane");
    if (!shellSvc) {
      setDefaultPane.hidden = true;
      document.getElementById("alwaysCheckDefault").disabled = true;
      return;
    }
    let selectedIndex =
      shellSvc.isDefaultBrowser(false, true) ? 1 : 0;
    setDefaultPane.selectedIndex = selectedIndex;
  },

  


  setDefaultBrowser: function()
  {
    let shellSvc = getShellService();
    if (!shellSvc)
      return;
    shellSvc.setDefaultBrowser(true, false);
    let selectedIndex =
      shellSvc.isDefaultBrowser(false, true) ? 1 : 0;
    document.getElementById("setDefaultPane").selectedIndex = selectedIndex;
  }
#endif
};
