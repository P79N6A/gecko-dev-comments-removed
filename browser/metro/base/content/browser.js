




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;

Cu.import("resource://gre/modules/PageThumbs.jsm");


const kStartURI = "about:newtab";


const kTouchTimeout = 300;
const kSetInactiveStateTimeout = 100;

const kTabThumbnailDelayCapture = 500;

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";


var _richgridTileSizes = {};


window.sizeToContent = function() {
  Cu.reportError("window.sizeToContent is not allowed in this window");
}

function getTabModalPromptBox(aWindow) {
  let browser = Browser.getBrowserForWindow(aWindow);
  return Browser.getTabModalPromptBox(browser);
}




function getBrowser() {
  return Browser.selectedBrowser;
}

var Browser = {
  _debugEvents: false,
  _tabs: [],
  _selectedTab: null,
  _tabId: 0,
  windowUtils: window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils),

  get defaultBrowserWidth() {
    return window.innerWidth;
  },

  startup: function startup() {
    var self = this;

    try {
      messageManager.loadFrameScript("chrome://browser/content/Util.js", true);
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/Content.js", true);
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/FormHelper.js", true);
      messageManager.loadFrameScript("chrome://browser/content/library/SelectionPrototype.js", true);
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/SelectionHandler.js", true);
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/ContextMenuHandler.js", true);
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/ConsoleAPIObserver.js", true);
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/PluginHelper.js", true);
    } catch (e) {
      
      dump("###########" + e + "\n");
    }

    if (!Services.metro) {
      
      
      Services.metro = {
        activationURI: "",
        pinTileAsync: function () {},
        unpinTileAsync: function () {}
      };
    }

    
    Elements.browsers.customDragger = new Browser.MainDragger();

    
    Elements.browsers.webProgress = WebProgress.init();

    
    
    InputSourceHelper.init();
    ClickEventHandler.init();

    TouchModule.init();
    GestureModule.init();
    BrowserTouchHandler.init();
    PopupBlockerObserver.init();
    APZCObserver.init();

    
    this.contentScrollbox = Elements.browsers;
    this.contentScrollboxScroller = {
      scrollBy: function(aDx, aDy) {
        let view = getBrowser().getRootView();
        view.scrollBy(aDx, aDy);
      },

      scrollTo: function(aX, aY) {
        let view = getBrowser().getRootView();
        view.scrollTo(aX, aY);
      },

      getPosition: function(aScrollX, aScrollY) {
        let view = getBrowser().getRootView();
        let scroll = view.getPosition();
        aScrollX.value = scroll.x;
        aScrollY.value = scroll.y;
      }
    };

    ContentAreaObserver.init();

    function fullscreenHandler() {
      if (Browser.selectedBrowser.contentWindow.document.mozFullScreenElement)
        Elements.stack.setAttribute("fullscreen", "true");
      else
        Elements.stack.removeAttribute("fullscreen");
    }
    window.addEventListener("mozfullscreenchange", fullscreenHandler, true);

    BrowserUI.init();

    window.controllers.appendController(this);
    window.controllers.appendController(BrowserUI);

    Services.obs.addObserver(SessionHistoryObserver, "browser:purge-session-history", false);

    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = new nsBrowserAccess();

    Elements.browsers.addEventListener("DOMUpdatePageReport", PopupBlockerObserver.onUpdatePageReport, false);

    
    Util.forceOnline();

    
    
    let commandURL = null;
    try {
      let argsObj = window.arguments[0].wrappedJSObject;
      if (argsObj && argsObj.pageloadURL) {
        
        commandURL = argsObj.pageloadURL;
      } else if (window.arguments && window.arguments[0]) {
        
        commandURL = window.arguments[0];
      }
    } catch (ex) {
      Util.dumpLn(ex);
    }

    messageManager.addMessageListener("DOMLinkAdded", this);
    messageManager.addMessageListener("Browser:FormSubmit", this);
    messageManager.addMessageListener("Browser:CanUnload:Return", this);
    messageManager.addMessageListener("scroll", this);
    messageManager.addMessageListener("Browser:CertException", this);
    messageManager.addMessageListener("Browser:BlockedSite", this);

    Task.spawn(function() {
      
      let activationURI = yield this.getShortcutOrURI(Services.metro.activationURI);

      let self = this;
      function loadStartupURI() {
        if (activationURI) {
          let webNav = Ci.nsIWebNavigation;
          let flags = webNav.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP |
                      webNav.LOAD_FLAGS_FIXUP_SCHEME_TYPOS;
          self.addTab(activationURI, true, null, { flags: flags });
        } else {
          let uri = commandURL || Browser.getHomePage();
          self.addTab(uri, true);
        }
      }

      
      let ss = Cc["@mozilla.org/browser/sessionstore;1"]
               .getService(Ci.nsISessionStore);
      let shouldRestore = ss.shouldRestore();
      if (shouldRestore) {
        let bringFront = false;
        
        if (activationURI && activationURI != kStartURI) {
          this.addTab(activationURI, true, null, { flags: Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP });
        } else if (commandURL && commandURL != kStartURI) {
          this.addTab(commandURL, true);
        } else {
          bringFront = true;
          
          
          
          let dummy = this.addTab("about:blank", true);
          let dummyCleanup = {
            observe: function(aSubject, aTopic, aData) {
              Services.obs.removeObserver(dummyCleanup, "sessionstore-windows-restored");
              if (aData == "fail")
                loadStartupURI();
              dummy.chromeTab.ignoreUndo = true;
              Browser.closeTab(dummy, { forceClose: true });
            }
          };
          Services.obs.addObserver(dummyCleanup, "sessionstore-windows-restored", false);
        }
        ss.restoreLastSession(bringFront);
      } else {
        loadStartupURI();
      }

      
      InputSourceHelper.fireUpdate();

      
      let event = document.createEvent("Events");
      event.initEvent("UIReady", true, false);
      window.dispatchEvent(event);
    }.bind(this));
  },

  shutdown: function shutdown() {
    APZCObserver.shutdown();
    BrowserUI.uninit();
    ClickEventHandler.uninit();
    ContentAreaObserver.shutdown();
    Appbar.shutdown();

    messageManager.removeMessageListener("Browser:FormSubmit", this);
    messageManager.removeMessageListener("scroll", this);
    messageManager.removeMessageListener("Browser:CertException", this);
    messageManager.removeMessageListener("Browser:BlockedSite", this);

    Services.obs.removeObserver(SessionHistoryObserver, "browser:purge-session-history");

    window.controllers.removeController(this);
    window.controllers.removeController(BrowserUI);
  },

  getHomePage: function getHomePage(aOptions) {
    aOptions = aOptions || { useDefault: false };

    let url = kStartURI;
    try {
      let prefs = aOptions.useDefault ? Services.prefs.getDefaultBranch(null) : Services.prefs;
      url = prefs.getComplexValue("browser.startup.homepage", Ci.nsIPrefLocalizedString).data;
    }
    catch(e) { }

    return url;
  },

  get browsers() {
    return this._tabs.map(function(tab) { return tab.browser; });
  },

  





  loadURI: function loadURI(aURI, aParams) {
    let browser = this.selectedBrowser;

    
    
    dump("loadURI=" + aURI + "\ncurrentURI=" + browser.currentURI.spec + "\n");

    let params = aParams || {};
    try {
      let flags = params.flags || Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
      let postData = ("postData" in params && params.postData) ? params.postData.value : null;
      let referrerURI = "referrerURI" in params ? params.referrerURI : null;
      let charset = "charset" in params ? params.charset : null;
      dump("loading tab: " + aURI + "\n");
      browser.loadURIWithFlags(aURI, flags, referrerURI, charset, postData);
    } catch(e) {
      dump("Error: " + e + "\n");
    }
  },

  






  getShortcutOrURI: function getShortcutOrURI(aURL, aPostDataRef) {
    return Task.spawn(function() {
      if (!aURL)
        throw new Task.Result(aURL);

      let shortcutURL = null;
      let keyword = aURL;
      let param = "";

      let offset = aURL.indexOf(" ");
      if (offset > 0) {
        keyword = aURL.substr(0, offset);
        param = aURL.substr(offset + 1);
      }

      if (!aPostDataRef)
        aPostDataRef = {};

      let engine = Services.search.getEngineByAlias(keyword);
      if (engine) {
        let submission = engine.getSubmission(param);
        aPostDataRef.value = submission.postData;
        throw new Task.Result(submission.uri.spec);
      }

      try {
        [shortcutURL, aPostDataRef.value] = PlacesUtils.getURLAndPostDataForKeyword(keyword);
      } catch (e) {}

      if (!shortcutURL)
        throw new Task.Result(aURL);

      let postData = "";
      if (aPostDataRef.value)
        postData = unescape(aPostDataRef.value);

      if (/%s/i.test(shortcutURL) || /%s/i.test(postData)) {
        let charset = "";
        const re = /^(.*)\&mozcharset=([a-zA-Z][_\-a-zA-Z0-9]+)\s*$/;
        let matches = shortcutURL.match(re);
        if (matches)
          [, shortcutURL, charset] = matches;
        else {
          
          try {
            
            
            charset = yield PlacesUtils.getCharsetForURI(Util.makeURI(shortcutURL));
          } catch (e) { dump("--- error " + e + "\n"); }
        }

        let encodedParam = "";
        if (charset)
          encodedParam = escape(convertFromUnicode(charset, param));
        else 
          encodedParam = encodeURIComponent(param);

        shortcutURL = shortcutURL.replace(/%s/g, encodedParam).replace(/%S/g, param);

        if (/%s/i.test(postData)) 
          aPostDataRef.value = getPostDataStream(postData, param, encodedParam, "application/x-www-form-urlencoded");
      } else if (param) {
        
        
        aPostDataRef.value = null;

        throw new Task.Result(aURL);
      }

      throw new Task.Result(shortcutURL);
    });
  },

  


  get selectedBrowser() {
    return (this._selectedTab && this._selectedTab.browser);
  },

  get tabs() {
    return this._tabs;
  },

  getTabModalPromptBox: function(aBrowser) {
    let browser = (aBrowser || getBrowser());
    let stack = browser.parentNode;
    let self = this;

    let promptBox = {
      appendPrompt : function(args, onCloseCallback) {
          let newPrompt = document.createElementNS(XUL_NS, "tabmodalprompt");
          newPrompt.setAttribute("promptType", args.promptType);
          stack.appendChild(newPrompt);
          browser.setAttribute("tabmodalPromptShowing", true);
          newPrompt.clientTop; 

          let tab = self.getTabForBrowser(browser);
          tab = tab.chromeTab;

          newPrompt.metroInit(args, tab, onCloseCallback);
          return newPrompt;
      },

      removePrompt : function(aPrompt) {
          stack.removeChild(aPrompt);

          let prompts = this.listPrompts();
          if (prompts.length) {
          let prompt = prompts[prompts.length - 1];
              prompt.Dialog.setDefaultFocus();
          } else {
              browser.removeAttribute("tabmodalPromptShowing");
              browser.focus();
          }
      },

      listPrompts : function(aPrompt) {
          let els = stack.getElementsByTagNameNS(XUL_NS, "tabmodalprompt");
          
          let prompts = Array.slice(els);
          return prompts;
      },
    };

    return promptBox;
  },

  getBrowserForWindowId: function getBrowserForWindowId(aWindowId) {
    for (let i = 0; i < this.browsers.length; i++) {
      if (this.browsers[i].contentWindowId == aWindowId)
        return this.browsers[i];
    }
    return null;
  },

  getBrowserForWindow: function(aWindow) {
    let windowID = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
    return this.getBrowserForWindowId(windowID);
  },

  getTabForBrowser: function getTabForBrowser(aBrowser) {
    let tabs = this._tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser == aBrowser)
        return tabs[i];
    }
    return null;
  },

  getTabAtIndex: function getTabAtIndex(index) {
    if (index >= this._tabs.length || index < 0)
      return null;
    return this._tabs[index];
  },

  getTabFromChrome: function getTabFromChrome(chromeTab) {
    for (var t = 0; t < this._tabs.length; t++) {
      if (this._tabs[t].chromeTab == chromeTab)
        return this._tabs[t];
    }
    return null;
  },

  createTabId: function createTabId() {
    return this._tabId++;
  },

  















  addTab: function browser_addTab(aURI, aBringFront, aOwner, aParams) {
    let params = aParams || {};

    if (aOwner && !('index' in params)) {
      
      params.index = this._tabs.indexOf(aOwner) + 1;
      
      while (this._tabs[params.index] && this._tabs[params.index].owner == aOwner) {
        params.index++;
      }
    }

    let newTab = new Tab(aURI, params, aOwner);

    if (params.index >= 0) {
      this._tabs.splice(params.index, 0, newTab);
    } else {
      this._tabs.push(newTab);
    }

    if (aBringFront)
      this.selectedTab = newTab;

    this._announceNewTab(newTab);
    return newTab;
  },

  closeTab: function closeTab(aTab, aOptions) {
    let tab = aTab instanceof XULElement ? this.getTabFromChrome(aTab) : aTab;
    if (!tab) {
      return;
    }

    if (aOptions && "forceClose" in aOptions && aOptions.forceClose) {
      this._doCloseTab(tab);
      return;
    }

    tab.browser.messageManager.sendAsyncMessage("Browser:CanUnload", {});
  },

  savePage: function() {
    ContentAreaUtils.saveDocument(this.selectedBrowser.contentWindow.document);
  },

  



  _announceNewTab: function (aTab) {
    let event = document.createEvent("UIEvents");
    event.initUIEvent("TabOpen", true, false, window, 0);
    aTab.chromeTab.dispatchEvent(event);
    aTab.browser.messageManager.sendAsyncMessage("Browser:TabOpen");
  },

  _doCloseTab: function _doCloseTab(aTab) {
    if (this._tabs.length === 1) {
      Browser.addTab(this.getHomePage());
    }

    let nextTab = this.getNextTab(aTab);

    
    this._tabs.forEach(function(item, index, array) {
      if (item.owner == aTab)
        item.owner = null;
    });

    
    let event = document.createEvent("Events");
    event.initEvent("TabClose", true, false);
    aTab.chromeTab.dispatchEvent(event);

    
    event = document.createEvent("Events");
    event.initEvent("TabClose", true, false);
    aTab.browser.contentWindow.dispatchEvent(event);

    aTab.browser.messageManager.sendAsyncMessage("Browser:TabClose");

    let container = aTab.chromeTab.parentNode;
    aTab.destroy();
    this._tabs.splice(this._tabs.indexOf(aTab), 1);

    this.selectedTab = nextTab;

    event = document.createEvent("Events");
    event.initEvent("TabRemove", true, false);
    container.dispatchEvent(event);
  },

  getNextTab: function getNextTab(aTab) {
    let tabIndex = this._tabs.indexOf(aTab);
    if (tabIndex == -1)
      return null;

    if (this._selectedTab == aTab || this._selectedTab.chromeTab.hasAttribute("closing")) {
      let nextTabIndex = tabIndex + 1;
      let nextTab = null;

      while (nextTabIndex < this._tabs.length && (!nextTab || nextTab.chromeTab.hasAttribute("closing"))) {
        nextTab = this.getTabAtIndex(nextTabIndex);
        nextTabIndex++;
      }

      nextTabIndex = tabIndex - 1;
      while (nextTabIndex >= 0 && (!nextTab || nextTab.chromeTab.hasAttribute("closing"))) {
        nextTab = this.getTabAtIndex(nextTabIndex);
        nextTabIndex--;
      }

      if (!nextTab || nextTab.chromeTab.hasAttribute("closing"))
        return null;

      
      if (aTab.owner && nextTab.owner != aTab.owner)
        nextTab = aTab.owner;

      if (!nextTab)
        return null;

      return nextTab;
    }

    return this._selectedTab;
  },

  get selectedTab() {
    return this._selectedTab;
  },

  set selectedTab(tab) {
    if (tab instanceof XULElement)
      tab = this.getTabFromChrome(tab);

    if (!tab)
      return;

    if (this._selectedTab == tab) {
      
      
      Elements.browsers.selectedPanel = tab.notification;
      return;
    }

    let isFirstTab = this._selectedTab == null;
    let lastTab = this._selectedTab;
    let browser = tab.browser;

    this._selectedTab = tab;

    if (lastTab)
      lastTab.active = false;

    if (tab)
      tab.active = true;

    BrowserUI.update();

    if (isFirstTab) {
      BrowserUI._titleChanged(browser);
    } else {
      
      BrowserUI.updateURI();

      let event = document.createEvent("Events");
      event.initEvent("TabSelect", true, false);
      event.lastTab = lastTab;
      tab.chromeTab.dispatchEvent(event);
    }

    tab.lastSelected = Date.now();
  },

  supportsCommand: function(cmd) {
    return false;
  },

  isCommandEnabled: function(cmd) {
    return false;
  },

  doCommand: function(cmd) {
  },

  getNotificationBox: function getNotificationBox(aBrowser) {
    let browser = aBrowser || this.selectedBrowser;
    return browser.parentNode.parentNode;
  },

  


  _handleCertException: function _handleCertException(aMessage) {
    let json = aMessage.json;
    if (json.action == "leave") {
      
      let url = Browser.getHomePage({ useDefault: true });
      this.loadURI(url);
    } else {
      
      try {
        
        let uri = Services.io.newURI(json.url, null, null);
        let sslExceptions = new SSLExceptions();

        if (json.action == "permanent")
          sslExceptions.addPermanentException(uri, window);
        else
          sslExceptions.addTemporaryException(uri, window);
      } catch (e) {
        dump("EXCEPTION handle content command: " + e + "\n" );
      }

      
      aMessage.target.reload();
    }
  },

  


  _handleBlockedSite: function _handleBlockedSite(aMessage) {
    let formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
    let json = aMessage.json;
    switch (json.action) {
      case "leave": {
        
        let url = Browser.getHomePage({ useDefault: true });
        this.loadURI(url);
        break;
      }
      case "report-malware": {
        
        try {
          let reportURL = formatter.formatURLPref("browser.safebrowsing.malware.reportURL");
          reportURL += json.url;
          this.loadURI(reportURL);
        } catch (e) {
          Cu.reportError("Couldn't get malware report URL: " + e);
        }
        break;
      }
      case "report-phishing": {
        
        let url = Services.urlFormatter.formatURLPref("app.support.baseURL");
        this.loadURI(url + "phishing-malware");
        break;
      }
    }
  },

  pinSite: function browser_pinSite() {
    
    var file = Components.classes["@mozilla.org/file/directory_service;1"].
           getService(Components.interfaces.nsIProperties).
           get("CurProcD", Components.interfaces.nsIFile);
    
    file = file.parent;
    file.append("tileresources");
    file.append("VisualElements_logo.png");
    var ios = Components.classes["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);
    var uriSpec = ios.newFileURI(file).spec;
    Services.metro.pinTileAsync(this._currentPageTileID,
                                Browser.selectedBrowser.contentTitle, 
                                Browser.selectedBrowser.contentTitle, 
                                "-url " + Browser.selectedBrowser.currentURI.spec,
                            uriSpec, uriSpec);
  },

  get _currentPageTileID() {
    
    let hasher = Cc["@mozilla.org/security/hash;1"].
                 createInstance(Ci.nsICryptoHash);
    hasher.init(Ci.nsICryptoHash.MD5);
    let stringStream = Cc["@mozilla.org/io/string-input-stream;1"].
                       createInstance(Ci.nsIStringInputStream);
    stringStream.data = Browser.selectedBrowser.currentURI.spec;
    hasher.updateFromStream(stringStream, -1);
    let hashASCII = hasher.finish(true);
    
    return ("FFTileID_" + hashASCII).replace('/', '_', 'g');
  },

  unpinSite: function browser_unpinSite() {
    if (!Services.metro.immersive)
      return;

    Services.metro.unpinTileAsync(this._currentPageTileID);
  },

  isSitePinned: function browser_isSitePinned() {
    if (!Services.metro.immersive)
      return false;

    return Services.metro.isTilePinned(this._currentPageTileID);
  },

  starSite: function browser_starSite(callback) {
    let uri = this.selectedBrowser.currentURI;
    let title = this.selectedBrowser.contentTitle;

    Bookmarks.addForURI(uri, title, callback);
  },

  unstarSite: function browser_unstarSite(callback) {
    let uri = this.selectedBrowser.currentURI;
    Bookmarks.removeForURI(uri, callback);
  },

  isSiteStarredAsync: function browser_isSiteStarredAsync(callback) {
    let uri = this.selectedBrowser.currentURI;
    Bookmarks.isURIBookmarked(uri, callback);
  },

  






  getScrollboxPosition: function getScrollboxPosition(scroller) {
    let x = {};
    let y = {};
    scroller.getPosition(x, y);
    return new Point(x.value, y.value);
  },

  forceChromeReflow: function forceChromeReflow() {
    let dummy = getComputedStyle(document.documentElement, "").width;
  },

  receiveMessage: function receiveMessage(aMessage) {
    let json = aMessage.json;
    let browser = aMessage.target;

    switch (aMessage.name) {
      case "DOMLinkAdded": {
        
        
        let rel = json.rel.toLowerCase().split(" ");
        if (rel.indexOf("icon") != -1) {
          
          
          let size = 16;
          if (json.sizes) {
            let sizes = json.sizes.toLowerCase().split(" ");
            sizes.forEach(function(item) {
              if (item != "any") {
                let [w, h] = item.split("x");
                size = Math.max(Math.min(w, h), size);
              }
            });
          }
          if (size > browser.appIcon.size) {
            browser.appIcon.href = json.href;
            browser.appIcon.size = size;
          }
        }
        else if ((rel.indexOf("apple-touch-icon") != -1) && (browser.appIcon.size < 57)) {
          
          
          browser.appIcon.href = json.href;
          browser.appIcon.size = 57;
        }
        break;
      }
      case "Browser:FormSubmit":
        browser.lastLocation = null;
        break;

      case "Browser:CanUnload:Return": {
        if (json.permit) {
          let tab = this.getTabForBrowser(browser);
          BrowserUI.animateClosingTab(tab);
        }
        break;
      }
      case "Browser:CertException":
        this._handleCertException(aMessage);
        break;
      case "Browser:BlockedSite":
        this._handleBlockedSite(aMessage);
        break;
    }
  },
};

