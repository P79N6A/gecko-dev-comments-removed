













































let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

function getBrowser() {
  return Browser.selectedBrowser;
}

const kDefaultBrowserWidth = 800;


const kTapOverlayTimeout = 200;

const endl = '\n';

function debug() {
  let bv = Browser._browserView;
  let tc = bv._tileManager._tileCache;
  let scrollbox = document.getElementById("content-scrollbox")
                .boxObject.QueryInterface(Ci.nsIScrollBoxObject);

  let x = {};
  let y = {};
  let w = {};
  let h = {};
  scrollbox.getPosition(x, y);
  scrollbox.getScrolledSize(w, h);
  let container = document.getElementById("tile-container");
  let [x, y] = [x.value, y.value];
  let [w, h] = [w.value, h.value];
  if (bv) {
    dump('----------------------DEBUG!-------------------------\n');
    dump(bv._browserViewportState.toString() + endl);

    dump(endl);

    dump('location from Browser: ' + Browser.selectedBrowser.contentWindow.location + endl);
    dump('location from BV     : ' + bv.getBrowser().contentWindow.location + endl);

    dump(endl + endl);

    let cr = bv._tileManager._criticalRect;
    dump('criticalRect from BV: ' + (cr ? cr.toString() : null) + endl);
    dump('visibleRect from BV : ' + bv.getVisibleRect().toString() + endl);
    dump('visibleRect from foo: ' + Browser.getVisibleRect().toString() + endl);

    dump('bv batchops depth:    ' + bv._batchOps.length + endl);
    dump('renderpause depth:    ' + bv._renderMode + endl);

    dump(endl);

    dump('window.innerWidth : ' + window.innerWidth  + endl);
    dump('window.innerHeight: ' + window.innerHeight + endl);

    dump(endl);

    dump('container width,height from BV: ' + bv._container.style.width + ', '
                                            + bv._container.style.height + endl);
    dump('container width,height via DOM: ' + container.style.width + ', '
                                            + container.style.height + endl);

    dump(endl);

    dump('scrollbox position    : ' + x + ', ' + y + endl);
    dump('scrollbox scrolledsize: ' + w + ', ' + h + endl);


    let sb = document.getElementById("content-scrollbox");
    dump('container location:     ' + Math.round(container.getBoundingClientRect().left) + " " +
                                      Math.round(container.getBoundingClientRect().top) + endl);

    dump(endl);

    let mouseModule = ih._modules[0];
    dump('ih grabber  : ' + ih._grabber           + endl);
    dump('ih grabdepth: ' + ih._grabDepth         + endl);
    dump('ih listening: ' + !ih._ignoreEvents     + endl);
    dump('ih suppress : ' + ih._suppressNextClick + endl);
    dump('mouseModule : ' + mouseModule           + endl);

    dump(endl);

    dump('tilecache capacity: ' + bv._tileManager._tileCache.getCapacity() + endl);
    dump('tilecache size    : ' + bv._tileManager._tileCache.size          + endl);
    dump('tilecache iBound  : ' + bv._tileManager._tileCache.iBound        + endl);
    dump('tilecache jBound  : ' + bv._tileManager._tileCache.jBound        + endl);

    dump('-----------------------------------------------------\n');
  }
}

function debugTile(i, j) {
  let bv = Browser._browserView;
  let tc = bv._tileManager._tileCache;
  let t  = tc.getTile(i, j);

  dump('------ DEBUGGING TILE (' + i + ',' + j + ') --------\n');

  dump('in bounds: ' + tc.inBounds(i, j) + endl);
  dump('occupied : ' + tc.isOccupied(i, j) + endl);
  if (t)
  {
  dump('toString : ' + t.toString(true) + endl);
  dump('free     : ' + t.free + endl);
  dump('dirtyRect: ' + t._dirtyTileCanvasRect + endl);

  let len = tc._tilePool.length;
  for (let k = 0; k < len; ++k)
    if (tc._tilePool[k] === t)
      dump('found in tilePool at index ' + k + endl);
  }

  dump('------------------------------------\n');
}

function onDebugKeyPress(ev) {
  let bv = Browser._browserView;

  if (!ev.ctrlKey)
    return;

  

  const a = 65;   
  const b = 66;   
  const c = 67;
  const d = 68;  
  const e = 69;
  const f = 70;  
  const g = 71;
  const h = 72;
  const i = 73;  
  const j = 74;
  const k = 75;
  const l = 76;  
  const m = 77;  
  const n = 78;
  const o = 79;
  const p = 80;  
  const q = 81;
  const r = 82;  
  const s = 83;
  const t = 84;  
  const u = 85;
  const v = 86;
  const w = 87;
  const x = 88;
  const y = 89;
  const z = 90;  

  if (window.tileMapMode) {
    function putChar(ev, col, row) {
      let tile = tc.getTile(col, row);
      switch (ev.charCode) {
      case h: 
        dump(tile ? (tile.free ? '*' : 'h') : ' ');
        break;
      case d: 
        dump(tile ? (tile.isDirty() ? 'd' : '*') : ' ');
        break;
      case o: 
        dump(tc.isOccupied(col, row) ? 'o' : ' ');
        break;
      }
    }

    let tc = Browser._browserView._tileManager._tileCache;
    let col, row;

    dump(endl);

    dump('  ');
    for (col = 0; col < tc.iBound; ++col)
      dump(col % 10);

    dump(endl);

    for (row = 0; row < tc.jBound; ++row) {

      dump((row % 10) + ' ');

      for (col = 0; col < tc.iBound; ++col) {
        putChar(ev, col, row);
      }

      dump(endl);
    }
    dump(endl + endl);

    for (let ii = 0; ii < tc._tilePool.length; ++ii) {
      let tile = tc._tilePool[ii];
      putChar(ev, tile.i, tile.j);
    }

    dump(endl + endl);

    window.tileMapMode = false;
    return;
  }

  switch (ev.charCode) {
  case f:
    var result = Browser.sacrificeTab();
    if (result)
      dump("Freed a tab\n");
    else
      dump("There are no tabs left to free\n");
    break;

  case r:
    bv.onAfterVisibleMove();
    

  case d:
    debug();

    break;
  case l:
    bv._tileManager.restartLazyCrawl(bv._tileManager._criticalRect);

    break;
  case b:
    window.tileMapMode = true;
    break;
  case t:
    let ijstrs = window.prompt('row,col plz').split(' ');
    for each (let ijstr in ijstrs) {
      let [i, j] = ijstr.split(',').map(function (x) { return parseInt(x); });
      debugTile(i, j);
    }

    break;
  case a:
    let cr = bv._tileManager._criticalRect;
    dump('>>>>>> critical rect is ' + (cr ? cr.toString() : cr) + '\n');
    if (cr) {
      let starti = cr.left  >> kTileExponentWidth;
      let endi   = cr.right >> kTileExponentWidth;

      let startj = cr.top    >> kTileExponentHeight;
      let endj   = cr.bottom >> kTileExponentHeight;

      for (var jj = startj; jj <= endj; ++jj)
        for (var ii = starti; ii <= endi; ++ii)
          debugTile(ii, jj);
    }

    break;
  case i:
    window.infoMode = !window.infoMode;
    break;
  case m:
    Util.dumpLn("renderMode:", bv._renderMode);
    Util.dumpLn("batchOps:",bv._batchOps.length);
    bv.resumeRendering();
    break;
  case p:
    let tc = bv._tileManager._tileCache;
    dump('************* TILE POOL ****************\n');
    for (let ii = 0, len = tc._tilePool.length; ii < len; ++ii) {
      if (window.infoMode)
        debugTile(tc._tilePool[ii].i, tc._tilePool[ii].j);
      else
        dump(tc._tilePool[ii].i + ',' + tc._tilePool[ii].j + '\n');
    }
    dump('****************************************\n');
    break;
  case z:
    bv.setZoomLevel(1.0);
    break;
  default:
    break;
  }
}
window.infoMode = false;
window.tileMapMode = false;

var ih = null;

