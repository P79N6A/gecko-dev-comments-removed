





































Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "PluralForm", function() {
  Cu.import("resource://gre/modules/PluralForm.jsm");
  return PluralForm;
});

XPCOMUtils.defineLazyGetter(this, "PlacesUtils", function() {
  Cu.import("resource://gre/modules/PlacesUtils.jsm");
  return PlacesUtils;
});

const TOOLBARSTATE_LOADING  = 1;
const TOOLBARSTATE_LOADED   = 2;

[
  [
    "gHistSvc",
    "@mozilla.org/browser/nav-history-service;1",
    [Ci.nsINavHistoryService, Ci.nsIBrowserHistory]
  ],
  [
    "gFaviconService",
     "@mozilla.org/browser/favicon-service;1",
     [Ci.nsIFaviconService]
  ],
  [
    "gIOService",
    "@mozilla.org/network/io-service;1",
    [Ci.nsIIOService],
  ],
  [
    "gURIFixup",
    "@mozilla.org/docshell/urifixup;1",
    [Ci.nsIURIFixup]
  ],
  [
    "gPrefService",
    "@mozilla.org/preferences-service;1",
    [Ci.nsIPrefBranch2]
  ],
  [
    "gFocusManager",
    "@mozilla.org/focus-manager;1",
    [Ci.nsIFocusManager]
  ],
  [
    "gWindowMediator",
    "@mozilla.org/appshell/window-mediator;1",
    [Ci.nsIWindowMediator]
  ],
  [
    "gObserverService",
    "@mozilla.org/observer-service;1",
    [Ci.nsIObserverService]
  ]
].forEach(function (service) {
  let [name, contract, ifaces] = service;
  window.__defineGetter__(name, function () {
    delete window[name];
    window[name] = Cc[contract].getService(ifaces.splice(0, 1)[0]);
    if (ifaces.length)
      ifaces.forEach(function (i) { return window[name].QueryInterface(i); });
    return window[name];
  });
});