Browser.MainDragger = function MainDragger() {
  this._horizontalScrollbar = document.getElementById("horizontal-scroller");
  this._verticalScrollbar = document.getElementById("vertical-scroller");
  this._scrollScales = { x: 0, y: 0 };

  Elements.browsers.addEventListener("PanBegin", this, false);
  Elements.browsers.addEventListener("PanFinished", this, false);
};

Browser.MainDragger.prototype = {
  isDraggable: function isDraggable(target, scroller) {
    return { x: true, y: true };
  },

  dragStart: function dragStart(clientX, clientY, target, scroller) {
    let browser = getBrowser();
    let bcr = browser.getBoundingClientRect();
    this._contentView = browser.getViewAt(clientX - bcr.left, clientY - bcr.top);
  },

  dragStop: function dragStop(dx, dy, scroller) {
    if (this._contentView && this._contentView._updateCacheViewport)
      this._contentView._updateCacheViewport();
    this._contentView = null;
  },

  dragMove: function dragMove(dx, dy, scroller, aIsKinetic) {
    let doffset = new Point(dx, dy);

    this._panContent(doffset);

    if (aIsKinetic && doffset.x != 0)
      return false;

    this._updateScrollbars();

    return !doffset.equals(dx, dy);
  },

  handleEvent: function handleEvent(aEvent) {
    let browser = getBrowser();
    switch (aEvent.type) {
      case "PanBegin": {
        let width = ContentAreaObserver.width, height = ContentAreaObserver.height;
        let contentWidth = browser.contentDocumentWidth * browser.scale;
        let contentHeight = browser.contentDocumentHeight * browser.scale;

        
        
        const ALLOWED_MARGIN = 5;
        const SCROLL_CORNER_SIZE = 8;
        this._scrollScales = {
          x: (width + ALLOWED_MARGIN) < contentWidth ? (width - SCROLL_CORNER_SIZE) / contentWidth : 0,
          y: (height + ALLOWED_MARGIN) < contentHeight ? (height - SCROLL_CORNER_SIZE) / contentHeight : 0
        }

        this._showScrollbars();
        break;
      }
      case "PanFinished":
        this._hideScrollbars();

        
        browser._updateCSSViewport();
        break;
    }
  },

  _panContent: function md_panContent(aOffset) {
    if (this._contentView && !this._contentView.isRoot()) {
      this._panContentView(this._contentView, aOffset);
      
      
    }
    
    this._panContentView(getBrowser().getRootView(), aOffset);
  },

  
  _panContentView: function _panContentView(contentView, doffset) {
    let pos0 = contentView.getPosition();
    contentView.scrollBy(doffset.x, doffset.y);
    let pos1 = contentView.getPosition();
    doffset.subtract(pos1.x - pos0.x, pos1.y - pos0.y);
  },

  _updateScrollbars: function _updateScrollbars() {
    let scaleX = this._scrollScales.x, scaleY = this._scrollScales.y;
    let contentScroll = Browser.getScrollboxPosition(Browser.contentScrollboxScroller);

    if (scaleX)
      this._horizontalScrollbar.style.MozTransform =
        "translateX(" + Math.round(contentScroll.x * scaleX) + "px)";

    if (scaleY) {
      let y = Math.round(contentScroll.y * scaleY);
      let x = 0;

      this._verticalScrollbar.style.MozTransform =
        "translate(" + x + "px," + y + "px)";
    }
  },

  _showScrollbars: function _showScrollbars() {
    this._updateScrollbars();
    let scaleX = this._scrollScales.x, scaleY = this._scrollScales.y;
    if (scaleX) {
      this._horizontalScrollbar.width = ContentAreaObserver.width * scaleX;
      this._horizontalScrollbar.setAttribute("panning", "true");
    }

    if (scaleY) {
      this._verticalScrollbar.height = ContentAreaObserver.height * scaleY;
      this._verticalScrollbar.setAttribute("panning", "true");
    }
  },

  _hideScrollbars: function _hideScrollbars() {
    this._scrollScales.x = 0;
    this._scrollScales.y = 0;
    this._horizontalScrollbar.removeAttribute("panning");
    this._verticalScrollbar.removeAttribute("panning");
    this._horizontalScrollbar.removeAttribute("width");
    this._verticalScrollbar.removeAttribute("height");
    this._horizontalScrollbar.style.MozTransform = "";
    this._verticalScrollbar.style.MozTransform = "";
  }
};