var Browser = {
  _tabs : [],
  _selectedTab : null,
  windowUtils: window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils),
  contentScrollbox: null,
  contentScrollboxScroller: null,
  controlsScrollbox: null,
  controlsScrollboxScroller: null,
  pageScrollbox: null,
  pageScrollboxScroller: null,
  styles: {},

  startup: function startup() {
    var self = this;

    let needOverride = Util.needHomepageOverride();
    if (needOverride == "new profile")
      this.initNewProfile();

    let container = document.getElementById("tile-container");
    let bv = this._browserView = new BrowserView(container, Browser.getVisibleRect);

    
    container.customClicker = new ContentCustomClicker(bv);

    
    let contentScrollbox = this.contentScrollbox = document.getElementById("content-scrollbox");
    this.contentScrollboxScroller = contentScrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    contentScrollbox.customDragger = new Browser.MainDragger(bv);

    
    let controlsScrollbox = this.controlsScrollbox = document.getElementById("controls-scrollbox");
    this.controlsScrollboxScroller = controlsScrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    controlsScrollbox.customDragger = {
      isDraggable: function isDraggable(target, content) { return false; },
      dragStart: function dragStart(cx, cy, target, scroller) {},
      dragStop: function dragStop(dx, dy, scroller) { return false; },
      dragMove: function dragMove(dx, dy, scroller) { return false; }
    };

    
    let pageScrollbox = this.pageScrollbox = document.getElementById("page-scrollbox");
    this.pageScrollboxScroller = pageScrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    pageScrollbox.customDragger = controlsScrollbox.customDragger;

    
    bv.beginBatchOperation();

    let stylesheet = document.styleSheets[0];
    for each (let style in ["window-width", "window-height", "toolbar-height", "browser", "browser-handheld", "browser-viewport"]) {
      let index = stylesheet.insertRule("." + style + " {}", stylesheet.cssRules.length);
      this.styles[style] = stylesheet.cssRules[index].style;
    }

    function resizeHandler(e) {
      if (e.target != window)
        return;

      
      let w = window.innerWidth;
      let h = window.innerHeight;
      let maximize = (document.documentElement.getAttribute("sizemode") == "maximized");
      if (maximize && w > screen.width)
        return;

      bv.beginBatchOperation();

      let toolbarHeight = Math.round(document.getElementById("toolbar-main").getBoundingClientRect().height);
      let scaledDefaultH = (kDefaultBrowserWidth * (h / w));
      let scaledScreenH = (window.screen.width * (h / w));

      Browser.styles["window-width"].width = w + "px";
      Browser.styles["window-height"].height = h + "px";
      Browser.styles["toolbar-height"].height = toolbarHeight + "px";
      Browser.styles["browser"].width = kDefaultBrowserWidth + "px";
      Browser.styles["browser"].height = scaledDefaultH + "px";
      Browser.styles["browser-handheld"].width = window.screen.width + "px";
      Browser.styles["browser-handheld"].height = scaledScreenH + "px";

      
      let browser = Browser.selectedBrowser;
      if (browser.contentDocument instanceof XULDocument)
        BrowserView.Util.ensureMozScrolledAreaEvent(browser, w, h);

      
      BrowserUI.sizeControls(w, h);

      bv.updateDefaultZoom();
      if (bv.isDefaultZoom())
        
        Browser.hideSidebars();
      bv.onAfterVisibleMove();

      bv.commitBatchOperation();
      
      let curEl = document.activeElement;
      if (curEl && curEl.scrollIntoView)
        curEl.scrollIntoView(false);
    }
    window.addEventListener("resize", resizeHandler, false);

    function fullscreenHandler() {
      if (!window.fullScreen)
        document.getElementById("toolbar-main").setAttribute("fullscreen", "true");
      else
        document.getElementById("toolbar-main").removeAttribute("fullscreen");
    }
    window.addEventListener("fullscreen", fullscreenHandler, false);

    function notificationHandler() {
      
      Browser.forceChromeReflow();
      bv.onAfterVisibleMove();
    }
    let notifications = document.getElementById("notifications");
    notifications.addEventListener("AlertActive", notificationHandler, false);
    notifications.addEventListener("AlertClose", notificationHandler, false);

    
    container.addEventListener("contextmenu", ContextHelper, false);

    
    ih = new InputHandler(container);

    BrowserUI.init();

    window.controllers.appendController(this);
    window.controllers.appendController(BrowserUI);

    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.addObserver(gXPInstallObserver, "xpinstall-install-blocked", false);
    os.addObserver(gSessionHistoryObserver, "browser:purge-session-history", false);

    
    os.addObserver(MemoryObserver, "memory-pressure", false);

    
    os.addObserver(BrowserSearch, "browser-search-engine-modified", false);

    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = new nsBrowserAccess();

    let browsers = document.getElementById("browsers");
    browsers.addEventListener("command", this._handleContentCommand, true);
    browsers.addEventListener("MozApplicationManifest", OfflineApps, false);
    browsers.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);

    
    Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);

    
    Util.forceOnline();

    
    let whereURI = this.getHomePage();
    if (needOverride == "new profile")
        whereURI = "about:firstrun";

    
    
    if (window.arguments && window.arguments[0] &&
        window.arguments[0] instanceof Ci.nsICommandLine) {
      try {
        var cmdLine = window.arguments[0];

        
        if (cmdLine.length == 1) {
          
          var uri = cmdLine.getArgument(0);
          if (uri != "" && uri[0] != '-') {
            whereURI = cmdLine.resolveURI(uri);
            if (whereURI)
              whereURI = whereURI.spec;
          }
        }

        
        var uriFlag = cmdLine.handleFlagWithParam("url", false);
        if (uriFlag) {
          whereURI = cmdLine.resolveURI(uriFlag);
          if (whereURI)
            whereURI = whereURI.spec;
        }
      } catch (e) {}
    }

    this.addTab(whereURI, true);

    
    if (gPrefService.getBoolPref("browser.console.showInPanel")){
      let button = document.getElementById("tool-console");
      button.hidden = false;
    }

    bv.commitBatchOperation();

    
    if (gPrefService.prefHasUserValue("extensions.disabledAddons")) {
      let addons = gPrefService.getCharPref("extensions.disabledAddons").split(",");
      if (addons.length > 0) {
        let disabledStrings = Elements.browserBundle.getString("alertAddonsDisabled");
        let label = PluralForm.get(addons.length, disabledStrings).replace("#1", addons.length);

        let alerts = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
        alerts.showAlertNotification(URI_GENERIC_ICON_XPINSTALL, Elements.browserBundle.getString("alertAddons"),
                                     label, false, "", null);
      }
      gPrefService.clearUserPref("extensions.disabledAddons");
    }

    
    ImagePreloader.cache();

    this._pluginObserver = new PluginObserver(bv);

    
    let event = document.createEvent("Events");
    event.initEvent("UIReady", true, false);
    window.dispatchEvent(event);
  },

  closing: function closing() {
    
    let numTabs = this._tabs.length;
    if (numTabs > 1) {
      let shouldPrompt = gPrefService.getBoolPref("browser.tabs.warnOnClose");
      if (shouldPrompt) {
        let prompt = Cc["@mozilla.org/embedcomp/prompt-service;1"].getService(Ci.nsIPromptService);
  
        
        let warnOnClose = { value: true };

        let messageBase = Elements.browserBundle.getString("tabs.closeWarning");
        let message = PluralForm.get(numTabs, messageBase).replace("#1", numTabs);

        let title = Elements.browserBundle.getString("tabs.closeWarningTitle");
        let closeText = Elements.browserBundle.getString("tabs.closeButton");
        let checkText = Elements.browserBundle.getString("tabs.closeWarningPromptMe");
        let buttons = (prompt.BUTTON_TITLE_IS_STRING * prompt.BUTTON_POS_0) +
                      (prompt.BUTTON_TITLE_CANCEL * prompt.BUTTON_POS_1);
        let pressed = prompt.confirmEx(window, title, message, buttons, closeText, null, null, checkText, warnOnClose);

        
        let reallyClose = (pressed == 0);
        if (reallyClose && !warnOnClose.value)
          gPrefService.setBoolPref("browser.tabs.warnOnClose", false);

        
        if (!reallyClose)
          return false;
      }
    }

    
    let lastBrowser = true;
    let e = gWindowMediator.getEnumerator("navigator:browser");
    while (e.hasMoreElements() && lastBrowser) {
      let win = e.getNext();
      if (win != window && win.toolbar.visible)
        lastBrowser = false;
    }
    if (!lastBrowser)
      return true;

    
    let closingCanceled = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
    gObserverService.notifyObservers(closingCanceled, "browser-lastwindow-close-requested", null);
    if (closingCanceled.data)
      return false;
  
    gObserverService.notifyObservers(null, "browser-lastwindow-close-granted", null);
    return true;
  },

  shutdown: function shutdown() {
    this._browserView.uninit();
    BrowserUI.uninit();
    this._pluginObserver.stop();

    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.removeObserver(gXPInstallObserver, "xpinstall-install-blocked");
    os.removeObserver(gSessionHistoryObserver, "browser:purge-session-history");
    os.removeObserver(MemoryObserver, "memory-pressure");
    os.removeObserver(BrowserSearch, "browser-search-engine-modified");

    window.controllers.removeController(this);
    window.controllers.removeController(BrowserUI);
  },

  initNewProfile: function initNewProfile() {
  },
  
  getHomePage: function () {
    let url = "about:home";
    try {
      url = gPrefService.getComplexValue("browser.startup.homepage", Ci.nsIPrefLocalizedString).data;
    } catch (e) { }

    return url;
  },

  get browsers() {
    return this._tabs.map(function(tab) { return tab.browser; });
  },

  scrollContentToTop: function scrollContentToTop() {
    this.contentScrollboxScroller.scrollTo(0, 0);
    this.pageScrollboxScroller.scrollTo(0, 0);
    this._browserView.onAfterVisibleMove();
  },

  
  scrollBrowserToContent: function scrollBrowserToContent() {
    let browser = this.selectedBrowser;
    if (browser) {
      let scroll = Browser.getScrollboxPosition(Browser.contentScrollboxScroller);
      let windowUtils = BrowserView.Util.getBrowserDOMWindowUtils(browser);
      browser.contentWindow.scrollTo(scroll.x, scroll.y);
    }
  },

  
  scrollContentToBrowser: function scrollContentToBrowser() {
    let pos = BrowserView.Util.getContentScrollOffset(this.selectedBrowser);
    if (pos.y != 0)
      Browser.hideTitlebar();

    Browser.contentScrollboxScroller.scrollTo(pos.x, pos.y);
    this._browserView.onAfterVisibleMove();
  },

  hideSidebars: function scrollSidebarsOffscreen() {
    let container = this.contentScrollbox;
    let rect = container.getBoundingClientRect();
    this.controlsScrollboxScroller.scrollBy(Math.round(rect.left), 0);
    this._browserView.onAfterVisibleMove();
  },

  hideTitlebar: function hideTitlebar() {
    let container = this.contentScrollbox;
    let rect = container.getBoundingClientRect();
    this.pageScrollboxScroller.scrollBy(0, Math.round(rect.top));
    this.tryUnfloatToolbar();
    this._browserView.onAfterVisibleMove();
  },

  


  get selectedBrowser() {
    return this._selectedTab.browser;
  },

  get tabs() {
    return this._tabs;
  },

  getTabForDocument: function(aDocument) {
    let tabs = this._tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser.contentDocument == aDocument)
        return tabs[i];
    }
    return null;
  },

  getTabAtIndex: function(index) {
    if (index > this._tabs.length || index < 0)
      return null;
    return this._tabs[index];
  },

  getTabFromChrome: function(chromeTab) {
    for (var t = 0; t < this._tabs.length; t++) {
      if (this._tabs[t].chromeTab == chromeTab)
        return this._tabs[t];
    }
    return null;
  },

  addTab: function(uri, bringFront) {
    let newTab = new Tab();
    this._tabs.push(newTab);

    if (bringFront)
      this.selectedTab = newTab;

    newTab.load(uri);

    let event = document.createEvent("Events");
    event.initEvent("TabOpen", true, false);
    newTab.chromeTab.dispatchEvent(event);

    return newTab;
  },

  closeTab: function(tab) {
    if (tab instanceof XULElement)
      tab = this.getTabFromChrome(tab);

    if (!tab)
      return;

    let tabIndex = this._tabs.indexOf(tab);

    let nextTab = this._selectedTab;
    if (this._selectedTab == tab) {
      nextTab = this.getTabAtIndex(tabIndex + 1) || this.getTabAtIndex(tabIndex - 1);
      if (!nextTab)
        return;
    }

    let event = document.createEvent("Events");
    event.initEvent("TabClose", true, false);
    tab.chromeTab.dispatchEvent(event);

    this.selectedTab = nextTab;

    tab.destroy();
    this._tabs.splice(tabIndex, 1);
  },

  get selectedTab() {
    return this._selectedTab;
  },

  set selectedTab(tab) {
    let bv = this._browserView;

    if (tab instanceof XULElement)
      tab = this.getTabFromChrome(tab);

    if (!tab || this._selectedTab == tab)
      return;

    if (this._selectedTab) {
      this._selectedTab.contentScrollOffset = this.getScrollboxPosition(this.contentScrollboxScroller);
      this._selectedTab.pageScrollOffset = this.getScrollboxPosition(this.pageScrollboxScroller);
    }

    let isFirstTab = this._selectedTab == null;
    let lastTab = this._selectedTab;
    this._selectedTab = tab;

    tab.ensureBrowserExists();

    bv.beginBatchOperation();

    bv.setBrowser(tab.browser, tab.browserViewportState);
    bv.forceContainerResize();
    bv.updateDefaultZoom();

    document.getElementById("tabs").selectedTab = tab.chromeTab;

    if (!isFirstTab) {
      
      BrowserUI.updateURI();
      getIdentityHandler().checkIdentity();

      let event = document.createEvent("Events");
      event.initEvent("TabSelect", true, false);
      event.lastTab = lastTab;
      tab.chromeTab.dispatchEvent(event);
    }

    tab.lastSelected = Date.now();

    
    if (tab.contentScrollOffset) {
      let { x: scrollX, y: scrollY } = tab.contentScrollOffset;
      Browser.contentScrollboxScroller.scrollTo(scrollX, scrollY);
    }
    if (tab.pageScrollOffset) {
      let { x: pageScrollX, y: pageScrollY } = tab.pageScrollOffset;
      Browser.pageScrollboxScroller.scrollTo(pageScrollX, pageScrollY);
    }

    bv.setAggressive(!tab._loading);

    bv.commitBatchOperation();
  },

  supportsCommand: function(cmd) {
    var isSupported = false;
    switch (cmd) {
      case "cmd_fullscreen":
        isSupported = true;
        break;
      default:
        isSupported = false;
        break;
    }
    return isSupported;
  },

  isCommandEnabled: function(cmd) {
    return true;
  },

  doCommand: function(cmd) {
    switch (cmd) {
      case "cmd_fullscreen":
        window.fullScreen = !window.fullScreen;
        break;
    }
  },

  getNotificationBox: function getNotificationBox() {
    return document.getElementById("notifications");
  },

  removeTransientNotificationsForTab: function removeTransientNotificationsForTab(aTab) {
    let notificationBox = this.getNotificationBox();
    let notifications = notificationBox.allNotifications;
    for (let n = notifications.length - 1; n >= 0; n--) {
      let notification = notifications[n];
      if (notification._chromeTab != aTab.chromeTab)
        continue;

      if (notification.persistence)
        notification.persistence--;
      else if (Date.now() > notification.timeout)
        notificationBox.removeNotification(notification);
    }
  },

  
  sacrificeTab: function sacrificeTab() {
    let tabToClear = this._tabs.reduce(function(prevTab, currentTab) {
      if (currentTab == Browser.selectedTab || !currentTab.browser) {
        return prevTab;
      } else {
        return (prevTab && prevTab.lastSelected <= currentTab.lastSelected) ? prevTab : currentTab;
      }
    }, null);

    if (tabToClear) {
      tabToClear.saveState();
      tabToClear._destroyBrowser();
      return true;
    } else {
      return false;
    }
  },

  





  _handleContentCommand: function _handleContentCommand(aEvent) {
    
    if (!aEvent.isTrusted)
      return;

    var ot = aEvent.originalTarget;
    var errorDoc = ot.ownerDocument;

    
    
    if (/^about:certerror\?e=nssBadCert/.test(errorDoc.documentURI)) {
      if (ot == errorDoc.getElementById("temporaryExceptionButton") ||
          ot == errorDoc.getElementById("permanentExceptionButton")) {
        try {
          
          let uri = gIOService.newURI(errorDoc.location.href, null, null);
          let sslExceptions = new SSLExceptions();

          if (ot == errorDoc.getElementById("permanentExceptionButton")) {
            sslExceptions.addPermanentException(uri);
          } else {
            sslExceptions.addTemporaryException(uri);
          }
        } catch (e) {
          dump("EXCEPTION handle content command: " + e + "\n" );
        }

        
        errorDoc.location.reload();
      }
      else if (ot == errorDoc.getElementById('getMeOutOfHereButton')) {
        
        var defaultPrefs = Cc["@mozilla.org/preferences-service;1"]
                          .getService(Ci.nsIPrefService).getDefaultBranch(null);
        var url = "about:blank";
        try {
          url = defaultPrefs.getCharPref("browser.startup.homepage");
          
          if (url.indexOf("|") != -1)
            url = url.split("|")[0];
        } catch (e) {  }

        Browser.selectedBrowser.loadURI(url, null, null, false);
      }
    }
    else if (/^about:neterror\?e=netOffline/.test(errorDoc.documentURI)) {
      if (ot == errorDoc.getElementById("errorTryAgain")) {
        
        Util.forceOnline();
      }
    }
  },

  







  computeSidebarVisibility: function computeSidebarVisibility(dx, dy) {
    function visibility(bar, visrect) {
      let w = bar.width;
      bar.restrictTo(visrect);
      return bar.width / w;
    }

    if (!dx) dx = 0;
    if (!dy) dy = 0;

    let leftbarCBR = document.getElementById('tabs-container').getBoundingClientRect();
    let ritebarCBR = document.getElementById('browser-controls').getBoundingClientRect();

    let leftbar = new Rect(Math.round(leftbarCBR.left) - dx, 0, Math.round(leftbarCBR.width), 1);
    let ritebar = new Rect(Math.round(ritebarCBR.left) - dx, 0, Math.round(ritebarCBR.width), 1);
    let leftw = leftbar.width;
    let ritew = ritebar.width;

    let visrect = new Rect(0, 0, window.innerWidth, 1);

    let leftvis = visibility(leftbar, visrect);
    let ritevis = visibility(ritebar, visrect);

    return [leftvis, ritevis, leftw, ritew];
  },

  














  snapSidebars: function snapSidebars() {
    let [leftvis, ritevis, leftw, ritew] = Browser.computeSidebarVisibility();

    let snappedX = 0;

    if (leftvis != 0 && leftvis != 1) {
      if (leftvis >= 0.6666) {
        snappedX = -((1 - leftvis) * leftw);
      } else {
        snappedX = leftvis * leftw;
      }
    }
    else if (ritevis != 0 && ritevis != 1) {
      if (ritevis >= 0.6666) {
        snappedX = (1 - ritevis) * ritew;
      } else {
        snappedX = -ritevis * ritew;
      }
    }

    return Math.round(snappedX);
  },

  tryFloatToolbar: function tryFloatToolbar(dx, dy) {
    if (this.floatedWhileDragging)
      return;

    let [leftvis, ritevis, leftw, ritew] = Browser.computeSidebarVisibility(dx, dy);
    if (leftvis > 0 || ritevis > 0) {
      BrowserUI.lockToolbar();
      this.floatedWhileDragging = true;
    }
  },

  tryUnfloatToolbar: function tryUnfloatToolbar(dx, dy) {
    if (!this.floatedWhileDragging)
      return true;

    let [leftvis, ritevis, leftw, ritew] = Browser.computeSidebarVisibility(dx, dy);
    if (leftvis == 0 && ritevis == 0) {
      BrowserUI.unlockToolbar();
      this.floatedWhileDragging = false;
      return true;
    }
    return false;
  },

  
  zoom: function zoom(aDirection) {
    let bv = this._browserView;
    let zoomLevel = bv.getZoomLevel();

    let zoomValues = ZoomManager.zoomValues;
    var i = zoomValues.indexOf(ZoomManager.snap(zoomLevel)) + (aDirection < 0 ? 1 : -1);
    if (i >= 0 && i < zoomValues.length)
      zoomLevel = zoomValues[i];

    zoomLevel = Math.max(zoomLevel, bv.getPageZoomLevel());

    let center = this.getVisibleRect().center().map(bv.viewportToBrowser);
    this.setVisibleRect(this._getZoomRectForPoint(center.x, center.y, zoomLevel));
  },

  


  _getZoomLevelForElement: function _getZoomLevelForElement(element) {
    const margin = 15;

    let bv = this._browserView;
    let elRect = bv.browserToViewportRect(Browser.getBoundingContentRect(element));

    let vis = bv.getVisibleRect();
    return BrowserView.Util.clampZoomLevel(bv.getZoomLevel() * vis.width / (elRect.width + margin * 2));
  },

  



  _getZoomRectForElement: function _getZoomRectForElement(element, elementY) {
    let bv = this._browserView;
    let oldZoomLevel = bv.getZoomLevel();
    let zoomLevel = this._getZoomLevelForElement(element);
    let zoomRatio = oldZoomLevel / zoomLevel;

    
    
    
    let zoomTolerance = (bv.isDefaultZoom()) ? .9 : .6666;
    if (zoomRatio >= zoomTolerance) {
      return null;
    } else {
      let elRect = this.getBoundingContentRect(element);
      return this._getZoomRectForPoint(elRect.center().x, elementY, zoomLevel);
    }
  },

  



  _getZoomRectForPoint: function _getZoomRectForPoint(x, y, zoomLevel) {
    let bv = this._browserView;
    let vis = bv.getVisibleRect();
    x = bv.browserToViewport(x);
    y = bv.browserToViewport(y);

    zoomLevel = Math.min(ZoomManager.MAX, zoomLevel);
    let zoomRatio = zoomLevel / bv.getZoomLevel();
    let newVisW = vis.width / zoomRatio, newVisH = vis.height / zoomRatio;
    let result = new Rect(x - newVisW / 2, y - newVisH / 2, newVisW, newVisH);

    
    return result.translateInside(bv._browserViewportState.viewportRect);
  },

  setVisibleRect: function setVisibleRect(rect) {
    let bv = this._browserView;
    let vis = bv.getVisibleRect();
    let zoomRatio = vis.width / rect.width;
    let zoomLevel = bv.getZoomLevel() * zoomRatio;
    let scrollX = rect.left * zoomRatio;
    let scrollY = rect.top * zoomRatio;

    
    

    
    bv.beginOffscreenOperation(rect);

    
    
    bv.beginBatchOperation();

    
    this.hideSidebars();
    this.hideTitlebar();

    bv.setZoomLevel(zoomLevel);

    
    bv.forceContainerResize();
    this.forceChromeReflow();
    this.contentScrollboxScroller.scrollTo(scrollX, scrollY);
    bv.onAfterVisibleMove();

    
    
    bv.forceViewportChange();

    bv.commitBatchOperation();
    bv.commitOffscreenOperation();
  },

  zoomToPoint: function zoomToPoint(cX, cY) {
    let [elementX, elementY] = this.transformClientToBrowser(cX, cY);
    let zoomRect = null;
    let element = this.elementFromPoint(elementX, elementY);
    let bv = this._browserView;
    if (element)
      zoomRect = this._getZoomRectForElement(element, elementY);
    if (!zoomRect && bv.isDefaultZoom())
      zoomRect = this._getZoomRectForPoint(elementX, elementY, bv.getZoomLevel() * 2);

    if (zoomRect) {
      this.setVisibleRect(zoomRect);
      return true;
    } else {
      return false;
    }
  },

  zoomFromPoint: function zoomFromPoint(cX, cY) {
    let bv = this._browserView;
    if (!bv.isDefaultZoom()) {
      let zoomLevel = bv.getDefaultZoomLevel();
      let [elementX, elementY] = this.transformClientToBrowser(cX, cY);
      let zoomRect = this._getZoomRectForPoint(elementX, elementY, zoomLevel);
      this.setVisibleRect(zoomRect);
    }
  },

  getContentClientRects: function getContentClientRects(contentElem) {
    
    let browser = Browser._browserView.getBrowser();

    if (!browser)
      return null;

    let offset = BrowserView.Util.getContentScrollOffset(browser);
    let nativeRects = contentElem.getClientRects();

    
    let rect;
    let cw = browser.contentWindow;
    for (let frame = contentElem.ownerDocument.defaultView; frame != cw; frame = frame.parent) {
      
      rect = frame.frameElement.getBoundingClientRect();
      offset.add(rect.left, rect.top);
    }

    let result = [];
    let r;
    for (let i = nativeRects.length - 1; i >= 0; i--) {
      r = nativeRects[i];
      result.push(new Rect(r.left + offset.x, r.top + offset.y, r.width, r.height));
    }
    return result;
  },

  getBoundingContentRect: function getBoundingContentRect(contentElem) {
    let document = contentElem.ownerDocument;
    while(document.defaultView.frameElement)
      document = document.defaultView.frameElement.ownerDocument;

    let tab = Browser.getTabForDocument(document);
    if (!tab || !tab.browser)
      return null;

    let browser = tab.browser;
    let offset = BrowserView.Util.getContentScrollOffset(browser);

    let r = contentElem.getBoundingClientRect();

    
    for (let frame = contentElem.ownerDocument.defaultView; frame != browser.contentWindow; frame = frame.parent) {
      
      let rect = frame.frameElement.getBoundingClientRect();
      offset.add(rect.left, rect.top);
    }

    return new Rect(r.left + offset.x, r.top + offset.y, r.width, r.height);
  },

  


  clientToBrowserView: function clientToBrowserView(x, y) {
    let container = document.getElementById("tile-container");
    let containerBCR = container.getBoundingClientRect();

    let x0 = Math.round(containerBCR.left);
    let y0;
    if (arguments.length > 1)
      y0 = Math.round(containerBCR.top);

    return (arguments.length > 1) ? [x - x0, y - y0] : (x - x0);
  },

  browserViewToClient: function browserViewToClient(x, y) {
    let container = document.getElementById("tile-container");
    let containerBCR = container.getBoundingClientRect();

    let x0 = Math.round(-containerBCR.left);
    let y0;
    if (arguments.length > 1)
      y0 = Math.round(-containerBCR.top);

    return (arguments.length > 1) ? [x - x0, y - y0] : (x - x0);
  },

  browserViewToClientRect: function browserViewToClientRect(rect) {
    let container = document.getElementById("tile-container");
    let containerBCR = container.getBoundingClientRect();
    return rect.clone().translate(Math.round(containerBCR.left), Math.round(containerBCR.top));
  },

  



  transformClientToBrowser: function transformClientToBrowser(cX, cY) {
    return this.clientToBrowserView(cX, cY).map(this._browserView.viewportToBrowser);
  },

  



  elementFromPoint: function elementFromPoint(x, y) {
    let browser = this._browserView.getBrowser();
    if (!browser) return null;

    
    
    let cwu = BrowserView.Util.getBrowserDOMWindowUtils(browser);
    let scrollX = {}, scrollY = {};
    cwu.getScrollXY(false, scrollX, scrollY);
    x = x - scrollX.value;
    y = y - scrollY.value;
    let elem = ElementTouchHelper.getClosest(cwu, x, y);

    
    while (elem && (elem instanceof HTMLIFrameElement || elem instanceof HTMLFrameElement)) {
      
      let rect = elem.getBoundingClientRect();
      x = x - rect.left;
      y = y - rect.top;
      let windowUtils = elem.contentDocument.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
      elem = ElementTouchHelper.getClosest(windowUtils, x, y);
    }

    return elem;
  },

  



  getVisibleRect: function getVisibleRect() {
    let stack = document.getElementById("tile-stack");
    let container = document.getElementById("tile-container");
    let containerBCR = container.getBoundingClientRect();

    let x = Math.round(-containerBCR.left);
    let y = Math.round(-containerBCR.top);
    let w = window.innerWidth;
    let h = stack.getBoundingClientRect().height;

    return new Rect(x, y, w, h);
  },

  






  getScrollboxPosition: function getScrollboxPosition(scroller) {
    let x = {};
    let y = {};
    scroller.getPosition(x, y);
    return new Point(x.value, y.value);
  },

  forceChromeReflow: function forceChromeReflow() {
    let dummy = getComputedStyle(document.documentElement, "").width;
  }
};