var BrowserUI = {
  _edit : null,
  _throbber : null,
  _favicon : null,
  _dialogs: [],

  _domWillOpenModalDialog: function(aBrowser) {
    
    

    for (let i = 0; i < Browser.tabs.length; i++) {
      if (Browser._tabs[i].browser == aBrowser) {
        Browser.selectedTab = Browser.tabs[i];
        break;
      }
    }

    return { };
  },

  _titleChanged : function(aBrowser) {
    var browser = Browser.selectedBrowser;
    if (browser && aBrowser != browser)
      return;

    var url = this.getDisplayURI(browser);
    var caption = browser.contentTitle || url;

    if (Util.isURLEmpty(url))
      caption = "";

    this._setURI(caption);
  },

  



  _domWindowClose: function(aBrowser) {
     
     let browsers = Browser.browsers;
     for (let i = 0; i < browsers.length; i++) {
      if (browsers[i] == aBrowser) {
        Browser.closeTab(Browser.getTabAtIndex(i));
        return { preventDefault: true };
      }
    }

    return { };
  },

  _metaAdded : function(aEvent) {
    let meta = aEvent.originalTarget;
    if (!meta)
      return;

    if (meta.name == "viewport" || meta.name == "HandheldFriendly") {
      
      var ownerDoc = meta.ownerDocument;
      if (!ownerDoc || ownerDoc.defaultView.frameElement)
        return;

      let tab = Browser.getTabForDocument(ownerDoc);
      tab.updateViewportMetadata();
    }
  },

  _updateButtons : function(aBrowser) {
    let back = document.getElementById("cmd_back");
    let forward = document.getElementById("cmd_forward");

    back.setAttribute("disabled", !aBrowser.canGoBack);
    forward.setAttribute("disabled", !aBrowser.canGoForward);
  },

  _updateToolbar: function _updateToolbar() {
    let icons = document.getElementById("urlbar-icons");
    let mode = icons.getAttribute("mode");
    if (Browser.selectedTab.isLoading() && mode != "loading") {
      icons.setAttribute("mode", "loading");
    }
    else if (mode != "view") {
      icons.setAttribute("mode", "view");
    }
  },

  _tabSelect : function(aEvent) {
    let browser = Browser.selectedBrowser;
    this._titleChanged(browser);
    this._updateToolbar();
    this._updateButtons(browser);
    this._updateIcon(browser.mIconURL);
    this.updateStar();
  },

  showToolbar : function showToolbar(aEdit) {
    this.hidePanel();
    this._editURI(aEdit);
    if (aEdit)
      this.showAutoComplete();
  },

  _toolbarLocked: 0,

  isToolbarLocked: function isToolbarLocked() {
    return this._toolbarLocked;
  },

  lockToolbar: function lockToolbar() {
    this._toolbarLocked++;
    document.getElementById("toolbar-moveable-container").top = "0";
    if (this._toolbarLocked == 1)
      Browser.forceChromeReflow();
  },

  unlockToolbar: function unlockToolbar() {
    if (!this._toolbarLocked)
      return;

    this._toolbarLocked--;
    if (!this._toolbarLocked)
      document.getElementById("toolbar-moveable-container").top = "";
  },

  _setURI: function _setURI(aCaption) {
    if (this.isAutoCompleteOpen())
      this._edit.defaultValue = aCaption;
    else
      this._edit.value = aCaption;
  },

  _editURI : function _editURI(aEdit) {
    var icons = document.getElementById("urlbar-icons");
    if (aEdit && icons.getAttribute("mode") != "edit") {
      icons.setAttribute("mode", "edit");
      this._edit.defaultValue = this._edit.value;

      let urlString = this.getDisplayURI(Browser.selectedBrowser);
      if (Util.isURLEmpty(urlString))
        urlString = "";
      this._edit.value = urlString;

      
      
      
      this._edit.blur();
      gFocusManager.setFocus(this._edit, Ci.nsIFocusManager.FLAG_NOSCROLL);
    }
    else if (!aEdit) {
      this._updateToolbar();
    }
  },

  _closeOrQuit: function _closeOrQuit() {
    
    let dialog = this.activeDialog;
    if (dialog) {
      dialog.close();
    } else {
      
      if (Browser.closing())
        window.close();
    }
  },

  get activeDialog() {
    
    if (this._dialogs.length)
      return this._dialogs[this._dialogs.length - 1];
    return null;
  },

  pushDialog : function pushDialog(aDialog) {
    
    if (aDialog) {
      this.lockToolbar();
      this._dialogs.push(aDialog);
      document.getElementById("toolbar-main").setAttribute("dialog", "true");
      Elements.contentShowing.setAttribute("disabled", "true");
    }
  },

  popDialog : function popDialog() {
    if (this._dialogs.length) {
      this._dialogs.pop();
      this.unlockToolbar();
    }

    
    if (!this._dialogs.length) {
      document.getElementById("toolbar-main").removeAttribute("dialog");
      Elements.contentShowing.removeAttribute("disabled");
    }
  },

  pushPopup: function pushPopup(aPanel, aElements) {
    this._hidePopup();
    this._popup =  { "panel": aPanel,
                     "elements": (aElements instanceof Array) ? aElements : [aElements] };
    this._dispatchPopupChanged();
  },

  popPopup: function popPopup() {
    this._popup = null;
    this._dispatchPopupChanged();
  },

  _dispatchPopupChanged: function _dispatchPopupChanged() {
    let stack = document.getElementById("stack");
    let event = document.createEvent("Events");
    event.initEvent("PopupChanged", true, false);
    event.popup = this._popup;
    stack.dispatchEvent(event);
  },

  _hidePopup: function _hidePopup() {
    if (!this._popup)
      return;
    let panel = this._popup.panel;
    if (panel.hide)
      panel.hide();
  },

  _isEventInsidePopup: function _isEventInsidePopup(aEvent) {
    if (!this._popup)
      return false;
    let elements = this._popup.elements;
    let targetNode = aEvent ? aEvent.target : null;
    while (targetNode && elements.indexOf(targetNode) == -1)
      targetNode = targetNode.parentNode;
    return targetNode ? true : false;
  },

  switchPane : function switchPane(id) {
    let button = document.getElementsByAttribute("linkedpanel", id)[0];
    if (button)
      button.checked = true;

    let pane = document.getElementById(id);
    document.getElementById("panel-items").selectedPanel = pane;
  },

  get toolbarH() {
    if (!this._toolbarH) {
      let toolbar = document.getElementById("toolbar-main");
      this._toolbarH = toolbar.boxObject.height;
    }
    return this._toolbarH;
  },

  get sidebarW() {
    if (!this._sidebarW) {
      let sidebar = document.getElementById("browser-controls");
      this._sidebarW = sidebar.boxObject.width;
    }
    return this._sidebarW;
  },

  get starButton() {
    delete this.starButton;
    return this.starButton = document.getElementById("tool-star");
  },

  sizeControls : function(windowW, windowH) {
    
    document.getElementById("tabs").resize();

    
    let popup = document.getElementById("popup_autocomplete");
    popup.top = this.toolbarH;
    popup.height = windowH - this.toolbarH;
    popup.width = windowW;

    
    let formHelper = document.getElementById("form-helper-container");
    formHelper.top = windowH - formHelper.getBoundingClientRect().height;
  },

  init : function() {
    this._edit = document.getElementById("urlbar-edit");
    this._throbber = document.getElementById("urlbar-throbber");
    this._favicon = document.getElementById("urlbar-favicon");
    this._favicon.addEventListener("error", this, false);

    let urlbarEditArea = document.getElementById("urlbar-editarea");
    urlbarEditArea.addEventListener("click", this, false);
    urlbarEditArea.addEventListener("mousedown", this, false);

    document.getElementById("toolbar-main").ignoreDrag = true;

    let tabs = document.getElementById("tabs");
    tabs.addEventListener("TabSelect", this, true);
    tabs.addEventListener("TabOpen", this, true);

    
    messageManager.addMessageListener("DOMLinkAdded", this);
    messageManager.addMessageListener("DOMTitleChanged", this);
    messageManager.addMessageListener("DOMWillOpenModalDialog", this);
    messageManager.addMessageListener("DOMWindowClose", this);

    
    messageManager.addMessageListener("Browser:SaveAs:Return", this);

    
    window.addEventListener("mousedown", this, true);

    
    window.addEventListener("keypress", this, true);

    
    window.addEventListener("AppCommand", this, true);

    
    
    messageManager.addMessageListener("DOMContentLoaded", function() {
      
      messageManager.removeMessageListener("DOMContentLoaded", arguments.callee, true);
      
      
      Elements.panelUI.hidden = false;

      
      ExtensionsView.init();
      DownloadsView.init();
      PreferencesView.init();
      ConsoleView.init();
    });
  },

  uninit : function() {
    ExtensionsView.uninit();
    ConsoleView.uninit();
  },

  update : function(aState) {
    let icons = document.getElementById("urlbar-icons");
    let browser = Browser.selectedBrowser;

    switch (aState) {
      case TOOLBARSTATE_LOADED:
        if (icons.getAttribute("mode") != "edit")
          this._updateToolbar();

        this._updateIcon(browser.mIconURL);
        this.unlockToolbar();
        break;

      case TOOLBARSTATE_LOADING:
        if (icons.getAttribute("mode") != "edit")
          this._updateToolbar();

        browser.mIconURL = "";
        this._updateIcon();
        this.lockToolbar();
        break;
    }
  },

  _updateIcon : function(aIconSrc) {
    this._favicon.src = aIconSrc || "";
    if (Browser.selectedTab.isLoading()) {
      this._throbber.hidden = false;
      this._throbber.setAttribute("loading", "true");
      this._favicon.hidden = true;
    }
    else {
      this._favicon.hidden = false;
      this._throbber.hidden = true;
      this._throbber.removeAttribute("loading");
    }
  },

  getDisplayURI : function(browser) {
    let loadGroup = browser.webNavigation.QueryInterface(Ci.nsIDocumentLoader).loadGroup;
    if (loadGroup.activeCount && loadGroup.defaultLoadRequest) {
      
      
      
      return loadGroup.defaultLoadRequest.QueryInterface(Ci.nsIChannel).originalURI.spec;
    }

    if (!this._URIFixup)
      this._URIFixup = Cc["@mozilla.org/docshell/urifixup;1"].getService(Ci.nsIURIFixup);

    let uri = browser.currentURI;
    try {
      uri = this._URIFixup.createExposableURI(uri);
    } catch (ex) {}

    return uri.spec;
  },

  
  updateURI : function() {
    var browser = Browser.selectedBrowser;

    
    if (!browser.currentURI)
      return;

    
    this._updateButtons(browser);

    
    FormHelper.close();

    
    this.updateStar();

    var urlString = this.getDisplayURI(browser);
    if (Util.isURLEmpty(urlString))
      urlString = "";

    this._setURI(urlString);
  },

  goToURI : function(aURI) {
    aURI = aURI || this._edit.value;
    if (!aURI)
      return;

    
    Util.forceOnline();

    
    Browser.hideSidebars();
    this.closeAutoComplete(true);

    this._edit.value = aURI;

    let fixupFlags = Ci.nsIURIFixup.FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP;
    let uri = gURIFixup.createFixupURI(aURI, fixupFlags);

    let loadFlags = Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
    getBrowser().loadURIWithFlags(uri.spec, loadFlags, null, null);

    gHistSvc.markPageAsTyped(uri);
  },

  showAutoComplete : function showAutoComplete() {
    if (this.isAutoCompleteOpen())
      return;

    BrowserSearch.updateSearchButtons();

    this._hidePopup();
    this._edit.showHistoryPopup();
  },

  closeAutoComplete: function closeAutoComplete(aResetInput) {
    if (!this.isAutoCompleteOpen())
      return;

    if (aResetInput)
      this._edit.popup.close();
    else
      this._edit.popup.closePopup();
  },

  isAutoCompleteOpen: function isAutoCompleteOpen() {
    return this._edit.popup.popupOpen;
  },

  doButtonSearch : function(button) {
    if (!("engine" in button) || !button.engine)
      return;

    
    button.parentNode.selectedItem = null;

    
    Browser.hideSidebars();
    this.closeAutoComplete(false);

    
    Util.forceOnline();

    let submission = button.engine.getSubmission(this._edit.value, null);
    let flags = Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    getBrowser().loadURIWithFlags(submission.uri.spec, flags, null, null, submission.postData);
  },

  updateStar : function() {
    if (PlacesUtils.getMostRecentBookmarkForURI(Browser.selectedBrowser.currentURI) != -1)
      this.starButton.setAttribute("starred", "true");
    else
      this.starButton.removeAttribute("starred");
  },

  newTab : function newTab(aURI) {
    aURI = aURI || "about:blank";
    let tab = Browser.addTab(aURI, true);

    this.hidePanel();

    if (aURI == "about:blank") {
      
      this.showToolbar(true);
    }
    else {
      
      Browser.hideSidebars();
      this.closeAutoComplete(true);
    }

    return tab;
  },

  closeTab : function closeTab(aTab) {
    
    Browser.closeTab(aTab || Browser.selectedTab);
  },

  selectTab : function selectTab(aTab) {
    Browser.selectedTab = aTab;
  },

  isTabsVisible: function isTabsVisible() {
    
    let [leftvis,_1,_2,_3] = Browser.computeSidebarVisibility();
    return (leftvis > 0.002);
  },

  showPanel: function showPanel(aPage) {
    Elements.panelUI.left = 0;
    Elements.panelUI.hidden = false;
    Elements.contentShowing.setAttribute("disabled", "true");

    if (aPage != undefined)
      this.switchPane(aPage);
  },

  hidePanel: function hidePanel() {
    Elements.panelUI.hidden = true;
    Elements.contentShowing.removeAttribute("disabled");
  },

  isPanelVisible: function isPanelVisible() {
    return (!Elements.panelUI.hidden && Elements.panelUI.left == 0);
  },

  switchTask: function switchTask() {
    try {
      let phone = Cc["@mozilla.org/phone/support;1"].createInstance(Ci.nsIPhoneSupport);
      phone.switchTask();
    } catch(e) { }
  },

#ifdef WINCE
  updateDefaultBrowser: function updateDefaultBrowser(aSet) {
    try {
      let phone = Cc["@mozilla.org/phone/support;1"].getService(Ci.nsIPhoneSupport);
      if (aSet)
        phone.setDefaultBrowser();
      else
        phone.restoreDefaultBrowser();
    } catch(e) { }
  },
#endif

  handleEscape: function () {
    
    let dialog = this.activeDialog;
    if (dialog) {
      dialog.close();
      return;
    }
    
    
    if (this._popup) {
      this._hidePopup();
      return;
    }
      
    
    let modalElementsLength = document.getElementsByClassName("modal-block").length;
    if (modalElementsLength > 0) 
      return;

    
    if (this.isPanelVisible()) {
      this.hidePanel();
      return;
    }

    
    Browser.selectedBrowser.goBack();
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      
      case "DOMMetaAdded":
        this._metaAdded(aEvent);
        break;
      case "TabSelect":
        this._tabSelect(aEvent);
        break;
      case "TabOpen":
      {
        let [tabsVisibility,,,] = Browser.computeSidebarVisibility();
        if (!(tabsVisibility == 1.0) && Browser.selectedTab.chromeTab != aEvent.target)
          NewTabPopup.show(aEvent.target);

        
        
        if (tabsVisibility > 0.0 && tabsVisibility < 1.0)
          Browser.hideSidebars();

        break;
      }
      
      case "keypress":
        if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE)
          this.handleEscape();
        break;
      case "AppCommand":
        aEvent.stopPropagation();
        switch (aEvent.command) {
          case "Menu":
            this.doCommand("cmd_menu");
            break;
          case "Search":
            this.doCommand("cmd_openLocation");
            break;
          default:
            break;
        }
        break;
      
      case "click":
        this.doCommand("cmd_openLocation");
        break;
      case "mousedown":
        if (!this._isEventInsidePopup(aEvent))
          this._hidePopup();

        let selectAll = gPrefService.getBoolPref("browser.urlbar.doubleClickSelectsAll");
        if (aEvent.detail == 2 && aEvent.button == 0 && selectAll && aEvent.target == this._edit) {
          this._edit.editor.selectAll();
          aEvent.preventDefault();
        }
        break;
      
      case "error":
        this._favicon.src = "";
        break;
    }
  },

  receiveMessage: function receiveMessage(aMessage) {
    let browser = aMessage.target;
    switch (aMessage.name) {
      case "DOMTitleChanged":
        this._titleChanged(browser);
        break;
      case "DOMWillOpenModalDialog":
        return this._domWillOpenModalDialog(browser);
        break;
      case "DOMWindowClose":
        return this._domWindowClose(browser);
        break;
      case "DOMLinkAdded":
        if (Browser.selectedBrowser == browser)
          this._updateIcon(Browser.selectedBrowser.mIconURL);
        break;
      case "Browser:SaveAs:Return":
        let json = aMessage.json;
        if (json.type != Ci.nsIPrintSettings.kOutputFormatPDF)
          return;

        let dm = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
        let db = dm.DBConnection;
        let stmt = db.createStatement("UPDATE moz_downloads SET endTime = :endTime, state = :state WHERE id = :id");
        stmt.params.endTime = Date.now() * 1000;
        stmt.params.state = Ci.nsIDownloadManager.DOWNLOAD_FINISHED;
        stmt.params.id = json.id;
        stmt.execute();
        stmt.finalize();

        let download = dm.getDownload(json.id);
        try {
          DownloadsView.downloadCompleted(download);
          let element = DownloadsView.getElementForDownload(json.id);
          element.setAttribute("state", Ci.nsIDownloadManager.DOWNLOAD_FINISHED);
          element.setAttribute("endTime", Date.now());
          element.setAttribute("referrer", json.referrer);
          DownloadsView._updateTime(element);
          DownloadsView._updateStatus(element);
        }
        catch(e) {}
        gObserverService.notifyObservers(download, "dl-done", null);
        break;
    }
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
      case "cmd_openLocation":
      case "cmd_star":
      case "cmd_bookmarks":
      case "cmd_quit":
      case "cmd_close":
      case "cmd_menu":
      case "cmd_newTab":
      case "cmd_closeTab":
      case "cmd_actions":
      case "cmd_panel":
      case "cmd_sanitize":
      case "cmd_zoomin":
      case "cmd_zoomout":
      case "cmd_volumeLeft":
      case "cmd_volumeRight":
      case "cmd_lockscreen":
        isSupported = true;
        break;
      default:
        isSupported = false;
        break;
    }
    return isSupported;
  },

  isCommandEnabled : function(cmd) {
    return true;
  },

  doCommand : function(cmd) {
    var browser = getBrowser();
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
      case "cmd_openLocation":
        this.showToolbar(true);
        break;
      case "cmd_star":
      {
        var bookmarkURI = browser.currentURI;
        var bookmarkTitle = browser.contentTitle || bookmarkURI.spec;

        let autoClose = false;

        if (PlacesUtils.getMostRecentBookmarkForURI(bookmarkURI) == -1) {
          let bmsvc = PlacesUtils.bookmarks;
          let bookmarkId = bmsvc.insertBookmark(BookmarkList.mobileRoot, bookmarkURI,
                                                bmsvc.DEFAULT_INDEX,
                                                bookmarkTitle);
          this.updateStar();

          
          autoClose = true;
        }

        
        BookmarkPopup.toggle(autoClose);
        break;
      }
      case "cmd_bookmarks":
        BookmarkList.show();
        break;
      case "cmd_quit":
        goQuitApplication();
        break;
      case "cmd_close":
        this._closeOrQuit();
        break;
      case "cmd_menu":
        getIdentityHandler().toggle();
        break;
      case "cmd_newTab":
        this.newTab();
        break;
      case "cmd_closeTab":
        this.closeTab();
        break;
      case "cmd_sanitize":
      {
        
        let button = document.getElementById("prefs-clear-data");
        button.disabled = true;
        setTimeout(function() { button.disabled = false; }, 5000);

        Sanitizer.sanitize();
        break;
      }
      case "cmd_panel":
      {
        if (BrowserUI.isPanelVisible())
          this.hidePanel();
        else
          this.showPanel();
        break;
      }
      case "cmd_zoomin":
        Browser.zoom(-1);
        break;
      case "cmd_zoomout":
        Browser.zoom(1);
        break;
      case "cmd_volumeLeft":
        
        Browser.zoom(Util.isPortrait() ? -1 : 1);
        break;
      case "cmd_volumeRight":
        
        Browser.zoom(Util.isPortrait() ? 1 : -1);
        break;
      case "cmd_lockscreen":
      {
        let locked = gPrefService.getBoolPref("toolkit.screen.lock");
        gPrefService.setBoolPref("toolkit.screen.lock", !locked);

        let strings = Elements.browserBundle;
        let alerts = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
        alerts.showAlertNotification(null, strings.getString("alertLockScreen"),
                                     strings.getString("alertLockScreen." + (!locked ? "locked" : "unlocked")), false, "", null);
        break;
      }
    }
  }
};

