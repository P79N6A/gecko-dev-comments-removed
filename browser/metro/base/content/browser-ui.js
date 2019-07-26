



"use strict";

Cu.import("resource://gre/modules/PageThumbs.jsm");
Cu.import("resource://gre/modules/devtools/dbg-server.jsm")






const kStartOverlayURI = "about:start";


const debugServerStateChanged = "devtools.debugger.remote-enabled";
const debugServerPortChanged = "devtools.debugger.remote-port";


const kNewTabAnimationDelayMsec = 1000;

const kOpenInNewTabAnimationDelayMsec = 3000;

const kSelectTabAnimationDelayMsec = 500;





let Elements = {};
[
  ["contentShowing",     "bcast_contentShowing"],
  ["urlbarState",        "bcast_urlbarState"],
  ["windowState",        "bcast_windowState"],
  ["mainKeyset",         "mainKeyset"],
  ["stack",              "stack"],
  ["tabList",            "tabs"],
  ["tabs",               "tabs-container"],
  ["controls",           "browser-controls"],
  ["panelUI",            "panel-container"],
  ["startUI",            "start-container"],
  ["tray",               "tray"],
  ["toolbar",            "toolbar"],
  ["browsers",           "browsers"],
  ["navbar",             "navbar"],
  ["autocomplete",       "urlbar-autocomplete"],
  ["contextappbar",      "contextappbar"],
  ["findbar",            "findbar"],
  ["contentViewport",    "content-viewport"],
  ["progress",           "progress-control"],
  ["progressContainer",  "progress-container"],
].forEach(function (aElementGlobal) {
  let [name, id] = aElementGlobal;
  XPCOMUtils.defineLazyGetter(Elements, name, function() {
    return document.getElementById(id);
  });
});





var Strings = {};
[
  ["browser",    "chrome://browser/locale/browser.properties"],
  ["brand",      "chrome://branding/locale/brand.properties"]
].forEach(function (aStringBundle) {
  let [name, bundle] = aStringBundle;
  XPCOMUtils.defineLazyGetter(Strings, name, function() {
    return Services.strings.createBundle(bundle);
  });
});

var BrowserUI = {
  get _edit() { return document.getElementById("urlbar-edit"); },
  get _back() { return document.getElementById("cmd_back"); },
  get _forward() { return document.getElementById("cmd_forward"); },

  lastKnownGoodURL: "", 
  ready: false, 

  init: function() {
    
    if (Services.prefs.getBoolPref(debugServerStateChanged)) {
      this.runDebugServer();
    }
    Services.prefs.addObserver(debugServerStateChanged, this, false);
    Services.prefs.addObserver(debugServerPortChanged, this, false);

    
    messageManager.addMessageListener("DOMTitleChanged", this);
    messageManager.addMessageListener("DOMWillOpenModalDialog", this);
    messageManager.addMessageListener("DOMWindowClose", this);

    messageManager.addMessageListener("Browser:OpenURI", this);
    messageManager.addMessageListener("Browser:SaveAs:Return", this);
    messageManager.addMessageListener("Content:StateChange", this);

    
    window.addEventListener("keypress", this, true);

    window.addEventListener("MozPrecisePointer", this, true);
    window.addEventListener("MozImprecisePointer", this, true);

    Services.prefs.addObserver("browser.cache.disk_cache_ssl", this, false);
    Services.obs.addObserver(this, "metro_viewstate_changed", false);

    
    ContextUI.init();
    StartUI.init();
    PanelUI.init();
    FlyoutPanelsUI.init();
    PageThumbs.init();
    SettingsCharm.init();
    NavButtonSlider.init();

    
    BrowserUI._adjustDOMforViewState();

    
    
    messageManager.addMessageListener("pageshow", function onPageShow() {
      if (getBrowser().currentURI.spec == "about:blank")
        return;

      messageManager.removeMessageListener("pageshow", onPageShow);

      setTimeout(function() {
        let event = document.createEvent("Events");
        event.initEvent("UIReadyDelayed", true, false);
        window.dispatchEvent(event);
        BrowserUI.ready = true;
      }, 0);
    });

    
    messageManager.addMessageListener("IndexedDB:Prompt", function(aMessage) {
      return IndexedDB.receiveMessage(aMessage);
    });

    
    window.addEventListener("UIReadyDelayed", function delayedInit(aEvent) {
      Util.dumpLn("* delay load started...");
      window.removeEventListener("UIReadyDelayed",  delayedInit, false);

      
      Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
      Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2);

      messageManager.addMessageListener("Browser:MozApplicationManifest", OfflineApps);

      try {
        Downloads.init();
        DialogUI.init();
        FormHelperUI.init();
        FindHelperUI.init();
        PdfJs.init();
      } catch(ex) {
        Util.dumpLn("Exception in delay load module:", ex.message);
      }

      BrowserUI._pullDesktopControlledPrefs();

      
      BrowserUI.startupCrashCheck();

      Util.dumpLn("* delay load complete.");
    }, false);

#ifndef MOZ_OFFICIAL_BRANDING
    setTimeout(function() {
      let startup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup).getStartupInfo();
      for (let name in startup) {
        if (name != "process")
          Services.console.logStringMessage("[timing] " + name + ": " + (startup[name] - startup.process) + "ms");
      }
    }, 3000);
