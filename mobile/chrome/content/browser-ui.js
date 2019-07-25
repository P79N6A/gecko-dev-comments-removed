





































Cu.import("resource://gre/modules/utils.js");
Cu.import("resource://gre/modules/PluralForm.jsm");

const TOOLBARSTATE_LOADING  = 1;
const TOOLBARSTATE_LOADED   = 2;

const kDefaultFavIconURL = "chrome://browser/skin/images/favicon-default-30.png";

[
  [
    "gHistSvc",
    "@mozilla.org/browser/nav-history-service;1",
    [Ci.nsINavHistoryService, Ci.nsIBrowserHistory]
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
  _faviconLink : null,
  _dialogs: [],

  _domWillOpenModalDialog: function(e) {
    if (!e.isTrusted)
      return;

    
    

    let aWindow = e.target.top;
    for (let i = 0; i <= Browser._tabs.length; i++) {
      if (Browser._tabs[i].browser.contentWindow == aWindow) {
        Browser.selectedTab = Browser._tabs[i];
      }
    }
  },

  _titleChanged : function(aDocument) {
    var browser = Browser.selectedBrowser;
    if (browser && aDocument != browser.contentDocument)
      return;

    var caption = aDocument.title;
    if (!caption) {
      caption = this.getDisplayURI(browser);
      if (caption == "about:blank")
        caption = "";
    }

    this._setURI(caption);
  },

  



  _domWindowClose: function (aEvent) {
    if (!aEvent.isTrusted)
      return;

    
    let browsers = Browser.browsers;
    for (let i = 0; i < browsers.length; i++) {
      if (browsers[i].contentWindow == aEvent.target) {
        Browser.closeTab(Browser.getTabAtIndex(i));
        aEvent.preventDefault();
        break;
      }
    }
  },

  _linkAdded : function(aEvent) {
    let link = aEvent.originalTarget;
    if (!link || !link.href)
      return;

    if (/\bicon\b/i(link.rel)) {
      this._faviconLink = link.href;

      
      
      if (this._favicon.src != "")
        this._setIcon(this._faviconLink);
    }
    else if (/\bsearch\b/i(link.rel)) {
      var type = link.type && link.type.toLowerCase();
      type = type.replace(/^\s+|\s*(?:;.*)?$/g, "");
      if (type == "application/opensearchdescription+xml" && link.title && /^(?:https?|ftp):/i.test(link.href)) {
        var engine = { title: link.title, href: link.href };

        BrowserSearch.addPageSearchEngine(engine, link.ownerDocument);
      }
    }
  },

  _updateButtons : function(aBrowser) {
    var back = document.getElementById("cmd_back");
    var forward = document.getElementById("cmd_forward");

    back.setAttribute("disabled", !aBrowser.canGoBack);
    forward.setAttribute("disabled", !aBrowser.canGoForward);
  },

  _tabSelect : function(aEvent) {
    var browser = Browser.selectedBrowser;
    this._titleChanged(browser.contentDocument);
    this._updateButtons(browser);
    this.updateStar();
    this._favicon.src = browser.mIconURL || kDefaultFavIconURL;

    
    
    this._faviconLink = this._favicon.src;
    this.updateIcon();
  },

  _setIcon : function(aURI) {
    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    try {
      var faviconURI = ios.newURI(aURI, null, null);
    }
    catch (e) {
      faviconURI = null;
    }

    var fis = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);
    if (!faviconURI || faviconURI.schemeIs("javascript") || fis.isFailedFavicon(faviconURI))
      faviconURI = ios.newURI(kDefaultFavIconURL, null, null);

    var browser = getBrowser();
    browser.mIconURL = faviconURI.spec;

    fis.setAndLoadFaviconForPage(browser.currentURI, faviconURI, true);
    this._favicon.src = faviconURI.spec;
  },

  showToolbar : function showToolbar(aEdit) {
    this.hidePanel();
    this._editURI(aEdit);
  },

  _toolbarLocked: 0,
  lockToolbar: function lockToolbar() {
    this._toolbarLocked++;
    document.getElementById("toolbar-moveable-container").top = "0";
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
      if (urlString == "about:blank")
        urlString = "";
      this._edit.value = urlString;
    }
    else if (!aEdit && Browser.selectedTab.isLoading() && icons.getAttribute("mode") != "loading") {
      icons.setAttribute("mode", "loading");
    }
    else if (!aEdit && icons.getAttribute("mode") != "view") {
      icons.setAttribute("mode", "view");
    }
  },

  _closeOrQuit: function _closeOrQuit() {
    
    let dialog = this.activeDialog;
    if (dialog)
      dialog.close();
    else
      window.close();
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
    }
  },

  popDialog : function popDialog() {
    
    if (this._dialogs.length) {
      this._dialogs.pop();
      this.unlockToolbar();
    }

    
    if (!this._dialogs.length)
      document.getElementById("toolbar-main").removeAttribute("dialog");
  },

  pushPopup: function pushPopup(aPanel, aElements) {
    this._popup =  { "panel": aPanel, 
                     "elements": (aElements instanceof Array) ? aElements : [aElements] };
  },

  popPopup: function popPopup() {
    this._popup = null;
  },

  _updatePopup: function _updateContextualPanel(aEvent) {
    if (!this._popup)
      return;

    let element = this._popup.elements;
    let targetNode = aEvent.target;
    while (targetNode && element.indexOf(targetNode) == -1)
      targetNode = targetNode.parentNode;

    if (targetNode == null) {
      let panel = this._popup.panel;
      if (panel.hide)
        panel.hide();
      this.popPopup();
    }
  },

  switchPane : function(id) {
    document.getElementById("panel-items").selectedPanel = document.getElementById(id);
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

    
    document.getElementById("toolbar-container").height = this.toolbarH;
    document.getElementById("toolbar-main").width = windowW;

    
    document.getElementById("notifications").width = windowW;

    
    document.getElementById("findbar-container").width = windowW;

    
    document.getElementById("identity-container").width = windowW;

    
    let bmkeditor = document.getElementById("bookmark-container");
    bmkeditor.width = windowW;

    
    let bookmarks = document.getElementById("bookmarklist-container");
    bookmarks.height = windowH;
    bookmarks.width = windowW;
    
    
    let bookmarkPopup = document.getElementById("bookmark-popup");
    let bookmarkPopupW = windowW / 4;
    bookmarkPopup.height = windowH / 4;
    bookmarkPopup.width = bookmarkPopupW;
    let starRect = this.starButton.getBoundingClientRect();
    const popupMargin = 10;
    bookmarkPopup.top = Math.round(starRect.top) + popupMargin;
    bookmarkPopup.left = windowW - this.sidebarW - bookmarkPopupW - popupMargin;

    
    let selectlist = document.getElementById("select-container");
    selectlist.height = windowH;
    selectlist.width = windowW;

    
    let panel = document.getElementById("panel-container");
    panel.height = windowH;
    panel.width = windowW;
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

    let browsers = document.getElementById("browsers");
    browsers.addEventListener("DOMWindowClose", this, true);
    browsers.addEventListener("UIShowSelect", this, false, true);

    
    browsers.addEventListener("DOMTitleChanged", this, true);
    browsers.addEventListener("DOMLinkAdded", this, true);
    browsers.addEventListener("DOMWillOpenModalDialog", this, true);
    
    
    window.addEventListener("mousedown", this, true);

    
    window.addEventListener("keypress", this, true);

    ExtensionsView.init();
    DownloadsView.init();
    PreferencesView.init();
  },

  uninit : function() {
    ExtensionsView.uninit();
  },

  update : function(aState) {
    let icons = document.getElementById("urlbar-icons");

    
    
    let uri = Browser.selectedBrowser.contentDocument.documentURIObject;

    switch (aState) {
      case TOOLBARSTATE_LOADED:
        if (icons.getAttribute("mode") != "edit")
          icons.setAttribute("mode", "view");
        
        if (!this._faviconLink)
          this._faviconLink = uri.prePath + "/favicon.ico";
        this._setIcon(this._faviconLink);
        this.updateIcon();
        this._faviconLink = null;
        break;

      case TOOLBARSTATE_LOADING:
        if (icons.getAttribute("mode") != "edit")
          icons.setAttribute("mode", "loading");

        this._favicon.src = "";
        this._faviconLink = null;
        this.updateIcon();
        break;
    }
  },

  updateIcon : function() {
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
    var uri = browser.currentURI;

    if (!this._URIFixup)
      this._URIFixup = Cc["@mozilla.org/docshell/urifixup;1"].getService(Ci.nsIURIFixup);

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

    
    this.updateStar();

    var urlString = this.getDisplayURI(browser);
    if (urlString == "about:blank")
      urlString = "";

    this._setURI(urlString);
  },

  goToURI : function(aURI) {
    aURI = aURI || this._edit.value;
    if (!aURI)
      return;

    this._edit.popup.close();

    var flags = Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
    getBrowser().loadURIWithFlags(aURI, flags, null, null);

    gHistSvc.markPageAsTyped(gURIFixup.createFixupURI(aURI, 0));
  },

  search : function() {
    var queryURI = "http://www.google.com/search?q=" + this._edit.value + "&hl=en&lr=&btnG=Search";
    getBrowser().loadURI(queryURI, null, null, false);
  },

  showAutoComplete : function() {
    if (this.isAutoCompleteOpen())
      return;
    BrowserSearch.updateSearchButtons();
    this._edit.showHistoryPopup();
  },

  isAutoCompleteOpen: function isAutoCompleteOpen() {
    return this._edit.popup.popupOpen;
  },

  doButtonSearch : function(button) {
    if (!("engine" in button) || !button.engine)
      return;

    
    button.parentNode.selectedItem = null;
    
    this._edit.popup.closePopup();

    var submission = button.engine.getSubmission(this._edit.value, null);
    getBrowser().loadURI(submission.uri.spec, null, submission.postData, false);
  },

  updateStar : function() {
    if (PlacesUtils.getMostRecentBookmarkForURI(Browser.selectedBrowser.currentURI) != -1)
      this.starButton.setAttribute("starred", "true");
    else
      this.starButton.removeAttribute("starred");
  },

  goToBookmark : function goToBookmark(aEvent) {
    if (aEvent.originalTarget.localName == "button")
      return;

    var list = document.getElementById("urllist-items");
    this.goToURI(list.selectedItem.value);
  },

  showBookmarks : function () {
    BookmarkList.show();
  },

  newTab : function newTab(aURI) {
    aURI = aURI || "about:blank";
    let tab = Browser.addTab(aURI, true);

    this.hidePanel();
    
    if (aURI == "about:blank") {
      this.showToolbar(true);
      this.showAutoComplete();
    }
    return tab;
  },

  closeTab : function closeTab(aTab) {
    
    Browser.closeTab(aTab || Browser.selectedTab);
  },

  selectTab : function selectTab(aTab) {
    Browser.selectedTab = aTab;
  },

  hideTabs: function hideTabs() {






  },

  hideControls: function hideControls() {






  },

  isTabsVisible: function isTabsVisible() {
    
    let [leftvis,_1,_2,_3] = Browser.computeSidebarVisibility();
    return (leftvis > 0.002);
  },

  showPanel: function showPanel(aPage) {
    let panelUI = document.getElementById("panel-container");

    panelUI.hidden = false;
    panelUI.width = window.innerWidth;
    panelUI.height = window.innerHeight;

    if (aPage != undefined)
      this.switchPane(aPage);
  },

  hidePanel: function hidePanel() {
    let panelUI = document.getElementById("panel-container");
    panelUI.hidden = true;
  },
  
  switchTask: function switchTask() {
    try {
      let phone = Cc["@mozilla.org/phone/support;1"].createInstance(Ci.nsIPhoneSupport);
      phone.switchTask();
    } catch(e) { }
  },
  
  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      
      case "DOMWillOpenModalDialog":
        this._domWillOpenModalDialog(aEvent);
        break;
      case "DOMTitleChanged":
        this._titleChanged(aEvent.target);
        break;
      case "DOMLinkAdded":
        this._linkAdded(aEvent);
        break;
      case "DOMWindowClose":
        this._domWindowClose(aEvent);
        break;
      case "UIShowSelect":
        SelectHelper.show(aEvent.target);
        break;
      case "TabSelect":
        this._tabSelect(aEvent);
        break;
      case "TabOpen":
        if (!this.isTabsVisible() && 
            Browser.selectedTab.chromeTab != aEvent.target)
          NewTabPopup.show(aEvent.target);
        break;
      
      case "click":
        this.doCommand("cmd_openLocation");
        break;
      case "keypress":
        if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
          let dialog = this.activeDialog;
          if (dialog)
            dialog.close();
        }
        break;
      case "mousedown":
        this._updatePopup(aEvent);

        if (aEvent.detail == 2 &&
            aEvent.button == 0 &&
            gPrefService.getBoolPref("browser.urlbar.doubleClickSelectsAll")) {
          this._edit.editor.selectAll();
          aEvent.preventDefault();
        }
        break;
      
      case "error":
        this._favicon.src = "chrome://browser/skin/images/default-favicon.png";
        break;
    }
  },

  supportsCommand : function(cmd) {
    var isSupported = false;
    switch (cmd) {
      case "cmd_back":
      case "cmd_forward":
      case "cmd_reload":
      case "cmd_stop":
      case "cmd_search":
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
      case "cmd_stop":
        browser.stop();
        break;
      case "cmd_search":
        this.search();
        break;
      case "cmd_go":
        this.goToURI();
        break;
      case "cmd_openLocation":
        this.showToolbar(true);
        this.showAutoComplete();
        break;
      case "cmd_star":
      {
        var bookmarkURI = browser.currentURI;
        var bookmarkTitle = browser.contentDocument.title || bookmarkURI.spec;

        let autoClose = false;

        if (PlacesUtils.getMostRecentBookmarkForURI(bookmarkURI) == -1) {
          var bookmarkId = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarks.unfiledBookmarksFolder, bookmarkURI, PlacesUtils.bookmarks.DEFAULT_INDEX, bookmarkTitle);
          this.updateStar();

          
          autoClose = true;
        }

        
        BookmarkPopup.toggle(autoClose);
        break;
      }
      case "cmd_bookmarks":
        this.showBookmarks();
        break;
      case "cmd_quit":
        goQuitApplication();
        break;
      case "cmd_close":
        this._closeOrQuit();
        break;
      case "cmd_menu":
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
        let panelUI = document.getElementById("panel-container");
        if (panelUI.hidden)
          this.showPanel();
        else
          this.hidePanel();
        break;
      }
      case "cmd_zoomin":
        Browser._browserView.zoom(-1);
        break;
      case "cmd_zoomout":
        Browser._browserView.zoom(1);
        break;
    }
  }
};