var PageActions = {
  get _permissionManager() {
    delete this._permissionManager;
    return this._permissionManager = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
  },

  get _loginManager() {
    delete this._loginManager;
    return this._loginManager = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
  },

  
  _permissions: ["popup", "offline-app", "geo"],

  _forEachPermissions: function _forEachPermissions(aHost, aCallback) {
    let pm = this._permissionManager;
    for (let i = 0; i < this._permissions.length; i++) {
      let type = this._permissions[i];
      if (!pm.testPermission(aHost, type))
        continue;

      let perms = pm.enumerator;
      while (perms.hasMoreElements()) {
        let permission = perms.getNext().QueryInterface(Ci.nsIPermission);
        if (permission.host == aHost.asciiHost && permission.type == type)
          aCallback(type);
      }
    }
  },

  updatePagePermissions: function updatePagePermissions() {
    this.removeItems("preferences");

    let host = Browser.selectedBrowser.currentURI;
    let permissions = [];

    this._forEachPermissions(host, function(aType) {
      permissions.push(aType);
    });

    let lm = this._loginManager;
    if (!lm.getLoginSavingEnabled(host.prePath)) {
      permissions.push("password");
    }

    
    if (permissions.length) {
      let title = Elements.browserBundle.getString("pageactions.reset");
      let description = [];
      for each(permission in permissions)
        description.push(Elements.browserBundle.getString("pageactions." + permission));

      let node = this.appendItem("preferences", title, description.join(", "));
      node.addEventListener("click", function(event) {
          PageActions.clearPagePermissions();
          PageActions.removeItem(node);
        },
        false);
    }

    let siteLogins = [];
    let allLogins = lm.findLogins({}, host.prePath, "", null);
    for (let i = 0; i < allLogins.length; i++) {
      let login = allLogins[i];
      if (login.hostname != host.prePath)
        continue;

      siteLogins.push(login);
    }

    
    if (siteLogins.length) {
      let title = Elements.browserBundle.getString("pageactions.password.forget");
      let node = this.appendItem("preferences", title, "");
      node.addEventListener("click", function(event) {
          for (let i = 0; i < siteLogins.length; i++)
            lm.removeLogin(siteLogins[i]);
  
          PageActions.removeItem(node);
        },
        false);
    }
  },

  clearPagePermissions: function clearPagePermissions() {
    let pm = this._permissionManager;
    let host = Browser.selectedBrowser.currentURI;
    this._forEachPermissions(host, function(aType) {
      pm.remove(host.asciiHost, aType);
    });

    let lm = this._loginManager;
    if (!lm.getLoginSavingEnabled(host.prePath))
      lm.setLoginSavingEnabled(host.prePath, true);
  },

  _savePageAsPDF: function saveAsPDF() {
    let strings = Elements.browserBundle;
    let picker = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    picker.init(window, strings.getString("pageactions.saveas.pdf"), Ci.nsIFilePicker.modeSave);
    picker.appendFilter("PDF", "*.pdf");
    picker.defaultExtension = "pdf";

    let browser = Browser.selectedBrowser;
    let fileName = getDefaultFileName(null, browser.documentURI, browser.contentDocument, null);
#ifdef MOZ_PLATFORM_MAEMO
    fileName = fileName.replace(/[\*\:\?]+/g, " ");
#endif
    picker.defaultString = fileName + ".pdf";

    let dm = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
    picker.displayDirectory = dm.defaultDownloadsDirectory;
    let rv = picker.show();
    if (rv == Ci.nsIFilePicker.returnCancel)
      return;

    
    let db = dm.DBConnection;

    let stmt = db.createStatement(
      "INSERT INTO moz_downloads (name, source, target, startTime, endTime, state, referrer) " +
      "VALUES (:name, :source, :target, :startTime, :endTime, :state, :referrer)"
    );

    let current = Browser.selectedBrowser.currentURI.spec;
    stmt.params.name = picker.file.leafName;
    stmt.params.source = current;
    stmt.params.target = gIOService.newFileURI(picker.file).spec;
    stmt.params.startTime = Date.now() * 1000;
    stmt.params.endTime = Date.now() * 1000;
    stmt.params.state = Ci.nsIDownloadManager.DOWNLOAD_NOTSTARTED;
    stmt.params.referrer = current;
    stmt.execute();
    stmt.finalize();

    let newItemId = db.lastInsertRowID;
    let download = dm.getDownload(newItemId);
    try {
      DownloadsView.downloadStarted(download);
    }
    catch(e) {}
    gObserverService.notifyObservers(download, "dl-start", null);

    let data = {
      type: Ci.nsIPrintSettings.kOutputFormatPDF,
      id: newItemId,
      referrer: current,
      filePath: picker.file.path
    };

    Browser.selectedBrowser.messageManager.sendAsyncMessage("Browser:SaveAs", data);
  },

  updatePageSaveAs: function updatePageSaveAs() {
    this.removeItems("saveas");
    if (Browser.selectedBrowser.contentDocument instanceof XULDocument)
      return;

    let strings = Elements.browserBundle;
    let node = this.appendItem("saveas", strings.getString("pageactions.saveas.pdf"), "");
    node.onclick = function(event) {
      PageActions._savePageAsPDF();
    }
  },

  appendItem: function appendItem(aType, aTitle, aDesc) {
    let container = document.getElementById("pageactions-container");
    let item = document.createElement("pageaction");
    item.setAttribute("title", aTitle);
    item.setAttribute("description", aDesc);
    item.setAttribute("type", aType);
    container.appendChild(item);

    let identityContainer = document.getElementById("identity-container");
    identityContainer.setAttribute("hasmenu", "true");
    return item;
  },

  removeItem: function removeItem(aItem) {
    let container = document.getElementById("pageactions-container");
    container.removeChild(aItem);

    let identityContainer = document.getElementById("identity-container");
    identityContainer.setAttribute("hasmenu", container.hasChildNodes() ? "true" : "false");
  },

  removeItems: function removeItems(aType) {
    let container = document.getElementById("pageactions-container");
    let count = container.childNodes.length;
    for (let i = count - 1; i >= 0; i--) {
      if (aType == "" || container.childNodes[i].getAttribute("type") == aType)
        this.removeItem(container.childNodes[i]);
    }
  }
}

