




Components.utils.import("resource://gre/modules/Downloads.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");

var gMainPane = {
  _pane: null,

  


  init: function ()
  {
    this._pane = document.getElementById("paneMain");

#ifdef HAVE_SHELL_SERVICE
    this.updateSetDefaultBrowser();
#ifdef XP_WIN
    
    
    
    
    
    window.setInterval(this.updateSetDefaultBrowser, 1000);
#endif
#endif

    
    this._updateUseCurrentButton();
    window.addEventListener("focus", this._updateUseCurrentButton.bind(this), false);

    this.updateBrowserStartupLastSession();

#ifdef MOZ_DEV_EDITION
    let separateProfileModeCheckbox = document.getElementById("separateProfileMode");
    let listener = gMainPane.separateProfileModeChange.bind(gMainPane);
    separateProfileModeCheckbox.addEventListener("command", listener);

    let getStartedLink = document.getElementById("getStarted");
    let syncListener = gMainPane.onGetStarted.bind(gMainPane);
    getStartedLink.addEventListener("click", syncListener);

    Components.utils.import("resource://gre/modules/osfile.jsm");
    let uAppData = OS.Constants.Path.userApplicationDataDir;
    let ignoreSeparateProfile = OS.Path.join(uAppData, "ignore-dev-edition-profile");

    OS.File.stat(ignoreSeparateProfile).then(() => separateProfileModeCheckbox.checked = false,
                                             () => separateProfileModeCheckbox.checked = true);
#endif

    
    Components.classes["@mozilla.org/observer-service;1"]
              .getService(Components.interfaces.nsIObserverService)
              .notifyObservers(window, "main-pane-loaded", null);
  },

#ifdef MOZ_DEV_EDITION
  separateProfileModeChange: function ()
  {
    function quitApp() {
      Services.startup.quit(Components.interfaces.nsIAppStartup.eAttemptQuit |
                            Components.interfaces.nsIAppStartup.eRestartNotSameProfile);
    }
    function revertCheckbox(error) {
      separateProfileModeCheckbox.checked = !separateProfileModeCheckbox.checked;
      if (error) {
        Components.utils.reportError("Failed to toggle separate profile mode: " + error);
      }
    }

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
                         .createInstance(Components.interfaces.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested",
                                   "restart");
      shouldProceed = !cancelQuit.data;

      if (shouldProceed) {
        Components.utils.import("resource://gre/modules/osfile.jsm");
        let uAppData = OS.Constants.Path.userApplicationDataDir;
        let ignoreSeparateProfile = OS.Path.join(uAppData, "ignore-dev-edition-profile");

        if (separateProfileModeCheckbox.checked) {
          OS.File.remove(ignoreSeparateProfile).then(quitApp, revertCheckbox);
        } else {
          OS.File.writeAtomic(ignoreSeparateProfile, new Uint8Array()).then(quitApp, revertCheckbox);
        }
      }
    }

    
    revertCheckbox();
  },

  onGetStarted: function (aEvent) {
    const Cc = Components.classes, Ci = Components.interfaces;
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
               .getService(Components.interfaces.nsIWindowMediator);
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
    document.documentElement.openSubDialog("chrome://browser/content/preferences/selectBookmark.xul",
                                           "resizable", rv);  
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
    if (document.documentElement.instantApply) {
      const Cc = Components.classes, Ci = Components.interfaces;
      
      var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                 .getService(Components.interfaces.nsIWindowMediator);
      win = wm.getMostRecentWindow("navigator:browser");
    }
    else {
      win = window.opener;
    }

    if (win && win.document.documentElement
                  .getAttribute("windowtype") == "navigator:browser") {
      
      tabs = win.gBrowser.visibleTabs.slice(win.gBrowser._numPinnedTabs);
    }

    return tabs;
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
      Components.utils.reportError(ex);
      return;
    }
    let selectedIndex =
      shellSvc.isDefaultBrowser(false, true) ? 1 : 0;
    document.getElementById("setDefaultPane").selectedIndex = selectedIndex;
  }
#endif
};