Browser.MainDragger = function MainDragger(browserView) {
  this.bv = browserView;
  this.draggedFrame = null;
  this.contentScrollbox = null;
};

Browser.MainDragger.prototype = {
  isDraggable: function isDraggable(target, scroller) { return true; },

  dragStart: function dragStart(clientX, clientY, target, scroller) {
    
    this.bv.pauseRendering();

    
    
    
    
    this.bv._idleServiceObserver.pause();

    let [x, y] = Browser.transformClientToBrowser(clientX, clientY);
    let element = Browser.elementFromPoint(x, y);

    this.draggedFrame = null;
    this.contentScrollbox = null;

    
    let htmlElement = element;
    if (htmlElement && htmlElement instanceof HTMLElement) {
      let win = htmlElement.ownerDocument.defaultView;
      for (; htmlElement; htmlElement = htmlElement.parentNode) {
        try {
          let cs = win.getComputedStyle(htmlElement, null);
          let overflowX = cs.getPropertyValue("overflow-x");
          let overflowY = cs.getPropertyValue("overflow-y");

          let scrollableY = overflowY != "hidden" && overflowY != "visible" && htmlElement.offsetHeight < htmlElement.scrollHeight;
          let scrollableX = overflowX != "hidden" && overflowX != "visible" && htmlElement.offsetWidth  < htmlElement.scrollWidth
            && !(htmlElement instanceof HTMLSelectElement); 

          if (scrollableX || scrollableY) {
            this.contentScrollbox = this._createDivScrollBox(htmlElement);
            return;
          }
        } catch(e) {}
      }
    }

    
    let xulElement = element;
    if (xulElement && xulElement instanceof XULElement) {
      for (; xulElement; xulElement = xulElement.parentNode) {
        if (xulElement.localName == "treechildren") {
          this.contentScrollbox = this._createTreeScrollBox(xulElement.parentNode);
          return;
        }
        let wrapper = xulElement.wrappedJSObject;
        let scrollable = false;
        try {
          scrollable = (wrapper.scrollBoxObject != null) || (wrapper.boxObject.QueryInterface(Ci.nsIScrollBoxObject));
        } catch(e) {}
        if (scrollable) {
          this.contentScrollbox = wrapper.scrollBoxObject || wrapper.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
          return;
        }
      }
    }

    if (element)
      this.draggedFrame = element.ownerDocument.defaultView;
  },

  dragStop: function dragStop(dx, dy, scroller) {
    this.draggedFrame = null;
    this.dragMove(Browser.snapSidebars(), 0, scroller);

    Browser.tryUnfloatToolbar();

    this.bv.resumeRendering();

    
    this.bv._idleServiceObserver.resume();
  },

  dragMove: function dragMove(dx, dy, scroller) {
    let elem = this.draggedFrame;
    let doffset = new Point(dx, dy);
    let render = false;

    
    let panOffset = this._panControlsAwayOffset(doffset);

    
    if (this.contentScrollbox && !doffset.isZero()) {
      this._panScrollbox(this.contentScrollbox, doffset);
      render = true;
    }

    
    if (elem) {
      while (elem.frameElement && !doffset.isZero()) {
        this._panFrame(elem, doffset);
        elem = elem.frameElement;
        render = true;
      }
    }

    
    this._panScroller(Browser.contentScrollboxScroller, doffset);

    
    
    doffset.add(panOffset);
    Browser.tryFloatToolbar(doffset.x, 0);
    this._panScroller(Browser.controlsScrollboxScroller, doffset);
    this._panScroller(Browser.pageScrollboxScroller, doffset);

    this.bv.onAfterVisibleMove();

    if (render)
      this.bv.renderNow();

    return !doffset.equals(dx, dy);
  },

  


  _createDivScrollBox: function(div) {
    let sbo = {
      getScrolledSize: function(width, height) {
        width.value = div.scrollWidth;
        height.value = div.scrollHeight;
      },
  
      getPosition: function(x, y) {
        x.value = div.scrollLeft;
        y.value = div.scrollTop;
      },
  
      scrollBy: function(dx, dy) {
        div.scrollTop += dy;
        div.scrollLeft += dx;
      }
   }
   return sbo;
  },

 


  _createTreeScrollBox: function(tree) {
    let treeBox = tree.boxObject.QueryInterface(Ci.nsITreeBoxObject);
    let sbo = {
      pageLength: treeBox.getPageLength(),
      rowHeight: treeBox.rowHeight,
      rowWidth: treeBox.rowWidth,
      rowCount: treeBox.view.rowCount,
      targetY: treeBox.getFirstVisibleRow() * treeBox.rowHeight,
      getScrolledSize: function(width, height) {
        width.value = this.rowWidth;
        height.value = this.rowHeight * this.rowCount;
      },

      getPosition: function(x, y) {
        x.value = treeBox.horizontalPosition;
        y.value = this.targetY;
      },

      scrollBy: function(dx, dy) {
        this.targetY += dy;
        if (this.targetY < 0)
          this.targetY = 0;
        let targetRow = Math.floor(this.targetY / this.rowHeight);
        if ((targetRow + this.pageLength) > this.rowCount) {
          targetRow = this.rowCount - this.pageLength;
          this.targetY = targetRow * this.rowHeight;
        }
        treeBox.scrollToRow(targetRow);
        treeBox.scrollToHorizontalPosition(treeBox.horizontalPosition + dx);
      }
    }
    return sbo;
  },

  


  _panScrollbox: function(sbo, doffset) {
    let origX = {}, origY = {}, newX = {}, newY = {};

    sbo.getPosition(origX, origY);
    sbo.scrollBy(doffset.x, doffset.y);
    sbo.getPosition(newX, newY);

    doffset.subtract(newX.value - origX.value, newY.value - origY.value);
  },

  
  _panControlsAwayOffset: function(doffset) {
    let x = 0, y = 0, rect;

    rect = Rect.fromRect(Browser.pageScrollbox.getBoundingClientRect()).map(Math.round);
    if (doffset.x < 0 && rect.right < window.innerWidth)
      x = Math.max(doffset.x, rect.right - window.innerWidth);
    if (doffset.x > 0 && rect.left > 0)
      x = Math.min(doffset.x, rect.left);

    let height = document.getElementById("tile-stack").getBoundingClientRect().height;
    rect = Rect.fromRect(Browser.contentScrollbox.getBoundingClientRect()).map(Math.round);
    if (doffset.y < 0 && rect.bottom < height)
      y = Math.max(doffset.y, rect.bottom - height);
    if (doffset.y > 0 && rect.top > 0)
      y = Math.min(doffset.y, rect.top);

    doffset.subtract(x, y);
    return new Point(x, y);
  },

  
  _panScroller: function _panScroller(scroller, doffset) {
    let { x: x0, y: y0 } = Browser.getScrollboxPosition(scroller);
    scroller.scrollBy(doffset.x, doffset.y);
    let { x: x1, y: y1 } = Browser.getScrollboxPosition(scroller);
    doffset.subtract(x1 - x0, y1 - y0);
  },

  
  _panFrame: function _panFrame(frame, doffset) {
    let origX = {}, origY = {}, newX = {}, newY = {};
    let windowUtils = frame.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);

    windowUtils.getScrollXY(false, origX, origY);
    frame.scrollBy(doffset.x, doffset.y);
    windowUtils.getScrollXY(false, newX, newY);

    doffset.subtract(newX.value - origX.value, newY.value - origY.value);
  }
};