function nsBrowserAccess() { }

nsBrowserAccess.prototype = {
  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIBrowserDOMWindow) || aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  },

  _getOpenAction: function _getOpenAction(aURI, aOpener, aWhere, aContext) {
    let where = aWhere;
    








    if (where == Ci.nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW) {
      
      switch (aContext) {
        
        case Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL :
          where = Services.prefs.getIntPref("browser.link.open_external");
          break;
        
        default :
          where = Services.prefs.getIntPref("browser.link.open_newwindow");
      }
    }
    if (where == Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW) {
      Util.dumpLn("Invalid request - we can't open links in new windows.");
      where = Ci.nsIBrowserDOMWindow.OPEN_NEWTAB;
    }
    return where;
  },

  _getBrowser: function _getBrowser(aURI, aOpener, aWhere, aContext) {
    let isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);
    
    if (isExternal && aURI && aURI.schemeIs("chrome"))
      return null;

    let location;
    let browser;
    let loadflags = isExternal ?
                      Ci.nsIWebNavigation.LOAD_FLAGS_FROM_EXTERNAL :
                      Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    let openAction = this._getOpenAction(aURI, aOpener, aWhere, aContext);

    if (openAction == Ci.nsIBrowserDOMWindow.OPEN_NEWTAB) {
      let owner = isExternal ? null : Browser.selectedTab;
      let tab = BrowserUI.openLinkInNewTab("about:blank", true, owner);
      browser = tab.browser;
    } else {
      browser = Browser.selectedBrowser;
    }

    try {
      let referrer;
      if (aURI && browser) {
        if (aOpener) {
          location = aOpener.location;
          referrer = Services.io.newURI(location, null, null);
        }
        browser.loadURIWithFlags(aURI.spec, loadflags, referrer, null, null);
      }
      browser.focus();
    } catch(e) { }

    return browser;
  },

  openURI: function browser_openURI(aURI, aOpener, aWhere, aContext) {
    let browser = this._getBrowser(aURI, aOpener, aWhere, aContext);
    return browser ? browser.contentWindow : null;
  },

  openURIInFrame: function browser_openURIInFrame(aURI, aOpener, aWhere, aContext) {
    let browser = this._getBrowser(aURI, aOpener, aWhere, aContext);
    return browser ? browser.QueryInterface(Ci.nsIFrameLoaderOwner) : null;
  },

  isTabContentWindow: function(aWindow) {
    return Browser.browsers.some(function (browser) browser.contentWindow == aWindow);
  },
};