#endif
  },

  uninit: function() {
    messageManager.removeMessageListener("Browser:MozApplicationManifest", OfflineApps);

    PanelUI.uninit();
    StartUI.uninit();
    Downloads.uninit();
    SettingsCharm.uninit();
    messageManager.removeMessageListener("Content:StateChange", this);
    PageThumbs.uninit();
    this.stopDebugServer();
  },

  


  runDebugServer: function runDebugServer(aPort) {
    let port = aPort || Services.prefs.getIntPref(debugServerPortChanged);
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
      DebuggerServer.addActors('chrome://browser/content/dbg-metro-actors.js');
    }
    DebuggerServer.openListener(port);
  },

  stopDebugServer: function stopDebugServer() {
    if (DebuggerServer.initialized) {
      DebuggerServer.destroy();
    }
  },

  
  
  
  
  changeDebugPort:function changeDebugPort(aPort) {
    if (DebuggerServer.initialized) {
      this.stopDebugServer();
      this.runDebugServer(aPort);
    }
  },

  



  get isContentShowing() {
    return Elements.contentShowing.getAttribute("disabled") != true;
  },

  showContent: function showContent(aURI) {
    StartUI.update(aURI);
    ContextUI.dismissTabs();
    ContextUI.dismissContextAppbar();
    FlyoutPanelsUI.hide();
    PanelUI.hide();
  },

  



  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },

  get lastCrashID() {
    return Cc["@mozilla.org/xre/runtime;1"].getService(Ci.nsIXULRuntime).lastRunCrashID;
  },

  startupCrashCheck: function startupCrashCheck() {
#ifdef MOZ_CRASHREPORTER
    if (!CrashReporter.enabled) {
      return;
    }
    let lastCrashID = this.lastCrashID;
    if (!lastCrashID || !lastCrashID.length) {
      return;
    }
    let shouldReport = Services.prefs.getBoolPref("app.crashreporter.autosubmit");
    let didPrompt = Services.prefs.getBoolPref("app.crashreporter.prompted");

    if (!shouldReport && !didPrompt) {
      
      
      
      Services.prefs.setBoolPref("app.crashreporter.prompted", true);
      DialogUI.importModal(document, "chrome://browser/content/prompt/crash.xul");
      return;
    }

    
    if (!shouldReport && didPrompt) {
      return;
    }

    Util.dumpLn("Submitting last crash id:", lastCrashID);
    try {
      this.CrashSubmit.submit(lastCrashID);
    } catch (ex) {
      Util.dumpLn(ex);
    }
#endif
  },


  



  
  NO_STARTUI_VISIBILITY:  1, 

  



  update: function(aFlags) {
    let flags = aFlags || 0;
    if (!(flags & this.NO_STARTUI_VISIBILITY)) {
      let uri = this.getDisplayURI(Browser.selectedBrowser);
      StartUI.update(uri);
    }
    this._updateButtons();
    this._updateToolbar();
  },

  
  updateURI: function(aOptions) {
    let uri = this.getDisplayURI(Browser.selectedBrowser);
    let cleanURI = Util.isURLEmpty(uri) ? "" : uri;
    this._edit.value = cleanURI;
  },

  getDisplayURI: function(browser) {
    let uri = browser.currentURI;
    let spec = uri.spec;

    try {
      spec = gURIFixup.createExposableURI(uri).spec;
    } catch (ex) {}

    try {
      let charset = browser.characterSet;
      let textToSubURI = Cc["@mozilla.org/intl/texttosuburi;1"].
                         getService(Ci.nsITextToSubURI);
      spec = textToSubURI.unEscapeNonAsciiURI(charset, spec);
    } catch (ex) {}

    return spec;
  },

  goToURI: function(aURI) {
    aURI = aURI || this._edit.value;
    if (!aURI)
      return;

    this._edit.value = aURI;

    
    Util.forceOnline();

    BrowserUI.showContent(aURI);
    Browser.selectedBrowser.focus();

    Task.spawn(function() {
      let postData = {};
      aURI = yield Browser.getShortcutOrURI(aURI, postData);
      Browser.loadURI(aURI, { flags: Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP, postData: postData });

      
      
      let fixupFlags = Ci.nsIURIFixup.FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP;
      let uri = gURIFixup.createFixupURI(aURI, fixupFlags);
      gHistSvc.markPageAsTyped(uri);

      BrowserUI._titleChanged(Browser.selectedBrowser);
    });
  },

  doOpenSearch: function doOpenSearch(aName) {
    
    let searchValue = this._edit.value;
    let engine = Services.search.getEngineByName(aName);
    let submission = engine.getSubmission(searchValue, null);

    this._edit.value = submission.uri.spec;

    
    Util.forceOnline();

    BrowserUI.showContent();
    Browser.selectedBrowser.focus();

    Task.spawn(function () {
      Browser.loadURI(submission.uri.spec, { postData: submission.postData });

      
      Browser.selectedBrowser.userTypedValue = submission.uri.spec;
      BrowserUI._titleChanged(Browser.selectedBrowser);
    });
  },

  



  newTab: function newTab(aURI, aOwner) {
    aURI = aURI || kStartOverlayURI;
    let tab = Browser.addTab(aURI, true, aOwner);
    return tab;
  },

  setOnTabAnimationEnd: function setOnTabAnimationEnd(aCallback) {
    Elements.tabs.addEventListener("animationend", function onAnimationEnd() {
      Elements.tabs.removeEventListener("animationend", onAnimationEnd);
      aCallback();
    });
  },

  closeTab: function closeTab(aTab) {
    
    let tab = aTab || Browser.selectedTab;
    Browser.closeTab(tab);
  },

  animateClosingTab: function animateClosingTab(tabToClose) {
    tabToClose.chromeTab.setAttribute("closing", "true");

    let wasCollapsed = !ContextUI.tabbarVisible;
    if (wasCollapsed) {
      ContextUI.displayTabs();
    }

    this.setOnTabAnimationEnd(function() {
      Browser.closeTab(tabToClose, { forceClose: true } );
      if (wasCollapsed)
        ContextUI.dismissTabsWithDelay(kNewTabAnimationDelayMsec);
    });
  },

  





  undoCloseTab: function undoCloseTab(aIndex) {
    var tab = null;
    aIndex = aIndex || 0;
    var ss = Cc["@mozilla.org/browser/sessionstore;1"].
                getService(Ci.nsISessionStore);
    if (ss.getClosedTabCount(window) > (aIndex)) {
      tab = ss.undoCloseTab(window, aIndex);
    }
    return tab;
  },

  
  
  closeTabForBrowser: function closeTabForBrowser(aBrowser) {
    
    let browsers = Browser.browsers;
    for (let i = 0; i < browsers.length; i++) {
      if (browsers[i] == aBrowser) {
        Browser.closeTab(Browser.getTabAtIndex(i));
        return { preventDefault: true };
      }
    }

    return {};
  },

  selectTab: function selectTab(aTab) {
    Browser.selectedTab = aTab;
  },

  selectTabAndDismiss: function selectTabAndDismiss(aTab) {
    this.selectTab(aTab);
    ContextUI.dismissTabsWithDelay(kSelectTabAnimationDelayMsec);
  },

  selectTabAtIndex: function selectTabAtIndex(aIndex) {
    
    if (aIndex < 0)
      aIndex += Browser._tabs.length;

    if (aIndex >= 0 && aIndex < Browser._tabs.length)
      Browser.selectedTab = Browser._tabs[aIndex];
  },

  selectNextTab: function selectNextTab() {
    if (Browser._tabs.length == 1 || !Browser.selectedTab) {
     return;
    }

    let tabIndex = Browser._tabs.indexOf(Browser.selectedTab) + 1;
    if (tabIndex >= Browser._tabs.length) {
      tabIndex = 0;
    }

    Browser.selectedTab = Browser._tabs[tabIndex];
  },

  selectPreviousTab: function selectPreviousTab() {
    if (Browser._tabs.length == 1 || !Browser.selectedTab) {
      return;
    }

    let tabIndex = Browser._tabs.indexOf(Browser.selectedTab) - 1;
    if (tabIndex < 0) {
      tabIndex = Browser._tabs.length - 1;
    }

    Browser.selectedTab = Browser._tabs[tabIndex];
  },

  
  
  selectTabForBrowser: function selectTabForBrowser(aBrowser) {
    for (let i = 0; i < Browser.tabs.length; i++) {
      if (Browser._tabs[i].browser == aBrowser) {
        Browser.selectedTab = Browser.tabs[i];
        break;
      }
    }
  },

  updateUIFocus: function _updateUIFocus() {
    if (Elements.contentShowing.getAttribute("disabled") == "true" && Browser.selectedBrowser)
      Browser.selectedBrowser.messageManager.sendAsyncMessage("Browser:Blur", { });
  },

  blurFocusedElement: function blurFocusedElement() {
    let focusedElement = document.commandDispatcher.focusedElement;
    if (focusedElement)
      focusedElement.blur();
  },

  blurNavBar: function blurNavBar() {
    if (this._edit.focused) {
      this._edit.blur();
      return true;
    }
    return false;
  },

  observe: function BrowserUI_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "nsPref:changed":
        switch (aData) {
          case "browser.cache.disk_cache_ssl":
            this._sslDiskCacheEnabled = Services.prefs.getBoolPref(aData);
            break;
          case debugServerStateChanged:
            if (Services.prefs.getBoolPref(aData)) {
              this.runDebugServer();
            } else {
              this.stopDebugServer();
            }
            break;
          case debugServerPortChanged:
            this.changeDebugPort(Services.prefs.getIntPref(aData));
            break;
        }
        break;
      case "metro_viewstate_changed":
        this._adjustDOMforViewState();
        if (aData == "snapped") {
          FlyoutPanelsUI.hide();
          Elements.autocomplete.setAttribute("orient", "vertical");
        }
        else {
          Elements.autocomplete.setAttribute("orient", "horizontal");
        }

        break;
    }
  },

  



  



  _pullDesktopControlledPrefs: function() {
    function pullDesktopControlledPrefType(prefType, prefFunc) {
      try {
        registry.create(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                      "Software\\Mozilla\\Firefox\\Metro\\Prefs\\" + prefType,
                      Ci.nsIWindowsRegKey.ACCESS_ALL);
        for (let i = 0; i < registry.valueCount; i++) {
          let prefName = registry.getValueName(i);
          let prefValue = registry.readStringValue(prefName);
          if (prefType == Ci.nsIPrefBranch.PREF_BOOL) {
            prefValue = prefValue == "true";
          }
          Services.prefs[prefFunc](prefName, prefValue);
        }
      } catch (ex) {
        Util.dumpLn("Could not pull for prefType " + prefType + ": " + ex);
      } finally {
        registry.close();
      }
    }
    let registry = Cc["@mozilla.org/windows-registry-key;1"].
                   createInstance(Ci.nsIWindowsRegKey);
    pullDesktopControlledPrefType(Ci.nsIPrefBranch.PREF_INT, "setIntPref");
    pullDesktopControlledPrefType(Ci.nsIPrefBranch.PREF_BOOL, "setBoolPref");
    pullDesktopControlledPrefType(Ci.nsIPrefBranch.PREF_STRING, "setCharPref");
  },

  _adjustDOMforViewState: function() {
    if (MetroUtils.immersive) {
      let currViewState = "";
      switch (MetroUtils.snappedState) {
        case Ci.nsIWinMetroUtils.fullScreenLandscape:
          currViewState = "landscape";
          break;
        case Ci.nsIWinMetroUtils.fullScreenPortrait:
          currViewState = "portrait";
          break;
        case Ci.nsIWinMetroUtils.filled:
          currViewState = "filled";
          break;
        case Ci.nsIWinMetroUtils.snapped:
          currViewState = "snapped";
          break;
      }
      Elements.windowState.setAttribute("viewstate", currViewState);
    }
  },

  _titleChanged: function(aBrowser) {
    let url = this.getDisplayURI(aBrowser);

    let tabCaption;
    if (aBrowser.contentTitle) {
      tabCaption = aBrowser.contentTitle;
    } else if (!Util.isURLEmpty(aBrowser.userTypedValue)) {
      tabCaption = aBrowser.userTypedValue;
    } else if (!Util.isURLEmpty(url)) {
      tabCaption = url;
    } else {
      tabCaption = Util.getEmptyURLTabTitle();
    }

    let tab = Browser.getTabForBrowser(aBrowser);
    if (tab)
      tab.chromeTab.updateTitle(tabCaption);
  },

  _updateButtons: function _updateButtons() {
    let browser = Browser.selectedBrowser;
    if (!browser) {
      return;
    }
    if (browser.canGoBack) {
      this._back.removeAttribute("disabled");
    } else {
      this._back.setAttribute("disabled", true);
    }
    if (browser.canGoForward) {
      this._forward.removeAttribute("disabled");
    } else {
      this._forward.setAttribute("disabled", true);
    }
  },

  _updateToolbar: function _updateToolbar() {
    let mode = Elements.urlbarState.getAttribute("mode");
    let isLoading = Browser.selectedTab.isLoading();

    if (isLoading && mode != "loading")
      Elements.urlbarState.setAttribute("mode", "loading");
    else if (!isLoading && mode != "edit")
      Elements.urlbarState.setAttribute("mode", "view");
  },

  _closeOrQuit: function _closeOrQuit() {
    
    if (!BrowserUI.isContentShowing()) {
      BrowserUI.showContent();
    } else {
      
      if (Browser.closing()) {
        window.close();
        let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
        appStartup.quit(Ci.nsIAppStartup.eForceQuit);
      }
    }
  },

  _onPreciseInput: function _onPreciseInput() {
    document.getElementById("bcast_preciseInput").setAttribute("input", "precise");
    let uri = Util.makeURI("chrome://browser/content/cursor.css");
    if (StyleSheetSvc.sheetRegistered(uri, Ci.nsIStyleSheetService.AGENT_SHEET)) {
      StyleSheetSvc.unregisterSheet(uri,
                                    Ci.nsIStyleSheetService.AGENT_SHEET);
    }
  },

  _onImpreciseInput: function _onImpreciseInput() {
    document.getElementById("bcast_preciseInput").setAttribute("input", "imprecise");
    let uri = Util.makeURI("chrome://browser/content/cursor.css");
    if (!StyleSheetSvc.sheetRegistered(uri, Ci.nsIStyleSheetService.AGENT_SHEET)) {
      StyleSheetSvc.loadAndRegisterSheet(uri,
                                         Ci.nsIStyleSheetService.AGENT_SHEET);
    }
  },

  



  handleEvent: function handleEvent(aEvent) {
    var target = aEvent.target;
    switch (aEvent.type) {
      
      case "keypress":
        if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE)
          this.handleEscape(aEvent);
        break;
      case "MozPrecisePointer":
        this._onPreciseInput();
        break;
      case "MozImprecisePointer":
        this._onImpreciseInput();
        break;
    }
  },

  
  
  handleEscape: function (aEvent) {
    aEvent.stopPropagation();
    aEvent.preventDefault();

    if (this._edit.popupOpen) {
      this._edit.endEditing(true);
      return;
    }

    
    if (DialogUI._popup) {
      DialogUI._hidePopup();
      return;
    }

    
    if (DialogUI.modals.length > 0) {
      return;
    }

    
    if (PanelUI.isVisible) {
      PanelUI.hide();
      return;
    }

    
    if (FindHelperUI.isActive) {
      FindHelperUI.hide();
      return;
    }

    if (StartUI.hide()) {
      
      ContextUI.dismiss();
      return;
    }

    if (Browser.selectedTab.isLoading()) {
      Browser.selectedBrowser.stop();
      return;
    }

    if (ContextUI.dismiss()) {
      return;
    }
  },

  handleBackspace: function handleBackspace() {
    switch (Services.prefs.getIntPref("browser.backspace_action")) {
      case 0:
        CommandUpdater.doCommand("cmd_back");
        break;
      case 1:
        CommandUpdater.doCommand("cmd_scrollPageUp");
        break;
    }
  },

  handleShiftBackspace: function handleShiftBackspace() {
    switch (Services.prefs.getIntPref("browser.backspace_action")) {
      case 0:
        CommandUpdater.doCommand("cmd_forward");
        break;
      case 1:
        CommandUpdater.doCommand("cmd_scrollPageDown");
        break;
    }
  },

  openFile: function() {
    try {
      const nsIFilePicker = Ci.nsIFilePicker;
      let fp = Cc["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
      let self = this;
      let fpCallback = function fpCallback_done(aResult) {
        if (aResult == nsIFilePicker.returnOK) {
          self.goToURI(fp.fileURL.spec);
        }
      };

      let windowTitle = Strings.browser.GetStringFromName("browserForOpenLocation");
      fp.init(window, windowTitle, nsIFilePicker.modeOpen);
      fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText |
                       nsIFilePicker.filterImages | nsIFilePicker.filterXML |
                       nsIFilePicker.filterHTML);
      fp.open(fpCallback);
    } catch (ex) {
      dump ('BrowserUI openFile exception: ' + ex + '\n');
    }
  },

  savePage: function() {
    Browser.savePage();
  },

  receiveMessage: function receiveMessage(aMessage) {
    let browser = aMessage.target;
    let json = aMessage.json;
    switch (aMessage.name) {
      case "DOMTitleChanged":
        this._titleChanged(browser);
        break;
      case "DOMWillOpenModalDialog":
        this.selectTabForBrowser(browser);
        break;
      case "DOMWindowClose":
        return this.closeTabForBrowser(browser);
        break;
      
      case "Browser:OpenURI":
        let referrerURI = null;
        if (json.referrer)
          referrerURI = Services.io.newURI(json.referrer, null, null);
        
        this.goToURI(json.uri);
        break;
      case "Content:StateChange":
        let currBrowser = Browser.selectedBrowser;
        if (this.shouldCaptureThumbnails(currBrowser)) {
          PageThumbs.captureAndStore(currBrowser);
          let currPage = currBrowser.currentURI.spec;
          Services.obs.notifyObservers(null, "Metro:RefreshTopsiteThumbnail", currPage);
        }
        break;
    }

    return {};
  },

  
  
  shouldCaptureThumbnails: function shouldCaptureThumbnails(aBrowser) {
    
    if (aBrowser != Browser.selectedBrowser) {
      return false;
    }
    
    
    let doc = aBrowser.contentDocument;
    if (doc instanceof SVGDocument || doc instanceof XMLDocument) {
      return false;
    }

    
    
    
    
    if(Elements.windowState.getAttribute("viewstate") == "snapped") {
      return false;
    }
    
    if (aBrowser.docShell.busyFlags != Ci.nsIDocShell.BUSY_FLAGS_NONE) {
      return false;
    }

    
    if (aBrowser.currentURI.schemeIs("about")) {
      return false;
    }

    
    let channel = aBrowser.docShell.currentDocumentChannel;
    if (!channel) {
      return false;
    }

    
    
    let uri = channel.originalURI;
    if (uri.schemeIs("about")) {
      return false;
    }

    
    let httpChannel;
    try {
      httpChannel = channel.QueryInterface(Ci.nsIHttpChannel);
    } catch (e) {  }

    if (httpChannel) {
      
      try {
        if (Math.floor(httpChannel.responseStatus / 100) != 2) {
          return false;
        }
      } catch (e) {
        
        
        return false;
      }

      
      if (httpChannel.isNoStoreResponse()) {
        return false;
      }

      
      if (uri.schemeIs("https") && !this.sslDiskCacheEnabled) {
        return false;
      }
    }

    return true;
  },

  _sslDiskCacheEnabled: null,

  get sslDiskCacheEnabled() {
    if (this._sslDiskCacheEnabled === null) {
      this._sslDiskCacheEnabled = Services.prefs.getBoolPref("browser.cache.disk_cache_ssl");
    }
    return this._sslDiskCacheEnabled;
  },

  supportsCommand : function(cmd) {
    var isSupported = false;
    switch (cmd) {
      case "cmd_back":
      case "cmd_forward":
      case "cmd_reload":
      case "cmd_forceReload":
      case "cmd_stop":
      case "cmd_go":
      case "cmd_home":
      case "cmd_openLocation":
      case "cmd_addBookmark":
      case "cmd_bookmarks":
      case "cmd_history":
      case "cmd_remoteTabs":
      case "cmd_quit":
      case "cmd_close":
      case "cmd_newTab":
      case "cmd_closeTab":
      case "cmd_undoCloseTab":
      case "cmd_actions":
      case "cmd_panel":
      case "cmd_flyout_back":
      case "cmd_sanitize":
      case "cmd_volumeLeft":
      case "cmd_volumeRight":
      case "cmd_openFile":
      case "cmd_savePage":
        isSupported = true;
        break;
      default:
        isSupported = false;
        break;
    }
    return isSupported;
  },

  isCommandEnabled : function(cmd) {
    let elem = document.getElementById(cmd);
    if (elem && elem.getAttribute("disabled") == "true")
      return false;
    return true;
  },

  doCommand : function(cmd) {
    if (!this.isCommandEnabled(cmd))
      return;
    let browser = getBrowser();
    switch (cmd) {
      case "cmd_back":
        browser.goBack();
        break;
      case "cmd_forward":
        browser.goForward();
        break;
      case "cmd_reload":
        browser.reload();
        break;
      case "cmd_forceReload":
      {
        
        browser.lastLocation = null;

        const reloadFlags = Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY |
                            Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE;
        browser.reloadWithFlags(reloadFlags);
        break;
      }
      case "cmd_stop":
        browser.stop();
        break;
      case "cmd_go":
        this.goToURI();
        break;
      case "cmd_home":
        this.goToURI(Browser.getHomePage());
        break;
      case "cmd_openLocation":
        ContextUI.displayNavbar();
        this._edit.beginEditing(true);
        this._edit.select();
        break;
      case "cmd_addBookmark":
        ContextUI.displayNavbar();
        Appbar.onStarButton(true);
        break;
      case "cmd_bookmarks":
        PanelUI.show("bookmarks-container");
        break;
      case "cmd_history":
        PanelUI.show("history-container");
        break;
      case "cmd_remoteTabs":
#ifdef MOZ_SERVICES_SYNC
        if (Weave.Status.checkSetup() == Weave.CLIENT_NOT_CONFIGURED) {
          FlyoutPanelsUI.show('SyncFlyoutPanel');
        } else {
          PanelUI.show("remotetabs-container");
        }
#endif
        break;
      case "cmd_quit":
        
        this._closeOrQuit();
        break;
      case "cmd_close":
        this._closeOrQuit();
        break;
      case "cmd_newTab":
        this.newTab();
        this._edit.beginEditing(false);
        ContextUI.peekTabs(kNewTabAnimationDelayMsec);
        break;
      case "cmd_closeTab":
        this.closeTab();
        break;
      case "cmd_undoCloseTab":
        this.undoCloseTab();
        break;
      case "cmd_sanitize":
        SanitizeUI.onSanitize();
        break;
      case "cmd_flyout_back":
        FlyoutPanelsUI.onBackButton();
        break;
      case "cmd_panel":
        PanelUI.toggle();
        break;
      case "cmd_volumeLeft":
        
        Browser.zoom(Util.isPortrait() ? -1 : 1);
        break;
      case "cmd_volumeRight":
        
        Browser.zoom(Util.isPortrait() ? 1 : -1);
        break;
      case "cmd_openFile":
        this.openFile();
        break;
      case "cmd_savePage":
        this.savePage();
        break;
    }
  },

  crashReportingPrefChanged: function crashReportingPrefChanged(aState) {
    CrashReporter.submitReports = aState;
  }
};