var NewTabPopup = {
  _timeout: 0,
  _tabs: [],

  get box() {
    delete this.box;
    return this.box = document.getElementById("newtab-popup");
  },

  _updateLabel: function() {
    let newtabStrings = Elements.browserBundle.getString("newtabpopup.opened");
    let label = PluralForm.get(this._tabs.length, newtabStrings).replace("#1", this._tabs.length);

    this.box.firstChild.setAttribute("value", label);
  },

  hide: function() {
    if (this._timeout) {
      clearTimeout(this._timeout);
      this._timeout = 0;
    }

    this._tabs = [];
    this.box.hidden = true;
    BrowserUI.popPopup();
  },

  show: function(aTab) {
    BrowserUI.pushPopup(this, this.box);

    this._tabs.push(aTab);
    this._updateLabel();

    this.box.top = aTab.getBoundingClientRect().top + (aTab.getBoundingClientRect().height / 3);
    this.box.hidden = false;

    if (this._timeout)
      clearTimeout(this._timeout);

    this._timeout = setTimeout(function(self) {
      self.hide();
    }, 2000, this);
  },

  selectTab: function() {
    BrowserUI.selectTab(this._tabs.pop());
    this.hide();
  }
}

var BookmarkPopup = {
  get box() {
    delete this.box;
    return this.box = document.getElementById("bookmark-popup");
  },

  _bookmarkPopupTimeout: -1,

  hide : function hide() {
    if (this._bookmarkPopupTimeout != -1) {
      clearTimeout(this._bookmarkPopupTimeout);
      this._bookmarkPopupTimeout = -1;
    }
    this.box.hidden = true;
    BrowserUI.popPopup();
  },

  show : function show(aAutoClose) {
    const margin = 10;

    this.box.hidden = false;

    let [,,,controlsW] = Browser.computeSidebarVisibility();
    this.box.left = window.innerWidth - (this.box.getBoundingClientRect().width + controlsW + margin);
    this.box.top  = BrowserUI.starButton.getBoundingClientRect().top + margin;

    if (aAutoClose) {
      this._bookmarkPopupTimeout = setTimeout(function (self) {
        self._bookmarkPopupTimeout = -1;
        self.hide();
      }, 2000, this);
    }

    
    BrowserUI.pushPopup(this, [this.box, BrowserUI.starButton]);
  },

  toggle : function toggle(aAutoClose) {
    if (this.box.hidden)
      this.show(aAutoClose);
    else
      this.hide();
  }
}

