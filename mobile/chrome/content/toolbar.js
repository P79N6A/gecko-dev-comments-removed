




































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
    var browser = Browser.content.browser;

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
        this.search();
        break;
      }
      case "cmd_go":
      {
        this.goToURI();
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
    }
  }
};