var StartUI = {
  get isVisible() { return this.isStartPageVisible; },
  get isStartPageVisible() { return Elements.windowState.hasAttribute("startpage"); },

  get maxResultsPerSection() {
    return Services.prefs.getIntPref("browser.display.startUI.maxresults");
  },

  sections: [
    "TopSitesStartView",
    "BookmarksStartView",
    "HistoryStartView",
    "RemoteTabsStartView"
  ],

  init: function init() {
    Elements.startUI.addEventListener("contextmenu", this, false);
    Elements.startUI.addEventListener("click", this, false);
    Elements.startUI.addEventListener("MozMousePixelScroll", this, false);

    this.sections.forEach(function (sectionName) {
      let section = window[sectionName];
      if (section.init)
        section.init();
    });

  },

  uninit: function() {
    this.sections.forEach(function (sectionName) {
      let section = window[sectionName];
      if (section.uninit)
        section.uninit();
    });
  },

  
  show: function show() {
    if (this.isStartPageVisible)
      return false;

    ContextUI.displayNavbar();

    Elements.contentShowing.setAttribute("disabled", "true");
    Elements.windowState.setAttribute("startpage", "true");

    this.sections.forEach(function (sectionName) {
      let section = window[sectionName];
      if (section.show)
        section.show();
    });
    return true;
  },

  
  hide: function hide(aURI) {
    aURI = aURI || Browser.selectedBrowser.currentURI.spec;
    if (!this.isStartPageVisible || this.isStartURI(aURI))
      return false;

    Elements.contentShowing.removeAttribute("disabled");
    Elements.windowState.removeAttribute("startpage");
    return true;
  },

  
  isStartURI: function isStartURI(aURI) {
    aURI = aURI || Browser.selectedBrowser.currentURI.spec;
    return aURI == kStartOverlayURI || aURI == "about:home";
  },

  
  update: function update(aURI) {
    aURI = aURI || Browser.selectedBrowser.currentURI.spec;
    if (this.isStartURI(aURI)) {
      this.show();
    } else if (aURI != "about:blank") { 
      this.hide(aURI);
    }
  },

  onClick: function onClick(aEvent) {
    
    
    if (BrowserUI.blurNavBar()) {
      
      
      ContentAreaObserver.navBarWillBlur();
    }

    if (aEvent.button == 0)
      ContextUI.dismissTabs();
  },

  onNarrowTitleClick: function onNarrowTitleClick(sectionId) {
    let section = document.getElementById(sectionId);

    if (section.hasAttribute("expanded"))
      return;

    for (let expandedSection of Elements.startUI.querySelectorAll(".meta-section[expanded]"))
      expandedSection.removeAttribute("expanded")

    section.setAttribute("expanded", "true");
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "contextmenu":
        let event = document.createEvent("Events");
        event.initEvent("MozEdgeUICompleted", true, false);
        window.dispatchEvent(event);
        break;
      case "click":
        this.onClick(aEvent);
        break;

      case "MozMousePixelScroll":
        let startBox = document.getElementById("start-scrollbox");
        let [, scrollInterface] = ScrollUtils.getScrollboxFromElement(startBox);

        if (Elements.windowState.getAttribute("viewstate") == "snapped") {
          scrollInterface.scrollBy(0, aEvent.detail);
        } else {
          scrollInterface.scrollBy(aEvent.detail, 0);
        }

        aEvent.preventDefault();
        aEvent.stopPropagation();
        break;
    }
  }
};