var BookmarkHelper = {
  _panel: null,
  _editor: null,

  edit: function BH_edit(aURI) {
    if (!aURI)
      aURI = getBrowser().currentURI;

    let itemId = PlacesUtils.getMostRecentBookmarkForURI(aURI);
    if (itemId == -1)
      return;

    let title = PlacesUtils.bookmarks.getItemTitle(itemId);
    let tags = PlacesUtils.tagging.getTagsForURI(aURI, {});

    const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    this._editor = document.createElementNS(XULNS, "placeitem");
    this._editor.setAttribute("id", "bookmark-item");
    this._editor.setAttribute("flex", "1");
    this._editor.setAttribute("type", "bookmark");
    this._editor.setAttribute("ui", "manage");
    this._editor.setAttribute("title", title);
    this._editor.setAttribute("uri", aURI.spec);
    this._editor.setAttribute("itemid", itemId);
    this._editor.setAttribute("tags", tags.join(", "));
    this._editor.setAttribute("onclose", "BookmarkHelper.hide()");
    document.getElementById("bookmark-form").appendChild(this._editor);

    let toolbar = document.getElementById("toolbar-main");
    let top = toolbar.top + toolbar.boxObject.height;

    this._panel = document.getElementById("bookmark-container");
    this._panel.top = (top < 0 ? 0 : top);
    this._panel.hidden = false;
    BrowserUI.pushPopup(this, this._panel);

    let self = this;
    Browser.forceChromeReflow();
    self._editor.startEditing();
  },

  save: function BH_save() {
    this._editor.stopEditing(true);
  },

  hide: function BH_hide() {
    BrowserUI.updateStar();

    
    
    this._editor.parentNode.removeChild(this._editor);
    this._editor = null;

    this._panel.hidden = true;
    BrowserUI.popPopup();
  },
};

var BookmarkList = {
  _panel: null,
  _bookmarks: null,
  _manageButtton: null,

  get mobileRoot() {
    let items = PlacesUtils.annotations.getItemsWithAnnotation("mobile/bookmarksRoot", {});
    if (!items.length)
      throw "Couldn't find mobile bookmarks root!";

    delete this.mobileRoot;
    return this.mobileRoot = items[0];
  },

  show: function() {
    this._panel = document.getElementById("bookmarklist-container");
    this._panel.width = window.innerWidth;
    this._panel.height = window.innerHeight;
    this._panel.hidden = false;
    BrowserUI.pushDialog(this);

    this._bookmarks = document.getElementById("bookmark-items");
    this._bookmarks.addEventListener("BookmarkRemove", this, true);
    this._bookmarks.manageUI = false;
    this._bookmarks.openFolder();

    this._manageButton = document.getElementById("tool-bookmarks-manage");
    this._manageButton.disabled = (this._bookmarks.items.length == 0);
  },

  close: function() {
    BrowserUI.updateStar();

    if (this._bookmarks.manageUI)
      this.toggleManage();
    this._bookmarks.blur();
    this._bookmarks.removeEventListener("BookmarkRemove", this, true);

    this._panel.hidden = true;
    BrowserUI.popDialog();
  },

  toggleManage: function() {
    this._bookmarks.manageUI = !(this._bookmarks.manageUI);
    this._manageButton.checked = this._bookmarks.manageUI;
  },

  openBookmark: function() {
    let item = this._bookmarks.activeItem;
    if (item.spec) {
      this.close();
      BrowserUI.goToURI(item.spec);
    }
  },

  handleEvent: function(aEvent) {
    if (aEvent.type == "BookmarkRemove") {
      if (this._bookmarks.isRootFolder && this._bookmarks.items.length == 1) {
        this._manageButton.disabled = true;
        this.toggleManage();
      }
    }
  }
};

