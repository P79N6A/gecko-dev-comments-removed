






































const Cc = Components.classes;
const Ci = Components.interfaces;

const TOOLBARSTATE_LOADING        = 1;
const TOOLBARSTATE_LOADED         = 2;
const TOOLBARSTATE_TYPING         = 3;
const TOOLBARSTATE_INDETERMINATE  = 4;

var LocationBar = {
  _urlbar : null,
  _throbber : null,
  _favicon : null,
  _faviconAdded : false,

  _linkAdded : function(aEvent) {
    var link = aEvent.originalTarget;
    var rel = link.rel && link.rel.toLowerCase();
    if (!link || !link.ownerDocument || !rel || !link.href)
      return;

    var rels = rel.split(/\s+/);
    if (rels.indexOf("icon") != -1) {
      this._favicon.setAttribute("src", link.href);
      this._throbber.setAttribute("src", "");
      this._faviconAdded = true;
    }
  },

  init : function() {
    this._urlbar = document.getElementById("urlbar");
    this._urlbar.addEventListener("focus", this, false);
    this._urlbar.addEventListener("input", this, false);

    this._throbber = document.getElementById("urlbar-throbber");
    this._favicon = document.getElementById("urlbar-favicon");
    this._favicon.addEventListener("error", this, false);

    Browser.content.addEventListener("DOMLinkAdded", this, true);
  },

  update : function(aState) {
    var go = document.getElementById("cmd_go");
    var search = document.getElementById("cmd_search");
    var reload = document.getElementById("cmd_reload");
    var stop = document.getElementById("cmd_stop");

    if (aState == TOOLBARSTATE_INDETERMINATE) {
      this._faviconAdded = false;
      aState = TOOLBARSTATE_LOADED;
      this.setURI();
    }

    if (aState == TOOLBARSTATE_LOADING) {
      go.collapsed = true;
      search.collapsed = true;
      reload.collapsed = true;
      stop.collapsed = false;

      this._throbber.setAttribute("src", "chrome://browser/skin/images/throbber.gif");
      this._favicon.setAttribute("src", "");
      this._faviconAdded = false;
    }
    else if (aState == TOOLBARSTATE_LOADED) {
      go.collapsed = true;
      search.collapsed = true;
      reload.collapsed = false;
      stop.collapsed = true;

      this._throbber.setAttribute("src", "");
      if (this._faviconAdded == false) {
        var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
        var faviconURI = ios.newURI(Browser.content.browser.currentURI.prePath + "/favicon.ico", null, null);

        var fis = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);
        if (fis.isFailedFavicon(faviconURI))
          faviconURI = ios.newURI("chrome://browser/skin/images/default-favicon.png", null, null);

        this._favicon.setAttribute("src", faviconURI.spec);
        this._faviconAdded = true;
      }
    }
    else if (aState == TOOLBARSTATE_TYPING) {
      reload.collapsed = true;
      stop.collapsed = true;
      go.collapsed = false;
      search.collapsed = false;

      this._throbber.setAttribute("src", "chrome://browser/skin/images/throbber.png");
      this._favicon.setAttribute("src", "");
    }
  },

  
  setURI : function() {
    
    var browser = Browser.content.browser;
    var back = document.getElementById("cmd_back");
    var forward = document.getElementById("cmd_forward");

    back.setAttribute("disabled", !browser.canGoBack);
    forward.setAttribute("disabled", !browser.canGoForward);

    
    var star = document.getElementById("tool_star");
    star.removeAttribute("starred");
    var bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
    var bookmarks = bms.getBookmarkIdsForURI(browser.currentURI, {});
    if (bookmarks.length > 0) {
      star.setAttribute("starred", "true");
    }

    var uri = browser.currentURI.spec;
    if (uri == "about:blank") {
      uri = "";
      this._urlbar.focus();
    }

    this._urlbar.value = uri;
  },

  revertURI : function() {
    
    if (this._urlbar.popupOpen == false)
      this.setURI();

    
    if (this._urlbar.value && this._urlbar.hasAttribute("focused"))
      this._urlbar.select();

    return (this._urlbar.popupOpen == false);
  },

  goToURI : function() {
    Browser.content.loadURI(this._urlbar.value, null, null, false);
  },

  search : function() {
    var queryURI = "http://www.google.com/search?q=" + this._urlbar.value + "&hl=en&lr=&btnG=Search";
    Browser.content.loadURI(queryURI, null, null, false);
  },

  getURLBar : function() {
    return this._urlbar;
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "DOMLinkAdded":
        this._linkAdded(aEvent);
        break;
      case "focus":
        setTimeout(function() { aEvent.target.select(); }, 0);
        break;
      case "input":
        this.update(TOOLBARSTATE_TYPING);
        break;
      case "error":
        this._favicon.setAttribute("src", "chrome://browser/skin/images/default-favicon.png");
        break;
    }
  }
};