var PopupBlockerObserver = {
  init: function init() {
    Elements.browsers.addEventListener("mousedown", this, true);
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "mousedown":
        let box = Browser.getNotificationBox();
        let notification = box.getNotificationWithValue("popup-blocked");
        if (notification && !notification.contains(aEvent.target))
          box.removeNotification(notification);
        break;
    }
  },

  onUpdatePageReport: function onUpdatePageReport(aEvent) {
    var cBrowser = Browser.selectedBrowser;
    if (aEvent.originalTarget != cBrowser)
      return;

    if (!cBrowser.pageReport)
      return;

    let result = Services.perms.testExactPermission(Browser.selectedBrowser.currentURI, "popup");
    if (result == Ci.nsIPermissionManager.DENY_ACTION)
      return;

    
    
    
    if (!cBrowser.pageReport.reported) {
      if (Services.prefs.getBoolPref("privacy.popups.showBrowserMessage")) {
        var brandShortName = Strings.brand.GetStringFromName("brandShortName");
        var popupCount = cBrowser.pageReport.length;

        let strings = Strings.browser;
        let message = PluralForm.get(popupCount, strings.GetStringFromName("popupWarning.message"))
                                .replace("#1", brandShortName)
                                .replace("#2", popupCount);

        var notificationBox = Browser.getNotificationBox();
        var notification = notificationBox.getNotificationWithValue("popup-blocked");
        if (notification) {
          notification.label = message;
        }
        else {
          var buttons = [
            {
              isDefault: false,
              label: strings.GetStringFromName("popupButtonAllowOnce2"),
              accessKey: "",
              callback: function() { PopupBlockerObserver.showPopupsForSite(); }
            },
            {
              label: strings.GetStringFromName("popupButtonAlwaysAllow3"),
              accessKey: "",
              callback: function() { PopupBlockerObserver.allowPopupsForSite(true); }
            },
            {
              label: strings.GetStringFromName("popupButtonNeverWarn3"),
              accessKey: "",
              callback: function() { PopupBlockerObserver.allowPopupsForSite(false); }
            }
          ];

          const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
          notificationBox.appendNotification(message, "popup-blocked",
                                             "chrome://browser/skin/images/infobar-popup.png",
                                             priority, buttons);
        }
      }
      
      
      cBrowser.pageReport.reported = true;
    }
  },

  allowPopupsForSite: function allowPopupsForSite(aAllow) {
    var currentURI = Browser.selectedBrowser.currentURI;
    Services.perms.add(currentURI, "popup", aAllow
                       ?  Ci.nsIPermissionManager.ALLOW_ACTION
                       :  Ci.nsIPermissionManager.DENY_ACTION);

    Browser.getNotificationBox().removeCurrentNotification();
  },

  showPopupsForSite: function showPopupsForSite() {
    let uri = Browser.selectedBrowser.currentURI;
    let pageReport = Browser.selectedBrowser.pageReport;
    if (pageReport) {
      for (let i = 0; i < pageReport.length; ++i) {
        var popupURIspec = pageReport[i].popupWindowURI.spec;

        
        
        
        
        
        if (popupURIspec == "" || !Util.isURLMemorable(popupURIspec) || popupURIspec == uri.spec)
          continue;

        let popupFeatures = pageReport[i].popupWindowFeatures;
        let popupName = pageReport[i].popupWindowName;

        Browser.addTab(popupURIspec, false, Browser.selectedTab);
      }
    }
  }
};