var PanelUI = {
  get _panels() { return document.getElementById("panel-items"); },

  get isVisible() {
    return !Elements.panelUI.hidden;
  },

  views: {
    "console-container": "ConsolePanelView",
  },

  init: function() {
    
    setTimeout(function () {
      for each (let viewName in this.views) {
        let view = window[viewName];
        if (view.init)
          view.init();
      }
    }.bind(this), 0);

    
    this._panels.addEventListener("ToolPanelShown", function(aEvent) {
      let viewName = this.views[this._panels.selectedPanel.id];
      let view = window[viewName];
      if (view.show)
        view.show();
    }.bind(this), true);
  },

  uninit: function() {
    for each (let viewName in this.views) {
      let view = window[viewName];
      if (view.uninit)
        view.uninit();
    }
  },

  switchPane: function switchPane(aPanelId) {
    BrowserUI.blurFocusedElement();

    let panel = aPanelId ? document.getElementById(aPanelId) : this._panels.selectedPanel;
    let oldPanel = this._panels.selectedPanel;

    if (oldPanel != panel) {
      this._panels.selectedPanel = panel;

      this._fire("ToolPanelHidden", oldPanel);
    }

    this._fire("ToolPanelShown", panel);
  },

  isPaneVisible: function isPaneVisible(aPanelId) {
    return this.isVisible && this._panels.selectedPanel.id == aPanelId;
  },

  show: function show(aPanelId) {
    Elements.panelUI.hidden = false;
    Elements.contentShowing.setAttribute("disabled", "true");

    this.switchPane(aPanelId);
  },

  hide: function hide() {
    if (!this.isVisible)
      return;

    Elements.panelUI.hidden = true;
    Elements.contentShowing.removeAttribute("disabled");
    BrowserUI.blurFocusedElement();

    this._fire("ToolPanelHidden", this._panels);
  },

  toggle: function toggle() {
    if (this.isVisible) {
      this.hide();
    } else {
      this.show();
    }
  },

  _fire: function _fire(aName, anElement) {
    let event = document.createEvent("Events");
    event.initEvent(aName, true, true);
    anElement.dispatchEvent(event);
  }
};