var FormHelper = {
  _open: false,
  _utils: null,
  _nodes: null,
  get _container() {
    delete this._container;
    return this._container = document.getElementById("form-helper-container");
  },

  get _helperSpacer() {
    delete this._helperSpacer;
    return this._helperSpacer = document.getElementById("form-helper-spacer");
  },

  get _selectContainer() {
    delete this._selectContainer;
    return this._selectContainer = document.getElementById("select-container");
  },

  get _autofillContainer() {
    delete this._autofillContainer;
    return this._autofillContainer = document.getElementById("form-helper-autofill");
  },

  _getRectForElement: function formHelper_getRectForElement(aElement) {
    const kDistanceMax = 100;
    let elRect = Browser.getBoundingContentRect(aElement);
    let bv = Browser._browserView;

    let labels = this.getLabelsFor(aElement);
    for (let i=0; i<labels.length; i++) {
      let labelRect = Browser.getBoundingContentRect(labels[i]);
      if (labelRect.left < elRect.left) {
        let isClose = Math.abs(labelRect.left - elRect.left) - labelRect.width < kDistanceMax &&
                      Math.abs(labelRect.top - elRect.top) - labelRect.height < kDistanceMax;
        if (isClose) {
          let width = labelRect.width + elRect.width + (elRect.left - labelRect.left - labelRect.width);
          return new Rect(labelRect.left, labelRect.top, width, elRect.height).expandToIntegers();
        }
      }
    }
    return elRect;
  },

  _update: function(aPreviousElement, aNewElement) {
    this._updateSelect(aPreviousElement, aNewElement);

    
    if (aNewElement instanceof HTMLInputElement && aNewElement.type == "text") {
      let suggestions = this._getSuggestions();
      this._setSuggestions(suggestions);
    } else {
      this._autofillContainer.collapsed = true;
    }

    let height = Math.floor(this._container.getBoundingClientRect().height);
    this._container.top = window.innerHeight - height;

    document.getElementById("form-helper-previous").disabled = !this._getPrevious();
    document.getElementById("form-helper-next").disabled = !this._getNext();
  },

  _updateSelect: function(aPreviousElement, aNewElement) {
    let previousIsSelect = this._isValidSelectElement(aPreviousElement);
    let currentIsSelect = this._isValidSelectElement(aNewElement);

    if (currentIsSelect && !previousIsSelect) {
      this._selectContainer.style.maxHeight = (window.innerHeight / 1.8) + "px";

      let rootNode = this._container;
      rootNode.insertBefore(this._selectContainer, rootNode.lastChild);

      SelectHelper.show(aNewElement);
    }
    else if (currentIsSelect && previousIsSelect) {
      SelectHelper.reset();
      SelectHelper.show(aNewElement);
    }
    else if (!currentIsSelect && previousIsSelect) {
      let rootNode = this._container.parentNode;
      rootNode.insertBefore(this._selectContainer, rootNode.lastChild);

      SelectHelper.close();
    }
  },

  _isValidElement: function(aElement) {
    if (aElement.disabled)
      return false;

    if (aElement.getAttribute("role") == "button" && aElement.hasAttribute("tabindex"))
      return this._isElementVisible(aElement);

    if (this._isValidSelectElement(aElement) || aElement instanceof HTMLTextAreaElement)
      return this._isElementVisible(aElement);

    if (aElement instanceof HTMLInputElement || aElement instanceof HTMLButtonElement) {
      if (aElement.type == "hidden")
        return false;
      return this._isElementVisible(aElement);
    }

    return false;
  },

  _isValidSelectElement: function(aElement) {
    return SelectHelper.canShowUIFor(aElement);
  },

  _isElementVisible: function(aElement) {
    let style = aElement.ownerDocument.defaultView.getComputedStyle(aElement, null);
    if (!style)
      return false;

    let isVisible = (style.getPropertyValue("visibility") != "hidden");
    let isOpaque = (style.getPropertyValue("opacity") != 0);

    let rect = aElement.getBoundingClientRect();
    return isVisible && isOpaque && (rect.height != 0 || rect.width != 0);
  },

  _getAllDocuments: function formHelper_getAllDocuments(aDocument, aResult) {
    
    aResult.push(aDocument);
    let frames = aDocument.defaultView.frames;
    if (!frames)
      return aResult;

    for (let i = 0; i < frames.length; i++)
      this._getAllDocuments(frames[i].document, aResult);

    return aResult;
  },

  _getAll: function() {
    let elements = [];

    
    let documents = [];
    this._getAllDocuments(getBrowser().contentDocument, documents);

    for (let i = 0; i < documents.length; i++) {
      let nodes = documents[i].querySelectorAll("input, button, select, textarea, [role=button]");
      nodes = this._filterRadioButtons(nodes).filter(this._isValidElement, this);
      elements = elements.concat(nodes);
    }

    function orderByTabIndex(a, b) {
      
      
      
      if (a.tabIndex == 0 || b.tabIndex == 0)
        return b.tabIndex;

      return a.tabIndex > b.tabIndex;
    }
    return elements.sort(orderByTabIndex);
  },

  



  _filterRadioButtons: function(nodes) {
    
    let chosenRadios = {};
    for (let i=0; i < nodes.length; i++) {
      let node = nodes[i];
      if (node.type == "radio" && (!chosenRadios.hasOwnProperty(node.name) || node.checked))
        chosenRadios[node.name] = node;
    }

    
    var result = [];
    for (let i=0; i < nodes.length; i++) {
      let node = nodes[i];
      if (node.type == "radio" && chosenRadios[node.name] != node)
        continue;
      result.push(node);
    }
    return result;
  },

  _getPrevious: function() {
    let index = this._nodes.indexOf(this._currentElement);
    let node = (index != -1 ? this._nodes[--index] : null);
    while (node && !this._isElementVisible(node))
      node = this._nodes[--index];
    return node;
  },

  _getNext: function() {
    let index = this._nodes.indexOf(this._currentElement);
    let node = (index != -1 ? this._nodes[++index] : null);
    while (node && !this._isElementVisible(node))
      node = this._nodes[++index];
    return node;
  },

  _fac: Cc["@mozilla.org/satchel/form-autocomplete;1"].getService(Ci.nsIFormAutoComplete),
  _getSuggestions: function() {
    let suggestions = [];
    let currentValue = this._currentElement.value;
    let results = this._fac.autoCompleteSearch(this._currentElement.name, currentValue, this._currentElement, null);
    if (results.matchCount > 0) {
      for (let i = 0; i < results.matchCount; i++) {
        let value = results.getValueAt(i);
        suggestions.push(value);
      }
    }

    return suggestions;
  },

  _setSuggestions: function(aSuggestions) {
    let autofill = this._autofillContainer;
    while (autofill.hasChildNodes())
      autofill.removeChild(autofill.lastChild);

    let fragment = document.createDocumentFragment();
    for (let i = 0; i < aSuggestions.length; i++) {
      let value = aSuggestions[i];
      let button = document.createElement("label");
      button.setAttribute("value", value);
      fragment.appendChild(button);
    }
    autofill.appendChild(fragment);
    autofill.collapsed = !aSuggestions.length;
  },

  doAutoFill: function formHelperDoAutoFill(aElement) {
    if (!this._currentElement)
     return;

    
    if (aElement instanceof Ci.nsIDOMXULLabelElement)
      this._currentElement.value = aElement.value;
  },

  getLabelsFor: function(aElement) {
    let associatedLabels = [];
    if (this._isValidElement(aElement)) {
      let labels = aElement.ownerDocument.getElementsByTagName("label");
      for (let i=0; i<labels.length; i++) {
        if (labels[i].getAttribute("for") == aElement.id)
          associatedLabels.push(labels[i]);
      }
    }

    if (aElement.parentNode instanceof HTMLLabelElement)
      associatedLabels.push(aElement.parentNode);

    return associatedLabels.filter(this._isElementVisible);
  },

  _currentElement: null,
  getCurrentElement: function() {
    return this._currentElement;
  },

  setCurrentElement: function(aElement) {
    if (!aElement)
      return;

    let previousElement = this._currentElement;
    this._currentElement = aElement;
    this._utils = aElement.ownerDocument.defaultView
                                        .QueryInterface(Ci.nsIInterfaceRequestor)
                                        .getInterface(Ci.nsIDOMWindowUtils);
    try {
      this._utils.QueryInterface(Ci.nsIDOMWindowUtils_1_9_2);
    }
    catch(e) {}

    this._update(previousElement, aElement);

    let containerHeight = this._container.getBoundingClientRect().height;
    this._helperSpacer.setAttribute("height", containerHeight);

    this.zoom(aElement);
    gFocusManager.setFocus(aElement, Ci.nsIFocusManager.FLAG_NOSCROLL);
  },

  goToPrevious: function formHelperGoToPrevious() {
    let previous = this._getPrevious();
    this.setCurrentElement(previous);
  },

  goToNext: function formHelperGoToNext() {
    let next = this._getNext();
    this.setCurrentElement(next);
  },

  open: function formHelperOpen(aElement) {
    if (this._open == true && aElement == this._currentElement &&
        gFocusManager.focusedElement == this._currentElement)
      return false;

    this._open = true;
    this._restore = { zoom: Browser._browserView.getZoomLevel(),
                      scroll: Browser.getScrollboxPosition(Browser.contentScrollboxScroller)
                    };

    window.addEventListener("keyup", this, false);
    let bv = Browser._browserView;
    bv.ignorePageScroll(true);

    this._container.hidden = false;
    this._helperSpacer.hidden = false;

    this._nodes = this._getAll();
    this.setCurrentElement(aElement);

    let evt = document.createEvent("UIEvents");
    evt.initUIEvent("FormUI", true, true, window, this._open);
    this._container.dispatchEvent(evt);

    return true;
  },

  close: function formHelperHide() {
    if (!this._open)
      return;

    this._updateSelect(this._currentElement, null);

    this._helperSpacer.hidden = true;

    
    let bv = Browser._browserView;
    Browser.forceChromeReflow();
    Browser.contentScrollboxScroller.scrollBy(0, 0);
    bv.onAfterVisibleMove();

    bv.ignorePageScroll(false);

    window.removeEventListener("keyup", this, false);
    this._container.hidden = true;
    this._currentElement = null;
    this._utils = null;

    if (gPrefService.getBoolPref("formhelper.restore") && this._restore) {
      bv.setZoomLevel(this._restore.zoom);
      Browser.contentScrollboxScroller.scrollTo(this._restore.scroll.x, this._restore.scroll.y);
      bv.onAfterVisibleMove();
      this._restore = null;
    }

    this._open = false;

    let evt = document.createEvent("UIEvents");
    evt.initUIEvent("FormUI", true, true, window, this._open);
    this._container.dispatchEvent(evt);
  },

  handleEvent: function formHelperHandleEvent(aEvent) {
    let isChromeFocused = gFocusManager.getFocusedElementForWindow(window, false, {}) == gFocusManager.focusedElement;
    if (isChromeFocused)
      return;

    let currentElement = this.getCurrentElement();
    switch (aEvent.keyCode) {
      case aEvent.DOM_VK_DOWN:
        if (currentElement instanceof HTMLTextAreaElement) {
          let existSelection = currentElement.selectionEnd - currentElement.selectionStart;
          let isEnd = (currentElement.textLength == currentElement.selectionEnd);
          if (!isEnd || existSelection)
            return;
        }

        this.goToNext();
        break;

      case aEvent.DOM_VK_UP:
        if (currentElement instanceof HTMLTextAreaElement) {
          let existSelection = currentElement.selectionEnd - currentElement.selectionStart;
          let isStart = (currentElement.selectionEnd == 0);
          if (!isStart || existSelection)
            return;
        }

        this.goToPrevious();
        break;

      case aEvent.DOM_VK_RETURN:
        break;

      default:
        let caret = this._getRectForCaret();
        if (caret) {
          let bv = Browser._browserView;

          
          let visible = bv.getVisibleRect();
          caret = bv.browserToViewportRect(caret);
    
          let [deltaX, deltaY] = this._getOffsetForCaret(caret, visible);
    
          
          if (deltaX != 0 || deltaY != 0) {
            Browser.contentScrollboxScroller.scrollBy(deltaX, deltaY);
            bv.onAfterVisibleMove();
          }
        }

        let target = aEvent.target;
        if (currentElement instanceof HTMLInputElement && currentElement.type == "text") {
          let suggestions = this._getSuggestions();
          this._setSuggestions(suggestions);

          let height = Math.floor(this._container.getBoundingClientRect().height);
          this._container.top = window.innerHeight - height;
          this._helperSpacer.setAttribute("height", height);

          
          
          if (suggestions.length == 0) {
            let bv = Browser._browserView;
            Browser.forceChromeReflow();
            Browser.contentScrollboxScroller.scrollBy(0, 0);
            bv.onAfterVisibleMove();
          }
        } else if (currentElement == target && this._isValidSelectElement(target)) {
          SelectHelper.unselectAll();
          SelectHelper.selectByIndex(target.selectedIndex);
        }
        break;
    }
  },

  zoom: function formHelperZoom(aElement) {
    let bv = Browser._browserView;
    if (!bv.allowZoom)
      return;

    let zoomLevel = bv.getZoomLevel();
    if (gPrefService.getBoolPref("formhelper.autozoom")) {
      zoomLevel = Browser._getZoomLevelForElement(aElement);
      zoomLevel = Math.min(Math.max(kBrowserFormZoomLevelMin, zoomLevel), kBrowserFormZoomLevelMax);
    }

    let elRect = this._getRectForElement(aElement);
    let zoomRect = Browser._getZoomRectForPoint(elRect.center().x, elRect.y, zoomLevel);

    let caretRect = this._getRectForCaret();
    if (caretRect) {
      caretRect = Browser._browserView.browserToViewportRect(caretRect);
      if (!zoomRect.contains(caretRect)) {
        let [deltaX, deltaY] = this._getOffsetForCaret(caretRect, zoomRect);
        zoomRect.translate(deltaX, deltaY);
      }
    }

    Browser.setVisibleRect(zoomRect);
  },
  
  _getRectForCaret: function formHelper_getRectForCaret() {
    let currentElement = this.getCurrentElement();
    if ((currentElement instanceof HTMLTextAreaElement ||
        (currentElement instanceof HTMLInputElement && currentElement.type == "text")) &&
        gFocusManager.focusedElement == this._currentElement) {

      let rect = this._utils.sendQueryContentEvent(this._utils.QUERY_CARET_RECT, currentElement.selectionEnd, 0, 0, 0);
      if (!rect)
        return null;

      let bv = Browser._browserView;
      let scroll = BrowserView.Util.getContentScrollOffset(Browser.selectedBrowser);
      let caret = new Rect(scroll.x + rect.left, scroll.y + rect.top, rect.width, rect.height);
      return caret;
    }
    
    return null;
  },

  _getOffsetForCaret: function formHelper_getOffsetForCaret(aCaretRect, aRect) {
    
    let deltaX = 0;
    if (aCaretRect.right > aRect.right)
      deltaX = aCaretRect.right - aRect.right;
    if (aCaretRect.left < aRect.left)
      deltaX = aCaretRect.left - aRect.left;
      
    
    let deltaY = 0;
    if (aCaretRect.bottom > aRect.bottom)
      deltaY = aCaretRect.bottom - aRect.bottom;
    if (aCaretRect.top < aRect.top)
      deltaY = aCaretRect.top - aRect.top;

    return [deltaX, deltaY];
  },

  canShowUIFor: function(aElement) {
    if (!aElement)
      return false;

    
    
    
    let formExceptions = {button: true, checkbox: true, file: true, image: true, radio: true, reset: true, submit: true};
    if (aElement instanceof HTMLInputElement && formExceptions[aElement.type])
      return false;

    if (aElement instanceof HTMLButtonElement || (aElement.getAttribute("role") == "button" && aElement.hasAttribute("tabindex")))
      return false;

    return this._isValidElement(aElement);
  }
};