var SessionHistoryObserver = {
  observe: function sho_observe(aSubject, aTopic, aData) {
    if (aTopic != "browser:purge-session-history")
      return;

    let newTab = Browser.addTab("about:start", true);
    let tab = Browser._tabs[0];
    while(tab != newTab) {
      Browser.closeTab(tab, { forceClose: true } );
      tab = Browser._tabs[0];
    }

    PlacesUtils.history.removeAllPages();

    
    BrowserUI._edit.editor.transactionManager.clear();
  }
};

function getNotificationBox(aBrowser) {
  return Browser.getNotificationBox(aBrowser);
}

function showDownloadManager(aWindowContext, aID, aReason) {
  
}

function Tab(aURI, aParams, aOwner) {
  this._id = null;
  this._browser = null;
  this._notification = null;
  this._loading = false;
  this._progressActive = false;
  this._progressCount = 0;
  this._chromeTab = null;
  this._eventDeferred = null;
  this._updateThumbnailTimeout = null;

  this._private = false;
  if ("private" in aParams) {
    this._private = aParams.private;
  } else if (aOwner) {
    this._private = aOwner._private;
  }

  this.owner = aOwner || null;

  
  
  this.lastSelected = 0;

  
  
  this.create(aURI, aParams || {}, aOwner);

  
  this.active = false;
}