var DialogUI = {
  _popup: null,

  init: function() {
    window.addEventListener("mousedown", this, true);
  },

  



  get modals() {
    return document.getElementsByClassName("modal-block");
  },

  importModal: function importModal(aParent, aSrc, aArguments) {
  
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
    xhr.open("GET", aSrc, false);
    xhr.overrideMimeType("text/xml");
    xhr.send(null);
    if (!xhr.responseXML)
      return null;

    let currentNode;
    let nodeIterator = xhr.responseXML.createNodeIterator(xhr.responseXML, NodeFilter.SHOW_TEXT, null, false);
    while (!!(currentNode = nodeIterator.nextNode())) {
      let trimmed = currentNode.nodeValue.replace(/^\s\s*/, "").replace(/\s\s*$/, "");
      if (!trimmed.length)
        currentNode.parentNode.removeChild(currentNode);
    }

    let doc = xhr.responseXML.documentElement;

    let dialog  = null;

    
    
    let contentMenuContainer = document.getElementById("context-container");
    let parentNode = contentMenuContainer.parentNode;

    
    let event = document.createEvent("Events");
    event.initEvent("DOMWillOpenModalDialog", true, false);
    let dispatcher = aParent || getBrowser();
    dispatcher.dispatchEvent(event);

    
    
    let back = document.getElementById("dialog-modal-block");
    if (!back) {
      back = document.createElement("box");
    } else {
      while (back.hasChildNodes()) {
        back.removeChild(back.firstChild);
      }
    }
    back.setAttribute("class", "modal-block");
    back.setAttribute("id", "dialog-modal-block");
    dialog = back.appendChild(document.importNode(doc, true));
    parentNode.insertBefore(back, contentMenuContainer);

    dialog.arguments = aArguments;
    dialog.parent = aParent;
    return dialog;
  },

  



  pushPopup: function pushPopup(aPanel, aElements, aParent) {
    this._hidePopup();
    this._popup =  { "panel": aPanel,
                     "elements": (aElements instanceof Array) ? aElements : [aElements] };
    this._dispatchPopupChanged(true, aPanel);
  },

  popPopup: function popPopup(aPanel) {
    if (!this._popup || aPanel != this._popup.panel)
      return;
    this._popup = null;
    this._dispatchPopupChanged(false, aPanel);
  },

  _hidePopup: function _hidePopup() {
    if (!this._popup)
      return;
    let panel = this._popup.panel;
    if (panel.hide)
      panel.hide();
  },

  



  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "mousedown":
        if (!this._isEventInsidePopup(aEvent))
          this._hidePopup();
        break;
      default:
        break;
    }
  },

  _dispatchPopupChanged: function _dispatchPopupChanged(aVisible, aElement) {
    let event = document.createEvent("UIEvents");
    event.initUIEvent("PopupChanged", true, true, window, aVisible);
    aElement.dispatchEvent(event);
  },

  _isEventInsidePopup: function _isEventInsidePopup(aEvent) {
    if (!this._popup)
      return false;
    let elements = this._popup.elements;
    let targetNode = aEvent.target;
    while (targetNode && elements.indexOf(targetNode) == -1) {
      if (targetNode instanceof Element && targetNode.hasAttribute("for"))
        targetNode = document.getElementById(targetNode.getAttribute("for"));
      else
        targetNode = targetNode.parentNode;
    }
    return targetNode ? true : false;
  }
};