var Bookmarks = {
  bookmarks : null,
  panel : null,
  item : null,

  edit : function(aURI) {
    this.bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
    this.panel = document.getElementById("bookmark_edit");
    this.panel.hidden = false; 

    var bookmarkIDs = this.bookmarks.getBookmarkIdsForURI(aURI, {});
    if (bookmarkIDs.length > 0) {
      this.item = bookmarkIDs[0];
      document.getElementById("bookmark_url").value = aURI.spec;
      document.getElementById("bookmark_name").value = this.bookmarks.getItemTitle(this.item);

      this.panel.openPopup(document.getElementById("tool_star"), "after_end", 0, 0, false, false);
    }
  },

  remove : function() {
    if (this.item) {
      this.bookmarks.removeItem(this.item);
      document.getElementById("tool_star").removeAttribute("starred");
    }
    this.close();
  },

  save : function() {
    if (this.panel && this.item) {
      var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
      var bookmarkURI = ios.newURI(document.getElementById("bookmark_url").value, null, null);
      if (bookmarkURI) {
        this.bookmarks.setItemTitle(this.item, document.getElementById("bookmark_name").value);
        this.bookmarks.changeBookmarkURI(this.item, bookmarkURI);
      }
    }
    this.close();
  },

  close : function() {
    if (this.panel) {
      this.item = null;
      this.panel.hidePopup();
      this.panel = null;
    }
  },

  list : function() {
    this.bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
    this.panel = document.getElementById("bookmark_picker");
    this.panel.hidden = false; 

    var list = document.getElementById("bookmark_list");
    while (list.childNodes.length > 0) {
      list.removeChild(list.childNodes[0]);
    }

    var fis = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);

    var items = this.getBookmarks();
    if (items.length > 0) {
      for (var i=0; i<items.length; i++) {
        var itemId = items[i];
        var listItem = document.createElement("richlistitem");
        listItem.setAttribute("class", "bookmarklist-item");

        var box = document.createElement("vbox");
        box.setAttribute("pack", "center");
        var image = document.createElement("image");
        image.setAttribute("class", "bookmarklist-image");
        image.setAttribute("src", fis.getFaviconImageForPage(this.bookmarks.getBookmarkURI(itemId)).spec);
        box.appendChild(image);
        listItem.appendChild(box);

        var label = document.createElement("label");
        label.setAttribute("class", "bookmarklist-text");
        label.setAttribute("value", this.bookmarks.getItemTitle(itemId));
        label.setAttribute("flex", "1");
        label.setAttribute("crop", "end");
        label.setAttribute("onclick", "Bookmarks.open(" + itemId + ");");
        listItem.appendChild(label);
        list.appendChild(listItem);
      }
      this.panel.openPopup(document.getElementById("tool_bookmarks"), "after_end", 0, 0, false, false);
    }
  },

  open : function(aItem) {
    var bookmarkURI = this.bookmarks.getBookmarkURI(aItem);
    Browser.content.loadURI(bookmarkURI.spec, null, null, false);
    this.close();
  },

  getBookmarks : function() {
    var items = [];

    var history = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
    var options = history.getNewQueryOptions();
    var query = history.getNewQuery();
    query.setFolders([this.bookmarks.bookmarksMenuFolder], 1);
    var result = history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    for (var i=0; i<cc; ++i) {
      var node = rootNode.getChild(i);
      if (node.type == node.RESULT_TYPE_URI) {
        items.push(node.itemId);
      }
    }
    rootNode.containerOpen = false;

    return items;
  }
};