var NewTabPopup = {
  _timeout: 0,
  _tabs: [],

  get box() {
    delete this.box;
    return this.box = document.getElementById("newtab-popup");
  },

  _updateLabel: function() {
    let newtabStrings = document.getElementById("bundle_browser").getString("newtabpopup.opened");
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
  },
  
  show: function(aTab) {
    this._tabs.push(aTab);
    this._updateLabel();

    this.box.top = aTab.getBoundingClientRect().top + (aTab.getBoundingClientRect().height / 3);
    this.box.hidden = false;

    if (this._timeout)
      clearTimeout(this._timeout);

    this._timeout = setTimeout(function(self) {
      self.hide();
    }, 2000, this);

    BrowserUI.pushPopup(this, this.box);
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

  hide : function () {
    if (this._bookmarkPopupTimeout != -1) {
      clearTimeout(this._bookmarkPopupTimeout);
      this._bookmarkPopupTimeout = -1;
    }
    this.box.hidden = true;
  },
  
  show : function (aAutoClose) {
    this.box.hidden = false;

    if (aAutoClose) {
      this._bookmarkPopupTimeout = setTimeout(function (self) {
        self._bookmarkPopupTimeout = -1;
        self.hide();
      }, 2000, this);
    }

    
    BrowserUI.pushPopup(this, [this.box, BrowserUI.starButton]);
  },
  
  toggle : function (aAutoClose) {
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

    this._editor = document.createElement("placeitem");
    this._editor.setAttribute("id", "bookmark-item");
    this._editor.setAttribute("flex", "1");
    this._editor.setAttribute("type", "bookmark");
    this._editor.setAttribute("ui", "manage");
    this._editor.setAttribute("title", title);
    this._editor.setAttribute("uri", aURI.spec);
    this._editor.setAttribute("tags", tags.join(", "));
    this._editor.setAttribute("onclose", "BookmarkHelper.close()");
    document.getElementById("bookmark-form").appendChild(this._editor);

    let toolbar = document.getElementById("toolbar-main");
    let top = toolbar.top + toolbar.boxObject.height;

    this._panel = document.getElementById("bookmark-container");
    this._panel.top = (top < 0 ? 0 : top);
    this._panel.hidden = false;
    BrowserUI.pushDialog(this);

    let self = this;
    setTimeout(function() {
      self._editor.init(itemId);
      self._editor.startEditing();
    }, 0);
  },

  close: function BH_close() {
    BrowserUI.updateStar();

    
    
    this._editor.parentNode.removeChild(this._editor);
    this._editor = null;

    this._panel.hidden = true;
    BrowserUI.popDialog();
  },
};

var BookmarkList = {
  _panel: null,
  _bookmarks: null,

  show: function() {
    this._panel = document.getElementById("bookmarklist-container");
    this._panel.width = window.innerWidth;
    this._panel.height = window.innerHeight;
    this._panel.hidden = false;
    BrowserUI.pushDialog(this);

    this._bookmarks = document.getElementById("bookmark-items");
    this._bookmarks.manageUI = false;
    this._bookmarks.openFolder();
  },

  close: function() {
    BrowserUI.updateStar();
    document.getElementById("tool-bookmarks-manage").checked = false;

    if (this._bookmarks.isEditing)
      this._bookmarks.stopEditing();
    this._bookmarks.blur();
    this._panel.hidden = true;
    BrowserUI.popDialog();
  },

  toggleManage: function() {
    this._bookmarks.manageUI = !(this._bookmarks.manageUI);
  },

  openBookmark: function() {
    let item = this._bookmarks.activeItem;
    if (item.spec) {
      this.close();
      BrowserUI.goToURI(item.spec);
    }
  }
};

function SelectWrapper(aControl) {
  this._control = aControl;
}

SelectWrapper.prototype = {
  get selectedIndex() { return this.control.selectedIndex; },
  get multiple() { return this._control.multiple; },
  get options() { return this._control.options; },
  get children() { return this._control.children; },
  
  getText: function(aChild) { return aChild.text; },
  isOption: function(aChild) { return aChild instanceof HTMLOptionElement; },
  isGroup: function(aChild) { return aChild instanceof HTMLOptGroupElement; },
  select: function(aIndex, aSelected, aClearAll) {
    let selectElement = this._control.wrappedJSObject.selectElement;
    selectElement.setOptionsSelectedByIndex(aIndex, aIndex, aSelected, aClearAll, false, true);
  },
  focus: function() { this._control.focus(); },
  fireOnChange: function() {
    let control = this._control.wrappedJSObject;
    if ("onchange" in control)
      control.onchange();
  }
};

function MenulistWrapper(aControl) {
  this._control = aControl;
}

MenulistWrapper.prototype = {
  get selectedIndex() { return this.control.selectedIndex; },
  get multiple() { return false; },
  get options() { return this._control.menupopup.children; },
  get children() { return this._control.menupopup.children; },
  
  getText: function(aChild) { return aChild.label; },
  isOption: function(aChild) { return aChild instanceof Ci.nsIDOMXULSelectControlItemElement; },
  isGroup: function(aChild) { return false },
  select: function(aIndex, aSelected, aClearAll) {
    this._control.selectedIndex = aIndex;
  },
  focus: function() { this._control.focus(); },
  fireOnChange: function() {
    let control = this._control;
    if ("onchange" in control)
      control.onchange();
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

    if (control.type == 'select-one') {
      indexes.push(control.selectedIndex);
    }
    else {
      for (let i = 0; i < control.options.length; i++) {
        if (control.options[i].selected)
          indexes.push(i);
      }
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

    this._list.focus();
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

    let itemHeight = aElement.getBoundingClientRect().height;
    let visibleItemsCount = this._list.boxObject.height / itemHeight;
    if ((index + 1) > visibleItemsCount) {
      let delta = Math.ceil(visibleItemsCount / 2);
      let scrollBoxObject = this._list.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
      scrollBoxObject.scrollTo(0, ((index + 1) - delta) * itemHeight);
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

  close: function() {
    this._updateControl();

    this._list.removeEventListener("click", this, false);
    this._panel.hidden = true;

    
    let empty = this._list.cloneNode(false);
    this._list.parentNode.replaceChild(empty, this._list);
    this._list = empty;

    this._control.focus();
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
            
            this._forEachOption(
              function(aItem, aIndex) {
                aItem.selected = false;
              }
            );

            
            item.selected = true;
            this._control.select(item.optionIndex, true, true);
          }
        }
        break;
    }
  }
};

function removeBookmarksForURI(aURI) {
  
  
  
  let itemIds = PlacesUtils.getBookmarksForURI(aURI);
  itemIds.forEach(PlacesUtils.bookmarks.removeItem);

  BrowserUI.updateStar();
}