function nsBrowserAccess()
{
}

nsBrowserAccess.prototype = {
  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIBrowserDOMWindow) || aIID.equals(Ci.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },

  openURI: function(aURI, aOpener, aWhere, aContext) {
    var isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);
    if (isExternal && aURI && aURI.schemeIs("chrome")) {
      
      return null;
    }

    var loadflags = isExternal ?
                       Ci.nsIWebNavigation.LOAD_FLAGS_FROM_EXTERNAL :
                       Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    var location;
    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW) {
      switch (aContext) {
        case Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL :
          aWhere = gPrefService.getIntPref("browser.link.open_external");
          break;
        default : 
          aWhere = gPrefService.getIntPref("browser.link.open_newwindow");
      }
    }

    var newWindow;
    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW) {
      var url = aURI ? aURI.spec : "about:blank";
      newWindow = openDialog("chrome://browser/content/browser.xul", "_blank",
                             "all,dialog=no", url, null, null, null);
    } else {
      if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_NEWTAB)
        newWindow = Browser.addTab("about:blank", true).browser.contentWindow;
      else
        newWindow = aOpener ? aOpener.top : browser.contentWindow;
    }

    try {
      var referrer;
      if (aURI) {
        if (aOpener) {
          location = aOpener.location;
          referrer = gIOService.newURI(location, null, null);
        }
        newWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIWebNavigation)
                 .loadURI(aURI.spec, loadflags, referrer, null, null);
      }
      newWindow.focus();
    } catch(e) { }

    return newWindow;
  },

  isTabContentWindow: function(aWindow) {
    return Browser.browsers.some(function (browser) browser.contentWindow == aWindow);
  }
};

const BrowserSearch = {
  engines: null,
  _allEngines: [],

  observe: function bs_observe(aSubject, aTopic, aData) {
    if (aTopic != "browser-search-engine-modified")
      return;

    switch (aData) {
      case "engine-added":
      case "engine-removed":
        
        
        if (ExtensionsView._list)
          ExtensionsView.getAddonsFromLocal();

        
      case "engine-changed":
        
        
        

        
        this._engines = null;
        break;
      case "engine-current":
        
        break;
    }
  },

  get _currentEngines() {
    let doc = getBrowser().contentDocument;
    return this._allEngines.filter(function(element) element.doc === doc, this);
  },

  get searchService() {
    delete this.searchService;
    return this.searchService = Cc["@mozilla.org/browser/search-service;1"].getService(Ci.nsIBrowserSearchService);
  },

  get engines() {
    if (this._engines)
      return this._engines;
    return this._engines = this.searchService.getVisibleEngines({ });
  },

  addPageSearchEngine: function (aEngine, aDocument) {
    
    let browsers = Browser.browsers;
    this._allEngines = this._allEngines.filter(function(element) {
       return browsers.some(function (browser) browser.contentDocument == element.doc);
    }, this);

    
    if (!this._allEngines.some(function (e) {
        return (e.engine.title == aEngine.title) && (e.doc == aDocument);
    })) this._allEngines.push( {engine:aEngine, doc:aDocument});
  },

  updatePageSearchEngines: function() {
    PageActions.removeItems("search");

    
    
    let newEngines = this._currentEngines.filter(function(element) {
      return !this.engines.some(function (e) e.name == element.engine.title);
    }, this);

    if (newEngines.length == 0)
      return;

    
    let kMaxSearchEngine = 1;
    for (let i = 0; i < kMaxSearchEngine; i++) {
      let engine = newEngines[i].engine;
      let item = PageActions.appendItem("search",
                                        Elements.browserBundle.getString("pageactions.search.addNew"),
                                        engine.title);

      item.engine = engine;
      item.onclick = function() {
        BrowserSearch.addPermanentSearchEngine(item.engine);
        PageActions.removeItem(item);
      };
    }
  },

  addPermanentSearchEngine: function (aEngine) {
    let iconURL = BrowserUI._favicon.src;
    this.searchService.addEngine(aEngine.href, Ci.nsISearchEngine.DATA_XML, iconURL, false);

    this._engines = null;
  },

  updateSearchButtons: function() {
    let container = document.getElementById("search-buttons");
    if (this._engines && container.hasChildNodes())
      return;

    
    while (container.hasChildNodes())
      container.removeChild(container.lastChild);

    let engines = this.engines;
    for (let e = 0; e < engines.length; e++) {
      let button = document.createElement("radio");
      let engine = engines[e];
      button.id = engine.name;
      button.setAttribute("label", engine.name);
      button.className = "searchengine";
      if (engine.iconURI)
        button.setAttribute("src", engine.iconURI.spec);
      container.appendChild(button);
      button.engine = engine;
    }
  }
}