var Browser = {
  _content : null,

  _titleChanged : function(aEvent) {
    if (aEvent.target != this.content.browser.contentDocument)
      return;

      document.title = "Firefox - " + aEvent.target.title;
  },

  _tabOpen : function(aEvent) {
    aEvent.originalTarget.zoomController = new ZoomController(aEvent.originalTarget);
    aEvent.originalTarget.mouseController = new MouseController(aEvent.originalTarget);
    aEvent.originalTarget.progressController = new ProgressController(aEvent.originalTarget);
  },

  _tabClose : function(aEvent) {
  },

  _tabSelect : function(aEvent) {
    LocationBar.update(TOOLBARSTATE_INDETERMINATE);
  },

  _popupShowing : function(aEvent) {
    var target = document.popupNode;
    var isContentSelected = !document.commandDispatcher.focusedWindow.getSelection().isCollapsed;
    var isTextField = target instanceof HTMLTextAreaElement;
    if (target instanceof HTMLInputElement && (target.type == "text" || target.type == "password"))
      isTextField = true;
    var isTextSelected= (isTextField && target.selectionStart != target.selectionEnd);

    





















    InlineSpellCheckerUI.clearSuggestionsFromMenu();
    InlineSpellCheckerUI.uninit();

    var separator = document.getElementById("menusep_spellcheck");
    separator.hidden = true;
    var addToDictionary = document.getElementById("menuitem_addToDictionary");
    addToDictionary.hidden = true;
    var noSuggestions = document.getElementById("menuitem_noSuggestions");
    noSuggestions.hidden = true;

    
    var win = target.ownerDocument.defaultView;
    if (win) {
      var isEditable = false;
      try {
        var editingSession = win.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIWebNavigation)
                                .QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIEditingSession);
        isEditable = editingSession.windowIsEditable(win);
      }
      catch(ex) {
        
      }
    }

    var editor = null;
    if (isTextField && !target.readOnly)
      editor = target.QueryInterface(Ci.nsIDOMNSEditableElement).editor;

    if (isEditable)
      editor = editingSession.getEditorForWindow(win);
