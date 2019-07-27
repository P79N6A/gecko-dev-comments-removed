



Components.utils.import("resource://gre/modules/Downloads.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");

var gMainPane = {
  


  init: function ()
  {
    function setEventListener(aId, aEventType, aCallback)
    {
      document.getElementById(aId)
              .addEventListener(aEventType, aCallback.bind(gMainPane));
    }

#ifdef HAVE_SHELL_SERVICE
    this.updateSetDefaultBrowser();
#ifdef XP_WIN
    
    
    
    
    
    window.setInterval(this.updateSetDefaultBrowser, 1000);
#endif
#endif

    
    this._updateUseCurrentButton();
    window.addEventListener("focus", this._updateUseCurrentButton.bind(this), false);

    this.updateBrowserStartupLastSession();

#ifdef XP_WIN
    
    try {
      let sysInfo = Cc["@mozilla.org/system-info;1"].
                    getService(Ci.nsIPropertyBag2);
      let ver = parseFloat(sysInfo.getProperty("version"));
      let showTabsInTaskbar = document.getElementById("showTabsInTaskbar");
      showTabsInTaskbar.hidden = ver < 6.1;
    } catch (ex) {}
#endif

    setEventListener("browser.privatebrowsing.autostart", "change",
                     gMainPane.updateBrowserStartupLastSession);
    setEventListener("browser.download.dir", "change",
                     gMainPane.displayDownloadDirPref);
#ifdef HAVE_SHELL_SERVICE
    setEventListener("setDefaultButton", "command",
                     gMainPane.setDefaultBrowser);
#endif
    setEventListener("useCurrent", "command",
                     gMainPane.setHomePageToCurrent);
    setEventListener("useBookmark", "command",
                     gMainPane.setHomePageToBookmark);
    setEventListener("restoreDefaultHomePage", "command",
                     gMainPane.restoreDefaultHomePage);
    setEventListener("chooseFolder", "command",
                     gMainPane.chooseFolder);

#ifdef E10S_TESTING_ONLY
    setEventListener("e10sAutoStart", "command",
                     gMainPane.enableE10SChange);
    let e10sCheckbox = document.getElementById("e10sAutoStart");
    e10sCheckbox.checked = Services.appinfo.browserTabsRemoteAutostart;

    
    
    if (!Services.appinfo.browserTabsRemoteAutostart) {
      let e10sBlockedReason = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
      let appinfo = Services.appinfo.QueryInterface(Ci.nsIObserver);
      appinfo.observe(e10sBlockedReason, "getE10SBlocked", "")
      if (e10sBlockedReason.data) {
        if (e10sBlockedReason.data == "Safe mode") {
          
          
          
          
          
          
          e10sCheckbox.checked = true;
        } else {
          e10sCheckbox.disabled = true;
          e10sCheckbox.label += " (disabled: " + e10sBlockedReason.data + ")";
        }
      }
    }

    
    
#endif

#ifdef MOZ_DEV_EDITION
    Cu.import("resource://gre/modules/osfile.jsm");
    let uAppData = OS.Constants.Path.userApplicationDataDir;
    let ignoreSeparateProfile = OS.Path.join(uAppData, "ignore-dev-edition-profile");

    setEventListener("separateProfileMode", "command", gMainPane.separateProfileModeChange);
    let separateProfileModeCheckbox = document.getElementById("separateProfileMode");
    setEventListener("getStarted", "click", gMainPane.onGetStarted);

    OS.File.stat(ignoreSeparateProfile).then(() => separateProfileModeCheckbox.checked = false,
                                             () => separateProfileModeCheckbox.checked = true);
#endif

    
    Components.classes["@mozilla.org/observer-service;1"]
              .getService(Components.interfaces.nsIObserverService)
              .notifyObservers(window, "main-pane-loaded", null);
  },

#ifdef E10S_TESTING_ONLY
  enableE10SChange: function ()
  {
    let e10sCheckbox = document.getElementById("e10sAutoStart");
    let e10sPref = document.getElementById("browser.tabs.remote.autostart");
    let e10sTempPref = document.getElementById("e10sTempPref");

    let prefsToChange;
    if (e10sCheckbox.checked) {
      
      prefsToChange = [e10sPref];
    } else {
      
      prefsToChange = [e10sPref];
      if (e10sTempPref.value) {
       prefsToChange.push(e10sTempPref);
      }
    }

    const Cc = Components.classes, Ci = Components.interfaces;
    let brandName = document.getElementById("bundleBrand").getString("brandShortName");
    let bundle = document.getElementById("bundlePreferences");
    let msg = bundle.getFormattedString(e10sCheckbox.checked ?
                                        "featureEnableRequiresRestart" : "featureDisableRequiresRestart",
                                        [brandName]);
    let title = bundle.getFormattedString("shouldRestartTitle", [brandName]);
    let shouldProceed = Services.prompt.confirm(window, title, msg)
    if (shouldProceed) {
      let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"]
                         .createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested",
                                   "restart");
      shouldProceed = !cancelQuit.data;

      if (shouldProceed) {
        for (let prefToChange of prefsToChange) {
          prefToChange.value = e10sCheckbox.checked;
        }

        let tmp = {};
        Components.utils.import("resource://gre/modules/UpdateChannel.jsm", tmp);
        if (!e10sCheckbox.checked && tmp.UpdateChannel.get() == "nightly") {
          Services.prefs.setBoolPref("browser.requestE10sFeedback", true);
          Services.prompt.alert(window, brandName, "After restart, a tab will open to input.mozilla.org where you can provide us feedback about your e10s experience.");
        }
        Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit |  Ci.nsIAppStartup.eRestart);
      }
    }

    
    e10sCheckbox.checked = e10sPref.value || e10sTempPref.value;
  },