function ContentCustomClicker(browserView) {
  this._browserView = browserView;
  this._overlay = document.getElementById("content-overlay");
  this._overlayTimeout = 0;
  this._width = 0;
  this._height = 0;
}

ContentCustomClicker.prototype = {
    
    _dispatchMouseEvent: function _dispatchMouseEvent(element, name, cX, cY) {
      let browser = this._browserView.getBrowser();
      if (browser) {
        let [x, y] = Browser.transformClientToBrowser(cX, cY);
        let cwu = BrowserView.Util.getBrowserDOMWindowUtils(browser);

        
        let rect = element.getBoundingClientRect();
        if (cwu.nodesFromRect && (element.ownerDocument.defaultView.frameElement ||
                                 (x < rect.left || (x > rect.left + rect.width) ||
                                 (y < rect.top || y > rect.top + rect.height)))) {

            let rect = Browser.getBoundingContentRect(element);
            x = rect.left + (rect.width / 2);
            y = rect.top + (rect.height / 2);
        }
        else {
          let scrollX = {}, scrollY = {};
          cwu.getScrollXY(false, scrollX, scrollY);
          x = x - scrollX.value;
          y = y - scrollY.value;
        }

        cwu.sendMouseEvent(name, x, y, 0, 1, 0, true);
      }
    },

    
    _getFocusable: function _getFocusable(node) {
      if (node && node.mozMatchesSelector("*:link,*:visited,*:link *,*:visited *,*[role=button],button,input,option,select,textarea,label"))
        return node;
      return null;
    },

    _showCanvas: function _showCanvas(cX, cY) {
      
      
      let bv = this._browserView;
      let overlay = this._overlay;
      let ctx = overlay.getContext("2d");
      let [elementX, elementY] = Browser.transformClientToBrowser(cX, cY);
      let element = this._getFocusable(Browser.elementFromPoint(elementX, elementY));
      if (!element)
        return;

      let rects = Browser.getContentClientRects(element);
      let union = rects.reduce(function(a, b) {
        return a.expandToContain(b);
      }, new Rect(0, 0, 0, 0)).map(bv.browserToViewport);

      let vis = Browser.getVisibleRect();
      let canvasArea = vis.intersect(union);
      this._ensureSize(canvasArea.width, canvasArea.height);

      ctx.save();
      ctx.translate(-canvasArea.left, -canvasArea.top);
      bv.browserToViewportCanvasContext(ctx);

      overlay.style.left = canvasArea.left + "px";
      overlay.style.top = canvasArea.top + "px";
      ctx.fillStyle = "rgba(0, 145, 255, .5)";
      let rect;
      let i;
      for (i = rects.length - 1; i >= 0; i--) {
        rect = rects[i];
        ctx.fillRect(rect.left, rect.top, rect.width, rect.height);
      }
      ctx.restore();
      overlay.style.display = "block";
    },

    
    _hideCanvas: function _hideCanvas() {
      let overlay = this._overlay;
      overlay.style.display = "none";
      overlay.getContext("2d").clearRect(0, 0, this._width, this._height);

      if (this._overlayTimeout) {
        clearTimeout(this._overlayTimeout);
        this._overlayTimeout = 0;
      }
    },

    
    _ensureSize: function _ensureSize(width, height) {
      if (this._width <= width) {
        this._width = width;
        this._overlay.width = width;
      }
      if (this._height <= height) {
        this._height = height;
        this._overlay.height = height;
      }
    },

    mouseDown: function mouseDown(cX, cY) {
      if (!this._overlayTimeout)
        this._overlayTimeout = setTimeout(function(self) { self._showCanvas(cX, cY); }, kTapOverlayTimeout, this);
    },

    mouseUp: function mouseUp(cX, cY) {
    },

    panBegin: function panBegin() {
      this._hideCanvas();
    },

    singleClick: function singleClick(cX, cY, modifiers) {
      this._hideCanvas();

      let [elementX, elementY] = Browser.transformClientToBrowser(cX, cY);
      let element = Browser.elementFromPoint(elementX, elementY);
      if (modifiers == 0) {
        if (element instanceof HTMLOptionElement)
          element = element.parentNode;

        if (gPrefService.getBoolPref("formhelper.enabled")) {
          if (FormHelper.canShowUIFor(element) && FormHelper.open(element))
            return;
        }
        else if (SelectHelper.canShowUIFor(element)) {
          SelectHelper.show(element);
          return;
        }

        gFocusManager.setFocus(element, Ci.nsIFocusManager.FLAG_NOSCROLL);

        let self = this;
        Util.executeSoon(function() {
          self._dispatchMouseEvent(element, "mousedown", cX, cY);
          self._dispatchMouseEvent(element, "mouseup", cX, cY);
        });
      }
      else if (modifiers == Ci.nsIDOMNSEvent.CONTROL_MASK) {
        let uri = Util.getHrefForElement(element);
        if (uri)
          Browser.addTab(uri, false);
      }
    },

    doubleClick: function doubleClick(cX1, cY1, cX2, cY2) {
      this._hideCanvas();

      const kDoubleClickRadius = 32;

      let maxRadius = kDoubleClickRadius * Browser._browserView.getZoomLevel();
      let isClickInRadius = (Math.abs(cX1 - cX2) < maxRadius && Math.abs(cY1 - cY2) < maxRadius);
      if (isClickInRadius && !Browser.zoomToPoint(cX1, cY1))
        Browser.zoomFromPoint(cX1, cY1);
    },

    toString: function toString() {
      return "[ContentCustomClicker] { }";
    }
};


const ElementTouchHelper = {
  get radius() {
    delete this.radius;
    return this.radius = { "top": gPrefService.getIntPref("browser.ui.touch.top"),
                           "right": gPrefService.getIntPref("browser.ui.touch.right"),
                           "bottom": gPrefService.getIntPref("browser.ui.touch.bottom"),
                           "left": gPrefService.getIntPref("browser.ui.touch.left")
                         };
  },

  get weight() {
    delete this.weight;
    return this.weight = { "visited": gPrefService.getIntPref("browser.ui.touch.weight.visited")
                         };
  },

  
  getClosest: function getClosest(aWindowUtils, aX, aY) {
    let target = aWindowUtils.elementFromPoint(aX, aY,
                                               true,   
                                               false); 

    
    if (!aWindowUtils.nodesFromRect || this._isElementClickable(target))
      return target;

    let nodes = aWindowUtils.nodesFromRect(aX, aY, this.radius.bottom,
                                                   this.radius.right,
                                                   this.radius.top,
                                                   this.radius.left, true, false);

    let threshold = Number.POSITIVE_INFINITY;
    for (let i = 0; i < nodes.length; i++) {
      let current = nodes[i];
      if (!current.mozMatchesSelector || !this._isElementClickable(current))
        continue;

      let rect = current.getBoundingClientRect();
      let distance = this._computeDistanceFromRect(aX, aY, rect);

      
      if (current && current.mozMatchesSelector("*:visited"))
        distance *= (this.weight.visited / 100);

      if (distance < threshold) {
        target = current;
        threshold = distance;
      }
    }

    return target;
  },

  _isElementClickable: function _isElementClickable(aElement) {
    return aElement && aElement.mozMatchesSelector("*:link,*:visited,*[role=button],button,input,select,label");
  },

  _computeDistanceFromRect: function _computeDistanceFromRect(aX, aY, aRect) {
    let x = 0, y = 0;
    let xmost = aRect.left + aRect.width;
    let ymost = aRect.top + aRect.height;

    
    
    if (aRect.left < aX && aX < xmost)
      x = Math.min(xmost - aX, aX - aRect.left);
    else if (aX < aRect.left)
      x = aRect.left - aX;
    else if (aX > xmost)
      x = aX - xmost;

    
    
    if (aRect.top < aY && aY < ymost)
      y = Math.min(ymost - aY, aY - aRect.top);
    else if (aY < aRect.top)
      y = aRect.top - aY;
    if (aY > ymost)
      y = aY - ymost;

    return Math.sqrt(Math.pow(x, 2) + Math.pow(y, 2));
  }
};




function IdentityHandler() {
  this._staticStrings = {};
  this._staticStrings[this.IDENTITY_MODE_DOMAIN_VERIFIED] = {
    encryption_label: Elements.browserBundle.getString("identity.encrypted2")
  };
  this._staticStrings[this.IDENTITY_MODE_IDENTIFIED] = {
    encryption_label: Elements.browserBundle.getString("identity.encrypted2")
  };
  this._staticStrings[this.IDENTITY_MODE_UNKNOWN] = {
    encryption_label: Elements.browserBundle.getString("identity.unencrypted2")
  };

  this._cacheElements();
}

