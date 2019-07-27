



XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");

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
    let e10sPref = document.getElementById("browser.tabs.remote.autostart");
    let e10sTempPref = document.getElementById("e10sTempPref");
    e10sCheckbox.checked = e10sPref.value || e10sTempPref.value;
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
        if (!e10sCheckbox.checked) {
          Services.prefs.setBoolPref("browser.requestE10sFeedback", true);
          Services.prompt.alert(window, brandName, "After restart, a tab will open to input.mozilla.org where you can provide us feedback about your e10s experience.");
        }
        Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit |  Ci.nsIAppStartup.eRestart);
      }
    }

    
    e10sCheckbox.checked = e10sPref.value || e10sTempPref.value;
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

  




  chooseFolder: function ()
  {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    const nsILocalFile = Components.interfaces.nsILocalFile;

    let bundlePreferences = document.getElementById("bundlePreferences");
    let title = bundlePreferences.getString("chooseDownloadFolderTitle");
    let folderListPref = document.getElementById("browser.download.folderList");
    let currentDirPref = this._indexToFolder(folderListPref.value); 
    let defDownloads = this._indexToFolder(1); 
    let fp = Components.classes["@mozilla.org/filepicker;1"].
             createInstance(nsIFilePicker);
    let fpCallback = function fpCallback_done(aResult) {
      if (aResult == nsIFilePicker.returnOK) {
        let file = fp.file.QueryInterface(nsILocalFile);
        let downloadDirPref = document.getElementById("browser.download.dir");

        downloadDirPref.value = file;
        folderListPref.value = this._folderToIndex(file);
        
        
        
        
      }
    }.bind(this);

    fp.init(window, title, nsIFilePicker.modeGetFolder);
    fp.appendFilters(nsIFilePicker.filterAll);
    
    if (currentDirPref && currentDirPref.exists()) {
      fp.displayDirectory = currentDirPref;
    } 
    else if (defDownloads && defDownloads.exists()) {
      fp.displayDirectory = defDownloads;
    } 
    else {
      fp.displayDirectory = this._indexToFolder(0);
    }
    fp.open(fpCallback);
  },

  



  displayDownloadDirPref: function ()
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
      iconUrlSpec = fph.getURLSpecFromFile(this._indexToFolder(1));
    } else {
      
      downloadFolder.label = bundlePreferences.getString("desktopFolderName");
      iconUrlSpec = fph.getURLSpecFromFile(desk);
    }
    downloadFolder.image = "moz-icon://" + iconUrlSpec + "?size=16";

    
    return undefined;
  },

  


  _getDisplayNameOfFile: function (aFolder)
  {
    
    
    return aFolder ? aFolder.path : "";
  },

  








  _getDownloadsFolder: function (aFolder)
  {
    switch (aFolder) {
      case "Desktop":
        var fileLoc = Components.classes["@mozilla.org/file/directory_service;1"]
                                    .getService(Components.interfaces.nsIProperties);
        return fileLoc.get("Desk", Components.interfaces.nsILocalFile);
      break;
      case "Downloads":
        var dnldMgr = Components.classes["@mozilla.org/download-manager;1"]
                                .getService(Components.interfaces.nsIDownloadManager);
        return dnldMgr.defaultDownloadsDirectory;
      break;
    }
    throw "ASSERTION FAILED: folder type should be 'Desktop' or 'Downloads'";
  },

  









  _folderToIndex: function (aFolder)
  {
    if (!aFolder || aFolder.equals(this._getDownloadsFolder("Desktop")))
      return 0;
    else if (aFolder.equals(this._getDownloadsFolder("Downloads")))
      return 1;
    return 2;
  },

  








  _indexToFolder: function (aIndex)
  {
    switch (aIndex) {
      case 0:
        return this._getDownloadsFolder("Desktop");
      case 1:
        return this._getDownloadsFolder("Downloads");
    }
    var currentDirPref = document.getElementById("browser.download.dir");
    return currentDirPref.value;
  },

  


  getFolderListPref: function ()
  {
    var folderListPref = document.getElementById("browser.download.folderList");
    switch (folderListPref.value) {
      case 0: 
      case 1: 
        return folderListPref.value;
      break;
      case 2: 
        var currentDirPref = document.getElementById("browser.download.dir");
        if (currentDirPref.value) {
          
          
          return this._folderToIndex(currentDirPref.value);
        }
        return 0;
      break;
    }
  },

  



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
    shellSvc.setDefaultBrowser(true, false);
    let selectedIndex =
      shellSvc.isDefaultBrowser(false, true) ? 1 : 0;
    document.getElementById("setDefaultPane").selectedIndex = selectedIndex;
  }
#endif
};