function SelectWrapper(aControl) {
  this._control = aControl;
}

SelectWrapper.prototype = {
  get selectedIndex() { return this._control.selectedIndex; },
  get multiple() { return this._control.multiple; },
  get options() { return this._control.options; },
  get children() { return this._control.children; },

  getText: function(aChild) { return aChild.text; },
  isOption: function(aChild) { return aChild instanceof HTMLOptionElement; },
  isGroup: function(aChild) { return aChild instanceof HTMLOptGroupElement; },
  select: function(aIndex, aSelected, aClearAll) {
    let selectElement = this._control.QueryInterface(Ci.nsISelectElement);
    selectElement.setOptionsSelectedByIndex(aIndex, aIndex, aSelected, aClearAll, false, true);
  },
  focus: function() { this._control.focus(); },
  fireOnChange: function() {
    let control = this._control;
    let evt = document.createEvent("Events");
    evt.initEvent("change", true, true, window, 0,
                  false, false,
                  false, false, null);
    setTimeout(function() {
      control.dispatchEvent(evt)
    }, 0);
  }
};

function MenulistWrapper(aControl) {
  this._control = aControl;
}




MenulistWrapper.prototype = {
  get selectedIndex() {
    let control = this._control.wrappedJSObject || this._control;
    let result = control.selectedIndex;
    return (typeof result == "number" && !isNaN(result) ? result : -1);
  },
  get multiple() { return false; },
  get options() {
    let control = this._control.wrappedJSObject || this._control;
    return control.menupopup.children;
  },
  get children() {
    let control = this._control.wrappedJSObject || this._control;
    return control.menupopup.children;
  },

  getText: function(aChild) { return aChild.label; },
  isOption: function(aChild) { return aChild instanceof Ci.nsIDOMXULSelectControlItemElement; },
  isGroup: function(aChild) { return false },
  select: function(aIndex, aSelected, aClearAll) {
    let control = this._control.wrappedJSObject || this._control;
    control.selectedIndex = aIndex;
  },
  focus: function() { this._control.focus(); },
  fireOnChange: function() {
    let control = this._control;
    let evt = document.createEvent("XULCommandEvent");
    evt.initCommandEvent("command", true, true, window, 0,
                         false, false,
                         false, false, null);
    setTimeout(function() {
      control.dispatchEvent(evt)
    }, 0);
  }
};

var SelectHelper = {
  _panel: null,
  _list: null,
  _control: null,
  _selectedIndexes: [],

  _getSelectedIndexes: function() {
    let indexes = [];
    let control = this._control;

    if (control.multiple) {
      for (let i = 0; i < control.options.length; i++) {
        if (control.options[i].selected)
          indexes.push(i);
      }
    }
    else {
      indexes.push(control.selectedIndex);
    }

    return indexes;
  },

  show: function(aControl) {
    if (!aControl)
      return;

    if (aControl instanceof HTMLSelectElement)
      this._control = new SelectWrapper(aControl);
    else if (aControl instanceof Ci.nsIDOMXULMenuListElement)
      this._control = new MenulistWrapper(aControl);
    else
      throw "Unknown list element";

    this._selectedIndexes = this._getSelectedIndexes();

    this._list = document.getElementById("select-list");
    this._list.setAttribute("multiple", this._control.multiple ? "true" : "false");

    let firstSelected = null;

    let optionIndex = 0;
    let children = this._control.children;
    for (let i=0; i<children.length; i++) {
      let child = children[i];
      if (this._control.isGroup(child)) {
        let group = document.createElement("option");
        group.setAttribute("label", child.label);
        this._list.appendChild(group);
        group.className = "optgroup";

        let subchildren = child.children;
        for (let ii=0; ii<subchildren.length; ii++) {
          let subchild = subchildren[ii];
          let item = document.createElement("option");
          item.setAttribute("label", this._control.getText(subchild));
          this._list.appendChild(item);
          item.className = "in-optgroup";
          item.optionIndex = optionIndex++;
          if (subchild.selected) {
            item.setAttribute("selected", "true");
            firstSelected = firstSelected ? firstSelected : item;
          }
        }
      } else if (this._control.isOption(child)) {
        let item = document.createElement("option");
        item.setAttribute("label", this._control.getText(child));
        this._list.appendChild(item);
        item.optionIndex = optionIndex++;
        if (child.selected) {
          item.setAttribute("selected", "true");
          firstSelected = firstSelected ? firstSelected : item;
        }
      }
    }

    this._panel = document.getElementById("select-container");
    this._panel.hidden = false;

    this._scrollElementIntoView(firstSelected);

    this._list.addEventListener("click", this, false);
  },

  _scrollElementIntoView: function(aElement) {
    if (!aElement)
      return;

    let index = -1;
    this._forEachOption(
      function(aItem, aIndex) {
        if (aElement.optionIndex == aItem.optionIndex)
          index = aIndex;
      }
    );

    if (index == -1)
      return;

    let scrollBoxObject = this._list.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    let itemHeight = aElement.getBoundingClientRect().height;
    let visibleItemsCount = this._list.boxObject.height / itemHeight;
    if ((index + 1) > visibleItemsCount) {
      let delta = Math.ceil(visibleItemsCount / 2);
      scrollBoxObject.scrollTo(0, ((index + 1) - delta) * itemHeight);
    }
    else {
      scrollBoxObject.scrollTo(0, 0);
    }
  },

  _forEachOption: function(aCallback) {
      let children = this._list.children;
      for (let i = 0; i < children.length; i++) {
        let item = children[i];
        if (!item.hasOwnProperty("optionIndex"))
          continue;
        aCallback(item, i);
      }
  },

  _updateControl: function() {
    let currentSelectedIndexes = this._getSelectedIndexes();

    let isIdentical = currentSelectedIndexes.length == this._selectedIndexes.length;
    if (isIdentical) {
      for (let i = 0; i < currentSelectedIndexes.length; i++) {
        if (currentSelectedIndexes[i] != this._selectedIndexes[i]) {
          isIdentical = false;
          break;
        }
      }
    }

    if (!isIdentical)
      this._control.fireOnChange();
  },

  _isValidElement: function(aElement) {
    if (!aElement || aElement.disabled)
      return false;

    return (aElement instanceof HTMLSelectElement) || (aElement instanceof Ci.nsIDOMXULMenuListElement);
  },

  reset: function() {
    this._updateControl();
    let empty = this._list.cloneNode(false);
    this._list.parentNode.replaceChild(empty, this._list);
    this._list = empty;
  },

  close: function() {
    this._list.removeEventListener("click", this, false);
    this._panel.hidden = true;

    this.reset();
  },

  canShowUIFor: function(aElement) {
    return this._isValidElement(aElement);
  },

  unselectAll: function() {
    this._forEachOption(function(aItem, aIndex) aItem.selected = false);
  },

  selectByIndex: function(aIndex) {
    for (let i = 0; i < this._list.childNodes.length; i++) {
      let option = this._list.childNodes[i];
      if (option.optionIndex == aIndex) {
        option.selected = true;
        this._scrollElementIntoView(option);
        break;
      }
    }
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "click":
        let item = aEvent.target;
        if (item && item.hasOwnProperty("optionIndex")) {
          if (this._control.multiple) {
            
            item.selected = !item.selected;
            this._control.select(item.optionIndex, item.selected, false);
          }
          else {
            this.unselectAll();

            
            item.selected = true;
            this._control.select(item.optionIndex, true, true);
          }
        }
        break;
    }
  }
};