#endif

#ifdef MOZ_DEV_EDITION
  separateProfileModeChange: function ()
  {
    function quitApp() {
      Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit |  Ci.nsIAppStartup.eRestartNotSameProfile);
    }
    function revertCheckbox(error) {
      separateProfileModeCheckbox.checked = !separateProfileModeCheckbox.checked;
      if (error) {
        Cu.reportError("Failed to toggle separate profile mode: " + error);
      }
    }

    const Cc = Components.classes, Ci = Components.interfaces;
    let separateProfileModeCheckbox = document.getElementById("separateProfileMode");
    let brandName = document.getElementById("bundleBrand").getString("brandShortName");
    let bundle = document.getElementById("bundlePreferences");
    let msg = bundle.getFormattedString(separateProfileModeCheckbox.checked ?
                                        "featureEnableRequiresRestart" : "featureDisableRequiresRestart",
                                        [brandName]);
    let title = bundle.getFormattedString("shouldRestartTitle", [brandName]);
    let shouldProceed = Services.prompt.confirm(window, title, msg)
    if (shouldProceed) {
      let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"]
                         .createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested",
                                   "restart");
      shouldProceed = !cancelQuit.data;

      if (shouldProceed) {
        Cu.import("resource://gre/modules/osfile.jsm");
        let uAppData = OS.Constants.Path.userApplicationDataDir;
        let ignoreSeparateProfile = OS.Path.join(uAppData, "ignore-dev-edition-profile");

        if (separateProfileModeCheckbox.checked) {
          OS.File.remove(ignoreSeparateProfile).then(quitApp, revertCheckbox);
        } else {
          OS.File.writeAtomic(ignoreSeparateProfile, new Uint8Array()).then(quitApp, revertCheckbox);
        }
        return;
      }
    }

    
    revertCheckbox();
  },

  onGetStarted: function (aEvent) {
    const Cc = Components.classes, Ci = Components.interfaces;
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                .getService(Ci.nsIWindowMediator);
    let win = wm.getMostRecentWindow("navigator:browser");

    if (win) {
      let accountsTab = win.gBrowser.addTab("about:accounts");
      win.gBrowser.selectedTab = accountsTab;
    }
  },