IdentityHandler.prototype = {

  
  IDENTITY_MODE_IDENTIFIED       : "verifiedIdentity", 
  IDENTITY_MODE_DOMAIN_VERIFIED  : "verifiedDomain",   
  IDENTITY_MODE_UNKNOWN          : "unknownIdentity",  

  
  _lastStatus : null,
  _lastLocation : null,

  


  _cacheElements: function() {
    this._identityBox = document.getElementById("identity-box");
    this._identityPopup = document.getElementById("identity-container");
    this._identityPopupContentBox = document.getElementById("identity-popup-content-box");
    this._identityPopupContentHost = document.getElementById("identity-popup-content-host");
    this._identityPopupContentOwner = document.getElementById("identity-popup-content-owner");
    this._identityPopupContentSupp = document.getElementById("identity-popup-content-supplemental");
    this._identityPopupContentVerif = document.getElementById("identity-popup-content-verifier");
    this._identityPopupEncLabel = document.getElementById("identity-popup-encryption-label");
  },

  



  getIdentityData: function() {
    var result = {};
    var status = this._lastStatus.QueryInterface(Ci.nsISSLStatus);
    var cert = status.serverCert;

    
    result.subjectOrg = cert.organization;

    
    if (cert.subjectName) {
      result.subjectNameFields = {};
      cert.subjectName.split(",").forEach(function(v) {
        var field = v.split("=");
        this[field[0]] = field[1];
      }, result.subjectNameFields);

      
      result.city = result.subjectNameFields.L;
      result.state = result.subjectNameFields.ST;
      result.country = result.subjectNameFields.C;
    }

    
    result.caOrg =  cert.issuerOrganization || cert.issuerCommonName;
    result.cert = cert;

    return result;
  },

  



  checkIdentity: function() {
    let state = Browser.selectedTab.getIdentityState();
    let location = getBrowser().contentWindow.location;
    let currentStatus = getBrowser().securityUI.QueryInterface(Ci.nsISSLStatusProvider).SSLStatus;

    this._lastStatus = currentStatus;
    this._lastLocation = {};
    try {
      
      this._lastLocation = { host: location.host, hostname: location.hostname, port: location.port };
    } catch (ex) { }

    if (state & Ci.nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL)
      this.setMode(this.IDENTITY_MODE_IDENTIFIED);
    else if (state & Ci.nsIWebProgressListener.STATE_SECURE_HIGH)
      this.setMode(this.IDENTITY_MODE_DOMAIN_VERIFIED);
    else
      this.setMode(this.IDENTITY_MODE_UNKNOWN);
  },

  


  getEffectiveHost: function() {
    
    if (!this._eTLDService)
      this._eTLDService = Cc["@mozilla.org/network/effective-tld-service;1"]
                         .getService(Ci.nsIEffectiveTLDService);
    try {
      return this._eTLDService.getBaseDomainFromHost(this._lastLocation.hostname);
    } catch (e) {
      
      
      return this._lastLocation.hostname;
    }
  },

  



  setMode: function(newMode) {
    this._identityBox.setAttribute("mode", newMode);
    this.setIdentityMessages(newMode);

    
    if (!this._identityPopup.hidden)
      this.setPopupMessages(newMode);
  },

  





  setIdentityMessages: function(newMode) {
    let strings = Elements.browserBundle;

    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();

      
      
      var lookupHost = this._lastLocation.host;
      if (lookupHost.indexOf(':') < 0)
        lookupHost += ":443";

      
      if (!this._overrideService)
        this._overrideService = Cc["@mozilla.org/security/certoverride;1"].getService(Ci.nsICertOverrideService);

      
      
      var tooltip = strings.getFormattedString("identity.identified.verifier",
                                               [iData.caOrg]);

      
      
      
      
      if (this._overrideService.hasMatchingOverride(this._lastLocation.hostname,
                                                    (this._lastLocation.port || 443),
                                                    iData.cert, {}, {}))
        tooltip = strings.getString("identity.identified.verified_by_you");
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
      tooltip = strings.getFormattedString("identity.identified.verifier",
                                           [iData.caOrg]);
    }
    else {
      tooltip = strings.getString("identity.unknown.tooltip");
    }

    
    this._identityBox.tooltipText = tooltip;
  },

  






  setPopupMessages: function(newMode) {
    this._identityPopup.setAttribute("mode", newMode);
    this._identityPopupContentBox.className = newMode;

    
    this._identityPopupEncLabel.textContent = this._staticStrings[newMode].encryption_label;

    
    var supplemental = "";
    var verifier = "";

    let strings = Elements.browserBundle;

    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();
      var host = this.getEffectiveHost();
      var owner = strings.getString("identity.ownerUnknown2");
      verifier = this._identityBox.tooltipText;
      supplemental = "";
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
      host = this.getEffectiveHost();
      owner = iData.subjectOrg;
      verifier = this._identityBox.tooltipText;

      
      if (iData.city)
        supplemental += iData.city + " ";
      if (iData.state && iData.country)
        supplemental += strings.getFormattedString("identity.identified.state_and_country",
                                                   [iData.state, iData.country]);
      else if (iData.state) 
        supplemental += iData.state;
      else if (iData.country) 
        supplemental += iData.country;
    }
    else {
      
      host = "";
      owner = "";
    }

    
    this._identityPopupContentHost.textContent = host;
    this._identityPopupContentOwner.textContent = owner;
    this._identityPopupContentSupp.textContent = supplemental;
    this._identityPopupContentVerif.textContent = verifier;

    
    BrowserSearch.updatePageSearchEngines();

    
    PageActions.updatePagePermissions();

    PageActions.updatePageSaveAs();
  },

  show: function ih_show() {
    
    while (BrowserUI.activeDialog)
      BrowserUI.activeDialog.close();

    this._identityPopup.hidden = false;
    this._identityPopup.top = BrowserUI.toolbarH;
    this._identityPopup.focus();

    this._identityBox.setAttribute("open", "true");

    
    this.setPopupMessages(this._identityBox.getAttribute("mode") || this.IDENTITY_MODE_UNKNOWN);

    BrowserUI.pushPopup(this, [this._identityPopup, this._identityBox]);
    BrowserUI.lockToolbar();
  },

  hide: function ih_hide() {
    this._identityPopup.hidden = true;
    this._identityBox.removeAttribute("open");

    BrowserUI.popPopup();
    BrowserUI.unlockToolbar();
  },

  


  handleIdentityButtonEvent: function(event) {
    event.stopPropagation();

    if ((event.type == "click" && event.button != 0) ||
        (event.type == "keypress" && event.charCode != KeyEvent.DOM_VK_SPACE &&
         event.keyCode != KeyEvent.DOM_VK_RETURN))
      return; 

    if (this._identityPopup.hidden)
      this.show();
    else
      this.hide();
  }
};

var gIdentityHandler;





function getIdentityHandler() {
  if (!gIdentityHandler)
    gIdentityHandler = new IdentityHandler();
  return gIdentityHandler;
}





const gPopupBlockerObserver = {
  _kIPM: Ci.nsIPermissionManager,

  onUpdatePageReport: function onUpdatePageReport(aEvent)
  {
    var cBrowser = Browser.selectedBrowser;
    if (aEvent.originalTarget != cBrowser)
      return;

    if (!cBrowser.pageReport)
      return;

    let pm = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
    let result = pm.testExactPermission(Browser.selectedBrowser.currentURI, "popup");
    if (result == Ci.nsIPermissionManager.DENY_ACTION)
      return;

    
    
    
    if (!cBrowser.pageReport.reported) {
      if(gPrefService.getBoolPref("privacy.popups.showBrowserMessage")) {
        var brandBundle = document.getElementById("bundle_brand");
        var brandShortName = brandBundle.getString("brandShortName");
        var message;
        var popupCount = cBrowser.pageReport.length;

        let strings = Elements.browserBundle;
        if (popupCount > 1)
          message = strings.getFormattedString("popupWarningMultiple", [brandShortName, popupCount]);
        else
          message = strings.getFormattedString("popupWarning", [brandShortName]);

        var notificationBox = Browser.getNotificationBox();
        var notification = notificationBox.getNotificationWithValue("popup-blocked");
        if (notification) {
          notification.label = message;
        }
        else {
          var buttons = [
            {
              label: strings.getString("popupButtonAllowOnce"),
              accessKey: null,
              callback: function() { gPopupBlockerObserver.showPopupsForSite(); }
            },
            {
              label: strings.getString("popupButtonAlwaysAllow2"),
              accessKey: null,
              callback: function() { gPopupBlockerObserver.allowPopupsForSite(); }
            },
            {
              label: strings.getString("popupButtonNeverWarn2"),
              accessKey: null,
              callback: function() { gPopupBlockerObserver.denyPopupsForSite(); }
            }
          ];

          const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
          notificationBox.appendNotification(message, "popup-blocked",
                                             "",
                                             priority, buttons);
        }
      }
      
      
      cBrowser.pageReport.reported = true;
    }
  },

  allowPopupsForSite: function allowPopupsForSite(aEvent) {
    var currentURI = Browser.selectedBrowser.currentURI;
    var pm = Cc["@mozilla.org/permissionmanager;1"].getService(this._kIPM);
    pm.add(currentURI, "popup", this._kIPM.ALLOW_ACTION);

    Browser.getNotificationBox().removeCurrentNotification();
  },

  denyPopupsForSite: function denyPopupsForSite(aEvent) {
    var currentURI = Browser.selectedBrowser.currentURI;
    var pm = Cc["@mozilla.org/permissionmanager;1"].getService(this._kIPM);
    pm.add(currentURI, "popup", this._kIPM.DENY_ACTION);

    Browser.getNotificationBox().removeCurrentNotification();
  },

  showPopupsForSite: function showPopupsForSite() {
    let uri = Browser.selectedBrowser.currentURI;
    let pageReport = Browser.selectedBrowser.pageReport;
    if (pageReport) {
      for (let i = 0; i < pageReport.length; ++i) {
        var popupURIspec = pageReport[i].popupWindowURI.spec;

        
        
        
        
        
        if (popupURIspec == "" || popupURIspec == "about:blank" ||
            popupURIspec == uri.spec)
          continue;

        let popupFeatures = pageReport[i].popupWindowFeatures;
        let popupName = pageReport[i].popupWindowName;

        Browser.addTab(popupURIspec, false);
      }
    }
  }
};

const gXPInstallObserver = {
  observe: function xpi_observer(aSubject, aTopic, aData)
  {
    var brandBundle = document.getElementById("bundle_brand");
    switch (aTopic) {
      case "xpinstall-install-blocked":
        var installInfo = aSubject.QueryInterface(Ci.nsIXPIInstallInfo);
        var host = installInfo.originatingURI.host;
        var brandShortName = brandBundle.getString("brandShortName");
        var notificationName, messageString, buttons;
        var strings = Elements.browserBundle;
        if (!gPrefService.getBoolPref("xpinstall.enabled")) {
          notificationName = "xpinstall-disabled";
          if (gPrefService.prefIsLocked("xpinstall.enabled")) {
            messageString = strings.getString("xpinstallDisabledMessageLocked");
            buttons = [];
          }
          else {
            messageString = strings.getFormattedString("xpinstallDisabledMessage",
                                                             [brandShortName, host]);
            buttons = [{
              label: strings.getString("xpinstallDisabledButton"),
              accessKey: null,
              popup: null,
              callback: function editPrefs() {
                gPrefService.setBoolPref("xpinstall.enabled", true);
                return false;
              }
            }];
          }
        }
        else {
          notificationName = "xpinstall";
          messageString = strings.getFormattedString("xpinstallPromptWarning",
                                                           [brandShortName, host]);

          buttons = [{
            label: strings.getString("xpinstallPromptAllowButton"),
            accessKey: null,
            popup: null,
            callback: function() {
              
              var mgr = Cc["@mozilla.org/xpinstall/install-manager;1"].createInstance(Ci.nsIXPInstallManager);
              mgr.initManagerWithInstallInfo(installInfo);
              return false;
            }
          }];
        }

        var nBox = Browser.getNotificationBox();
        if (!nBox.getNotificationWithValue(notificationName)) {
          const priority = nBox.PRIORITY_WARNING_MEDIUM;
          const iconURL = "chrome://mozapps/skin/update/update.png";
          nBox.appendNotification(messageString, notificationName, iconURL, priority, buttons);
        }
        break;
    }
  }
};

const gSessionHistoryObserver = {
  observe: function sho_observe(subject, topic, data) {
    if (topic != "browser:purge-session-history")
      return;

    let back = document.getElementById("cmd_back");
    back.setAttribute("disabled", "true");
    let forward = document.getElementById("cmd_forward");
    forward.setAttribute("disabled", "true");

    let urlbar = document.getElementById("urlbar-edit");
    if (urlbar) {
      
      urlbar.editor.transactionManager.clear();
    }
  }
};

var MemoryObserver = {
  observe: function mo_observe() {
    let memory = Cc["@mozilla.org/xpcom/memory-service;1"].getService(Ci.nsIMemory);
    do {
      Browser.windowUtils.garbageCollect();
    } while (memory.isLowMemory() && Browser.sacrificeTab());
  }
};

function getNotificationBox(aWindow) {
  return Browser.getNotificationBox();
}

function importDialog(parent, src, arguments) {
  
  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
  xhr.open("GET", src, false);
  xhr.overrideMimeType("text/xml");
  xhr.send(null);
  if (!xhr.responseXML)
    return null;

  let currentNode;
  let nodeIterator = xhr.responseXML.createNodeIterator(xhr.responseXML,
                                                    NodeFilter.SHOW_TEXT,
                                                    null,
                                                    false);
  while (currentNode = nodeIterator.nextNode()) {
    let trimmed = currentNode.nodeValue.replace(/^\s\s*/, "").replace(/\s\s*$/, "");
    if (!trimmed.length)
      currentNode.parentNode.removeChild(currentNode);
  }

  let doc = xhr.responseXML.documentElement;

  var dialog  = null;

  
  let selectContainer = document.getElementById("select-container");
  let parent = selectContainer.parentNode;

  
  let event = document.createEvent("Events");
  event.initEvent("DOMWillOpenModalDialog", true, false);
  let dispatcher = parent || getBrowser();
  dispatcher.dispatchEvent(event);

  
  let back = document.createElement("box");
  back.setAttribute("class", "modal-block");
  dialog = back.appendChild(document.importNode(doc, true));
  parent.insertBefore(back, selectContainer);

  dialog.arguments = arguments;
  dialog.parent = parent;
  return dialog;
}