dump("ready\n");
    if (editor) {
dump("editor\n");
dump("anchor="+editor.selection.anchorNode+"\n");
dump("offset="+editor.selection.anchorOffset+"\n");
dump(editor.selectionController.getSelection(Ci.nsISelectionController.SELECTION_SPELLCHECK).rangeCount);
      InlineSpellCheckerUI.init(editor);
dump(InlineSpellCheckerUI.canSpellCheck);

      InlineSpellCheckerUI.initFromEvent(editor.selection.anchorNode, editor.selection.anchorOffset);

      var onMisspelling = InlineSpellCheckerUI.overMisspelling;
      if (onMisspelling) {
dump("misspelling\n");
        separator.hidden = false;
        addToDictionary.hidden = false;
        var menu = document.getElementById("popup_content");
        var suggestions = InlineSpellCheckerUI.addSuggestionsToMenu(menu, addToDictionary, 5);
        noSuggestions.hidden = (suggestions > 0);
      }
    }
  },

  startup : function() {
    this.prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch2);

    window.controllers.appendController(this);

    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var styleSheets = Cc["@mozilla.org/content/style-sheet-service;1"].getService(Ci.nsIStyleSheetService);

    
    var hideCursor = this.prefs.getBoolPref("browser.ui.cursor") == false;
    if (hideCursor) {
      window.QueryInterface(Ci.nsIDOMChromeWindow).setCursor("none");

      var styleURI = ios.newURI("chrome://browser/content/content.css", null, null);
      styleSheets.loadAndRegisterSheet(styleURI, styleSheets.AGENT_SHEET);
    }

    
    var styleURI = ios.newURI("chrome://browser/content/scrollbars.css", null, null);
    styleSheets.loadAndRegisterSheet(styleURI, styleSheets.AGENT_SHEET);

    this._content = document.getElementById("content");
    this._content.addEventListener("DOMTitleChanged", this, true);
    this._content.addEventListener("TabOpen", this, true);
    this._content.addEventListener("TabClose", this, true);
    this._content.addEventListener("TabSelect", this, true);
    document.getElementById("popup_content").addEventListener("popupshowing", this, false);

    this._content.addBrowser("about:blank", null, null, false);

    LocationBar.init();
    Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);

    
    var whereURI = null;
    try {
      
      whereURI = this.prefs.getCharPref("browser.startup.homepage");
    }
    catch (e) {
    }

    
    if (window.arguments && window.arguments[0]) {
      var cmdLine = window.arguments[0].QueryInterface(Ci.nsICommandLine);
      if (cmdLine.length == 1) {
        whereURI = cmdLine.resolveURI(cmdLine.getArgument(0));
        if (whereURI)
          whereURI = whereURI.spec;
      }
    }

    if (whereURI) {
      var self = this;
      setTimeout(function() { self.content.browser.loadURI(whereURI, null, null, false); }, 10);
    }
  },

  get content() {
    return this._content;
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "DOMTitleChanged":
        this._titleChanged(aEvent);
        break;
      case "TabOpen":
        this._tabOpen(aEvent);
        break;
      case "TabClose":
        this._tabClose(aEvent);
        break;
      case "TabSelect":
        this._tabSelect(aEvent);
        break;
      case "popupshowing":
        this._popupShowing(aEvent);
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
      case "cmd_newTab":
      case "cmd_closeTab":
      case "cmd_switchTab":
      case "cmd_menu":
      case "cmd_fullscreen":
      case "cmd_addons":
      case "cmd_downloads":
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
    var browser = this.content.browser;

    switch (cmd) {
      case "cmd_back":
        browser.stop();
        browser.goBack();
        break;
      case "cmd_forward":
        browser.stop();
        browser.goForward();
        break;
      case "cmd_reload":
        browser.reload();
        break;
      case "cmd_stop":
        browser.stop();
        break;
      case "cmd_search":
      {
        LocationBar.search();
        break;
      }
      case "cmd_go":
      {
        LocationBar.goToURI();
        break;
      }
      case "cmd_star":
      {
        var bookmarkURI = browser.currentURI;
        var bookmarkTitle = browser.contentDocument.title;

        var bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
        if (bookmarks.getBookmarkIdsForURI(bookmarkURI, {}).length == 0) {
          var bookmarkId = bookmarks.insertBookmark(bookmarks.bookmarksMenuFolder, bookmarkURI, bookmarks.DEFAULT_INDEX, bookmarkTitle);
          document.getElementById("tool_star").setAttribute("starred", "true");

          var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
          var favicon = document.getElementById("urlbar-favicon");
          var faviconURI = ios.newURI(favicon.src, null, null);

          var fis = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);
          fis.setAndLoadFaviconForPage(bookmarkURI, faviconURI, true);
        }
        else {
          Bookmarks.edit(bookmarkURI);
        }
        break;
      }
      case "cmd_bookmarks":
        Bookmarks.list();
        break;
      case "cmd_newTab":
        this.content.addBrowser("about:blank", null, null, false);
        break;
      case "cmd_closeTab":
        this.content.removeBrowser();
        break;
      case "cmd_switchTab":
        this.content.select();
        break;
      case "cmd_menu":
      {





        var menu = document.getElementById("mainmenu");
        menu.openPopup(window.screenX, window.screenY, true);
        break;
      }
      case "cmd_fullscreen":
        window.fullScreen = window.fullScreen ? false : true;
        break;
      case "cmd_addons":
      {
        const EMTYPE = "Extension:Manager";

        var aOpenMode = "extensions";
        var wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
        var needToOpen = true;
        var windowType = EMTYPE + "-" + aOpenMode;
        var windows = wm.getEnumerator(windowType);
        while (windows.hasMoreElements()) {
          var theEM = windows.getNext().QueryInterface(Ci.nsIDOMWindowInternal);
          if (theEM.document.documentElement.getAttribute("windowtype") == windowType) {
            theEM.focus();
            needToOpen = false;
            break;
          }
        }

        if (needToOpen) {
          const EMURL = "chrome://mozapps/content/extensions/extensions.xul?type=" + aOpenMode;
          const EMFEATURES = "chrome,dialog=no,resizable=yes";
          window.openDialog(EMURL, "", EMFEATURES);
        }
        break;
      }
      case "cmd_downloads":
        Cc["@mozilla.org/download-manager-ui;1"].getService(Ci.nsIDownloadManagerUI).show(window);
    }
  }
};


function ProgressController(aBrowser) {
  this.init(aBrowser);
}

