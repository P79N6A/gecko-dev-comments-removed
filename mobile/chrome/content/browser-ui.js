





































Components.utils.import("resource://gre/modules/utils.js");

const TOOLBARSTATE_LOADING  = 1;
const TOOLBARSTATE_LOADED   = 2;

const URLBAR_FORCE  = 1;
const URLBAR_EDIT   = 2;

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
  _autocompleteNavbuttons : null,
  _favicon : null,
  _faviconLink : null,
  _dialogs: [],

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
    this._edit.value = caption;
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

  showToolbar : function showToolbar(aFlags) {
    this.hidePanel();

    aFlags = aFlags || 0;

    if (aFlags & URLBAR_FORCE) {
      ws.freeze("toolbar-main");
      ws.moveFrozenTo("toolbar-main", 0, 0);
    }
    else {
      ws.unfreeze("toolbar-main");
    }

    this._editToolbar(aFlags & URLBAR_EDIT);
  },

  _editToolbar : function _editToolbar(aEdit) {
    var icons = document.getElementById("urlbar-icons");
    if (aEdit && icons.getAttribute("mode") != "edit") {
      icons.setAttribute("mode", "edit");
      this._edit.defaultValue = this._edit.value;

      let urlString = this.getDisplayURI(Browser.selectedBrowser);
      if (urlString == "about:blank")
        urlString = "";
      this._edit.value = urlString;

      
      
      
      this._edit.inputField.blur();

      this._edit.inputField.focus();
      this._edit.select();
    }
    else if (!aEdit && icons.getAttribute("mode") != "view") {
      icons.setAttribute("mode", "view");
      this._edit.inputField.blur();
      this._edit.reallyClosePopup();
    }
  },

  _closeOrQuit: function _closeOrQuit() {
    
    let dialog = this.activeDialog;
    if (dialog)
      dialog.close();
    else
      CommandUpdater.doCommand("cmd_quit");
  },

  get activeDialog() {
    
    if (this._dialogs.length)
      return this._dialogs[this._dialogs.length - 1];
    return null;
  },

  pushDialog : function pushDialog(aDialog) {
    
    if (aDialog) {
      this._dialogs.push(aDialog);
      document.getElementById("toolbar-main").setAttribute("dialog", "true");
    }
  },

  popDialog : function popDialog() {
    
    if (this._dialogs.length)
      this._dialogs.pop();

    
    if (!this._dialogs.length)
      document.getElementById("toolbar-main").removeAttribute("dialog");
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

  _initControls : false,
  sizeControls : function(windowW, windowH) {
    let toolbar = document.getElementById("toolbar-main");
    let tabs = document.getElementById("tabs-container");
    let controls = document.getElementById("browser-controls");
    if (!this._initControls) {
      this._initControls = true;
      ws.moveUnfrozenTo("toolbar-main", null, -this.toolbarH);
      ws.moveUnfrozenTo("tabs-container", -tabs.boxObject.width, this.toolbarH);
      ws.moveUnfrozenTo("browser-controls", null, this.toolbarH);
    }

    toolbar.width = windowW;

    let popup = document.getElementById("popup_autocomplete");
    popup.top = this.toolbarH;
    popup.height = windowH - this.toolbarH;
    popup.width = windowW;

    
    document.getElementById("notifications").width = windowW;

    
    document.getElementById("findbar-container").width = windowW;

    
    document.getElementById("identity-container").width = windowW;

    
    let sideBarHeight = windowH - this.toolbarH;
    controls.height = sideBarHeight;
    tabs.height = sideBarHeight;

    
    let bmkeditor = document.getElementById("bookmark-container");
    bmkeditor.width = windowW;

    
    let bookmarks = document.getElementById("bookmarklist-container");
    bookmarks.height = windowH;
    bookmarks.width = windowW;

    
    let selectlist = document.getElementById("select-container");
    selectlist.height = windowH;
    selectlist.width = windowW;

    
    let panel = document.getElementById("panel-container");
    panel.height = windowH;
    panel.width = windowW;
  },

  init : function() {
    this._edit = document.getElementById("urlbar-edit");
    this._edit.addEventListener("keypress", this, true);
    this._throbber = document.getElementById("urlbar-throbber");
    this._favicon = document.getElementById("urlbar-favicon");
    this._favicon.addEventListener("error", this, false);
    this._autocompleteNavbuttons = document.getElementById("autocomplete_navbuttons");

    let urlbarEditArea = document.getElementById("urlbar-editarea");
    urlbarEditArea.addEventListener("click", this, false);
    urlbarEditArea.addEventListener("mousedown", this, false);

    document.getElementById("tabs").addEventListener("TabSelect", this, true);

    let browsers = document.getElementById("browsers");
    browsers.addEventListener("DOMWindowClose", this, true);
    browsers.addEventListener("UIShowSelect", this, false, true);

    
    browsers.addEventListener("DOMTitleChanged", this, true);
    browsers.addEventListener("DOMLinkAdded", this, true);


    ExtensionsView.init();
    DownloadsView.init();
  },

  uninit : function() {
    ExtensionsView.uninit();
  },

  update : function(aState) {
    let icons = document.getElementById("urlbar-icons");
    let uri = Browser.selectedBrowser.currentURI;

    switch (aState) {
      case TOOLBARSTATE_LOADED:
        icons.setAttribute("mode", "view");
        if (uri.spec == "about:blank")
          this.showToolbar(URLBAR_EDIT);
        else
          this.showToolbar();

        if (!this._faviconLink)
          this._faviconLink = uri.prePath + "/favicon.ico";
        this._setIcon(this._faviconLink);
        this.updateIcon();
        this._faviconLink = null;
        break;

      case TOOLBARSTATE_LOADING:
        this.showToolbar(URLBAR_FORCE);
        
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

  
  setURI : function() {
    var browser = Browser.selectedBrowser;

    
    if (!browser.currentURI)
      return;

    
    this._updateButtons(browser);

    
    this.updateStar();

    var urlString = this.getDisplayURI(browser);
    if (urlString == "about:blank")
      urlString = "";

    this._edit.value = urlString;
  },

  goToURI : function(aURI) {
    this._edit.reallyClosePopup();

    if (!aURI)
      aURI = this._edit.value;

    var flags = Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
    getBrowser().loadURIWithFlags(aURI, flags, null, null);

    gHistSvc.markPageAsTyped(gURIFixup.createFixupURI(aURI, 0));
  },

  search : function() {
    var queryURI = "http://www.google.com/search?q=" + this._edit.value + "&hl=en&lr=&btnG=Search";
    getBrowser().loadURI(queryURI, null, null, false);
  },

  showAutoComplete : function(showDefault) {
    this.updateSearchEngines();
    this._edit.showHistoryPopup();
  },

  doButtonSearch : function(button) {
    if (!("engine" in button) || !button.engine)
      return;

    var urlbar = this._edit;
    urlbar.open = false;
    var value = urlbar.value;

    var submission = button.engine.getSubmission(value, null);
    getBrowser().loadURI(submission.uri.spec, null, submission.postData, false);
  },

  engines : null,
  updateSearchEngines : function() {
    if (this.engines)
      return;

    var searchService = Cc["@mozilla.org/browser/search-service;1"].getService(Ci.nsIBrowserSearchService);
    var engines = searchService.getVisibleEngines({ });
    this.engines = engines;

    const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var container = this._autocompleteNavbuttons;
    for (var e = 0; e < engines.length; e++) {
      var button = document.createElementNS(kXULNS, "toolbarbutton");
      var engine = engines[e];
      button.id = engine.name;
      button.setAttribute("label", engine.name);
      button.className = "searchengine show-text button-dark";
      if (engine.iconURI)
        button.setAttribute("image", engine.iconURI.spec);
      container.appendChild(button);
      button.engine = engine;
    }
  },

  updateStar : function() {
    var star = document.getElementById("tool-star");
    if (PlacesUtils.getMostRecentBookmarkForURI(Browser.selectedBrowser.currentURI) != -1)
      star.setAttribute("starred", "true");
    else
      star.removeAttribute("starred");
  },

  goToBookmark : function goToBookmark(aEvent) {
    if (aEvent.originalTarget.localName == "button")
      return;

    var list = document.getElementById("urllist-items");
    BrowserUI.goToURI(list.selectedItem.value);
  },

  showHistory : function() {
    
  },

  showBookmarks : function () {
    BookmarkList.show();
  },

  newTab : function newTab(aURI) {
    ws.panTo(0, -this.toolbarH);
    return Browser.addTab(aURI || "about:blank", true);
  },

  closeTab : function closeTab(aTab) {
    
    Browser.closeTab(aTab || Browser.selectedTab);
  },

  selectTab : function selectTab(aTab) {
    Browser.selectedTab = aTab;
  },

  hideTabs: function hideTabs() {
    if (ws.isWidgetVisible("tabs-container")) {
      let widthOfTabs = document.getElementById("tabs-container").boxObject.width;
      ws.panBy(widthOfTabs, 0, true);
    }
  },

  hideControls: function hideControls() {
    if (ws.isWidgetVisible("browser-controls")) {
      let widthOfControls = document.getElementById("browser-controls").boxObject.width;
      ws.panBy(-widthOfControls, 0, true);
    }
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

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      
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
      
      case "click":
        this.doCommand("cmd_openLocation");
        break;
      case "keypress":
        if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
          this._edit.reset();
          this._editToolbar(false);
        }
        break;
      case "mousedown":
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
        this.showToolbar(URLBAR_EDIT | URLBAR_FORCE);
        setTimeout(function () { BrowserUI.showAutoComplete(); }, 0);
        break;
      case "cmd_star":
      {
        this.hideControls();

        var bookmarkURI = browser.currentURI;
        var bookmarkTitle = browser.contentDocument.title || bookmarkURI.spec;

        if (PlacesUtils.getMostRecentBookmarkForURI(bookmarkURI) == -1) {
          var bookmarkId = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarks.unfiledBookmarksFolder, bookmarkURI, PlacesUtils.bookmarks.DEFAULT_INDEX, bookmarkTitle);
          this.updateStar();

          var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
          var faviconURI = ios.newURI(this._favicon.src, null, null);

          PlacesUtils.favicons.setAndLoadFaviconForPage(bookmarkURI, faviconURI, true);
        }
        else {
          BookmarkHelper.edit(bookmarkURI);
        }
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
        Browser.canvasBrowser.zoom(-1);
        break;
      case "cmd_zoomout":
        Browser.canvasBrowser.zoom(1);
        break;
    }
  }
};