#endif

  

  


















  syncFromHomePref: function ()
  {
    let homePref = document.getElementById("browser.startup.homepage");

    
    
    if (homePref.value.toLowerCase() == "about:home")
      return "";

    
    
    
    if (homePref.value == "")
      return "about:blank";

    
    return undefined;
  },

  syncToHomePref: function (value)
  {
    
    if (value == "")
      return "about:home";

    
    return undefined;
  },

  




  setHomePageToCurrent: function ()
  {
    let homePage = document.getElementById("browser.startup.homepage");
    let tabs = this._getTabsForHomePage();
    function getTabURI(t) t.linkedBrowser.currentURI.spec;

    
    if (tabs.length)
      homePage.value = tabs.map(getTabURI).join("|");
  },

  




  setHomePageToBookmark: function ()
  {
    var rv = { urls: null, names: null };
    var dialog = gSubDialog.open("chrome://browser/content/preferences/selectBookmark.xul",
                                 "resizable=yes, modal=yes", rv,
                                 this._setHomePageToBookmarkClosed.bind(this, rv));
  },

  _setHomePageToBookmarkClosed: function(rv, aEvent) {
    if (aEvent.detail.button != "accept")
      return;
    if (rv.urls && rv.names) {
      var homePage = document.getElementById("browser.startup.homepage");

      
      homePage.value = rv.urls.join("|");
    }
  },

  



  _updateUseCurrentButton: function () {
    let useCurrent = document.getElementById("useCurrent");


    let tabs = this._getTabsForHomePage();

    if (tabs.length > 1)
      useCurrent.label = useCurrent.getAttribute("label2");
    else
      useCurrent.label = useCurrent.getAttribute("label1");

    
    if (document.getElementById
        ("pref.browser.homepage.disable_button.current_page").locked)
      return;

    useCurrent.disabled = !tabs.length
  },

  _getTabsForHomePage: function ()
  {
    var win;
    var tabs = [];

    const Cc = Components.classes, Ci = Components.interfaces;
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                .getService(Ci.nsIWindowMediator);
    win = wm.getMostRecentWindow("navigator:browser");

    if (win && win.document.documentElement
                  .getAttribute("windowtype") == "navigator:browser") {
      

      tabs = win.gBrowser.visibleTabs.slice(win.gBrowser._numPinnedTabs);
      
      tabs = tabs.filter(this.isAboutPreferences);
    }
    
    return tabs;
  },
  
  


  isAboutPreferences: function (aElement, aIndex, aArray)
  {
    return (aElement.linkedBrowser.currentURI.spec != "about:preferences");
  },

  


  restoreDefaultHomePage: function ()
  {
    var homePage = document.getElementById("browser.startup.homepage");
    homePage.value = homePage.defaultValue;
  },

  

  





























  



  readUseDownloadDir: function ()
  {
    var downloadFolder = document.getElementById("downloadFolder");
    var chooseFolder = document.getElementById("chooseFolder");
    var preference = document.getElementById("browser.download.useDownloadDir");
    downloadFolder.disabled = !preference.value;
    chooseFolder.disabled = !preference.value;

    
    return undefined;
  },

  




  chooseFolder() this.chooseFolderTask().catch(Components.utils.reportError),
  chooseFolderTask: Task.async(function* ()
  {
    let bundlePreferences = document.getElementById("bundlePreferences");
    let title = bundlePreferences.getString("chooseDownloadFolderTitle");
    let folderListPref = document.getElementById("browser.download.folderList");
    let currentDirPref = yield this._indexToFolder(folderListPref.value);
    let defDownloads = yield this._indexToFolder(1);
    let fp = Components.classes["@mozilla.org/filepicker;1"].
             createInstance(Components.interfaces.nsIFilePicker);

    fp.init(window, title, Components.interfaces.nsIFilePicker.modeGetFolder);
    fp.appendFilters(Components.interfaces.nsIFilePicker.filterAll);
    
    if (currentDirPref && currentDirPref.exists()) {
      fp.displayDirectory = currentDirPref;
    } 
    else if (defDownloads && defDownloads.exists()) {
      fp.displayDirectory = defDownloads;
    } 
    else {
      fp.displayDirectory = yield this._indexToFolder(0);
    }

    let result = yield new Promise(resolve => fp.open(resolve));
    if (result != Components.interfaces.nsIFilePicker.returnOK) {
      return;
    }

    let downloadDirPref = document.getElementById("browser.download.dir");
    downloadDirPref.value = fp.file;
    folderListPref.value = yield this._folderToIndex(fp.file);
    
    
    
    
  }),

  



  displayDownloadDirPref()
  {
    this.displayDownloadDirPrefTask().catch(Components.utils.reportError);

    
    return undefined;
  },

  displayDownloadDirPrefTask: Task.async(function* ()
  {
    var folderListPref = document.getElementById("browser.download.folderList");
    var bundlePreferences = document.getElementById("bundlePreferences");
    var downloadFolder = document.getElementById("downloadFolder");
    var currentDirPref = document.getElementById("browser.download.dir");

    
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
    var fph = ios.getProtocolHandler("file")
                 .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
    var iconUrlSpec;

    
    if (folderListPref.value == 2) {
      
      downloadFolder.label = this._getDisplayNameOfFile(currentDirPref.value);
      iconUrlSpec = fph.getURLSpecFromFile(currentDirPref.value);
    } else if (folderListPref.value == 1) {
      
      
      
      
      
      
      
      
      
      
      downloadFolder.label = bundlePreferences.getString("downloadsFolderName");
      iconUrlSpec = fph.getURLSpecFromFile(yield this._indexToFolder(1));
    } else {
      
      downloadFolder.label = bundlePreferences.getString("desktopFolderName");
      iconUrlSpec = fph.getURLSpecFromFile(yield this._getDownloadsFolder("Desktop"));
    }
    downloadFolder.image = "moz-icon://" + iconUrlSpec + "?size=16";
  }),

  


  _getDisplayNameOfFile: function (aFolder)
  {
    
    
    return aFolder ? aFolder.path : "";
  },

  








  _getDownloadsFolder: Task.async(function* (aFolder)
  {
    switch (aFolder) {
      case "Desktop":
        var fileLoc = Components.classes["@mozilla.org/file/directory_service;1"]
                                    .getService(Components.interfaces.nsIProperties);
        return fileLoc.get("Desk", Components.interfaces.nsILocalFile);
      case "Downloads":
        let downloadsDir = yield Downloads.getSystemDownloadsDirectory();
        return new FileUtils.File(downloadsDir);
    }
    throw "ASSERTION FAILED: folder type should be 'Desktop' or 'Downloads'";
  }),

  









  _folderToIndex: Task.async(function* (aFolder)
  {
    if (!aFolder || aFolder.equals(yield this._getDownloadsFolder("Desktop")))
      return 0;
    else if (aFolder.equals(yield this._getDownloadsFolder("Downloads")))
      return 1;
    return 2;
  }),

  








  _indexToFolder: Task.async(function* (aIndex)
  {
    switch (aIndex) {
      case 0:
        return yield this._getDownloadsFolder("Desktop");
      case 1:
        return yield this._getDownloadsFolder("Downloads");
    }
    var currentDirPref = document.getElementById("browser.download.dir");
    return currentDirPref.value;
  }),

  



  updateBrowserStartupLastSession: function()
  {
    let pbAutoStartPref = document.getElementById("browser.privatebrowsing.autostart");
    let startupPref = document.getElementById("browser.startup.page");
    let menu = document.getElementById("browserStartupPage");
    let option = document.getElementById("browserStartupLastSession");
    if (pbAutoStartPref.value) {
      option.setAttribute("disabled", "true");
      if (option.selected) {
        menu.selectedItem = document.getElementById("browserStartupHomePage");
      }
    } else {
      option.removeAttribute("disabled");
      startupPref.updateElements(); 
    }
  },

  

  






















  




  readLinkTarget: function() {
    var openNewWindow = document.getElementById("browser.link.open_newwindow");
    return openNewWindow.value != 2;
  },

  





  writeLinkTarget: function() {
    var linkTargeting = document.getElementById("linkTargeting");
    return linkTargeting.checked ? 3 : 2;
  }

#ifdef HAVE_SHELL_SERVICE
  ,
  







  



  updateSetDefaultBrowser: function()
  {
    let shellSvc = getShellService();
    let defaultBrowserBox = document.getElementById("defaultBrowserBox");
    if (!shellSvc) {
      defaultBrowserBox.hidden = true;
      return;
    }
    let setDefaultPane = document.getElementById("setDefaultPane");
    let selectedIndex = shellSvc.isDefaultBrowser(false, true) ? 1 : 0;
    setDefaultPane.selectedIndex = selectedIndex;
  },

  


  setDefaultBrowser: function()
  {
    let shellSvc = getShellService();
    if (!shellSvc)
      return;
    try {
      shellSvc.setDefaultBrowser(true, false);
    } catch (ex) {
      Cu.reportError(ex);
      return;
    }
    let selectedIndex =
      shellSvc.isDefaultBrowser(false, true) ? 1 : 0;
    document.getElementById("setDefaultPane").selectedIndex = selectedIndex;
  }
#endif
};
