




Cu.import("resource://gre/modules/PageThumbs.jsm");







const TOOLBARSTATE_LOADING  = 1;
const TOOLBARSTATE_LOADED   = 2;


const kHideContextAndTrayDelayMsec = 3000;


const kNewTabAnimationDelayMsec = 500;



const kStartOverlayURI = "about:start";





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
  ["contextappbar",      "contextappbar"],
  ["contentViewport",    "content-viewport"],
  ["progress",           "progress-control"],
  ["progressContainer",  "progress-container"],
  ["contentNavigator",   "content-navigator"],
  ["aboutFlyout",        "about-flyoutpanel"],
  ["prefsFlyout",        "prefs-flyoutpanel"],
  ["syncFlyout",         "sync-flyoutpanel"]
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
  init: function() {
    
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
    Services.prefs.addObserver("browser.urlbar.formatting.enabled", this, false);
    Services.prefs.addObserver("browser.urlbar.trimURLs", this, false);
    Services.obs.addObserver(this, "metro_viewstate_changed", false);
    
    this._edit.inputField.controllers.insertControllerAt(0, this._copyCutURIController);

    
    ContextUI.init();
    StartUI.init();
    PanelUI.init();
    FlyoutPanelsUI.init();
    PageThumbs.init();
    SettingsCharm.init();

    
    BrowserUI._adjustDOMforViewState();

    
    
    messageManager.addMessageListener("pageshow", function() {
      if (getBrowser().currentURI.spec == "about:blank")
        return;

      messageManager.removeMessageListener("pageshow", arguments.callee, true);

      setTimeout(function() {
        let event = document.createEvent("Events");
        event.initEvent("UIReadyDelayed", true, false);
        window.dispatchEvent(event);
      }, 0);
    });

    
    messageManager.addMessageListener("IndexedDB:Prompt", function(aMessage) {
      return IndexedDB.receiveMessage(aMessage);
    });

    
    window.addEventListener("UIReadyDelayed", function(aEvent) {
      Util.dumpLn("* delay load started...");
      window.removeEventListener(aEvent.type, arguments.callee, false);

      
      Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
      Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2);

      messageManager.addMessageListener("Browser:MozApplicationManifest", OfflineApps);

      try {
        Downloads.init();
        DialogUI.init();
        FormHelperUI.init();
        FindHelperUI.init();
        PdfJs.init();
#ifdef MOZ_SERVICES_SYNC
        Sync.init();
#endif
      } catch(ex) {
        Util.dumpLn("Exception in delay load module:", ex.message);
      }

      BrowserUI._pullDesktopControlledPrefs();

      
      if (BrowserUI.startupCrashCheck()) {
        Browser.selectedTab = BrowserUI.newOrSelectTab("about:crash");
      }
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
  },


  



  get isContentShowing() {
    return Elements.contentShowing.getAttribute("disabled") != true;
  },

  showContent: function showContent(aURI) {
    DialogUI.closeAllDialogs();
    StartUI.update(aURI);
    ContextUI.dismissTabs();
    ContextUI.dismissAppbar();
    FlyoutPanelsUI.hide();
    PanelUI.hide();
  },

  



  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },

  startupCrashCheck: function startupCrashCheck() {
#ifdef MOZ_CRASHREPORTER
    if (!Services.prefs.getBoolPref("app.reportCrashes"))
      return false;
    if (CrashReporter.enabled) {
      var lastCrashID = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo).lastRunCrashID;
      if (lastCrashID.length) {
        this.CrashSubmit.submit(lastCrashID);
        return true;
      }
    }