var BookmarkHelper = {
  _panel: null,
  _editor: null,

  edit: function BH_edit(aURI) {
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
    this._editor.setAttribute("onmove", "FolderPicker.show(this);");
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

    window.addEventListener("keypress", this, true);
  },

  close: function BH_close() {
    window.removeEventListener("keypress", this, true);
    BrowserUI.updateStar();

    
    
    this._editor.parentNode.removeChild(this._editor);
    this._editor = null;

    this._panel.hidden = true;
    BrowserUI.popDialog();
  },

  handleEvent: function BH_handleEvent(aEvent) {
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE)
      this.close();
  }
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

    window.addEventListener("keypress", this, true);
  },

  close: function() {
    window.removeEventListener("keypress", this, true);
    BrowserUI.updateStar();

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
      this._panel.hidden = true;
      BrowserUI.goToURI(item.spec);
    }
  },

  handleEvent: function(aEvent) {
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE)
      this.close();
  }
};

var FolderPicker = {
  _control: null,
  _panel: null,

  show: function(aControl) {
    this._panel = document.getElementById("folder-container");
    this._panel.width = window.innerWidth;
    this._panel.height = window.innerHeight;
    this._panel.hidden = false;
    BrowserUI.pushDialog(this);

    this._control = aControl;

    let folders = document.getElementById("folder-items");
    folders.openFolder();
  },

  close: function() {
    this._panel.hidden = true;
    BrowserUI.popDialog();
  },

  moveItem: function() {
    let folders = document.getElementById("folder-items");
    let itemId = (this._control.activeItem ? this._control.activeItem.itemId : this._control.itemId);
    let folderId = PlacesUtils.bookmarks.getFolderIdForItem(itemId);
    if (folders.selectedItem.itemId != folderId) {
      PlacesUtils.bookmarks.moveItem(itemId, folders.selectedItem.itemId, PlacesUtils.bookmarks.DEFAULT_INDEX);
      if (this._control.removeItem)
        this._control.removeItem(this._control.activeItem);
    }
    this.close();
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

    this._control = aControl;
    this._selectedIndexes = this._getSelectedIndexes();

    this._list = document.getElementById("select-list");
    this._list.setAttribute("multiple", this._control.multiple ? "true" : "false");

    let firstSelected = null;
    
    let optionIndex = 0;
    let children = this._control.children;
    for (let i=0; i<children.length; i++) {
      let child = children[i];
      if (child instanceof HTMLOptGroupElement) {
        let group = document.createElement("option");
        group.setAttribute("label", child.label);
        this._list.appendChild(group);
        group.className = "optgroup";

        let subchildren = child.children;
        for (let ii=0; ii<subchildren.length; ii++) {
          let subchild = subchildren[ii];
          let item = document.createElement("option");
          item.setAttribute("label", subchild.text);
          this._list.appendChild(item);
          item.className = "in-optgroup";
          item.optionIndex = optionIndex++;
          if (subchild.selected) {
            item.setAttribute("selected", "true");
            firstSelected = firstSelected ? firstSelected : item;
          }
        }
      } else if (child instanceof HTMLOptionElement) {
        let item = document.createElement("option");
        item.setAttribute("label", child.textContent);
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

    if (!isIdentical) {
      let control = this._control.wrappedJSObject;
      if ("onchange" in control)
        control.onchange();
    }
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
        let selectElement = this._control.wrappedJSObject.selectElement;
        if (item && item.hasOwnProperty("optionIndex")) {
          if (this._control.multiple) {
            
            item.selected = !item.selected;
            selectElement.setOptionsSelectedByIndex(item.optionIndex, item.optionIndex, item.selected, false, false, true);
          }
          else {
            
            this._forEachOption(
              function(aItem, aIndex) {
                aItem.selected = false;
              }
            );

            
            item.selected = true;
            selectElement.setOptionsSelectedByIndex(item.optionIndex, item.optionIndex, true, true, false, true);
          }
        }
        break;
    }
  }
};