const kXLinkNamespace = "http://www.w3.org/1999/xlink";

var ContextHelper = {
  popupNode: null,
  onLink: false,
  onSaveableLink: false,
  onVoiceLink: false,
  onImage: false,
  onLoadedImage: false,
  linkURL: "",
  linkProtocol: null,
  mediaURL: "",

  _clearState: function ch_clearState() {
    this.popupNode = null;
    this.onLink = false;
    this.onSaveableLink = false;
    this.onVoiceLink = false;
    this.onImage = false;
    this.onLoadedImage = false;
    this.linkURL = "";
    this.linkProtocol = null;
    this.mediaURL = "";
  },

  _getLinkURL: function ch_getLinkURL(aLink) {
    let href = aLink.href;  
    if (href)
      return href;

    href = aLink.getAttributeNS(kXLinkNamespace, "href");
    if (!href || !href.match(/\S/)) {
      
      
      throw "Empty href";
    }

    return Util.makeURLAbsolute(aLink.baseURI, href);
  },

  _getURI: function ch_getURI(aURL) {
    try {
      return makeURI(aURL);
    } catch (ex) { }

    return null;
  },
  
  _getProtocol: function ch_getProtocol(aURI) {
    if (aURI)
      return aURI.scheme;
    return null;
  },
  
  _isSaveable: function ch_isSaveable(aProtocol) {
    
    return aProtocol && !(aProtocol == "mailto" || aProtocol == "javascript" || aProtocol == "news" || aProtocol == "snews");
  },

  _isVoice: function ch_isVoice(aProtocol) {
    
    return aProtocol && (aProtocol == "tel" || aProtocol == "callto" || aProtocol == "sip" || aProtocol == "voipto");
  },

  handleEvent: function ch_handleEvent(aEvent) {
    this._clearState();

    let [elementX, elementY] = Browser.transformClientToBrowser(aEvent.clientX, aEvent.clientY);
    this.popupNode = Browser.elementFromPoint(elementX, elementY);

    
    if (this.popupNode.nodeType == Node.ELEMENT_NODE) {
      
      if (this.popupNode instanceof Ci.nsIImageLoadingContent && this.popupNode.currentURI) {
        this.onImage = true;
        this.mediaURL = this.popupNode.currentURI.spec;

        let request = this.popupNode.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
        if (request && (request.imageStatus & request.STATUS_SIZE_AVAILABLE))
          this.onLoadedImage = true;
      }
    }

    let elem = this.popupNode;
    while (elem) {
      if (elem.nodeType == Node.ELEMENT_NODE) {
        
        if (!this.onLink &&
             ((elem instanceof HTMLAnchorElement && elem.href) ||
              (elem instanceof HTMLAreaElement && elem.href) ||
              elem instanceof HTMLLinkElement ||
              elem.getAttributeNS(kXLinkNamespace, "type") == "simple")) {

          
          this.linkURL = this._getLinkURL(elem);
          this.linkProtocol = this._getProtocol(this._getURI(this.linkURL));
          this.onLink = true;
          this.onSaveableLink = this._isSaveable(this.linkProtocol);
          this.onVoiceLink = this._isVoice(this.linkProtocol);
        }
      }

      elem = elem.parentNode;
    }

    let first = last = null;
    let commands = document.getElementById("context-commands");
    for (let i=0; i<commands.childElementCount; i++) {
      let command = commands.children[i];
      let types = command.getAttribute("type").split(/\s+/);
      command.removeAttribute("selector");
      if (types.indexOf("image") != -1 && this.onImage) {
        first = (first ? first : command);
        last = command;
        command.hidden = false;
        continue;
      } else if (types.indexOf("image-loaded") != -1 && this.onLoadedImage) {
        first = (first ? first : command);
        last = command;
        command.hidden = false;
        continue;
      } else if (types.indexOf("link") != -1 && this.onSaveableLink) {
        first = (first ? first : command);
        last = command;
        command.hidden = false;
        continue;
      } else if (types.indexOf("callto") != -1 && this.onVoiceLink) {
        first = (first ? first : command);
        last = command;
        command.hidden = false;
        continue;
      } else if (types.indexOf("mailto") != -1 && this.onLink && this.linkProtocol == "mailto") {
        first = (first ? first : command);
        last = command;
        command.hidden = false;
        continue;
      }
      command.hidden = true;
    }

    if (!first) {
      this._clearState();
      return;
    }
    
    first.setAttribute("selector", "first-child");
    last.setAttribute("selector", "last-child");

    let label = document.getElementById("context-hint");
    if (this.onImage)
      label.value = this.mediaURL;
    if (this.onLink)
      label.value = this.linkURL;

    let container = document.getElementById("context-popup");
    container.hidden = false;

    
    let preferredHeight = 0;
    for (let i=0; i<container.childElementCount; i++) {
      preferredHeight += container.children[i].getBoundingClientRect().height;
    }

    let rect = container.getBoundingClientRect();
    let height = Math.min(preferredHeight, 0.75 * window.innerWidth);
    let width = Math.min(rect.width, 0.75 * window.innerWidth);

    container.height = height;
    container.width = width;
    container.top = (window.innerHeight - height) / 2;
    container.left = (window.innerWidth - width) / 2;

    BrowserUI.pushPopup(this, [container]);
  },
  
  hide: function ch_hide() {
    this._clearState();
    
    let container = document.getElementById("context-popup");
    container.hidden = true;

    BrowserUI.popPopup();
  }
};

var ContextCommands = {
  openInNewTab: function cc_openInNewTab(aEvent) {
    Browser.addTab(ContextHelper.linkURL, false);
  },

  saveImage: function cc_saveImage(aEvent) {
    let doc = ContextHelper.popupNode.ownerDocument;
    saveImageURL(ContextHelper.mediaURL, null, "SaveImageTitle", false, false, doc.documentURIObject);
  }
}

function removeBookmarksForURI(aURI) {
  
  
  
  let itemIds = PlacesUtils.getBookmarksForURI(aURI);
  itemIds.forEach(PlacesUtils.bookmarks.removeItem);

  BrowserUI.updateStar();
}