var SettingsCharm = {
  _entries: new Map(),
  _nextId: 0,

  




  addEntry: function addEntry(aEntry) {
    try {
      let id = MetroUtils.addSettingsPanelEntry(aEntry.label);
      this._entries.set(id, aEntry);
    } catch (e) {
      
      Cu.reportError(e);
    }
  },

  init: function SettingsCharm_init() {
    Services.obs.addObserver(this, "metro-settings-entry-selected", false);

    
    this.addEntry({
        label: Strings.browser.GetStringFromName("optionsCharm"),
        onselected: function() FlyoutPanelsUI.show('PrefsFlyoutPanel')
    });
    
    this.addEntry({
        label: Strings.browser.GetStringFromName("syncCharm"),
        onselected: function() FlyoutPanelsUI.show('SyncFlyoutPanel')
    });
    
    this.addEntry({
        label: Strings.browser.GetStringFromName("aboutCharm1"),
        onselected: function() FlyoutPanelsUI.show('AboutFlyoutPanel')
    });
    
    this.addEntry({
        label: Strings.browser.GetStringFromName("helpOnlineCharm"),
        onselected: function() {
          let url = Services.urlFormatter.formatURLPref("app.support.baseURL");
          BrowserUI.newTab(url, Browser.selectedTab);
        }
    });
  },

  observe: function SettingsCharm_observe(aSubject, aTopic, aData) {
    if (aTopic == "metro-settings-entry-selected") {
      let entry = this._entries.get(parseInt(aData, 10));
      if (entry)
        entry.onselected();
    }
  },

  uninit: function SettingsCharm_uninit() {
    Services.obs.removeObserver(this, "metro-settings-entry-selected");
  }
};