Tab.prototype = {
  get browser() {
    return this._browser;
  },

  get notification() {
    return this._notification;
  },

  get chromeTab() {
    return this._chromeTab;
  },

  get isPrivate() {
    return this._private;
  },

  get pageShowPromise() {
    return this._eventDeferred ? this._eventDeferred.promise : null;
  },

  startLoading: function startLoading() {
    if (this._loading) {
      let stack = new Error().stack;
      throw "Already Loading!\n" + stack;
    }
    this._loading = true;
  },

  endLoading: function endLoading() {
    this._loading = false;
    this.updateFavicon();
  },

  isLoading: function isLoading() {
    return this._loading;
  },

  create: function create(aURI, aParams, aOwner) {
    this._eventDeferred = Promise.defer();

    this._chromeTab = Elements.tabList.addTab(aParams.index);
    if (this.isPrivate) {
      this._chromeTab.setAttribute("private", "true");
    }

    this._id = Browser.createTabId();
    let browser = this._createBrowser(aURI, null);

    let self = this;
    function onPageShowEvent(aEvent) {
      browser.removeEventListener("pageshow", onPageShowEvent);
      if (self._eventDeferred) {
        self._eventDeferred.resolve(self);
      }
      self._eventDeferred = null;
    }
    browser.addEventListener("pageshow", onPageShowEvent, true);
    browser.addEventListener("DOMWindowCreated", this, false);
    browser.addEventListener("StartUIChange", this, false);
    Elements.browsers.addEventListener("SizeChanged", this, false);

    browser.messageManager.addMessageListener("Content:StateChange", this);

    if (aOwner)
      this._copyHistoryFrom(aOwner);
    this._loadUsingParams(browser, aURI, aParams);
  },

  updateViewport: function (aEvent) {
    
    let browser = this.browser;

    
    let paddingBottom = parseInt(getComputedStyle(browser).paddingBottom, 10);
    let height = browser.clientHeight - paddingBottom;

    browser.setWindowSize(browser.clientWidth, height);
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "DOMWindowCreated":
      case "StartUIChange":
        this.updateViewport();
        break;
      case "SizeChanged":
        this.updateViewport();
        this._delayUpdateThumbnail();
        break;
      case "AlertClose": {
        if (this == Browser.selectedTab) {
          this.updateViewport();
        }
        break;
      }
    }
  },

  receiveMessage: function(aMessage) {
    switch (aMessage.name) {
      case "Content:StateChange":
        
        this.updateThumbnail();
        
        if (aMessage.json.stateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
          this._delayUpdateThumbnail();
        }
        break;
    }
  },

  _delayUpdateThumbnail: function() {
    clearTimeout(this._updateThumbnailTimeout);
    this._updateThumbnailTimeout = setTimeout(() => {
      this.updateThumbnail();
    }, kTabThumbnailDelayCapture);
  },

  destroy: function destroy() {
    this._browser.messageManager.removeMessageListener("Content:StateChange", this);
    this._browser.removeEventListener("DOMWindowCreated", this, false);
    this._browser.removeEventListener("StartUIChange", this, false);
    Elements.browsers.removeEventListener("SizeChanged", this, false);
    clearTimeout(this._updateThumbnailTimeout);

    Elements.tabList.removeTab(this._chromeTab);
    this._chromeTab = null;
    this._destroyBrowser();
  },

  resurrect: function resurrect() {
    let dead = this._browser;
    let active = this.active;

    
    let session = { data: dead.__SS_data, extra: dead.__SS_extdata };

    
    
    let currentURL = dead.__SS_restore ? session.data.entries[0].url : dead.currentURI.spec;
    let sibling = dead.nextSibling;

    
    this._destroyBrowser();
    let browser = this._createBrowser(currentURL, sibling);
    if (active)
      this.active = true;

    
    browser.__SS_data = session.data;
    browser.__SS_extdata = session.extra;
    browser.__SS_restore = true;
  },

  _copyHistoryFrom: function _copyHistoryFrom(tab) {
    let otherHistory = tab._browser._webNavigation.sessionHistory;
    let history = this._browser._webNavigation.sessionHistory;

    
    history.QueryInterface(Ci.nsISHistoryInternal);

    for (let i = 0, length = otherHistory.index; i <= length; i++)
      history.addEntry(otherHistory.getEntryAtIndex(i, false), true);
  },

  _loadUsingParams: function _loadUsingParams(aBrowser, aURI, aParams) {
    let flags = aParams.flags || Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    let postData = ("postData" in aParams && aParams.postData) ? aParams.postData.value : null;
    let referrerURI = "referrerURI" in aParams ? aParams.referrerURI : null;
    let charset = "charset" in aParams ? aParams.charset : null;
    aBrowser.loadURIWithFlags(aURI, flags, referrerURI, charset, postData);
  },

  _createBrowser: function _createBrowser(aURI, aInsertBefore) {
    if (this._browser)
      throw "Browser already exists";

    
    
    
    let notification = this._notification = document.createElement("notificationbox");

    let browser = this._browser = document.createElement("browser");
    browser.id = "browser-" + this._id;
    this._chromeTab.linkedBrowser = browser;

    browser.setAttribute("type", "content-targetable");

    let useRemote = Services.appinfo.browserTabsRemote;
    let useLocal = Util.isLocalScheme(aURI);
    browser.setAttribute("remote", (!useLocal && useRemote) ? "true" : "false");

    
    let stack = document.createElementNS(XUL_NS, "stack");
    stack.className = "browserStack";
    stack.appendChild(browser);
    stack.setAttribute("flex", "1");
    notification.appendChild(stack);
    Elements.browsers.insertBefore(notification, aInsertBefore);

    notification.dir = "reverse";
    notification.addEventListener("AlertClose", this);

     
    ContentAreaObserver.onBrowserCreated(browser);

    if (this.isPrivate) {
      let ctx = browser.docShell.QueryInterface(Ci.nsILoadContext);
      ctx.usePrivateBrowsing = true;
    }

    
    browser.stop();

    let fl = browser.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
    fl.renderMode = Ci.nsIFrameLoader.RENDER_MODE_ASYNC_SCROLL;

    return browser;
  },

  _destroyBrowser: function _destroyBrowser() {
    if (this._browser) {
      let notification = this._notification;
      notification.removeEventListener("AlertClose", this);
      let browser = this._browser;
      browser.active = false;

      this._notification = null;
      this._browser = null;
      this._loading = false;

      Elements.browsers.removeChild(notification);
    }
  },

  updateThumbnail: function updateThumbnail() {
    if (!this.isPrivate) {
      PageThumbs.captureToCanvas(this.browser.contentWindow, this._chromeTab.thumbnailCanvas);
    }
  },

  updateFavicon: function updateFavicon() {
    this._chromeTab.updateFavicon(this._browser.mIconURL);
  },

  set active(aActive) {
    if (!this._browser)
      return;

    let notification = this._notification;
    let browser = this._browser;

    if (aActive) {
      notification.classList.add("active-tab-notificationbox");
      browser.setAttribute("type", "content-primary");
      Elements.browsers.selectedPanel = notification;
      browser.active = true;
      Elements.tabList.selectedTab = this._chromeTab;
      browser.focus();
    } else {
      notification.classList.remove("active-tab-notificationbox");
      browser.messageManager.sendAsyncMessage("Browser:Blur", { });
      browser.setAttribute("type", "content-targetable");
      browser.active = false;
    }
  },

  get active() {
    if (!this._browser)
      return false;
    return this._browser.getAttribute("type") == "content-primary";
  },

  toString: function() {
    return "[Tab " + (this._browser ? this._browser.currentURI.spec : "(no browser)") + "]";
  }
};