#endif
    return false;
  },


  



  update: function(aState) {
    this._updateToolbar();
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

  
  updateURI: function(aOptions) {
    aOptions = aOptions || {};

    let uri = this.getDisplayURI(Browser.selectedBrowser);
    let cleanURI = Util.isURLEmpty(uri) ? "" : uri;
    this._setURI(cleanURI);

    if ("captionOnly" in aOptions && aOptions.captionOnly)
      return;

    StartUI.update(uri);
    this._updateButtons();
    this._updateToolbar();
  },

  goToURI: function(aURI) {
    aURI = aURI || this._edit.value;
    if (!aURI)
      return;

    
    Util.forceOnline();

    BrowserUI.showContent(aURI);
    content.focus();
    this._setURI(aURI);

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

  handleUrlbarEnter: function handleUrlbarEnter(aEvent) {
    let url = this._edit.value;
    if (aEvent instanceof KeyEvent)
      url = this._canonizeURL(url, aEvent);
    this.goToURI(url);
  },

  _canonizeURL: function _canonizeURL(aUrl, aTriggeringEvent) {
    if (!aUrl)
      return "";

    
    
    if (/^\s*[^.:\/\s]+(?:\/.*|\s*)$/i.test(aUrl)) {
      let accel = aTriggeringEvent.ctrlKey;
      let shift = aTriggeringEvent.shiftKey;
      let suffix = "";

      switch (true) {
        case (accel && shift):
          suffix = ".org/";
          break;
        case (shift):
          suffix = ".net/";
          break;
        case (accel):
          try {
            suffix = gPrefService.getCharPref("browser.fixup.alternate.suffix");
            if (suffix.charAt(suffix.length - 1) != "/")
              suffix += "/";
          } catch(e) {
            suffix = ".com/";
          }
          break;
      }

      if (suffix) {
        
        aUrl = aUrl.trim();

        
        
        let firstSlash = aUrl.indexOf("/");
        if (firstSlash >= 0) {
          aUrl = aUrl.substring(0, firstSlash) + suffix + aUrl.substring(firstSlash + 1);
        } else {
          aUrl = aUrl + suffix;
        }
        aUrl = "http://www." + aUrl;
      }
    }
    return aUrl;
  },

  doOpenSearch: function doOpenSearch(aName) {
    
    let searchValue = this._edit.value;

    
    Util.forceOnline();
    BrowserUI.showContent();

    let engine = Services.search.getEngineByName(aName);
    let submission = engine.getSubmission(searchValue, null);
    Browser.loadURI(submission.uri.spec, { postData: submission.postData });

    
    Browser.selectedBrowser.userTypedValue = submission.uri.spec;
    this._titleChanged(Browser.selectedBrowser);
  },

  



  newTab: function newTab(aURI, aOwner) {
    aURI = aURI || kStartOverlayURI;
    let tab = Browser.addTab(aURI, true, aOwner);
    ContextUI.peekTabs();
    return tab;
  },

  newOrSelectTab: function newOrSelectTab(aURI, aOwner) {
    let tabs = Browser.tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser.currentURI.spec == aURI) {
        Browser.selectedTab = tabs[i];
        return;
      }
    }
    this.newTab(aURI, aOwner);
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

    let wasCollapsed = !ContextUI.isExpanded;
    if (wasCollapsed) {
      ContextUI.displayTabs();
    }

    this.setOnTabAnimationEnd(function() {
	    Browser.closeTab(tabToClose, { forceClose: true } );
        if (wasCollapsed)
          ContextUI.dismissWithDelay(kNewTabAnimationDelayMsec);
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
    ContextUI.dismiss();
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

  
  
  navEditKeyPress: function navEditKeyPress() {
    ContextUI.cancelDismiss();
  },

  observe: function BrowserUI_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "nsPref:changed":
        switch (aData) {
          case "browser.cache.disk_cache_ssl":
            this._sslDiskCacheEnabled = Services.prefs.getBoolPref(aData);
            break;
          case "browser.urlbar.formatting.enabled":
            this._formattingEnabled = Services.prefs.getBoolPref(aData);
            break;
          case "browser.urlbar.trimURLs":
            this._mayTrimURLs = Services.prefs.getBoolPref(aData);
            break;
        }
        break;
      case "metro_viewstate_changed":
        this._adjustDOMforViewState();
        let autocomplete = document.getElementById("start-autocomplete");
        if (aData == "snapped") {
          FlyoutPanelsUI.hide();
          
          
          Services.obs.notifyObservers(null, "metro_viewstate_dom_snapped", null);
          autocomplete.setAttribute("orient", "vertical");
        }
        else {
          autocomplete.setAttribute("orient", "horizontal");
        }
        break;
    }
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

  _trimURL: function _trimURL(aURL) {
    
    
    return aURL 
               .replace(/^((?:http|https|ftp):\/\/[^/]+)\/$/, "$1")
                
               .replace(/^http:\/\/((?!ftp\d*\.)[^\/@]+(?:\/|$))/, "$1");
  },

  trimURL: function trimURL(aURL) {
    return this.mayTrimURLs ? this._trimURL(aURL) : aURL;
  },

  _setURI: function _setURI(aURL) {
    this._edit.value = aURL;
    this.lastKnownGoodURL = aURL;
  },

  _getSelectedURIForClipboard: function _getSelectedURIForClipboard() {
    
    let inputVal = this._edit.inputField.value;
    let selectedVal = inputVal.substring(this._edit.selectionStart, this._edit.electionEnd);

    
    
    if (this._edit.selectionStart > 0 || this._edit.valueIsTyped)
      return selectedVal;
    
    
    if (!selectedVal.contains("/")) {
      let remainder = inputVal.replace(selectedVal, "");
      if (remainder != "" && remainder[0] != "/")
        return selectedVal;
    }

    let uriFixup = Cc["@mozilla.org/docshell/urifixup;1"].getService(Ci.nsIURIFixup);

    let uri;
    try {
      uri = uriFixup.createFixupURI(inputVal, Ci.nsIURIFixup.FIXUP_FLAG_USE_UTF8);
    } catch (e) {}
    if (!uri)
      return selectedVal;

    
    try {
      uri = uriFixup.createExposableURI(uri);
    } catch (ex) {}

    
    if (inputVal == selectedVal) {
      
      
      if (!uri.schemeIs("javascript") && !uri.schemeIs("data")) {
        
        selectedVal = uri.spec.replace(/[()]/g, function (c) escape(c));
      }

      return selectedVal;
    }

    
    let spec = uri.spec;
    let trimmedSpec = this.trimURL(spec);
    if (spec != trimmedSpec) {
      
      
      
      let trimmedSegments = spec.split(trimmedSpec);
      selectedVal = trimmedSegments[0] + selectedVal;
    }

    return selectedVal;
  },

  _copyCutURIController: {
    doCommand: function(aCommand) {
      let urlbar = BrowserUI._edit;
      let val = BrowserUI._getSelectedURIForClipboard();
      if (!val)
        return;

      if (aCommand == "cmd_cut" && this.isCommandEnabled(aCommand)) {
        let start = urlbar.selectionStart;
        let end = urlbar.selectionEnd;
        urlbar.inputField.value = urlbar.inputField.value.substring(0, start) +
                                  urlbar.inputField.value.substring(end);
        urlbar.selectionStart = urlbar.selectionEnd = start;
      }

      Cc["@mozilla.org/widget/clipboardhelper;1"]
        .getService(Ci.nsIClipboardHelper)
        .copyString(val, document);
    },

    supportsCommand: function(aCommand) {
      switch (aCommand) {
        case "cmd_copy":
        case "cmd_cut":
          return true;
      }
      return false;
    },

    isCommandEnabled: function(aCommand) {
      let urlbar = BrowserUI._edit;
      return this.supportsCommand(aCommand) &&
             (aCommand != "cmd_cut" || !urlbar.readOnly) &&
             urlbar.selectionStart < urlbar.selectionEnd;
    },

    onEvent: function(aEventName) {}
  },

  _urlbarClicked: function _urlbarClicked(aEvent) {
    let touchEvent = aEvent.mozInputSource == Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH;

    
    if (Elements.urlbarState.getAttribute("mode") != "edit") {
      this._editURI(true);
      if (touchEvent) {
        SelectionHelperUI.attachEditSession(ChromeSelectionHandler,
                                            aEvent.clientX, aEvent.clientY);
      }
      return;
    }

    
    if (touchEvent) {
      SelectionHelperUI.attachToCaret(ChromeSelectionHandler,
                                      aEvent.clientX, aEvent.clientY);
    }
  },

  _editURI: function _editURI(aShouldDismiss) {
    this._edit.focus();
    this._edit.select();

    Elements.urlbarState.setAttribute("mode", "edit");
    StartUI.show();
    if (aShouldDismiss) {
      ContextUI.dismissTabs();
    }
  },

  formatURI: function formatURI() {
    if (!this.formattingEnabled ||
        Elements.urlbarState.getAttribute("mode") == "edit")
      return;

    let controller = this._edit.editor.selectionController;
    let selection = controller.getSelection(controller.SELECTION_URLSECONDARY);
    selection.removeAllRanges();

    let textNode = this._edit.editor.rootElement.firstChild;
    let value = textNode.textContent;

    let protocol = value.match(/^[a-z\d.+\-]+:(?=[^\d])/);
    if (protocol &&
        ["http:", "https:", "ftp:"].indexOf(protocol[0]) == -1)
      return;
    let matchedURL = value.match(/^((?:[a-z]+:\/\/)?(?:[^\/]+@)?)(.+?)(?::\d+)?(?:\/|$)/);
    if (!matchedURL)
      return;

    let [, preDomain, domain] = matchedURL;
    let baseDomain = domain;
    let subDomain = "";
    
    if (domain[0] != "[") {
      try {
        baseDomain = Services.eTLD.getBaseDomainFromHost(domain);
        if (!domain.endsWith(baseDomain)) {
          
          let IDNService = Cc["@mozilla.org/network/idn-service;1"]
                           .getService(Ci.nsIIDNService);
          baseDomain = IDNService.convertACEtoUTF8(baseDomain);
        }
      } catch (e) {}
    }
    if (baseDomain != domain) {
      subDomain = domain.slice(0, -baseDomain.length);
    }

    let rangeLength = preDomain.length + subDomain.length;
    if (rangeLength) {
      let range = document.createRange();
      range.setStart(textNode, 0);
      range.setEnd(textNode, rangeLength);
      selection.addRange(range);
    }

    let startRest = preDomain.length + domain.length;
    if (startRest < value.length) {
      let range = document.createRange();
      range.setStart(textNode, startRest);
      range.setEnd(textNode, value.length);
      selection.addRange(range);
    }
  },

  _clearURIFormatting: function _clearURIFormatting() {
    if (!this.formattingEnabled)
      return;

    let controller = this._edit.editor.selectionController;
    let selection = controller.getSelection(controller.SELECTION_URLSECONDARY);
    selection.removeAllRanges();
  },

  _urlbarBlurred: function _urlbarBlurred() {
    let state = Elements.urlbarState;
    if (state.getAttribute("mode") == "edit")
      state.removeAttribute("mode");
    this._updateToolbar();
    this.formatURI();
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
      this._edit.value = this.lastKnownGoodURL;
      this._edit.closePopup();
      StartUI.hide();
      ContextUI.dismiss();
      return;
    }

    
    if (DialogUI._popup) {
      DialogUI._hidePopup();
      return;
    }

    
    let dialog = DialogUI.activeDialog;
    if (dialog) {
      dialog.close();
      return;
    }

    
    if (DialogUI.modals.length > 0) {
      return;
    }

    
    if (PanelUI.isVisible) {
      PanelUI.hide();
      return;
    }

    
    let contentHelper = Elements.contentNavigator;
    if (contentHelper.isActive) {
      contentHelper.model.hide();
      return;
    }

    if (StartUI.hide()) {
      
      ContextUI.dismiss();
      return;
    }

    if (ContextUI.dismiss()) {
      return;
    }

    if (Browser.selectedTab.isLoading()) {
      Browser.selectedBrowser.stop();
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

  _formattingEnabled: null,

  get formattingEnabled() {
    if (this._formattingEnabled === null) {
      this._formattingEnabled = Services.prefs.getBoolPref("browser.urlbar.formatting.enabled");
    }
    return this._formattingEnabled;
  },

  _mayTrimURLs: null,

  get mayTrimURLs() {
    if (this._mayTrimURLs === null) {
      this._mayTrimURLs = Services.prefs.getBoolPref("browser.urlbar.trimURLs");
    }
    return this._mayTrimURLs;
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
        this._editURI(true);
        break;
      case "cmd_addBookmark":
        Elements.navbar.show();
        Appbar.onStarButton(true);
        break;
      case "cmd_bookmarks":
        PanelUI.show("bookmarks-container");
        break;
      case "cmd_history":
        PanelUI.show("history-container");
        break;
      case "cmd_remoteTabs":
        if (Weave.Status.checkSetup() == Weave.CLIENT_NOT_CONFIGURED) {
          Sync.open();
        } else {
          PanelUI.show("remotetabs-container");
        }
        break;
      case "cmd_quit":
        
        this._closeOrQuit();
        break;
      case "cmd_close":
        this._closeOrQuit();
        break;
      case "cmd_newTab":
        this.newTab();
        this._editURI(false);
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
        FlyoutPanelsUI.hide();
        MetroUtils.showSettingsFlyout();
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





var ContextUI = {
  _expandable: true,
  _hidingId: 0,

  



  init: function init() {
    Elements.browsers.addEventListener("mousedown", this, true);
    Elements.browsers.addEventListener("touchstart", this, true);
    Elements.browsers.addEventListener("AlertActive", this, true);
    window.addEventListener("MozEdgeUIStarted", this, true);
    window.addEventListener("MozEdgeUICanceled", this, true);
    window.addEventListener("MozEdgeUICompleted", this, true);
    window.addEventListener("keypress", this, true);
    window.addEventListener("KeyboardChanged", this, false);

    Elements.tray.addEventListener("transitionend", this, true);

    Appbar.init();
  },

  



  get isVisible() {
    return (Elements.navbar.hasAttribute("visible") ||
            Elements.navbar.hasAttribute("startpage"));
  },
  get isExpanded() { return Elements.tray.hasAttribute("expanded"); },
  get isExpandable() { return this._expandable; },

  set isExpandable(aFlag) {
    this._expandable = aFlag;
    if (!this._expandable)
      this.dismiss();
  },

  



  toggle: function toggle() {
    if (!this._expandable) {
      
      
      return;
    }
    
    if (!this.dismiss()) {
      dump("* ContextUI is hidden, show it\n");
      this.show();
    }
  },

  
  
  show: function() {
    let shown = false;
    if (!this.isExpanded) {
      
      this._setIsExpanded(true);
      shown = true;
    }
    if (!Elements.navbar.isShowing) {
      
      Elements.navbar.show();
      shown = true;
    }

    this._clearDelayedTimeout();
    if (shown) {
      ContentAreaObserver.update(window.innerWidth, window.innerHeight);
    }
    return shown;
  },

  
  displayNavbar: function displayNavbar() {
    this._clearDelayedTimeout();
    Elements.navbar.show();
  },

  
  displayTabs: function displayTabs() {
    this._clearDelayedTimeout();
    this._setIsExpanded(true, true);
  },

  
  peekTabs: function peekTabs() {
    if (this.isExpanded) {
      setTimeout(function () {
        ContextUI.dismissWithDelay(kNewTabAnimationDelayMsec);
      }, 0);
    } else {
      BrowserUI.setOnTabAnimationEnd(function () {
        ContextUI.dismissWithDelay(kNewTabAnimationDelayMsec);
      });

      this.displayTabs();
    }
  },

  
  
  dismiss: function dismiss() {
    let dismissed = false;
    if (this.isExpanded) {
      this._setIsExpanded(false);
      dismissed = true;
    }
    if (Elements.navbar.isShowing) {
      this.dismissAppbar();
      dismissed = true;
    }
    this._clearDelayedTimeout();
    if (dismissed) {
      ContentAreaObserver.update(window.innerWidth, window.innerHeight);
    }
    return dismissed;
  },

  
  dismissWithDelay: function dismissWithDelay(aDelay) {
    aDelay = aDelay || kHideContextAndTrayDelayMsec;
    this._clearDelayedTimeout();
    this._hidingId = setTimeout(function () {
        ContextUI.dismiss();
      }, aDelay);
  },

  
  cancelDismiss: function cancelDismiss() {
    this._clearDelayedTimeout();
  },

  dismissTabs: function dimissTabs() {
    this._clearDelayedTimeout();
    this._setIsExpanded(false, true);
  },

  dismissAppbar: function dismissAppbar() {
    this._fire("MozAppbarDismiss");
  },

  



  
  _setIsExpanded: function _setIsExpanded(aFlag, setSilently) {
    
    if (!this.isExpandable || this.isExpanded == aFlag)
      return;

    if (aFlag)
      Elements.tray.setAttribute("expanded", "true");
    else
      Elements.tray.removeAttribute("expanded");

    if (!setSilently)
      this._fire("MozContextUIExpand");
  },

  



  _clearDelayedTimeout: function _clearDelayedTimeout() {
    if (this._hidingId) {
      clearTimeout(this._hidingId);
      this._hidingId = 0;
    }
  },

  



  _onEdgeUIStarted: function(aEvent) {
    this._hasEdgeSwipeStarted = true;
    this._clearDelayedTimeout();

    if (StartUI.hide()) {
      this.dismiss();
      return;
    }
    this.toggle();
  },

  _onEdgeUICanceled: function(aEvent) {
    this._hasEdgeSwipeStarted = false;
    StartUI.hide();
    this.dismiss();
  },

  _onEdgeUICompleted: function(aEvent) {
    if (this._hasEdgeSwipeStarted) {
      this._hasEdgeSwipeStarted = false;
      return;
    }

    this._clearDelayedTimeout();
    if (StartUI.hide()) {
      this.dismiss();
      return;
    }
    this.toggle();
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "MozEdgeUIStarted":
        this._onEdgeUIStarted(aEvent);
        break;
      case "MozEdgeUICanceled":
        this._onEdgeUICanceled(aEvent);
        break;
      case "MozEdgeUICompleted":
        this._onEdgeUICompleted(aEvent);
        break;
      case "mousedown":
        if (aEvent.button == 0 && this.isVisible)
          this.dismiss();
        break;
      case "touchstart":
      
      case "AlertActive":
        this.dismiss();
        break;
      case "keypress":
        if (String.fromCharCode(aEvent.which) == "z" &&
            aEvent.getModifierState("Win"))
          this.toggle();
        break;
      case "transitionend":
        setTimeout(function () {
          ContentAreaObserver.updateContentArea();
        }, 0);
        break;
      case "KeyboardChanged":
        this.dismissTabs();
        break;
    }
  },

  _fire: function (name) {
    let event = document.createEvent("Events");
    event.initEvent(name, true, true);
    window.dispatchEvent(event);
  }
};

var StartUI = {
  get isVisible() { return this.isStartPageVisible || this.isFiltering; },
  get isStartPageVisible() { return Elements.windowState.hasAttribute("startpage"); },
  get isFiltering() { return Elements.windowState.hasAttribute("filtering"); },

  get maxResultsPerSection() {
    return Services.prefs.getIntPref("browser.display.startUI.maxresults");
  },

  sections: [
    "TopSitesStartView",
    "TopSitesSnappedView",
    "BookmarksStartView",
    "HistoryStartView",
    "RemoteTabsStartView"
  ],

  init: function init() {
    Elements.startUI.addEventListener("autocompletestart", this, false);
    Elements.startUI.addEventListener("autocompleteend", this, false);
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

  
  filter: function filter() {
    if (this.isFiltering)
      return;

    BrowserUI._edit.openPopup();
    Elements.windowState.setAttribute("filtering", "true");
  },

  
  unfilter: function unfilter() {
    if (!this.isFiltering)
      return;

    BrowserUI._edit.closePopup();
    Elements.windowState.removeAttribute("filtering");
  },

  
  hide: function hide(aURI) {
    aURI = aURI || Browser.selectedBrowser.currentURI.spec;
    if (!this.isStartPageVisible || this.isStartURI(aURI))
      return false;

    Elements.contentShowing.removeAttribute("disabled");
    Elements.windowState.removeAttribute("startpage");

    this.unfilter();
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
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "autocompletestart":
        this.filter();
        break;
      case "autocompleteend":
        this.unfilter();
        break;
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

        scrollInterface.scrollBy(aEvent.detail, 0);

        aEvent.preventDefault();
        aEvent.stopPropagation();
        break;
    }
  }
};

var SyncPanelUI = {
  init: function() {
    
    Elements.syncFlyout.addEventListener("PopupChanged", function onShow(aEvent) {
      if (aEvent.detail && aEvent.target === Elements.syncFlyout) {
        Elements.syncFlyout.removeEventListener("PopupChanged", onShow, false);
        Sync.init();
      }
    }, false);
  }
};

var FlyoutPanelsUI = {
  init: function() {
    AboutPanelUI.init();
    PreferencesPanelView.init();
    SyncPanelUI.init();

    
    window.addEventListener("deactivate", function(window) {
      FlyoutPanelsUI.hide();
    });
  },

  hide: function() {
    Elements.aboutFlyout.hide();
    Elements.prefsFlyout.hide();
    Elements.syncFlyout.hide();
  }
};

var PanelUI = {
  get _panels() { return document.getElementById("panel-items"); },
  get _switcher() { return document.getElementById("panel-view-switcher"); },

  get isVisible() {
    return !Elements.panelUI.hidden;
  },

  views: {
    "bookmarks-container": "BookmarksPanelView",
    "downloads-container": "DownloadsPanelView",
    "console-container": "ConsolePanelView",
    "remotetabs-container": "RemoteTabsPanelView",
    "history-container" : "HistoryPanelView"
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
      this._switcher.value = panel.id;

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
  _dialogs: [],
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

    
    let back = document.createElement("box");
    back.setAttribute("class", "modal-block");
    dialog = back.appendChild(document.importNode(doc, true));
    parentNode.insertBefore(back, contentMenuContainer);

    dialog.arguments = aArguments;
    dialog.parent = aParent;
    return dialog;
  },

  



  get activeDialog() {
    
    if (this._dialogs.length)
      return this._dialogs[this._dialogs.length - 1];
    return null;
  },

  closeAllDialogs: function closeAllDialogs() {
    while (this.activeDialog)
      this.activeDialog.close();
  },

  pushDialog: function pushDialog(aDialog) {
    
    if (aDialog) {
      this._dialogs.push(aDialog);
      Elements.contentShowing.setAttribute("disabled", "true");
    }
  },

  popDialog: function popDialog() {
    if (this._dialogs.length)
      this._dialogs.pop();

    
    if (!this._dialogs.length)
      Elements.contentShowing.removeAttribute("disabled");
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
        onselected: function() Elements.prefsFlyout.show()
    });
    
    this.addEntry({
        label: Strings.browser.GetStringFromName("syncCharm"),
        onselected: function() Elements.syncFlyout.show()
    });
    
    this.addEntry({
        label: Strings.browser.GetStringFromName("aboutCharm1"),
        onselected: function() Elements.aboutFlyout.show()
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