function showDownloadManager(aWindowContext, aID, aReason) {
  BrowserUI.showPanel("downloads-container");
  
}

var AlertsHelper = {
  _timeoutID: -1,
  _listener: null,
  _cookie: "",
  _clickable: false,

  showAlertNotification: function ah_show(aImageURL, aTitle, aText, aTextClickable, aCookie, aListener) {
    this._clickable = aTextClickable || false;
    this._listener = aListener || null;
    this._cookie = aCookie || "";

    document.getElementById("alerts-image").setAttribute("src", aImageURL);
    document.getElementById("alerts-title").value = aTitle;
    document.getElementById("alerts-text").textContent = aText;

    let container = document.getElementById("alerts-container");
    container.hidden = false;

    let rect = container.getBoundingClientRect();
    container.top = window.innerHeight - (rect.height + 20);
    container.left = window.innerWidth - (rect.width + 20);

    let timeout = gPrefService.getIntPref("alerts.totalOpenTime");
    let self = this;
    this._timeoutID = setTimeout(function() { self._timeoutAlert(); }, timeout);
  },

  _timeoutAlert: function ah__timeoutAlert() {
    this._timeoutID = -1;
    let container = document.getElementById("alerts-container");
    container.hidden = true;

    if (this._listener)
      this._listener.observe(null, "alertfinished", this._cookie);

    
  },

  click: function ah_click(aEvent) {
    if (this._clickable && this._listener)
      this._listener.observe(null, "alertclickcallback", this._cookie);

    if (this._timeoutID != -1) {
      clearTimeout(this._timeoutID);
      this._timeoutAlert();
    }
  }
};

var HelperAppDialog = {
  _launcher: null,
  _container: null,

  show: function had_show(aLauncher) {
    this._launcher = aLauncher;
    document.getElementById("helperapp-target").value = this._launcher.suggestedFileName;

    if (!this._launcher.MIMEInfo.hasDefaultHandler)
      document.getElementById("helperapp-open").disabled = true;

    this._container = document.getElementById("helperapp-container");
    this._container.hidden = false;

    let rect = this._container.getBoundingClientRect();
    this._container.top = (window.innerHeight - rect.height) / 2;
    this._container.left = (window.innerWidth - rect.width) / 2;

    BrowserUI.pushPopup(this, this._container);
  },

  save: function had_save() {
    this._launcher.saveToDisk(null, false);
    this.hide();
  },

  open: function had_open() {
    this._launcher.launchWithApplication(null, false);
    this.hide();
  },

  hide: function had_hide() {
    document.getElementById("helperapp-target").value = "";
    this._container.hidden = true;

    BrowserUI.popPopup();
  }
};

function ProgressController(tab) {
  this._tab = tab;

  
  this.state = null;
  this._hostChanged = false; 
}

ProgressController.prototype = {
  get browser() {
    return this._tab.browser;
  },

  onStateChange: function onStateChange(aWebProgress, aRequest, aStateFlags, aStatus) {
    
    if (aWebProgress.DOMWindow != this._tab.browser.contentWindow)
      return;

    
    
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_START)
        this._networkStart();
      else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP)
        this._networkStop();
    }
    else if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
        this._documentStop();
      }
    }
  },

  
  onProgressChange: function onProgressChange(aWebProgress, aRequest, aCurSelf, aMaxSelf, aCurTotal, aMaxTotal) {
    
  },

  
  onLocationChange: function onLocationChange(aWebProgress, aRequest, aLocationURI) {
    
    if (aWebProgress.DOMWindow != this._tab.browser.contentWindow)
      return;

    let location = aLocationURI ? aLocationURI.spec : "";

    this._hostChanged = true;

    if (location != this.browser.lastSpec) {
      this.browser.lastSpec = this.browser.currentURI.spec;
      Browser.removeTransientNotificationsForTab(this._tab);
      this._tab.resetZoomLevel();

      if (this._tab == Browser.selectedTab) {
        BrowserUI.updateURI();

       
       
       Browser.scrollContentToTop();
      }
    }
  },

  



  onStatusChange: function onStatusChange(aWebProgress, aRequest, aStatus, aMessage) {
    
  },

  
  onSecurityChange: function onSecurityChange(aWebProgress, aRequest, aState) {
    
    if (this.state == aState && !this._hostChanged)
      return;

    this._hostChanged = false;
    this.state = aState;

    if (this._tab == Browser.selectedTab) {
      getIdentityHandler().checkIdentity();
    }
  },

  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIWebProgressListener) ||
        aIID.equals(Ci.nsISupportsWeakReference) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  _networkStart: function _networkStart() {
    this._tab.startLoading();

    if (this._tab == Browser.selectedTab) {
      BrowserUI.update(TOOLBARSTATE_LOADING);

      
      
      if (this._tab.browser.currentURI.spec == "about:blank")
        BrowserUI.updateURI();
    }

    
    let event = document.createEvent("Events");
    event.initEvent("URLChanged", true, false);
    this.browser.dispatchEvent(event);
  },

  _networkStop: function _networkStop() {
    this._tab.endLoading();

    if (this._tab == Browser.selectedTab) {
      BrowserUI.update(TOOLBARSTATE_LOADED);
      this.browser.docShell.isOffScreenBrowser = true;
    }

    if (this.browser.currentURI.spec != "about:blank")
      this._tab.updateThumbnail();
  },

  _documentStop: function _documentStop() {
    if (this._tab == Browser.selectedTab) {
      
      
      
      Util.executeSoon(function() {
        let scroll = Browser.getScrollboxPosition(Browser.contentScrollboxScroller);
        if (scroll.isZero())
          Browser.scrollContentToBrowser();
      });
    }
    else {
      let scroll = BrowserView.Util.getContentScrollOffset(this._tab.browser);
      this._tab.contentScrollOffset = new Point(scroll.x, scroll.y);
    }
  }
};

var OfflineApps = {
  get _pm() {
    delete this._pm;
    return this._pm = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
  },

  offlineAppRequested: function(aDocument) {
    if (!gPrefService.getBoolPref("browser.offline-apps.notify"))
      return;

    let currentURI = aDocument.documentURIObject;

    
    if (this._pm.testExactPermission(currentURI, "offline-app") != Ci.nsIPermissionManager.UNKNOWN_ACTION)
      return;

    try {
      if (gPrefService.getBoolPref("offline-apps.allow_by_default")) {
        
        return;
      }
    } catch(e) {
      
    }

    let host = currentURI.asciiHost;
    let notificationID = "offline-app-requested-" + host;
    let notificationBox = Browser.getNotificationBox();

    let notification = notificationBox.getNotificationWithValue(notificationID);
    let strings = Elements.browserBundle;
    if (notification) {
      notification.documents.push(aDocument);
    } else {
      let buttons = [{
        label: strings.getString("offlineApps.allow"),
        accessKey: null,
        callback: function() {
          for (let i = 0; i < notification.documents.length; i++)
            OfflineApps.allowSite(notification.documents[i]);
        }
      },{
        label: strings.getString("offlineApps.never"),
        accessKey: null,
        callback: function() {
          for (let i = 0; i < notification.documents.length; i++)
            OfflineApps.disallowSite(notification.documents[i]);
        }
      },{
        label: strings.getString("offlineApps.notNow"),
        accessKey: null,
        callback: function() {  }
      }];

      const priority = notificationBox.PRIORITY_INFO_LOW;
      let message = strings.getFormattedString("offlineApps.available", [host]);
      notification = notificationBox.appendNotification(message, notificationID,
                                                        "", priority, buttons);
      notification.documents = [aDocument];
    }
  },

  allowSite: function(aDocument) {
    this._pm.add(aDocument.documentURIObject, "offline-app", Ci.nsIPermissionManager.ALLOW_ACTION);

    
    
    this._startFetching(aDocument);
  },

  disallowSite: function(aDocument) {
    this._pm.add(aDocument.documentURIObject, "offline-app", Ci.nsIPermissionManager.DENY_ACTION);
  },

  _startFetching: function(aDocument) {
    if (!aDocument.documentElement)
      return;

    let manifest = aDocument.documentElement.getAttribute("manifest");
    if (!manifest)
      return;

    let manifestURI = gIOService.newURI(manifest, aDocument.characterSet, aDocument.documentURIObject);

    let updateService = Cc["@mozilla.org/offlinecacheupdate-service;1"].getService(Ci.nsIOfflineCacheUpdateService);
    updateService.scheduleUpdate(manifestURI, aDocument.documentURIObject);
  },

  handleEvent: function(aEvent) {
    if (aEvent.type == "MozApplicationManifest")
      this.offlineAppRequested(aEvent.originalTarget.defaultView.document);
  }
};

function Tab() {
  this._id = null;
  this._browser = null;
  this._browserViewportState = null;
  this._state = null;
  this._listener = null;
  this._loading = false;
  this._chromeTab = null;
  this._resizeAndPaint = Util.bind(this._resizeAndPaint, this);

  
  
  this.lastSelected = 0;

  this.create();
}

