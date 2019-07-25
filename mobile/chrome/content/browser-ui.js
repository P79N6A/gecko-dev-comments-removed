




































const TOOLBARSTATE_LOADING        = 1;
const TOOLBARSTATE_LOADED         = 2;
const TOOLBARSTATE_INDETERMINATE  = 3;

const UIMODE_NONE              = 0;
const UIMODE_URLVIEW           = 1;
const UIMODE_URLEDIT           = 2;
const UIMODE_BOOKMARK          = 3;
const UIMODE_BOOKMARKLIST      = 4;
const UIMODE_PANEL             = 5;

const kDefaultFavIconURL = "chrome://browser/skin/images/default-favicon.png";

var BrowserUI = {
  _panel : null,
  _caption : null,
  _edit : null,
  _throbber : null,
  _autocompleteNavbuttons : null,
  _favicon : null,
  _faviconAdded : false,

  _titleChanged : function(aDocument) {
    var browser = Browser.currentBrowser;
    if (browser && aDocument != browser.contentDocument)
      return;

    this._caption.value = aDocument.title;

    var docElem = document.documentElement;
    var title = "";
    if (aDocument.title)
      title = aDocument.title + docElem.getAttribute("titleseparator");
    document.title = title + docElem.getAttribute("titlemodifier");
  },

  _linkAdded : function(aEvent) {
    var link = aEvent.originalTarget;
    if (!link || !link.ownerDocument || !link.href)
      return;

    var rel = link.rel && link.rel.toLowerCase();
    var rels = rel.split(/\s+/);
    if (rels.indexOf("icon") != -1) {
      this._throbber.setAttribute("src", "");
      this._setIcon(link.href);
    }
  },

  _tabSelect : function(aEvent) {
    var browser = Browser.currentBrowser;
    this.setURI();
    this._titleChanged(browser.contentDocument);
    this._favicon.setAttribute("src", browser.mIconURL || kDefaultFavIconURL);

    let toolbar = document.getElementById("toolbar-main");
    let browserBox = document.getElementById("browser");
    if (Browser.content.currentTab.chromeTop) {
      
      browserBox.top = Browser.content.currentTab.chromeTop;
      browserBox.left = 0;
      toolbar.top = browserBox.top - toolbar.boxObject.height;
    }
    else {
      
      toolbar.top = 0;
      browserBox.top = toolbar.boxObject.height;
      browserBox.left = 0;
    }

    this.show(UIMODE_NONE);
  },

  _setIcon : function(aURI) {
    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var faviconURI = ios.newURI(aURI, null, null);

    var fis = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);
    if (faviconURI.schemeIs("javascript") ||
        fis.isFailedFavicon(faviconURI))
      faviconURI = ios.newURI(kDefaultFavIconURL, null, null);

    var browser = getBrowser();
    browser.mIconURL = faviconURI.spec;

    fis.setAndLoadFaviconForPage(browser.currentURI, faviconURI, true);
    this._favicon.setAttribute("src", faviconURI.spec);
    this._faviconAdded = true;
  },

  _getBookmarks : function(aFolders) {
    var items = [];

    var hs = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
    var options = hs.getNewQueryOptions();
    
    var query = hs.getNewQuery();
    query.setFolders(aFolders, 1);
    var result = hs.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    for (var i=0; i<cc; ++i) {
      var node = rootNode.getChild(i);
      items.push(node);
    }
    rootNode.containerOpen = false;

    return items;
  },

  _getHistory : function(aCount) {
    var items = [];

    var hs = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
    var options = hs.getNewQueryOptions();
    options.queryType = options.QUERY_TYPE_HISTORY;
    
    options.maxResults = aCount;
    
    var query = hs.getNewQuery();
    var result = hs.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    for (var i=0; i<cc; ++i) {
      var node = rootNode.getChild(i);
      items.push(node);
    }
    rootNode.containerOpen = false;

    return items;
  },

  _dragData :  {
    dragging : false,
    startX : 0,
    startY : 0,
    dragX : 0,
    dragY : 0,
    lastX : 0,
    lastY : 0,
    sTop : 0,
    sLeft : 0
  },

  _scrollToolbar : function bui_scrollToolbar(aEvent) {
    var [scrollWidth, ] = Browser.content._contentAreaDimensions;
    var [canvasW, ] = Browser.content._effectiveCanvasDimensions;

    var pannedUI = false;

    if (this._dragData.dragging && Browser.content.scrollY == 0) {
      let toolbar = document.getElementById("toolbar-main");
      let browser = document.getElementById("browser");
      let dy = this._dragData.lastY - aEvent.screenY;
      this._dragData.dragY += dy;

      
      

      let newTop = null;
      if (dy > 0 && (toolbar.top > -toolbar.boxObject.height && browser.left == 0)) {
        
        newTop = this._dragData.sTop - dy;

        
        if (newTop < -toolbar.boxObject.height)
          newTop = -toolbar.boxObject.height;

        
        Browser.content.dragData.sX = aEvent.screenX;
        Browser.content.dragData.sY = aEvent.screenY;
      }
      else if (dy < 0 && (toolbar.top < 0 && browser.left == 0)) {
        
        newTop = this._dragData.sTop - dy;

        
        if (newTop > 0)
          newTop = 0;
      }

      
      
      if (newTop != null) {
        toolbar.top = newTop;
        browser.top = newTop + toolbar.boxObject.height;

        
        Browser.content.currentTab.chromeTop = browser.top;

        pannedUI = true;
      }
    }

    if (this._dragData.dragging && (Browser.content.scrollX == 0 || (Browser.content.scrollX + canvasW) == scrollWidth)) {
      let tabbar = document.getElementById("tab-list-container");
      let sidebar = document.getElementById("browser-controls");
      let panelUI = document.getElementById("panel-container");
      let toolbar = document.getElementById("toolbar-main");
      let browser = document.getElementById("browser");
      let dx = this._dragData.lastX - aEvent.screenX;
      this._dragData.dragX += dx;

      if (Math.abs(this._dragData.screenX - aEvent.screenX) > 30) {
        let newLeft = this._dragData.sLeft - dx;
        let oldLeft = tabbar.left;

        let tabbarW = tabbar.boxObject.width;
        let sidebarW = sidebar.boxObject.width;
        let browserW = browser.boxObject.width;

        
        if (newLeft > 0)
          newLeft = 0;
        if (newLeft < -(tabbarW + sidebarW))
          newLeft = -(tabbarW + sidebarW);

        
        if (Math.abs(newLeft + tabbarW) < 30)
          newLeft = -tabbarW;
        tabbar.left = newLeft;

        
        let newToolbarLeft = newLeft;
        if (newToolbarLeft < 0)
          newToolbarLeft = 0;
        toolbar.left = newToolbarLeft;

        
        if (newLeft + tabbarW != 0)
          toolbar.top = 0;
        else
          toolbar.top = browser.top - toolbar.boxObject.height;

        browser.left = newLeft + tabbarW;
        sidebar.left = newLeft + tabbarW + browserW;
        panelUI.left = newLeft + tabbarW + browserW + sidebarW;

        pannedUI = true;
      }
    }

    if (pannedUI) {
      aEvent.stopPropagation();

      
      window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
            .getInterface(Components.interfaces.nsIDOMWindowUtils)
            .processUpdates();
    }
    else {
      
      this._dragData.lastX = aEvent.screenX;
      this._dragData.lastY = aEvent.screenY;
    }
  },

  _showToolbar : function(aShow) {
    var toolbar = document.getElementById("toolbar-main");
    var browser = document.getElementById("browser");

    if (aShow) {
      
      if (toolbar.top == -toolbar.boxObject.height) {
        
        toolbar.top = 0;
      }
      else if (toolbar.top < 0) {
        
        toolbar.top = 0;
        browser.top = toolbar.boxObject.height;
      }
    }
    else {
      
      if (browser.top == 0) {
        toolbar.top = -toolbar.boxObject.height;
      }
    }
  },

  _showPanel : function(aShow) {
      let tabbar = document.getElementById("tab-list-container");
      let sidebar = document.getElementById("browser-controls");
      let panelUI = document.getElementById("panel-container");
      let toolbar = document.getElementById("toolbar-main");
      let browser = document.getElementById("browser");

      let tabbarW = tabbar.boxObject.width;
      let sidebarW = sidebar.boxObject.width;
      let browserW = browser.boxObject.width;

      let newLeft = (aShow ? -browserW : -(tabbarW + sidebarW));
      tabbar.left = newLeft;

      let newToolbarLeft = newLeft + tabbarW;
      if (newToolbarLeft < -sidebarW)
        newToolbarLeft += sidebarW;
      else if (newToolbarLeft < 0)
        newToolbarLeft = 0;
      toolbar.left = newToolbarLeft;

      browser.left = newLeft + tabbarW;
      sidebar.left = newLeft + tabbarW + browserW;
      panelUI.left = newLeft + tabbarW + browserW + sidebarW;
      panelUI.width = browserW;
  },

  _layoutControls : true,
  _sizeControls : function(aEvent) {
    var rect = document.getElementById("browser-container").getBoundingClientRect();
    var containerW = rect.right - rect.left;
    var containerH = rect.bottom - rect.top;

    var browser = document.getElementById("browser");
    browser.width = containerW;
    browser.height = containerH;

    var sidebar = document.getElementById("browser-controls");
    var panelUI = document.getElementById("panel-container");
    var tabbar = document.getElementById("tab-list-container");
    if (this._layoutControls) {
      tabbar.left = -tabbar.boxObject.width;
      panelUI.left = containerW + sidebar.boxObject.width;
      sidebar.left = containerW;
      sidebar.height = panelUI.height = tabbar.height = containerH;
      this._layoutControls = false;
    }
    panelUI.width = containerW - sidebar.boxObject.width - tabbar.boxObject.width;

    var toolbar = document.getElementById("toolbar-main");
    var popup = document.getElementById("popup_autocomplete");
    toolbar.width = containerW;
    popup.height = containerH - toolbar.boxObject.height;
  },

  init : function() {
    this._caption = document.getElementById("urlbar-caption");
    this._caption.addEventListener("click", this, false);
    this._edit = document.getElementById("urlbar-edit");
    this._edit.addEventListener("blur", this, false);
    this._edit.addEventListener("keypress", this, true);
    this._edit.addEventListener("input", this, false);
    this._throbber = document.getElementById("urlbar-throbber");
    this._favicon = document.getElementById("urlbar-favicon");
    this._favicon.addEventListener("error", this, false);
    this._autocompleteNavbuttons = document.getElementById("autocomplete_navbuttons");

    Browser.content.addEventListener("DOMTitleChanged", this, true);
    Browser.content.addEventListener("DOMLinkAdded", this, true);

    document.getElementById("tab-list").addEventListener("TabSelect", this, true);

    Browser.content.addEventListener("mousedown", this, true);
    Browser.content.addEventListener("mouseup", this, true);
    Browser.content.addEventListener("mousemove", this, true);

    window.addEventListener("resize", this, false);
  },

  update : function(aState, aBrowser) {
    if (aState == TOOLBARSTATE_INDETERMINATE) {
      this._faviconAdded = false;
      aState = TOOLBARSTATE_LOADED;
      this.setURI();
    }

    var toolbar = document.getElementById("toolbar-main");
    if (aState == TOOLBARSTATE_LOADING) {
      this.show(UIMODE_URLVIEW);
      Browser.content.setLoading(aBrowser);

      toolbar.top = 0;
      toolbar.setAttribute("mode", "loading");
      this._throbber.setAttribute("src", "chrome://browser/skin/images/throbber.gif");
      this._favicon.setAttribute("src", "");
      this._faviconAdded = false;
    }
    else if (aState == TOOLBARSTATE_LOADED) {
      var container = document.getElementById("browser");
      container.top = toolbar.boxObject.height;

      toolbar.setAttribute("mode", "view");
      this._throbber.setAttribute("src", "");
      if (this._faviconAdded == false) {
        var faviconURI = aBrowser.currentURI.prePath + "/favicon.ico";
        this._setIcon(faviconURI);
      }
    }
  },

  
  setURI : function() {
    var browser = Browser.currentBrowser;

    
    if (!browser.currentURI)
      return;

    var back = document.getElementById("cmd_back");
    var forward = document.getElementById("cmd_forward");

    back.setAttribute("disabled", !browser.canGoBack);
    forward.setAttribute("disabled", !browser.canGoForward);

    
    var star = document.getElementById("tool-star");
    var bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
    var bookmarks = bms.getBookmarkIdsForURI(browser.currentURI, {});
    if (bookmarks.length > 0) {
      star.setAttribute("starred", "true");
    }
    else {
      star.removeAttribute("starred");
    }

    var uri = browser.currentURI;

    if (!this._URIFixup)
      this._URIFixup = Cc["@mozilla.org/docshell/urifixup;1"].getService(Ci.nsIURIFixup);

    try {
      uri = this._URIFixup.createExposableURI(uri);
    } catch (ex) {}

    var urlString = uri.spec;
    if (urlString == "about:blank") {
      urlString = "";
      this.show(UIMODE_URLEDIT);
    }

    this._caption.value = urlString;
    this._edit.value = urlString;
  },

  goToURI : function(aURI) {
    this._edit.reallyClosePopup();

    if (!aURI)
      aURI = this._edit.value;

    var flags = Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
    getBrowser().loadURIWithFlags(aURI, flags, null, null);
    this.show(UIMODE_URLVIEW);
  },

  search : function() {
    var queryURI = "http://www.google.com/search?q=" + this._edit.value + "&hl=en&lr=&btnG=Search";
    getBrowser().loadURI(queryURI, null, null, false);

    this.show(UIMODE_URLVIEW);
  },

  openDefaultHistory : function () {
    if (!this._edit.value) {
      this._autocompleteNavbuttons.hidden = true;
      this._edit.showHistoryPopup();
    }
  },

  doButtonSearch : function(button)
  {
    if (!("engine" in button) || !button.engine)
      return;

    var urlbar = this._edit;
    urlbar.open = false;
    var value = urlbar.value;
    if (!value)
      return;

    var submission = button.engine.getSubmission(value, null);
    getBrowser().loadURI(submission.uri.spec, null, submission.postData, false);
  },

  engines : null,
  updateSearchEngines : function () {
    if (this.engines)
      return;

    
    try {
      var searchService = Cc["@mozilla.org/browser/search-service;1"].
                          getService(Ci.nsIBrowserSearchService);
    } catch (ex) {
      this.engines = [ ];
      return;
    }

    var engines = searchService.getVisibleEngines({ });
    this.engines = engines;
    const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var container = this._autocompleteNavbuttons;
    for (var e = 0; e < engines.length; e++) {
      var button = document.createElementNS(kXULNS, "toolbarbutton");
      var engine = engines[e];
      button.id = engine.name;
      button.setAttribute("label", engine.name);
      if (engine.iconURI)
        button.setAttribute("image", engine.iconURI.spec);
      container.insertBefore(button, container.firstChild);
      button.engine = engine;
    }
  },

  mode : UIMODE_NONE,
  show : function(aMode) {
    if (this.mode == aMode)
      return;

    if (this.mode == UIMODE_BOOKMARKLIST && aMode != UIMODE_BOOKMARKLIST)
      window.removeEventListener("keypress", BrowserUI.closePopup, false);

    this.mode = aMode;

    var toolbar = document.getElementById("toolbar-main");
    var bookmark = document.getElementById("bookmark-container");
    var urllist = document.getElementById("urllist-container");
    var container = document.getElementById("browser-container");

    if (aMode == UIMODE_URLVIEW)
    {
      this._showToolbar(true);
      toolbar.setAttribute("mode", "view");
      this._edit.hidden = true;
      this._edit.reallyClosePopup();
      this._caption.hidden = false;
      bookmark.hidden = true;
      urllist.hidden = true;
    }
    else if (aMode == UIMODE_URLEDIT) {
      this._showToolbar(true);
      toolbar.setAttribute("mode", "edit");
      this._caption.hidden = true;
      this._edit.hidden = false;
      this._edit.focus();

      bookmark.hidden = true;
      urllist.hidden = true;
    }
    else if (aMode == UIMODE_BOOKMARK) {
      this._showToolbar(true);
      toolbar.setAttribute("mode", "view");
      this._edit.hidden = true;
      this._edit.reallyClosePopup();
      this._caption.hidden = false;

      urllist.hidden = true;
      bookmark.hidden = false;
      bookmark.width = container.boxObject.width;
    }
    else if (aMode == UIMODE_BOOKMARKLIST) {
      this._showToolbar(false);
      toolbar.setAttribute("mode", "view");
      this._edit.hidden = true;
      this._edit.reallyClosePopup();
      this._caption.hidden = false;

      window.addEventListener("keypress", this.closePopup, false);

      bookmark.hidden = true;
      urllist.hidden = false;
      urllist.width = container.boxObject.width;
      urllist.height = container.boxObject.height;
    }
    else if (aMode == UIMODE_PANEL) {
      this._showToolbar(true);
      toolbar.setAttribute("mode", "view");
      this._edit.hidden = true;
      this._edit.reallyClosePopup();
      this._caption.hidden = false;

      bookmark.hidden = true;

      let addons = document.getElementById("addons-container");
      if (addons.getAttribute("src") == "")
        addons.setAttribute("src", "chrome://mozapps/content/extensions/extensions.xul");
      let dloads = document.getElementById("downloads-container");
      if (dloads.getAttribute("src") == "")
        dloads.setAttribute("src", "chrome://mozapps/content/downloads/downloads.xul");
    }
    else if (aMode == UIMODE_NONE) {
      this._showToolbar(false);
      this._edit.reallyClosePopup();
      urllist.hidden = true;
      bookmark.hidden = true;
    }
  },

  _showPlaces : function(aItems) {
    var list = document.getElementById("urllist-items");
    while (list.firstChild) {
      list.removeChild(list.firstChild);
    }

    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var fis = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);

    for (var i=0; i<aItems.length; i++) {
      let node = aItems[i];
      let listItem = document.createElement("richlistitem");
      listItem.setAttribute("class", "urllist-item");
      listItem.setAttribute("value", node.uri);

      let box = document.createElement("vbox");
      box.setAttribute("pack", "center");
      let image = document.createElement("image");
      image.setAttribute("class", "urllist-image");
      let icon = node.icon ? node.icon.spec : fis.getFaviconImageForPage(ios.newURI(node.uri, null, null)).spec
      image.setAttribute("src", icon);
      box.appendChild(image);
      listItem.appendChild(box);

      let label = document.createElement("label");
      label.setAttribute("class", "urllist-text");
      label.setAttribute("value", node.title);
      label.setAttribute("flex", "1");
      label.setAttribute("crop", "end");
      listItem.appendChild(label);
      list.appendChild(listItem);
      listItem.addEventListener("click", function() { BrowserUI.goToURI(node.uri); }, true);
    }

    list.focus();
  },

  closePopup : function(aEvent)
  {
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE)
      BrowserUI.show(UIMODE_NONE);
  },

  showHistory : function() {
    this._showPlaces(this._getHistory(6));
  },

  showBookmarks : function () {
    this.show(UIMODE_BOOKMARKLIST);

    var bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
    this._showPlaces(this._getBookmarks([bms.bookmarksMenuFolder]));
  },

  newTab : function() {
    Browser.content.newTab(true);
    this.show(UIMODE_URLEDIT);
  },

  selectTab : function(aTab) {
    Browser.content.selectTab(aTab);
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      
      case "DOMTitleChanged":
        this._titleChanged(aEvent.target);
        break;
      case "DOMLinkAdded":
        this._linkAdded(aEvent);
        break;
      case "TabSelect":
        this._tabSelect(aEvent);
        break;
      
      case "click":
        this.show(UIMODE_URLEDIT);
        this.openDefaultHistory();
        break;
      case "input":
        if (this._edit.value) {
          this.updateSearchEngines();
          this._autocompleteNavbuttons.hidden = false;
        }
        break;
      case "keypress":
        if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
          this._edit.reallyClosePopup();
          this.show(UIMODE_URLVIEW);
        }
        break;
      
      case "error":
        this._favicon.setAttribute("src", "chrome://browser/skin/images/default-favicon.png");
        break;
      
      case "mousedown":
        this._dragData.dragging = true;
        this._dragData.dragX = 0;
        this._dragData.dragY = 0;
        this._dragData.screenX = this._dragData.lastX = aEvent.screenX;
        this._dragData.screenY = this._dragData.lastY = aEvent.screenY;
        this._dragData.sTop = document.getElementById("toolbar-main").top;
        this._dragData.sLeft = document.getElementById("tab-list-container").left;
        break;
      case "mouseup":
        this._dragData.dragging = false;
        break;
      case "mousemove":
        this._scrollToolbar(aEvent);
        break;
      
      case "resize":
        this._sizeControls(aEvent);
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
      case "cmd_star":
      case "cmd_bookmarks":
      case "cmd_menu":
      case "cmd_newTab":
      case "cmd_closeTab":
      case "cmd_actions":
      case "cmd_panel":
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
      case "cmd_star":
      {
        var bookmarkURI = browser.currentURI;
        var bookmarkTitle = browser.contentDocument.title;

        var bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
        if (bookmarks.getBookmarkIdsForURI(bookmarkURI, {}).length == 0) {
          var bookmarkId = bookmarks.insertBookmark(bookmarks.bookmarksMenuFolder, bookmarkURI, bookmarks.DEFAULT_INDEX, bookmarkTitle);
          document.getElementById("tool-star").setAttribute("starred", "true");

          var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
          var favicon = document.getElementById("urlbar-favicon");
          var faviconURI = ios.newURI(favicon.src, null, null);

          var fis = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);
          fis.setAndLoadFaviconForPage(bookmarkURI, faviconURI, true);

          this.show(UIMODE_NONE);
        }
        else {
          this.show(UIMODE_BOOKMARK);
          BookmarkHelper.edit(bookmarkURI);
        }
        break;
      }
      case "cmd_bookmarks":
        this.showBookmarks();
        break;
      case "cmd_menu":
        break;
      case "cmd_newTab":
        this.newTab();
        break;
      case "cmd_closeTab":
        Browser.content.removeTab(Browser.content.browser);
        break;
      case "cmd_actions":
        break;
      case "cmd_panel":
      {
        var mode = (this.mode != UIMODE_PANEL ? UIMODE_PANEL : UIMODE_URLVIEW);
        this.show(mode);
        this._showPanel(mode == UIMODE_PANEL);
        break;
      }
    }
  }
};