ProgressController.prototype = {
  _browser : null,

  init : function(aBrowser) {
    this._browser = aBrowser;
    this._browser.addProgressListener(this, Components.interfaces.nsIWebProgress.NOTIFY_ALL);
  },

  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
      if (aRequest && aWebProgress.DOMWindow == this._browser.contentWindow) {
        if (aStateFlags & Ci.nsIWebProgressListener.STATE_START) {
          LocationBar.update(TOOLBARSTATE_LOADING);
        }
        else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
          this._browser.zoomController.scale = 1;
          LocationBar.update(TOOLBARSTATE_LOADED);
        }
      }
    }

    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
        aWebProgress.DOMWindow.focus();
        
      }
    }
  },

  
  
  onProgressChange : function(aWebProgress, aRequest, aCurSelf, aMaxSelf, aCurTotal, aMaxTotal) {
  },

  
  onLocationChange : function(aWebProgress, aRequest, aLocation) {
    if (aWebProgress.DOMWindow == this._browser.contentWindow) {
      LocationBar.setURI(aLocation.spec);
    }
  },

  
  
  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage) {
  },

  
  onSecurityChange : function(aWebProgress, aRequest, aState) {
  },

  
  QueryInterface : function(aIID) {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};










var MouseController = function(browser) {
  this.init(browser);
}