Tab.prototype = {
  get browser() {
    return this._browser;
  },

  get browserViewportState() {
    return this._browserViewportState;
  },

  get chromeTab() {
    return this._chromeTab;
  },

  



  _resizeAndPaint: function _resizeAndPaint() {
    let bv = Browser._browserView;

    bv.commitBatchOperation();

    
    bv.beginBatchOperation();
    this._loadingTimeout = setTimeout(this._resizeAndPaint, 2000);
  },

  _startResizeAndPaint: function _startResizeAndPaint() {
    if (this._loadingTimeout)
      throw "Already have a loading timeout";

    Browser._browserView.beginBatchOperation();
    this._loadingTimeout = setTimeout(this._resizeAndPaint, 2000);
  },

  _stopResizeAndPaint: function _stopResizeAndPaint() {
    if (!this._loadingTimeout)
      throw "No loading timeout!";

    clearTimeout(this._loadingTimeout);
    delete this._loadingTimeout;
    Browser._browserView.commitBatchOperation();
  },

  
  getIdentityState: function getIdentityState() {
    return this._listener.state;
  },

  startLoading: function startLoading() {
    if (this._loading) throw "Already Loading!";

    this._loading = true;

    if (!this._loadingTimeout) {
      let bv = Browser._browserView;

      this._startResizeAndPaint();
      if (this == Browser.selectedTab) {
        bv.invalidateEntireView();
        bv.setAggressive(false);
        
        
        bv.ignorePageScroll(true);
        Browser.scrollBrowserToContent();
      }
    }
  },

  endLoading: function endLoading() {
    if (!this._loading) throw "Not Loading!";

    
    let browser = this._browser;
    let metaData = Util.getViewportMetadata(browser);

    
    browser.style.removeProperty("width");
    browser.style.removeProperty("height");

    if (metaData.reason == "handheld" || metaData.reason == "doctype") {
      browser.className = "browser-handheld";
    } else if (metaData.reason == "viewport") {  
      browser.className = "browser-viewport";
      if (metaData.autoSize) {
        browser.classList.add("window-width");
        browser.classList.add("window-height");
      }
      else {
        let screenW = window.innerWidth;
        let screenH = window.innerHeight;
        let viewportW = metaData.width;
        let viewportH = metaData.height;
        let validW = viewportW > 0;
        let validH = viewportH > 0;
  
        if (validW && !validH) {
          viewportH = viewportW * (screenH / screenW);
        } else if (!validW && validH) {
          viewportW = viewportH * (screenW / screenH);
        } else {
          viewportW = kDefaultBrowserWidth;
          viewportH = kDefaultBrowserWidth * (screenH / screenW);
        }

        browser.style.width = viewportW + "px";
        browser.style.height = viewportH + "px";
      }
    } else if (metaData.reason == "chrome") {
      browser.className = "browser-chrome window-width window-height";
    } else {
      browser.className = "browser";
    }

    
    
    let doc = browser.contentDocument;
    if (doc instanceof XULDocument || doc.body instanceof HTMLFrameSetElement) {
       let [width, height] = BrowserView.Util.getBrowserDimensions(browser);
       BrowserView.Util.ensureMozScrolledAreaEvent(browser, width, height);
    }

    this.setIcon(browser.mIconURL);
    this._loading = false;

    if (this == Browser.selectedTab) {
      let bv = Browser._browserView;
      bv.ignorePageScroll(false);
      bv.setAggressive(true);
    }

    this._stopResizeAndPaint();

    
    this.restoreState();
  },

  isLoading: function isLoading() {
    return this._loading;
  },

  load: function load(uri) {
    this._browser.setAttribute("src", uri);
  },

  create: function create() {
    
    this._browserViewportState = BrowserView.Util.createBrowserViewportState();

    this._chromeTab = document.getElementById("tabs").addTab();
    this._createBrowser();
  },

  destroy: function destroy() {
    document.getElementById("tabs").removeTab(this._chromeTab);
    this._chromeTab = null;
    this._destroyBrowser();
  },

  
  ensureBrowserExists: function ensureBrowserExists() {
    if (!this._browser) {
      this._createBrowser();
      this.browser.contentDocument.location = this._state._url;
    }
  },

  _createBrowser: function _createBrowser() {
    if (this._browser)
      throw "Browser already exists";

    
    let browser = this._browser = document.createElement("browser");

    browser.className = "browser";
    browser.setAttribute("style", "overflow: -moz-hidden-unscrollable; visibility: hidden;");
    browser.setAttribute("type", "content");

    
    document.getElementById("browsers").appendChild(browser);

    
    browser.stop();

    
    let flags = Ci.nsIWebProgress.NOTIFY_LOCATION |
                Ci.nsIWebProgress.NOTIFY_SECURITY |
                Ci.nsIWebProgress.NOTIFY_STATE_NETWORK |
                Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT;
    this._listener = new ProgressController(this);
    browser.webProgress.addProgressListener(this._listener, flags);
  },

  _destroyBrowser: function _destroyBrowser() {
    if (this._browser) {
      var browser = this._browser;
      browser.removeProgressListener(this._listener);

      this._browser = null;
      this._listener = null;
      this._loading = false;

      try { 
        this._stopResizeAndPaint();
      } catch(ex) {}

      Util.executeSoon(function() {
        document.getElementById("browsers").removeChild(browser);
      });
    }
  },

  
  saveState: function saveState() {
    let state = { };

    var browser = this._browser;
    var doc = browser.contentDocument;
    state._url = doc.location.href;
    state._scroll = BrowserView.Util.getContentScrollOffset(this.browser);
    if (doc instanceof HTMLDocument) {
      var tags = ["input", "textarea", "select"];

      for (var t = 0; t < tags.length; t++) {
        var elements = doc.getElementsByTagName(tags[t]);
        for (var e = 0; e < elements.length; e++) {
          var element = elements[e];
          var id;
          if (element.id)
            id = "#" + element.id;
          else if (element.name)
            id = "$" + element.name;

          if (id)
            state[id] = element.value;
        }
      }
    }

    this._state = state;
  },

  
  restoreState: function restoreState() {
    let state = this._state;
    if (!state)
      return;

    let doc = this._browser.contentDocument;

    for (var item in state) {
      var elem = null;
      if (item.charAt(0) == "#") {
        elem = doc.getElementById(item.substring(1));
      } else if (item.charAt(0) == "$") {
        var list = doc.getElementsByName(item.substring(1));
        if (list.length)
          elem = list[0];
      }

      if (elem)
        elem.value = state[item];
    }

    this.browser.contentWindow.scrollX = state._scroll.x;
    this.browser.contentWindow.scrollY = state._scroll.y;

    this._state = null;
  },

  



  resetZoomLevel: function resetZoomLevel() {
    let bvs = this._browserViewportState;
    bvs.defaultZoomLevel = bvs.zoomLevel;
  },

  updateThumbnail: function updateThumbnail() {
    if (!this._browser)
      return;

    let browserView = (Browser.selectedBrowser == this._browser) ? Browser._browserView : null;
    this._chromeTab.updateThumbnail(this._browser, browserView);
  },

  setIcon: function setIcon(aURI) {
    let faviconURI = null;
    if (aURI) {
      try {
        faviconURI = gIOService.newURI(aURI, null, null);
      }
      catch (e) {
        faviconURI = null;
      }
    }

    if (!faviconURI || faviconURI.schemeIs("javascript") || gFaviconService.isFailedFavicon(faviconURI)) {
      try {
        
        
        faviconURI = gIOService.newURI(this._browser.contentDocument.documentURIObject.prePath + "/favicon.ico", null, null);
        gFaviconService.setAndLoadFaviconForPage(this._browser.currentURI, faviconURI, true);
      }
      catch (e) {
        faviconURI = null;
      }
      if (faviconURI && gFaviconService.isFailedFavicon(faviconURI))
        faviconURI = null;
    }

    this._browser.mIconURL = faviconURI ? faviconURI.spec : "";
  },


  toString: function() {
    return "[Tab " + (this._browser ? this._browser.contentDocument.location.toString() : "(no browser)") + "]";
  }
};

var ImagePreloader = {
  cache: function ip_cache() {
    
    let images = ["button-active", "button-default",
                  "buttondark-active", "buttondark-default",
                  "toggleon-active", "toggleon-inactive",
                  "toggleoff-active", "toggleoff-inactive",
                  "toggleleft-active", "toggleleft-inactive",
                  "togglemiddle-active", "togglemiddle-inactive",
                  "toggleright-active", "toggleright-inactive",
                  "toggleboth-active", "toggleboth-inactive",
                  "toggledarkleft-active", "toggledarkleft-inactive",
                  "toggledarkmiddle-active", "toggledarkmiddle-inactive",
                  "toggledarkright-active", "toggledarkright-inactive",
                  "toggledarkboth-active", "toggledarkboth-inactive",
                  "toolbarbutton-active", "toolbarbutton-default",
                  "addons-active", "addons-default",
                  "downloads-active", "downloads-default",
                  "preferences-active", "preferences-default",
                  "settings-active", "settings-open"];

    let size = screen.width > 400 ? "-64" : "-36";
    for (let i = 0; i < images.length; i++) {
      let image = new Image();
      image.src = "chrome://browser/skin/images/" + images[i] + size + ".png";
    }
  }
}

const nsIObjectLoadingContent = Ci.nsIObjectLoadingContent_MOZILLA_1_9_2_BRANCH || Ci.nsIObjectLoadingContent;





function PluginObserver(bv) {
  this._emptyRect = new Rect(0, 0, 0, 0);
  this._contentShowing = document.getElementById("observe_contentShowing");
  this._bv = bv;
  this._started = false;
  this._isRendering = false;

  let disabled = gPrefService.getBoolPref("plugin.disable");
  if (!disabled)
    this.start();
}

PluginObserver.prototype = {
  
  POPUP_PADDING: 4,

  
  start: function() {
    if (this._started)
      return;
    this._started = true;

    document.getElementById("tabs-container").addEventListener("TabSelect", this, false);
    this._contentShowing.addEventListener("broadcast", this, false);
    let browsers = document.getElementById("browsers");
    browsers.addEventListener("RenderStateChanged", this, false);
    gObserverService.addObserver(this, "plugin-changed-event", false);
    Elements.stack.addEventListener("PopupChanged", this, false);

    let browser = Browser.selectedBrowser;
    if (browser) {
      browser.addEventListener("ZoomChanged", this, false);
      browser.addEventListener("MozAfterPaint", this, false);
    }
  },

  
  stop: function() {
    if (!this._started)
      return;
    this._started = false;

    document.getElementById("tabs-container").removeEventListener("TabSelect", this, false);
    this._contentShowing.removeEventListener("broadcast", this, false);
    let browsers = document.getElementById("browsers");
    browsers.removeEventListener("RenderStateChanged", this, false);
    gObserverService.removeObserver(this, "plugin-changed-event");
    Elements.stack.removeEventListener("PopupChanged", this, false);

    let browser = Browser.selectedBrowser;
    if (browser) {
      browser.removeEventListener("ZoomChanged", this, false);
      browser.removeEventListener("MozAfterPaint", this, false);
    }
  },

  
  observe: function observe(subject, topic, data) {
    if (topic == "plugin-changed-event")
      this.updateCurrentBrowser();
  },

  
  handleEvent: function handleEvent(ev) {
    if (ev.type == "TabSelect") {
      if (ev.lastTab) {
        let browser = ev.lastTab.browser;
        let oldDoc = browser.contentDocument;

        browser.removeEventListener("ZoomChanged", this, false);
        browser.removeEventListener("MozAfterPaint", this, false);
        this.updateEmbedRegions(this.getPluginNodes(oldDoc), this._emptyRect);
      }

      let browser = Browser.selectedBrowser;
      browser.addEventListener("ZoomChanged", this, false);
      browser.addEventListener("MozAfterPaint", this, false);
    }

    this.updateCurrentBrowser();
  },

  
  getPluginNodes: function getPluginNodes(doc) {
    let docs = Util.getAllDocuments(doc);
    let result = [];

    let i;
    let plugins;
    for (i = 0; i < docs.length; i++) {
      plugins = docs[i].querySelectorAll("embed,object");
      result.push.apply(result, Array.prototype.slice.call(plugins));
    }

    return result;
  },

  
  updateCurrentBrowser: function updateCurrentBrowser() {
    let doc = Browser.selectedTab.browser.contentDocument;

    let rect = this.getCriticalRect();
    if (rect == this._emptyRect && !this._isRendering)
      return;

    if (this._isRendering) {
      
      if (rect == this._emptyRect)
        this._isRendering = false;
      this.updateEmbedRegions(this.getPluginNodes(doc), rect);
    } else {
      
      let self = this;
      setTimeout(function() {
        self._isRendering = true;
        
        self.updateEmbedRegions(self.getPluginNodes(doc), self.getCriticalRect());
      }, 0);
    }
  },

  
  getCriticalRect: function getCriticalRect() {
    let bv = this._bv;
    if (Browser.selectedTab._loading)
      return this._emptyRect;
    if (!bv.isRendering())
      return this._emptyRect;
    if (Elements.contentShowing.hasAttribute("disabled"))
      return this._emptyRect;

    let vs = bv._browserViewportState;
    let vr = bv.getVisibleRect();
    let crit = BrowserView.Util.visibleRectToCriticalRect(vr, vs);
    crit = Browser.browserViewToClientRect(crit);

    if (BrowserUI.isToolbarLocked()) {
      let urlbar = document.getElementById("toolbar-container");
      let urlbarRect = urlbar.getBoundingClientRect();
      
      
      
      crit.top = Math.max(Math.round(urlbarRect.height) + 1, crit.top);
      
      
    }

    let popup = BrowserUI._popup;
    if (popup) {
      let p = this.POPUP_PADDING;
      let elements = BrowserUI._popup.elements;
      for (let i = elements.length - 1; i >= 0; i--) {
        let popupRect = Rect.fromRect(elements[i].getBoundingClientRect()).expandToIntegers();
        
        
        
        popupRect.setBounds(popupRect.left - p, popupRect.top - p, popupRect.right + p, popupRect.bottom + p);
        let areaRects = crit.subtract(popupRect);
        if (areaRects.length == 1) {
          
          crit = areaRects[0];
        } else if (areaRects.length > 1) {
          
          return this._emptyRect;
        }
      }
    }

    return crit;
  },

  



  updateEmbedRegions: function updateEmbedRegions(objects, crit) {
    let bv = this._bv;
    let oprivate, r, dest, clip;
    for (let i = objects.length - 1; i >= 0; i--) {
      r = bv.browserToViewportRect(Browser.getBoundingContentRect(objects[i]));
      dest = Browser.browserViewToClientRect(r);
      clip = dest.intersect(crit).translate(-dest.left, -dest.top);
      oprivate = objects[i].QueryInterface(nsIObjectLoadingContent);
      try {
        oprivate.setAbsoluteScreenPosition(Browser.contentScrollbox, dest, clip);
      } catch(e) {};
    }
  }
};