var BookmarkHelper = {
  _item : null,
  _uri : null,
  _bmksvc : null,
  _tagsvc : null,

  _getTagsArrayFromTagField : function() {
    
    var tags = document.getElementById("bookmark-tags").value.split(",");
    for (var i=0; i<tags.length; i++) {
      
      tags[i] = tags[i].replace(/^\s+/, "").replace(/\s+$/, "");

      
      if (tags[i] == "") {
        tags.splice(i, 1);
        i--;
      }
    }
    return tags;
  },

  edit : function(aURI) {
    this._bmksvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
    this._tagsvc = Cc["@mozilla.org/browser/tagging-service;1"].getService(Ci.nsITaggingService);

    this._uri = aURI;
    var bookmarkIDs = this._bmksvc.getBookmarkIdsForURI(this._uri, {});
    if (bookmarkIDs.length > 0) {
      this._item = bookmarkIDs[0];
      document.getElementById("bookmark-name").value = this._bmksvc.getItemTitle(this._item);
      var currentTags = this._tagsvc.getTagsForURI(this._uri, {});
      document.getElementById("bookmark-tags").value = currentTags.join(", ");
    }

    window.addEventListener("keypress", this, true);
  },

  remove : function() {
    if (this._item) {
      this._bmksvc.removeItem(this._item);
      document.getElementById("tool-star").removeAttribute("starred");
    }
    this.close();
  },

  save : function() {
    if (this._item) {
      
      this._bmksvc.setItemTitle(this._item, document.getElementById("bookmark-name").value);

      
      var tags = this._getTagsArrayFromTagField();
      var currentTags = this._tagsvc.getTagsForURI(this._uri, {});
      if (tags.length > 0 || currentTags.length > 0) {
        var tagsToRemove = [];
        var tagsToAdd = [];
        var i;
        for (i=0; i<currentTags.length; i++) {
          if (tags.indexOf(currentTags[i]) == -1)
            tagsToRemove.push(currentTags[i]);
        }
        for (i=0; i<tags.length; i++) {
          if (currentTags.indexOf(tags[i]) == -1)
            tagsToAdd.push(tags[i]);
        }

        if (tagsToAdd.length > 0)
          this._tagsvc.tagURI(this._uri, tagsToAdd);
        if (tagsToRemove.length > 0)
          this._tagsvc.untagURI(this._uri, tagsToRemove);
      }

    }
    this.close();
  },

  close : function() {
    window.removeEventListener("keypress", this, true);
    this._item = null;
    BrowserUI.show(UIMODE_NONE);
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "keypress":
        if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE)
          this.close();
        break;
    }
  }
};