function rendererFactory(aBrowser, aCanvas) {
  let wrapper = {};

  if (aBrowser.contentWindow) {
    let ctx = aCanvas.getContext("2d");
    let draw = function(browser, aLeft, aTop, aWidth, aHeight, aColor, aFlags) {
      ctx.drawWindow(browser.contentWindow, aLeft, aTop, aWidth, aHeight, aColor, aFlags);
      let e = document.createEvent("HTMLEvents");
      e.initEvent("MozAsyncCanvasRender", true, true);
      aCanvas.dispatchEvent(e);
    };
    wrapper.checkBrowser = function(browser) {
      return browser.contentWindow;
    };
    wrapper.drawContent = function(callback) {
      callback(ctx, draw);
    };
  }
  else {
    let ctx = aCanvas.MozGetIPCContext("2d");
    let draw = function(browser, aLeft, aTop, aWidth, aHeight, aColor, aFlags) {
      ctx.asyncDrawXULElement(browser, aLeft, aTop, aWidth, aHeight, aColor, aFlags);
    };
    wrapper.checkBrowser = function(browser) {
      return !browser.contentWindow;
    };
    wrapper.drawContent = function(callback) {
      callback(ctx, draw);
    };
  }

  return wrapper;
};


let ClickEventHandler = {
  init: function () {
    gEventListenerService.addSystemEventListener(Elements.browsers, "click", this, true);
  },

  uninit: function () {
    gEventListenerService.removeSystemEventListener(Elements.browsers, "click", this, true);
  },

  handleEvent: function (aEvent) {
    if (!aEvent.isTrusted || aEvent.defaultPrevented) {
      return;
    }
    let [href, node] = this._hrefAndLinkNodeForClickEvent(aEvent);
    if (href && (aEvent.button == 1 || aEvent.ctrlKey)) {
      
      BrowserUI.openLinkInNewTab(href, aEvent.shiftKey, Browser.selectedTab);
    }
  },

  









  _hrefAndLinkNodeForClickEvent: function(event) {
    function isHTMLLink(aNode) {
      return ((aNode instanceof content.HTMLAnchorElement && aNode.href) ||
              (aNode instanceof content.HTMLAreaElement && aNode.href) ||
              aNode instanceof content.HTMLLinkElement);
    }

    let node = event.target;
    while (node && !isHTMLLink(node)) {
      node = node.parentNode;
    }

    if (node)
      return [node.href, node];

    
    let href, baseURI;
    node = event.target;
    while (node && !href) {
      if (node.nodeType == content.Node.ELEMENT_NODE) {
        href = node.getAttributeNS("http://www.w3.org/1999/xlink", "href");
        if (href)
          baseURI = node.ownerDocument.baseURIObject;
      }
      node = node.parentNode;
    }

    
    
    
    return [href ? Services.io.newURI(href, null, baseURI).spec : null, null];
  }
};