MouseController.prototype = {
  _browser: null,
  _contextID : null,

  init: function(aBrowser)
  {
    this._browser = aBrowser;
    this._browser.addEventListener("mousedown", this, true);
    this._browser.addEventListener("mouseup",this, true);
  },

  handleEvent: function(e)
  {
    if (! e.type in this)
      dump("MouseController called with unknown event type " + e.type + "\n");
    this[e.type](e);
  },

  mousedown: function(aEvent)
  {
    var self = this;
    this._contextID = setTimeout(function() { self.contextMenu(aEvent); }, 900);

    if (aEvent.target instanceof HTMLInputElement ||
        aEvent.target instanceof HTMLTextAreaElement ||
        aEvent.target instanceof HTMLSelectElement)
      return;

    
    if (this.firstEvent &&
        (aEvent.timeStamp - this.firstEvent.timeStamp) < 400 &&
        Math.abs(aEvent.clientX - this.firstEvent.clientX) < 30 &&
        Math.abs(aEvent.clientY - this.firstEvent.clientY) < 30) {
      this.dblclick(aEvent);
      return;
    }

    this.lastEvent = this.firstEvent = aEvent;
    this.fingerDistance = 100;
    this.mousemove = aEvent.button != 2 ? this.mousePan : this.mouseZoom;
    this._browser.addEventListener("mousemove", this, true);

    

    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  mouseup: function(aEvent)
  {
    this._browser.removeEventListener("mousemove", this, true);
    if (this._contextID) {
      clearTimeout(this._contextID);
      this._contextID = null;
    }

    

    
    var totalDistance = Math.sqrt(
        Math.pow(this.firstEvent.clientX - aEvent.clientX, 2) +
        Math.pow(this.firstEvent.clientY - aEvent.clientY, 2));
    if (totalDistance > 10)
      aEvent.preventDefault();

    
    


























  },

  mouseZoom: function(e)
  {
    var deltaX = e.screenX - this.firstEvent.screenX + 100;
    var deltaY = e.screenY - this.firstEvent.screenY;
    var newDist = Math.sqrt(Math.pow(deltaX, 2) + Math.pow(deltaY, 2));
    var scale = newDist / this.fingerDistance;
    if (e.screenX < this.firstEvent.screenX && scale > 1)
      scale = 1 / scale;
    var newZoom = scale * this._browser.markupDocumentViewer.fullZoom;
    this.fingerDistance = Math.max(0.1, newDist);
    this._browser.zoomController.scale = newZoom;
    this.lastEvent = e;

    
    e.stopPropagation();
    e.preventDefault();
  },

  mousePan: function(aEvent)
  {
    var x = aEvent.clientX - this.lastEvent.clientX;
    var y = aEvent.clientY - this.lastEvent.clientY;
    if (Math.abs(x) < 5 && Math.abs(y) < 5)
      return;

    if (this._contextID) {
      clearTimeout(this._contextID);
      this._contextID = null;
    }

    if (this.lastEvent) {
      aEvent.momentum = {
        time: Math.max(aEvent.timeStamp - this.lastEvent.timeStamp, 1),
        x: x,
        y: y
      };
    }
    this._browser.contentWindow.scrollBy(-x, -y);
    this.lastEvent = aEvent;

    

    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  dblclick: function(aEvent)
  {
    
    var target = aEvent.target;
    aEvent.preventDefault();
    while (target && target.nodeName != "HTML") {
      var disp = window.getComputedStyle(target, "").getPropertyValue("display");
      if (!disp.match(/(inline)/g)) {
        this._browser.zoomController.toggleZoom(target);
        break;
      }
      else {
        target = target.parentNode;
      }
    }
    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  contextMenu: function(aEvent)
  {
    if (this._contextID && this._browser.contextMenu) {
      document.popupNode = aEvent.target;
      var popup = document.getElementById(this._browser.contextMenu);
      popup.openPopup(this._browser, "", aEvent.clientX, aEvent.clientY, true, false);

      this._browser.removeEventListener("mousemove", this, true);
      this._contextID = null;

      aEvent.stopPropagation();
      aEvent.preventDefault();
    }
  },

  drag : function(aEvent){
    aEvent.stopPropagation();
    aEvent.preventDefault();
    return true;
  },

  dragstart : function(aEvent){
    return this.drag(aEvent);
  },

  draggesture : function(aEvent){
    return this.drag(aEvent);
  }
}


function ZoomController(aBrowser) {
  this._browser = aBrowser;
};


ZoomController.prototype = {
  _minScale : 0.1,
  _maxScale : 3,
  _target : null,

  set scale(s)
  {
    var clamp = Math.min(this._maxScale, Math.max(this._minScale, s));
    clamp = Math.floor(clamp * 1000) / 1000;  
    if (clamp == this._browser.markupDocumentViewer.fullZoom)
      return;

    this._browser.markupDocumentViewer.fullZoom = clamp;

    
    var leftEdge = this._browser.contentWindow.scrollX + this._browser.contentWindow.document.documentElement.clientWidth;
    var scrollX = this._browser.contentWindow.document.documentElement.scrollWidth - leftEdge;
    if (scrollX < 0)
      this._browser.contentWindow.scrollBy(scrollX, 0);
  },

  get scale()
  {
    return this._browser.markupDocumentViewer.fullZoom;
  },

  reset: function()
  {
    this._minScale = ZoomController.prototype._minScale;
    this._maxScale = ZoomController.prototype._maxScale;
  },

  fitContent: function()
  {
    this._target = null;
    try {
      var oldScale = this.scale;
      this.scale = 1;    
      var body = this._browser.contentWindow.document.body;
      var html = this._browser.contentWindow.document.documentElement;
      var newScale = this.scale;
      var finalWidth = html.clientWidth;
    }
    catch(e) {
      dump(e + "\n");
      return;
    }

    var prefScrollWidth = Math.max(html.scrollWidth, body.scrollWidth); 
    if (prefScrollWidth > (this._browser.boxObject.width - 10) )  {
      
      
      newScale = (this._browser.boxObject.width ) / prefScrollWidth;
      finalWidth = prefScrollWidth;
    }
    body.style.minWidth = body.style.maxWidth = (finalWidth -20) + "px";
    this._minScale = Math.max(this._minScale, newScale);
    this.scale = newScale;
  },

  getPagePosition: function (el)
  {
    var r = el.getBoundingClientRect();
    retVal = {
      width: r.right - r.left,
      height: r.bottom - r.top,
      x: r.left + this._browser.contentWindow.scrollX,
      y: r.top + this._browser.contentWindow.scrollY
    };
    return retVal;
  },

  getWindowRect: function()
  {
    return {
      x: this._browser.contentWindow.scrollX,
      y: this._browser.contentWindow.scrollY,
      width: this._browser.boxObject.width / this.scale,
      height: this._browser.boxObject.height / this.scale
    };
  },

  toggleZoom: function(el)
  {
    if (!el) return;

    if (this.scale == 1 || el != this._target) {
      this.zoomToElement(el);
      this._target = el;
    }
    else {
      this.scale = 1;
      this._target = null;
    }
  },

  zoomToElement: function(el)
  {
    var margin = 8;

    
    var elRect = this.getPagePosition(el);
    this.scale = (this._browser.boxObject.width) / (elRect.width + 2 * margin);

    
    elRect = this.getPagePosition(el);
    winRect = this.getWindowRect();
    this._browser.contentWindow.scrollTo(Math.max(elRect.x - margin, 0), Math.max(0, elRect.y - margin));
  }
};
