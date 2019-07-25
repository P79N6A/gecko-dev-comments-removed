

















































let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;

function getBrowser() {
  return Browser.selectedBrowser;
}

const kBrowserFormZoomLevelMin = 0.8;
const kBrowserFormZoomLevelMax = 2.0;
const kBrowserViewZoomLevelPrecision = 10000;

const kDefaultBrowserWidth = 800;
const kFallbackBrowserWidth = 980;
const kDefaultMetadata = { autoSize: false, allowZoom: true, autoScale: true };


window.sizeToContent = function() {
  Cu.reportError("window.sizeToContent is not allowed in this window");
}

#ifdef MOZ_CRASH_REPORTER
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
  "@mozilla.org/xre/app-info;1", "nsICrashReporter");
#endif

function onDebugKeyPress(ev) {
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

  function doSwipe(aDirection) {
    let e = document.createEvent("SimpleGestureEvent");
    e.initSimpleGestureEvent("MozSwipeGesture", true, true, window, null,
                               0, 0, 0, 0, false, false, false, false, 0, null,
                               aDirection, 0);
    Browser.selectedTab.inputHandler.dispatchEvent(e);
  }
  switch (ev.charCode) {
  case w:
    doSwipe(Ci.nsIDOMSimpleGestureEvent.DIRECTION_UP);
    break;
  case s:
    doSwipe(Ci.nsIDOMSimpleGestureEvent.DIRECTION_DOWN);
    break;
  case d:
    doSwipe(Ci.nsIDOMSimpleGestureEvent.DIRECTION_RIGHT);
    break;
  case a:
    doSwipe(Ci.nsIDOMSimpleGestureEvent.DIRECTION_LEFT);
    break;
  case f:
    MemoryObserver.observe();
    dump("Forced a GC\n");
    break;
  case m:
    CommandUpdater.doCommand("cmd_menu");
    break;
#ifndef MOZ_PLATFORM_MAEMO
  case p:
    function dispatchMagnifyEvent(aName, aDelta) {
      let e = document.createEvent("SimpleGestureEvent");
      e.initSimpleGestureEvent("MozMagnifyGesture"+aName, true, true, window, null,
                               0, 0, 0, 0, false, false, false, false, 0, null, 0, aDelta);
      Browser.selectedTab.inputHandler.dispatchEvent(e);
    }
    dispatchMagnifyEvent("Start", 0);

    let frame = 0;
    let timer = new Util.Timeout();
    timer.interval(100, function() {
      dispatchMagnifyEvent("Update", 20);
      if (++frame > 10) {
        timer.clear();
        dispatchMagnifyEvent("", frame*20);
      }
    });
    break;
  case q:
    if (Util.isPortrait())
      window.top.resizeTo(800,480);
    else
      window.top.resizeTo(480,800);
    break;
#endif
  default:
    break;
  }
}

var Browser = {
  _tabs: [],
  _selectedTab: null,
  windowUtils: window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils),
  controlsScrollbox: null,
  controlsScrollboxScroller: null,
  controlsPosition: null,
  pageScrollbox: null,
  pageScrollboxScroller: null,
  styles: {},

  startup: function startup() {
    var self = this;

    try {
      messageManager.loadFrameScript("chrome://browser/content/Util.js", true);
      messageManager.loadFrameScript("chrome://browser/content/forms.js", true);
      messageManager.loadFrameScript("chrome://browser/content/content.js", true);
    } catch (e) {
      
      dump("###########" + e + "\n");
    }

    

    
    Elements.browsers.customDragger = new Browser.MainDragger();

    
    Elements.browsers.webProgress = new Browser.WebProgress();

    let keySender = new ContentCustomKeySender(Elements.browsers);
    let mouseModule = new MouseModule();
    let gestureModule = new GestureModule(Elements.browsers);
    let scrollWheelModule = new ScrollwheelModule(Elements.browsers);

    ContentTouchHandler.init();

    
    
    
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

    
    let controlsScrollbox = this.controlsScrollbox = document.getElementById("controls-scrollbox");
    this.controlsScrollboxScroller = controlsScrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    controlsScrollbox.customDragger = {
      isDraggable: function isDraggable(target, content) { return {}; },
      dragStart: function dragStart(cx, cy, target, scroller) {},
      dragStop: function dragStop(dx, dy, scroller) { return false; },
      dragMove: function dragMove(dx, dy, scroller) { return false; }
    };

    
    let pageScrollbox = this.pageScrollbox = document.getElementById("page-scrollbox");
    this.pageScrollboxScroller = pageScrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    pageScrollbox.customDragger = controlsScrollbox.customDragger;

    let stylesheet = document.styleSheets[0];
    for each (let style in ["window-width", "window-height", "viewable-height", "viewable-width", "toolbar-height"]) {
      let index = stylesheet.insertRule("." + style + " {}", stylesheet.cssRules.length);
      this.styles[style] = stylesheet.cssRules[index].style;
    }

    
    
    
    
    window.addEventListener("MozBeforeResize", function(aEvent) {
      if (aEvent.target != window)
        return;

      let { x: x1, y: y1 } = Browser.getScrollboxPosition(Browser.controlsScrollboxScroller);
      let { x: x2, y: y2 } = Browser.getScrollboxPosition(Browser.pageScrollboxScroller);
      let [,, leftWidth, rightWidth] = Browser.computeSidebarVisibility();

      let shouldHideSidebars = Browser.controlsPosition ? Browser.controlsPosition.hideSidebars : true;
      Browser.controlsPosition = { x: x1, y: y2, hideSidebars: shouldHideSidebars,
                                   leftSidebar: leftWidth, rightSidebar: rightWidth };
    }, true);

    function resizeHandler(e) {
      if (e.target != window)
        return;

      let w = window.innerWidth;
      let h = window.innerHeight;

      
      let maximize = (document.documentElement.getAttribute("sizemode") == "maximized");
      if (maximize && w > screen.width)
        return;

      let toolbarHeight = Math.round(document.getElementById("toolbar-main").getBoundingClientRect().height);

      Browser.styles["window-width"].width = w + "px";
      Browser.styles["window-height"].height = h + "px";
      Browser.styles["toolbar-height"].height = toolbarHeight + "px";

      
      BrowserUI.sizeControls(w, h);
      ViewableAreaObserver.update();

      
      let restorePosition = Browser.controlsPosition;
      if (restorePosition.hideSidebars) {
        restorePosition.hideSidebars = false;
        Browser.hideSidebars();
      } else {
        
        if (restorePosition.x) {
          let [,, leftWidth, rightWidth] = Browser.computeSidebarVisibility();
          let delta = ((restorePosition.leftSidebar - leftWidth) || (restorePosition.rightSidebar - rightWidth));
          restorePosition.x += (restorePosition.x == leftWidth) ? delta : -delta;
        }

        Browser.controlsScrollboxScroller.scrollTo(restorePosition.x, 0);
        Browser.pageScrollboxScroller.scrollTo(0, restorePosition.y);
        Browser.tryFloatToolbar(0, 0);
      }

      
      let currentElement = document.activeElement;
      let [scrollbox, scrollInterface] = ScrollUtils.getScrollboxFromElement(currentElement);
      if (scrollbox && scrollInterface && currentElement && currentElement != scrollbox) {
        
        while (currentElement.parentNode != scrollbox)
          currentElement = currentElement.parentNode;

        setTimeout(function() { scrollInterface.ensureElementIsVisible(currentElement) }, 0);
      }
    }
    window.addEventListener("resize", resizeHandler, false);

    function fullscreenHandler() {
      if (!window.fullScreen)
        document.getElementById("toolbar-main").setAttribute("fullscreen", "true");
      else
        document.getElementById("toolbar-main").removeAttribute("fullscreen");
    }
    window.addEventListener("fullscreen", fullscreenHandler, false);

    BrowserUI.init();

    window.controllers.appendController(this);
    window.controllers.appendController(BrowserUI);

    var os = Services.obs;
    os.addObserver(XPInstallObserver, "addon-install-blocked", false);
    os.addObserver(XPInstallObserver, "addon-install-started", false);
    os.addObserver(SessionHistoryObserver, "browser:purge-session-history", false);
    os.addObserver(ContentCrashObserver, "ipc:content-shutdown", false);
    os.addObserver(MemoryObserver, "memory-pressure", false);
    os.addObserver(BrowserSearch, "browser-search-engine-modified", false);

    
    os.addObserver(ViewableAreaObserver, "softkb-change", false);
    Elements.contentNavigator.addEventListener("SizeChanged", function() {
      ViewableAreaObserver.update();
    }, true);

    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = new nsBrowserAccess();

    Elements.browsers.addEventListener("DOMUpdatePageReport", PopupBlockerObserver.onUpdatePageReport, false);

    
    Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
    Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2);

    
    Util.forceOnline();

    
    
    
    let commandURL = null;
    if (window.arguments && window.arguments[0])
      commandURL = window.arguments[0];

    
    let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
    if (ss.shouldRestore()) {
      ss.restoreLastSession();

      
      if (commandURL && commandURL != this.getHomePage())
        this.addTab(commandURL, true);
    } else {
      this.addTab(commandURL || this.getHomePage(), true);
    }

    
    if (Services.prefs.prefHasUserValue("extensions.disabledAddons")) {
      let addons = Services.prefs.getCharPref("extensions.disabledAddons").split(",");
      if (addons.length > 0) {
        let disabledStrings = Strings.browser.GetStringFromName("alertAddonsDisabled");
        let label = PluralForm.get(addons.length, disabledStrings).replace("#1", addons.length);

        let alerts = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
        alerts.showAlertNotification(URI_GENERIC_ICON_XPINSTALL, Strings.browser.GetStringFromName("alertAddons"),
                                     label, false, "", null);
      }
      Services.prefs.clearUserPref("extensions.disabledAddons");
    }

    messageManager.addMessageListener("Browser:ViewportMetadata", this);
    messageManager.addMessageListener("Browser:FormSubmit", this);
    messageManager.addMessageListener("Browser:KeyPress", this);
    messageManager.addMessageListener("Browser:ZoomToPoint:Return", this);
    messageManager.addMessageListener("scroll", this);
    messageManager.addMessageListener("Browser:MozApplicationManifest", OfflineApps);
    messageManager.addMessageListener("Browser:CertException", this);

    
    let event = document.createEvent("Events");
    event.initEvent("UIReady", true, false);
    window.dispatchEvent(event);
  },

  _waitingToClose: false,
  closing: function closing() {
    
    if (this._waitingToClose)
      return false;

    
    let numTabs = this._tabs.length;
    if (numTabs > 1) {
      let shouldPrompt = Services.prefs.getBoolPref("browser.tabs.warnOnClose");
      if (shouldPrompt) {
        let prompt = Services.prompt;

        
        let warnOnClose = { value: true };

        let messageBase = Strings.browser.GetStringFromName("tabs.closeWarning");
        let message = PluralForm.get(numTabs, messageBase).replace("#1", numTabs);

        let title = Strings.browser.GetStringFromName("tabs.closeWarningTitle");
        let closeText = Strings.browser.GetStringFromName("tabs.closeButton");
        let checkText = Strings.browser.GetStringFromName("tabs.closeWarningPromptMe");
        let buttons = (prompt.BUTTON_TITLE_IS_STRING * prompt.BUTTON_POS_0) +
                      (prompt.BUTTON_TITLE_CANCEL * prompt.BUTTON_POS_1);

        this._waitingToClose = true;
        let pressed = prompt.confirmEx(window, title, message, buttons, closeText, null, null, checkText, warnOnClose);
        this._waitingToClose = false;

        
        let reallyClose = (pressed == 0);
        if (reallyClose && !warnOnClose.value)
          Services.prefs.setBoolPref("browser.tabs.warnOnClose", false);

        
        if (!reallyClose)
          return false;
      }
    }

    
    let lastBrowser = true;
    let e = Services.wm.getEnumerator("navigator:browser");
    while (e.hasMoreElements() && lastBrowser) {
      let win = e.getNext();
      if (win != window && win.toolbar.visible)
        lastBrowser = false;
    }
    if (!lastBrowser)
      return true;

    
    let closingCanceled = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
    Services.obs.notifyObservers(closingCanceled, "browser-lastwindow-close-requested", null);
    if (closingCanceled.data)
      return false;

    Services.obs.notifyObservers(null, "browser-lastwindow-close-granted", null);
    return true;
  },

  shutdown: function shutdown() {
    BrowserUI.uninit();

    messageManager.removeMessageListener("Browser:ViewportMetadata", this);
    messageManager.removeMessageListener("Browser:FormSubmit", this);
    messageManager.removeMessageListener("Browser:KeyPress", this);
    messageManager.removeMessageListener("Browser:ZoomToPoint:Return", this);
    messageManager.removeMessageListener("scroll", this);
    messageManager.removeMessageListener("Browser:MozApplicationManifest", OfflineApps);
    messageManager.removeMessageListener("Browser:CertException", this);

    var os = Services.obs;
    os.removeObserver(XPInstallObserver, "addon-install-blocked");
    os.removeObserver(XPInstallObserver, "addon-install-started");
    os.removeObserver(SessionHistoryObserver, "browser:purge-session-history");
    os.removeObserver(ContentCrashObserver, "ipc:content-shutdown");
    os.removeObserver(MemoryObserver, "memory-pressure");
    os.removeObserver(BrowserSearch, "browser-search-engine-modified");

    window.controllers.removeController(this);
    window.controllers.removeController(BrowserUI);
  },

  getHomePage: function getHomePage(aOptions) {
    aOptions = aOptions || { useDefault: false };

    let url = "about:home";
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

  scrollContentToTop: function scrollContentToTop(aOptions) {
    let x = {}, y = {};
    this.contentScrollboxScroller.getPosition(x, y);
    if (aOptions)
      x.value = ("x" in aOptions ? aOptions.x : x.value);

    this.contentScrollboxScroller.scrollTo(x.value, 0);
    this.pageScrollboxScroller.scrollTo(0, 0);
  },

  
  scrollContentToBottom: function scrollContentToBottom(aOptions) {
    let x = {}, y = {};
    this.contentScrollboxScroller.getPosition(x, y);
    if (aOptions)
      x.value = ("x" in aOptions ? aOptions.x : x.value);

    this.contentScrollboxScroller.scrollTo(x.value, Number.MAX_VALUE);
    this.pageScrollboxScroller.scrollTo(0, Number.MAX_VALUE);
  },

  hideSidebars: function scrollSidebarsOffscreen() {
    let rect = Elements.browsers.getBoundingClientRect();
    this.controlsScrollboxScroller.scrollBy(Math.round(rect.left), 0);
    this.tryUnfloatToolbar();
  },

  hideTitlebar: function hideTitlebar() {
    let rect = Elements.browsers.getBoundingClientRect();
    this.pageScrollboxScroller.scrollBy(0, Math.round(rect.top));
    this.tryUnfloatToolbar();
  },

  





  loadURI: function loadURI(aURI, aParams) {
    let browser = this.selectedBrowser;

    
    
    let currentURI = browser.currentURI.spec;
    let useLocal = Util.isLocalScheme(aURI);
    let hasLocal = Util.isLocalScheme(currentURI);

    if (hasLocal != useLocal) {
      let oldTab = this.selectedTab;

      
      Browser.addTab(aURI, true, oldTab, aParams);
      if (/^about:(blank|empty)$/.test(currentURI) && !browser.canGoBack && !browser.canGoForward) {
        oldTab.chromeTab.ignoreUndo = true;
        this.closeTab(oldTab);
        oldTab = null;
      }
    }
    else {
      let params = aParams || {};
      let flags = params.flags || Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
      browser.loadURIWithFlags(aURI, flags, params.referrerURI, params.charset, params.postData);
    }
  },

  





  getShortcutOrURI: function getShortcutOrURI(aURL, aPostDataRef) {
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
      return submission.uri.spec;
    }

    try {
      [shortcutURL, aPostDataRef.value] = PlacesUtils.getURLAndPostDataForKeyword(keyword);
    } catch (e) {}

    if (!shortcutURL)
      return aURL;

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
          
          
          charset = PlacesUtils.history.getCharsetForURI(Util.makeURI(shortcutURL));
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

      return aURL;
    }

    return shortcutURL;
  },

  


  get selectedBrowser() {
    return this._selectedTab.browser;
  },

  get tabs() {
    return this._tabs;
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
    if (index > this._tabs.length || index < 0)
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

  addTab: function browser_addTab(aURI, aBringFront, aOwner, aParams) {
    let params = aParams || {};
    let newTab = new Tab(aURI, params);
    newTab.owner = aOwner || null;
    this._tabs.push(newTab);

    if (aBringFront)
      this.selectedTab = newTab;

    let getAttention = ("getAttention" in params ? params.getAttention : !aBringFront);
    let event = document.createEvent("UIEvents");
    event.initUIEvent("TabOpen", true, false, window, getAttention);
    newTab.chromeTab.dispatchEvent(event);
    newTab.browser.messageManager.sendAsyncMessage("Browser:TabOpen");

    return newTab;
  },

  closeTab: function(aTab) {
    let tab = aTab;
    if (aTab instanceof XULElement)
      tab = this.getTabFromChrome(aTab);

    
    if (!tab || this._tabs.length < 2)
      return;

    
    if (tab == this._selectedTab && tab.isLoading())
      BrowserUI.unlockToolbar();

    let tabIndex = this._tabs.indexOf(tab);
    if (tabIndex == -1)
      return;

    let nextTab = this._selectedTab;
    if (nextTab == tab) {
      nextTab = tab.owner || this.getTabAtIndex(tabIndex + 1) || this.getTabAtIndex(tabIndex - 1);
      if (!nextTab)
        return;
    }

    
    this._tabs.forEach(function(aTab, aIndex, aArray) {
      if (aTab.owner == tab)
        aTab.owner = null;
    });

    let event = document.createEvent("Events");
    event.initEvent("TabClose", true, false);
    tab.chromeTab.dispatchEvent(event);
    tab.browser.messageManager.sendAsyncMessage("Browser:TabClose");

    tab.destroy();
    this._tabs.splice(tabIndex, 1);

    this.selectedTab = nextTab;
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

    if (this._selectedTab) {
      this._selectedTab.pageScrollOffset = this.getScrollboxPosition(this.pageScrollboxScroller);

      
      if (this._selectedTab.isLoading())
        BrowserUI.unlockToolbar();
    }

    let isFirstTab = this._selectedTab == null;
    let lastTab = this._selectedTab;
    let oldBrowser = lastTab ? lastTab._browser : null;
    let browser = tab.browser;

    this._selectedTab = tab;

    
    if (this._selectedTab.isLoading())
      BrowserUI.lockToolbar();

    if (lastTab)
      lastTab.active = false;

    if (tab)
      tab.active = true;

    if (!isFirstTab) {
      
      BrowserUI.updateURI();
      getIdentityHandler().checkIdentity();

      let event = document.createEvent("Events");
      event.initEvent("TabSelect", true, false);
      event.lastTab = lastTab;
      tab.chromeTab.dispatchEvent(event);
    }

    tab.lastSelected = Date.now();

    if (tab.pageScrollOffset) {
      let pageScroll = tab.pageScrollOffset;
      Browser.pageScrollboxScroller.scrollTo(pageScroll.x, pageScroll.y);
    }
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

  getNotificationBox: function getNotificationBox(aBrowser) {
    let browser = aBrowser || this.selectedBrowser;
    return browser.parentNode;
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
          sslExceptions.addPermanentException(uri);
        else
          sslExceptions.addTemporaryException(uri);
      } catch (e) {
        dump("EXCEPTION handle content command: " + e + "\n" );
      }

      
      aMessage.target.reload();
    }
  },

  






  computeSidebarVisibility: function computeSidebarVisibility(dx, dy) {
    function visibility(aSidebarRect, aVisibleRect) {
      let width = aSidebarRect.width;
      aSidebarRect.restrictTo(aVisibleRect);
      return aSidebarRect.width / width;
    }

    if (!dx) dx = 0;
    if (!dy) dy = 0;

    let [leftSidebar, rightSidebar] = [Elements.tabs.getBoundingClientRect(), Elements.controls.getBoundingClientRect()];
    if (leftSidebar.left > rightSidebar.left)
      [rightSidebar, leftSidebar] = [leftSidebar, rightSidebar]; 

    let visibleRect = new Rect(0, 0, window.innerWidth, 1);
    let leftRect = new Rect(Math.round(leftSidebar.left) - Math.round(dx), 0, Math.round(leftSidebar.width), 1);
    let rightRect = new Rect(Math.round(rightSidebar.left) - Math.round(dx), 0, Math.round(rightSidebar.width), 1);

    let leftTotalWidth = leftRect.width;
    let leftVisibility = visibility(leftRect, visibleRect);

    let rightTotalWidth = rightRect.width;
    let rightVisibility = visibility(rightRect, visibleRect);

    return [leftVisibility, rightVisibility, leftTotalWidth, rightTotalWidth];
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
    let tab = this.selectedTab;
    if (!tab.allowZoom)
      return;

    let browser = tab.browser;
    let oldZoomLevel = browser.scale;
    let zoomLevel = oldZoomLevel;

    let zoomValues = ZoomManager.zoomValues;
    let i = zoomValues.indexOf(ZoomManager.snap(zoomLevel)) + (aDirection < 0 ? 1 : -1);
    if (i >= 0 && i < zoomValues.length)
      zoomLevel = zoomValues[i];

    zoomLevel = tab.clampZoomLevel(zoomLevel);

    let browserRect = browser.getBoundingClientRect();
    let center = browser.transformClientToBrowser(browserRect.width / 2,
                                                  browserRect.height / 2);
    let rect = this._getZoomRectForPoint(center.x, center.y, zoomLevel);
    this.animatedZoomTo(rect);
  },

  
  _getZoomLevelForRect: function _getZoomLevelForRect(rect) {
    const margin = 15;
    return this.selectedTab.clampZoomLevel(window.innerWidth / (rect.width + margin * 2));
  },

  



  _getZoomRectForRect: function _getZoomRectForRect(rect, y) {
    let zoomLevel = this._getZoomLevelForRect(rect);
    return this._getZoomRectForPoint(rect.center().x, y, zoomLevel);
  },

  



  _getZoomRectForPoint: function _getZoomRectForPoint(x, y, zoomLevel) {
    let browser = getBrowser();
    x = x * browser.scale;
    y = y * browser.scale;

    zoomLevel = Math.min(ZoomManager.MAX, zoomLevel);
    let oldScale = browser.scale;
    let zoomRatio = zoomLevel / oldScale;
    let browserRect = browser.getBoundingClientRect();
    let newVisW = browserRect.width / zoomRatio, newVisH = browserRect.height / zoomRatio;
    let result = new Rect(x - newVisW / 2, y - newVisH / 2, newVisW, newVisH);

    
    return result.translateInside(new Rect(0, 0, browser.contentDocumentWidth * oldScale,
                                                 browser.contentDocumentHeight * oldScale));
  },

  animatedZoomTo: function animatedZoomTo(rect) {
    AnimatedZoom.animateTo(rect);
  },

  setVisibleRect: function setVisibleRect(rect) {
    let browser = getBrowser();
    let zoomRatio = window.innerWidth / rect.width;
    let zoomLevel = browser.scale * zoomRatio;
    let scrollX = rect.left * zoomRatio;
    let scrollY = rect.top * zoomRatio;

    this.hideSidebars();
    this.hideTitlebar();

    
    browser.scale = this.selectedTab.clampZoomLevel(zoomLevel);
    let view = browser.getRootView();
    if (view._contentView) {
      view._contentView.scrollTo(scrollX, scrollY);
      view._updateCacheViewport();
    }
  },

  zoomToPoint: function zoomToPoint(cX, cY, aRect) {
    let tab = this.selectedTab;
    if (!tab.allowZoom)
      return null;

    let zoomRect = null;
    if (aRect)
      zoomRect = this._getZoomRectForRect(aRect, cY);

    if (!zoomRect && tab.isDefaultZoomLevel()) {
      let scale = tab.clampZoomLevel(tab.browser.scale * 2);
      zoomRect = this._getZoomRectForPoint(cX, cY, scale);
    }

    if (zoomRect)
      this.animatedZoomTo(zoomRect);

    return zoomRect;
  },

  zoomFromPoint: function zoomFromPoint(cX, cY) {
    let tab = this.selectedTab;
    if (tab.allowZoom && !tab.isDefaultZoomLevel()) {
      let zoomLevel = tab.getDefaultZoomLevel();
      let zoomRect = this._getZoomRectForPoint(cX, cY, zoomLevel);
      this.animatedZoomTo(zoomRect);
    }
  },

  
  
  getScaleRatio: function getScaleRatio() {
    let prefValue = Services.prefs.getIntPref("browser.viewport.scaleRatio");
    if (prefValue > 0)
      return prefValue / 100;

    let dpi = this.windowUtils.displayDPI;
    if (dpi < 200) 
      return 1;
    else if (dpi < 300) 
      return 1.5;

    
    return Math.floor(dpi / 150);
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
      case "Browser:ViewportMetadata":
        let tab = this.getTabForBrowser(browser);
        
        
        if (tab)
          tab.updateViewportMetadata(json);
        break;

      case "Browser:FormSubmit":
        browser.lastLocation = null;
        break;

      case "Browser:KeyPress":
        let event = document.createEvent("KeyEvents");
        event.initKeyEvent("keypress", true, true, null,
                        json.ctrlKey, json.altKey, json.shiftKey, json.metaKey,
                        json.keyCode, json.charCode)
        document.getElementById("mainKeyset").dispatchEvent(event);
        break;

      case "Browser:ZoomToPoint:Return":
        if (json.zoomTo) {
          let rect = Rect.fromRect(json.zoomTo);
          this.zoomToPoint(json.x, json.y, rect);
        } else {
          this.zoomFromPoint(json.x, json.y);
        }
        break;

      case "scroll":
        if (browser == this.selectedBrowser) {
          this.hideTitlebar();
          this.hideSidebars();
        }
        break;
      case "Browser:CertException":
        this._handleCertException(aMessage);
        break;
    }
  }
};


Browser.MainDragger = function MainDragger() {
  this._horizontalScrollbar = document.getElementById("horizontal-scroller");
  this._verticalScrollbar = document.getElementById("vertical-scroller");
  this._scrollScales = { x: 0, y: 0 };

  Elements.browsers.addEventListener("PanBegin", this, false);
  Elements.browsers.addEventListener("PanFinished", this, false);
  Elements.contentNavigator.addEventListener("SizeChanged", this, false);
};

Browser.MainDragger.prototype = {
  isDraggable: function isDraggable(target, scroller) {
    return { x: true, y: true };
  },

  dragStart: function dragStart(clientX, clientY, target, scroller) {
    let browser = getBrowser();
    let bcr = browser.getBoundingClientRect();
    this._contentView = browser.getViewsAt(clientX - bcr.left, clientY - bcr.top);
  },

  dragStop: function dragStop(dx, dy, scroller) {
    this.dragMove(Browser.snapSidebars(), 0, scroller);
    Browser.tryUnfloatToolbar();
  },

  dragMove: function dragMove(dx, dy, scroller) {
    let doffset = new Point(dx, dy);

    if (!this._contentView.isRoot()) {
      this._panContentView(this._contentView, doffset);
      
      
    }

    
    let panOffset = this._panControlsAwayOffset(doffset);

    
    this._panContentView(getBrowser().getRootView(), doffset);

    
    
    doffset.add(panOffset);
    Browser.tryFloatToolbar(doffset.x, 0);
    this._panScroller(Browser.controlsScrollboxScroller, doffset);
    this._panScroller(Browser.pageScrollboxScroller, doffset);
    this._updateScrollbars();

    return !doffset.equals(dx, dy);
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "PanBegin": {
        let browser = Browser.selectedBrowser;
        let width = ViewableAreaObserver.width, height = ViewableAreaObserver.height;
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
        break;

      case "SizeChanged":
        let height = Elements.contentNavigator.getBoundingClientRect().height;
        this._horizontalScrollbar.setAttribute("bottom", 2 + height);
        break;
    }
  },

  
  _panControlsAwayOffset: function(doffset) {
    let x = 0, y = 0, rect;

    rect = Rect.fromRect(Browser.pageScrollbox.getBoundingClientRect()).map(Math.round);
    if (doffset.x < 0 && rect.right < window.innerWidth)
      x = Math.max(doffset.x, rect.right - window.innerWidth);
    if (doffset.x > 0 && rect.left > 0)
      x = Math.min(doffset.x, rect.left);

    
    let height = Elements.contentViewport.getBoundingClientRect().height;
    height -= Elements.contentNavigator.getBoundingClientRect().height;

    rect = Rect.fromRect(Browser.contentScrollbox.getBoundingClientRect()).map(Math.round);
    if (doffset.y < 0 && rect.bottom < height)
      y = Math.max(doffset.y, rect.bottom - height);
    if (doffset.y > 0 && rect.top > 0)
      y = Math.min(doffset.y, rect.top);

    doffset.subtract(x, y);
    return new Point(x, y);
  },

  
  _panContentView: function _panContentView(contentView, doffset) {
    let pos0 = contentView.getPosition();
    contentView.scrollBy(doffset.x, doffset.y);
    let pos1 = contentView.getPosition();
    doffset.subtract(pos1.x - pos0.x, pos1.y - pos0.y);
  },

  
  _panScroller: function _panScroller(scroller, doffset) {
    let scroll = Browser.getScrollboxPosition(scroller);
    scroller.scrollBy(doffset.x, doffset.y);
    let scroll1 = Browser.getScrollboxPosition(scroller);
    doffset.subtract(scroll1.x - scroll.x, scroll1.y - scroll.y);
  },

  _updateScrollbars: function _updateScrollbars() {
    let scaleX = this._scrollScales.x, scaleY = this._scrollScales.y;
    let contentScroll = Browser.getScrollboxPosition(Browser.contentScrollboxScroller);
    if (scaleX)
      this._horizontalScrollbar.left = contentScroll.x * scaleX;

    if (scaleY) {
      const SCROLLER_MARGIN = 2;
      this._verticalScrollbar.top = contentScroll.y * scaleY;

      
      
      if (Browser.floatedWhileDragging) {
        let [leftVis,,leftW,] = Browser.computeSidebarVisibility();
        this._verticalScrollbar.setAttribute("right", Math.max(SCROLLER_MARGIN, leftW * leftVis + SCROLLER_MARGIN));
      }
      else if (this._verticalScrollbar.getAttribute("right") != SCROLLER_MARGIN) {
        this._verticalScrollbar.setAttribute("right", SCROLLER_MARGIN);
      }
    }
  },

  _showScrollbars: function _showScrollbars() {
    let scaleX = this._scrollScales.x, scaleY = this._scrollScales.y;
    if (scaleX) {
      this._horizontalScrollbar.width = ViewableAreaObserver.width * scaleX;
      this._horizontalScrollbar.setAttribute("panning", "true");
    }

    if (scaleY) {
      this._verticalScrollbar.height = ViewableAreaObserver.height * scaleY;
      this._verticalScrollbar.setAttribute("panning", "true");
    }
  },

  _hideScrollbars: function _hideScrollbars() {
    this._horizontalScrollbar.removeAttribute("panning");
    this._verticalScrollbar.removeAttribute("panning");
  }
};


Browser.WebProgress = function WebProgress() {
  messageManager.addMessageListener("Content:StateChange", this);
  messageManager.addMessageListener("Content:LocationChange", this);
  messageManager.addMessageListener("Content:SecurityChange", this);
};

Browser.WebProgress.prototype = {
  receiveMessage: function receiveMessage(aMessage) {
    let json = aMessage.json;
    let tab = Browser.getTabForBrowser(aMessage.target);

    switch (aMessage.name) {
      case "Content:StateChange": {
        if (json.stateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
          if (json.stateFlags & Ci.nsIWebProgressListener.STATE_START)
            this._networkStart(tab);
          else if (json.stateFlags & Ci.nsIWebProgressListener.STATE_STOP)
            this._networkStop(tab);
        }

        if (json.stateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
          if (json.stateFlags & Ci.nsIWebProgressListener.STATE_STOP)
            this._documentStop(tab);
        }
        break;
      }

      case "Content:LocationChange": {
        let spec = json.location;
        let location = spec.split("#")[0]; 

        if (tab == Browser.selectedTab)
          BrowserUI.updateURI();

        let locationHasChanged = (location != tab.browser.lastLocation);
        if (locationHasChanged) {
          Browser.getNotificationBox(tab.browser).removeTransientNotifications();
          tab.resetZoomLevel();
          tab.hostChanged = true;
          tab.browser.lastLocation = location;
          tab.browser.userTypedValue = "";

#ifdef MOZ_CRASH_REPORTER
          if (CrashReporter.enabled)
            CrashReporter.annotateCrashReport("URL", spec);
#endif
          if (tab == Browser.selectedTab) {
            
            
            
            Browser.scrollContentToTop({ x: 0 });
          }

          tab.useFallbackWidth = false;
          tab.updateViewportSize();
        }

        let event = document.createEvent("UIEvents");
        event.initUIEvent("URLChanged", true, false, window, locationHasChanged);
        tab.browser.dispatchEvent(event);
        break;
      }

      case "Content:SecurityChange": {
        
        if (tab.state == json.state && !tab.hostChanged)
          return;

        tab.hostChanged = false;
        tab.state = json.state;

        if (tab == Browser.selectedTab)
          getIdentityHandler().checkIdentity();
        break;
      }
    }
  },

  _networkStart: function _networkStart(aTab) {
    aTab.startLoading();

    if (aTab == Browser.selectedTab) {
      BrowserUI.update(TOOLBARSTATE_LOADING);

      
      
      if (aTab.browser.currentURI.spec == "about:blank")
        BrowserUI.updateURI();
    }
  },

  _networkStop: function _networkStop(aTab) {
    aTab.endLoading();

    if (aTab == Browser.selectedTab)
      BrowserUI.update(TOOLBARSTATE_LOADED);

    if (aTab.browser.currentURI.spec != "about:blank")
      aTab.updateThumbnail();
  },

  _documentStop: function _documentStop(aTab) {
      
      
      aTab.pageScrollOffset = new Point(0, 0);
  }
};


function nsBrowserAccess() { }

nsBrowserAccess.prototype = {
  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIBrowserDOMWindow) || aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  },

  _getBrowser: function _getBrowser(aURI, aOpener, aWhere, aContext) {
    let isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);
    if (isExternal && aURI && aURI.schemeIs("chrome"))
      return null;

    let loadflags = isExternal ?
                      Ci.nsIWebNavigation.LOAD_FLAGS_FROM_EXTERNAL :
                      Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    let location;
    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW) {
      switch (aContext) {
        case Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL :
          aWhere = Services.prefs.getIntPref("browser.link.open_external");
          break;
        default : 
          aWhere = Services.prefs.getIntPref("browser.link.open_newwindow");
      }
    }

    let browser;
    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW) {
      let url = aURI ? aURI.spec : "about:blank";
      let newWindow = openDialog("chrome://browser/content/browser.xul", "_blank",
                                 "all,dialog=no", url, null, null, null);
      
      return null;
    } else if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_NEWTAB) {
      let owner = isExternal ? null : Browser.selectedTab;
      let tab = Browser.addTab("about:blank", true, owner, { getAttention: true });
      if (isExternal)
        tab.closeOnExit = true;
      browser = tab.browser;
    } else { 
      browser = Browser.selectedBrowser;
    }

    try {
      let referrer;
      if (aURI) {
        if (aOpener) {
          location = aOpener.location;
          referrer = Services.io.newURI(location, null, null);
        }
        browser.loadURIWithFlags(aURI.spec, loadflags, referrer, null, null);
      }
      browser.focus();
    } catch(e) { }

    BrowserUI.closeAutoComplete();
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
  }
};



const ContentTouchHandler = {
  
  
  _messageId: 0,

  init: function init() {
    document.addEventListener("TapDown", this, true);
    document.addEventListener("TapOver", this, false);
    document.addEventListener("TapUp", this, false);
    document.addEventListener("TapSingle", this, false);
    document.addEventListener("TapDouble", this, false);
    document.addEventListener("TapLong", this, false);

    document.addEventListener("PanBegin", this, false);
    document.addEventListener("PopupChanged", this, false);
    document.addEventListener("CancelTouchSequence", this, false);

    
    
    
    
    
    
    
    
    
    
    messageManager.addMessageListener("Browser:ContextMenu", this);

    messageManager.addMessageListener("Browser:Highlight", this);
  },

  handleEvent: function handleEvent(aEvent) {
    
    if (aEvent.target.localName == "browser")
      return;

    switch (aEvent.type) {
      case "PanBegin":
      case "PopupChanged":
      case "CancelTouchSequence":
        this._clearPendingMessages();
        break;

      default: {
        if (ContextHelper.popupState) {
          
          aEvent.preventDefault();
          break;
        }

        if (!this._targetIsContent(aEvent)) {
          
          this._clearPendingMessages();
          break;
        }

        switch (aEvent.type) {
          case "TapDown":
            this.tapDown(aEvent.clientX, aEvent.clientY);
            break;
          case "TapOver":
            this.tapOver(aEvent.clientX, aEvent.clientY);
            break;
          case "TapUp":
            this.tapUp(aEvent.clientX, aEvent.clientY);
            break;
          case "TapSingle":
            this.tapSingle(aEvent.clientX, aEvent.clientY, aEvent.modifiers);
            break;
          case "TapDouble":
            this.tapDouble(aEvent.clientX, aEvent.clientY, aEvent.modifiers);
            break;
          case "TapLong":
            this.tapLong(aEvent.clientX, aEvent.clientY);
            break;
        }
      }
    }
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (aMessage.json.messageId != this._messageId)
      return;

    switch (aMessage.name) {
      case "Browser:ContextMenu":
        
        let contextMenu = { name: aMessage.name, json: aMessage.json, target: aMessage.target };
        if (ContextHelper.showPopup(contextMenu)) {
          
          let event = document.createEvent("Events");
          event.initEvent("CancelTouchSequence", true, false);
          document.dispatchEvent(event);
        }
        break;
    }
  },

  
  _clearPendingMessages: function _clearPendingMessages() {
    this._messageId++;
    this._contextMenu = null;
    let browser = getBrowser();
    browser.messageManager.sendAsyncMessage("Browser:MouseCancel", {});
  },

  


  _targetIsContent: function _targetIsContent(aEvent) {
    
    
    
    
    let target = aEvent.target;
    return target && ("classList" in target && target.classList.contains("inputHandler"));
  },

  _dispatchMouseEvent: function _dispatchMouseEvent(aName, aX, aY, aOptions) {
    let browser = getBrowser();
    let pos = browser.transformClientToBrowser(aX || 0, aY || 0);

    let json = aOptions || {};
    json.x = pos.x;
    json.y = pos.y;
    json.messageId = this._messageId;
    browser.messageManager.sendAsyncMessage(aName, json);
  },

  tapDown: function tapDown(aX, aY) {
    
    let browser = getBrowser();
    let fl = browser.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
    browser.focus();
    try {
      fl.activateRemoteFrame();
    } catch (e) {}
    this._dispatchMouseEvent("Browser:MouseDown", aX, aY);
  },

  tapOver: function tapOver(aX, aY) {
    this._dispatchMouseEvent("Browser:MouseOver", aX, aY);
  },

  tapUp: function tapUp(aX, aY) {
    this._contextMenu = null;
    let browser = getBrowser();
    browser.messageManager.sendAsyncMessage("Browser:MouseCancel", {});
  },

  tapSingle: function tapSingle(aX, aY, aModifiers) {
    this._contextMenu = null;

    
    if (!ContextHelper.popupState)
      this._dispatchMouseEvent("Browser:MouseUp", aX, aY, { modifiers: aModifiers });
  },

  tapDouble: function tapDouble(aX, aY, aModifiers) {
    this._clearPendingMessages();

    let tab = Browser.selectedTab;
    if (!tab.allowZoom)
      return;

    let width = window.innerWidth / Browser.getScaleRatio();
    this._dispatchMouseEvent("Browser:ZoomToPoint", aX, aY, { width: width });
  },

  tapLong: function tapLong(aX, aY) {
    this._dispatchMouseEvent("Browser:MouseLong", aX, aY);
  },

  toString: function toString() {
    return "[ContentTouchHandler] { }";
  }
};



function ContentCustomKeySender(container) {
  container.addEventListener("keypress", this, false);
  container.addEventListener("keyup", this, false);
  container.addEventListener("keydown", this, false);
}

ContentCustomKeySender.prototype = {
  handleEvent: function handleEvent(aEvent) {
    if (Elements.contentShowing.getAttribute("disabled"))
      return;

    let browser = getBrowser();
    if (browser && browser.getAttribute("remote") == "true") {
      aEvent.stopPropagation();
      aEvent.preventDefault();

      browser.messageManager.sendAsyncMessage("Browser:KeyEvent", {
        type: aEvent.type,
        keyCode: aEvent.keyCode,
        charCode: (aEvent.type != "keydown") ? aEvent.charCode : null,
        modifiers: this._parseModifiers(aEvent)
      });
    }
  },

  _parseModifiers: function _parseModifiers(aEvent) {
    const masks = Ci.nsIDOMNSEvent;
    var mval = 0;
    if (aEvent.shiftKey)
      mval |= masks.SHIFT_MASK;
    if (aEvent.ctrlKey)
      mval |= masks.CONTROL_MASK;
    if (aEvent.altKey)
      mval |= masks.ALT_MASK;
    if (aEvent.metaKey)
      mval |= masks.META_MASK;
    return mval;
  },

  toString: function toString() {
    return "[ContentCustomKeySender] { }";
  }
};





function IdentityHandler() {
  this._staticStrings = {};
  this._staticStrings[this.IDENTITY_MODE_DOMAIN_VERIFIED] = {
    encryption_label: Strings.browser.GetStringFromName("identity.encrypted2")
  };
  this._staticStrings[this.IDENTITY_MODE_IDENTIFIED] = {
    encryption_label: Strings.browser.GetStringFromName("identity.encrypted2")
  };
  this._staticStrings[this.IDENTITY_MODE_UNKNOWN] = {
    encryption_label: Strings.browser.GetStringFromName("identity.unencrypted2")
  };

  
  Elements.browsers.addEventListener("URLChanged", this, true);

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
    return this._lastStatus.serverCert;
  },

  



  checkIdentity: function() {
    let browser = getBrowser();
    let state = browser.securityUI.state;
    let location = browser.currentURI;
    let currentStatus = browser.securityUI.SSLStatus;

    this._lastStatus = currentStatus;
    this._lastLocation = {};

    try {
      
      this._lastLocation = { host: location.hostPort, hostname: location.host, port: location.port == -1 ? "" : location.port};
    } catch(e) { }

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
    let strings = Strings.browser;

    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();

      
      
      var lookupHost = this._lastLocation.host;
      if (lookupHost.indexOf(':') < 0)
        lookupHost += ":443";

      
      if (!this._overrideService)
        this._overrideService = Cc["@mozilla.org/security/certoverride;1"].getService(Ci.nsICertOverrideService);

      
      
      var tooltip = strings.formatStringFromName("identity.identified.verifier", [iData.caOrg], 1);

      
      if (iData.isException)
        tooltip = strings.GetStringFromName("identity.identified.verified_by_you");
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
      tooltip = strings.formatStringFromName("identity.identified.verifier", [iData.caOrg], 1);
    }
    else {
      tooltip = strings.GetStringFromName("identity.unknown.tooltip");
    }

    
    this._identityBox.tooltipText = tooltip;
  },

  






  setPopupMessages: function(newMode) {
    this._identityPopup.setAttribute("mode", newMode);
    this._identityPopupContentBox.className = newMode;

    
    this._identityPopupEncLabel.textContent = this._staticStrings[newMode].encryption_label;

    
    var supplemental = "";
    var verifier = "";

    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();
      var host = this.getEffectiveHost();
      var owner = Strings.browser.GetStringFromName("identity.ownerUnknown2");
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
        supplemental += Strings.browser.formatStringFromName("identity.identified.state_and_country", [iData.state, iData.country], 2);
      else if (iData.state) 
        supplemental += iData.state;
      else if (iData.country) 
        supplemental += iData.country;
    } else {
      
      host = "";
      owner = "";
    }

    
    this._identityPopupContentHost.textContent = host;
    this._identityPopupContentOwner.textContent = owner;
    this._identityPopupContentSupp.textContent = supplemental;
    this._identityPopupContentVerif.textContent = verifier;

    PageActions.updateSiteMenu();
  },

  show: function ih_show() {
    Elements.contentShowing.setAttribute("disabled", "true");

    
    BrowserUI.activePanel = null;
    while (BrowserUI.activeDialog)
      BrowserUI.activeDialog.close();

    this._identityPopup.hidden = false;
    this._identityPopup.top = BrowserUI.toolbarH - this._identityPopup.offset;
    this._identityPopup.anchorTo(this._identityBox);

    this._identityBox.setAttribute("open", "true");

    
    this.setPopupMessages(this._identityBox.getAttribute("mode") || this.IDENTITY_MODE_UNKNOWN);

    BrowserUI.pushPopup(this, [this._identityPopup, this._identityBox, Elements.toolbarContainer]);
    BrowserUI.lockToolbar();
  },

  hide: function ih_hide() {
    Elements.contentShowing.setAttribute("disabled", "false");

    this._identityPopup.hidden = true;
    this._identityBox.removeAttribute("open");

    BrowserUI.popPopup(this);
    BrowserUI.unlockToolbar();
  },

  toggle: function ih_toggle() {
    if (this._identityPopup.hidden)
      this.show();
    else
      this.hide();
  },

  


  handleIdentityButtonEvent: function(aEvent) {
    aEvent.stopPropagation();

    if ((aEvent.type == "click" && aEvent.button != 0) ||
        (aEvent.type == "keypress" && aEvent.charCode != KeyEvent.DOM_VK_SPACE &&
         aEvent.keyCode != KeyEvent.DOM_VK_RETURN))
      return; 

    this.toggle();
  },

  handleEvent: function(aEvent) {
    if (aEvent.type == "URLChanged" && aEvent.target == Browser.selectedBrowser && !this._identityPopup.hidden)
      this.hide();
  }
};

var gIdentityHandler;





function getIdentityHandler() {
  if (!gIdentityHandler)
    gIdentityHandler = new IdentityHandler();
  return gIdentityHandler;
}





var PopupBlockerObserver = {
  onUpdatePageReport: function onUpdatePageReport(aEvent)
  {
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
        var message;
        var popupCount = cBrowser.pageReport.length;

        let strings = Strings.browser;
        if (popupCount > 1)
          message = strings.formatStringFromName("popupWarningMultiple", [brandShortName, popupCount], 2);
        else
          message = strings.formatStringFromName("popupWarning", [brandShortName], 1);

        var notificationBox = Browser.getNotificationBox();
        var notification = notificationBox.getNotificationWithValue("popup-blocked");
        if (notification) {
          notification.label = message;
        }
        else {
          var buttons = [
            {
              label: strings.GetStringFromName("popupButtonAllowOnce"),
              accessKey: null,
              callback: function() { PopupBlockerObserver.showPopupsForSite(); }
            },
            {
              label: strings.GetStringFromName("popupButtonAlwaysAllow2"),
              accessKey: null,
              callback: function() { PopupBlockerObserver.allowPopupsForSite(true); }
            },
            {
              label: strings.GetStringFromName("popupButtonNeverWarn2"),
              accessKey: null,
              callback: function() { PopupBlockerObserver.allowPopupsForSite(false); }
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

        
        
        
        
        
        if (popupURIspec == "" || popupURIspec == "about:blank" || popupURIspec == uri.spec)
          continue;

        let popupFeatures = pageReport[i].popupWindowFeatures;
        let popupName = pageReport[i].popupWindowName;

        Browser.addTab(popupURIspec, false, Browser.selectedTab);
      }
    }
  }
};

var XPInstallObserver = {
  observe: function xpi_observer(aSubject, aTopic, aData)
  {
    switch (aTopic) {
      case "addon-install-started":
        var messageString = Strings.browser.GetStringFromName("alertAddonsDownloading");
        ExtensionsView.showAlert(messageString);
        break;
      case "addon-install-blocked":
        var installInfo = aSubject.QueryInterface(Ci.amIWebInstallInfo);
        var host = installInfo.originatingURI.host;
        var brandShortName = Strings.brand.GetStringFromName("brandShortName");
        var notificationName, messageString, buttons;
        var strings = Strings.browser;
        var enabled = true;
        try {
          enabled = Services.prefs.getBoolPref("xpinstall.enabled");
        }
        catch (e) {}
        if (!enabled) {
          notificationName = "xpinstall-disabled";
          if (Services.prefs.prefIsLocked("xpinstall.enabled")) {
            messageString = strings.GetStringFromName("xpinstallDisabledMessageLocked");
            buttons = [];
          }
          else {
            messageString = strings.formatStringFromName("xpinstallDisabledMessage", [brandShortName, host], 2);
            buttons = [{
              label: strings.GetStringFromName("xpinstallDisabledButton"),
              accessKey: null,
              popup: null,
              callback: function editPrefs() {
                Services.prefs.setBoolPref("xpinstall.enabled", true);
                return false;
              }
            }];
          }
        }
        else {
          notificationName = "xpinstall";
          messageString = strings.formatStringFromName("xpinstallPromptWarning", [brandShortName, host], 2);

          buttons = [{
            label: strings.GetStringFromName("xpinstallPromptAllowButton"),
            accessKey: null,
            popup: null,
            callback: function() {
              
              installInfo.install();
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

var SessionHistoryObserver = {
  observe: function sho_observe(aSubject, aTopic, aData) {
    if (aTopic != "browser:purge-session-history")
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

var ContentCrashObserver = {
  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },

  observe: function cco_observe(aSubject, aTopic, aData) {
    if (aTopic != "ipc:content-shutdown") {
      Cu.reportError("ContentCrashObserver unexpected topic: " + aTopic);
      return;
    }

    if (!aSubject.QueryInterface(Ci.nsIPropertyBag2).hasKey("abnormal"))
      return;

    
    let env = Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment);
    let shutdown = env.get("MOZ_CRASHREPORTER_SHUTDOWN");
    if (shutdown) {
      let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
      appStartup.quit(Ci.nsIAppStartup.eForceQuit);
      return;
    }

    let hideUI = env.get("MOZ_CRASHREPORTER_NO_REPORT");
    if (hideUI)
      return;

    
    
    Browser.tabs.forEach(function(aTab) {
      if (aTab.browser.getAttribute("remote") == "true")
        aTab.resurrect();
    })

    let dumpID = aSubject.hasKey("dumpID") ? aSubject.getProperty("dumpID") : null;

    
    setTimeout(function(self) {
      
      
      let title = Strings.browser.GetStringFromName("tabs.crashWarningTitle");
      let message = Strings.browser.GetStringFromName("tabs.crashWarningMsg");
      let submitText = Strings.browser.GetStringFromName("tabs.crashSubmitReport");
      let reloadText = Strings.browser.GetStringFromName("tabs.crashReload");
      let closeText = Strings.browser.GetStringFromName("tabs.crashClose");
      let buttons = Ci.nsIPrompt.BUTTON_POS_1_DEFAULT +
                    (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_0) +
                    (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_1);

      
      if (!dumpID)
        submitText = null;

      let submit = { value: true };
      let reload = Services.prompt.confirmEx(window, title, message, buttons, closeText, reloadText, null, submitText, submit);
      if (reload) {
        
        let event = document.createEvent("Events");
        event.initEvent("TabSelect", true, false);
        event.lastTab = null;
        Browser.selectedTab.chromeTab.dispatchEvent(event);
      } else {
        
        
        if (Browser.tabs.length == 1) {
          
          let fallbackURL = Browser.getHomePage({ useDefault: true });
          Browser.addTab(fallbackURL, false, null, { getAttention: false });
        }

        
        
        Browser.closeTab(Browser.selectedTab);
      }

      
      if (submit.value && dumpID)
        self.CrashSubmit.submit(dumpID, Elements.stack, null, null);
    }, 0, this);
  }
};

var MemoryObserver = {
  _lastOOM: 0,

  observe: function mo_observe(aSubject, aTopic, aData) {
    if (aData == "heap-minimize") {
      
      return;
    }

    if (this._lastOOM != 0 &&
        Date.now() - this._lastOOM < 10000) {
      let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
      appStartup.quit(Ci.nsIAppStartup.eForceQuit);
      return;
    }

    for (let i = Browser.tabs.length - 1; i >= 0; i--) {
      let tab = Browser.tabs[i];
      if (tab == Browser.selectedTab)
        continue;
      tab.resurrect();
    }

    window.QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIDOMWindowUtils).garbageCollect();
    Cu.forceGC();
    this._lastOOM = Date.now();
  }
};

function getNotificationBox(aBrowser) {
  return Browser.getNotificationBox(aBrowser);
}

function importDialog(aParent, aSrc, aArguments) {
  
  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
  xhr.open("GET", aSrc, false);
  xhr.overrideMimeType("text/xml");
  xhr.send(null);
  if (!xhr.responseXML)
    return null;

  let currentNode;
  let nodeIterator = xhr.responseXML.createNodeIterator(xhr.responseXML, NodeFilter.SHOW_TEXT, null, false);
  while (currentNode = nodeIterator.nextNode()) {
    let trimmed = currentNode.nodeValue.replace(/^\s\s*/, "").replace(/\s\s*$/, "");
    if (!trimmed.length)
      currentNode.parentNode.removeChild(currentNode);
  }

  let doc = xhr.responseXML.documentElement;

  var dialog  = null;

  
  
  let menulistContainer = document.getElementById("menulist-container");
  let parentNode = menulistContainer.parentNode;

  
  let event = document.createEvent("Events");
  event.initEvent("DOMWillOpenModalDialog", true, false);
  let dispatcher = aParent || getBrowser();
  dispatcher.dispatchEvent(event);

  
  let back = document.createElement("box");
  back.setAttribute("class", "modal-block");
  dialog = back.appendChild(document.importNode(doc, true));
  parentNode.insertBefore(back, menulistContainer);

  dialog.arguments = aArguments;
  dialog.parent = aParent;
  return dialog;
}

function showDownloadManager(aWindowContext, aID, aReason) {
  BrowserUI.showPanel("downloads-container");
  
}


var OfflineApps = {
  offlineAppRequested: function(aRequest, aTarget) {
    if (!Services.prefs.getBoolPref("browser.offline-apps.notify"))
      return;

    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);

    
    if (Services.perms.testExactPermission(currentURI, "offline-app") != Ci.nsIPermissionManager.UNKNOWN_ACTION)
      return;

    try {
      if (Services.prefs.getBoolPref("offline-apps.allow_by_default")) {
        
        return;
      }
    } catch(e) {
      
    }

    let host = currentURI.asciiHost;
    let notificationID = "offline-app-requested-" + host;
    let notificationBox = Browser.getNotificationBox();

    let notification = notificationBox.getNotificationWithValue(notificationID);
    let strings = Strings.browser;
    if (notification) {
      notification.documents.push(aRequest);
    } else {
      let buttons = [{
        label: strings.GetStringFromName("offlineApps.allow"),
        accessKey: null,
        callback: function() {
          for (let i = 0; i < notification.documents.length; i++)
            OfflineApps.allowSite(notification.documents[i], aTarget);
        }
      },{
        label: strings.GetStringFromName("offlineApps.never"),
        accessKey: null,
        callback: function() {
          for (let i = 0; i < notification.documents.length; i++)
            OfflineApps.disallowSite(notification.documents[i]);
        }
      },{
        label: strings.GetStringFromName("offlineApps.notNow"),
        accessKey: null,
        callback: function() {  }
      }];

      const priority = notificationBox.PRIORITY_INFO_LOW;
      let message = strings.formatStringFromName("offlineApps.available", [host], 1);
      notification = notificationBox.appendNotification(message, notificationID, "", priority, buttons);
      notification.documents = [aRequest];
    }
  },

  allowSite: function(aRequest, aTarget) {
    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);
    Services.perms.add(currentURI, "offline-app", Ci.nsIPermissionManager.ALLOW_ACTION);

    
    
    
    aTarget.messageManager.sendAsyncMessage("Browser:MozApplicationCache:Fetch", aRequest);
  },

  disallowSite: function(aRequest) {
    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);
    Services.perms.add(currentURI, "offline-app", Ci.nsIPermissionManager.DENY_ACTION);
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (aMessage.name == "Browser:MozApplicationManifest") {
      this.offlineAppRequested(aMessage.json, aMessage.target);
    }
  }
};

function Tab(aURI, aParams) {
  this._id = null;
  this._browser = null;
  this._notification = null;
  this._loading = false;
  this._chromeTab = null;
  this._metadata = null;

  this.useFallbackWidth = false;
  this.owner = null;

  this.hostChanged = false;
  this.state = null;

  
  
  this.lastSelected = 0;

  
  
  this.create(aURI, aParams || {});
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

  get metadata() {
    return this._metadata || kDefaultMetadata;
  },

  get inputHandler() {
    if (!this._notification)
      return null;
    return this._notification.inputHandler;
  },

  
  updateViewportMetadata: function updateViewportMetadata(aMetadata) {
    if (aMetadata && aMetadata.autoScale) {
      let scaleRatio = aMetadata.scaleRatio = Browser.getScaleRatio();

      if ("defaultZoom" in aMetadata && aMetadata.defaultZoom > 0)
        aMetadata.defaultZoom *= scaleRatio;
      if ("minZoom" in aMetadata && aMetadata.minZoom > 0)
        aMetadata.minZoom *= scaleRatio;
      if ("maxZoom" in aMetadata && aMetadata.maxZoom > 0)
        aMetadata.maxZoom *= scaleRatio;
    }
    this._metadata = aMetadata;
    this.updateViewportSize();
  },

  


  updateViewportSize: function updateViewportSize() {
    let browser = this._browser;
    if (!browser)
      return;

    let screenW = ViewableAreaObserver.width;
    let screenH = ViewableAreaObserver.height;
    let viewportW, viewportH;

    let metadata = this.metadata;
    if (metadata.autoSize) {
      if ("scaleRatio" in metadata) {
        viewportW = screenW / metadata.scaleRatio;
        viewportH = screenH / metadata.scaleRatio;
      } else {
        viewportW = screenW;
        viewportH = screenH;
      }
    } else {
      viewportW = metadata.width;
      viewportH = metadata.height;

      
      let maxInitialZoom = metadata.defaultZoom || metadata.maxZoom;
      if (maxInitialZoom && viewportW)
        viewportW = Math.max(viewportW, screenW / maxInitialZoom);

      let validW = viewportW > 0;
      let validH = viewportH > 0;

      if (validW && !validH) {
        viewportH = viewportW * (screenH / screenW);
      } else if (!validW && validH) {
        viewportW = viewportH * (screenW / screenH);
      } else if (!validW && !validH) {
        viewportW = this.useFallbackWidth ? kFallbackBrowserWidth : kDefaultBrowserWidth;
        viewportH = kDefaultBrowserWidth * (screenH / screenW);

        
        
        viewportH = Math.max(viewportH, screenH * (browser.contentDocumentWidth / screenW));
      }
    }

    browser.setWindowSize(viewportW, viewportH);
  },

  restoreViewportPosition: function restoreViewportPosition(aOldWidth, aNewWidth) {
    let browser = this._browser;
    let view = browser.getRootView();
    let pos = view.getPosition();

    
    let oldScale = browser.scale;
    let newScale = this.clampZoomLevel(oldScale * aNewWidth / aOldWidth);
    browser.scale = newScale;

    
    let scaleRatio = newScale / oldScale;
    view.scrollTo(pos.x * scaleRatio, pos.y * scaleRatio);
  },

  startLoading: function startLoading() {
    if (this._loading) throw "Already Loading!";
    this._loading = true;
  },

  endLoading: function endLoading() {
    if (!this._loading) throw "Not Loading!";
    this._loading = false;
  },

  isLoading: function isLoading() {
    return this._loading;
  },

  create: function create(aURI, aParams) {
    this._chromeTab = document.getElementById("tabs").addTab();
    let browser = this._createBrowser(aURI, null);

    
    if ("delayLoad" in aParams && aParams.delayLoad)
      return;

    let flags = aParams.flags || Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    browser.loadURIWithFlags(aURI, flags, aParams.referrerURI, aParams.charset, aParams.postData);
  },

  destroy: function destroy() {
    document.getElementById("tabs").removeTab(this._chromeTab);
    this._chromeTab = null;
    this._destroyBrowser();
  },

  resurrect: function resurrect() {
    let dead = this._browser;

    
    let session = { data: dead.__SS_data, extra: dead.__SS_extdata };

    
    let currentURL = dead.currentURI.spec;
    let sibling = dead.nextSibling;

    
    this._destroyBrowser();
    let browser = this._createBrowser(currentURL, sibling);

    
    browser.__SS_data = session.data;
    browser.__SS_extdata = session.extra;
    browser.__SS_restore = true;
  },

  _createBrowser: function _createBrowser(aURI, aInsertBefore) {
    if (this._browser)
      throw "Browser already exists";

    
    let notification = this._notification = document.createElement("notificationbox");
    notification.classList.add("inputHandler");

    
    let browser = this._browser = document.createElement("browser");
    browser.setAttribute("class", "viewable-width viewable-height");
    this._chromeTab.linkedBrowser = browser;

    browser.setAttribute("type", "content");

    let useRemote = Services.prefs.getBoolPref("browser.tabs.remote");
    let useLocal = Util.isLocalScheme(aURI);
    browser.setAttribute("remote", (!useLocal && useRemote) ? "true" : "false");

    
    notification.appendChild(browser);
    Elements.browsers.insertBefore(notification, aInsertBefore);

    
    browser.stop();

    const i = Ci.nsIFrameLoader_MOZILLA_2_0_BRANCH;
    let fl = browser.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
    let enabler = fl.QueryInterface(i);
    enabler.renderMode = i.RENDER_MODE_ASYNC_SCROLL;

    browser.messageManager.addMessageListener("MozScrolledAreaChanged", (function() {
      
      setTimeout(this.scrolledAreaChanged.bind(this), 0);
    }).bind(this));

    return browser;
  },

  _destroyBrowser: function _destroyBrowser() {
    if (this._browser) {
      let notification = this._notification;
      let browser = this._browser;
      browser.active = false;

      this._notification = null;
      this._browser = null;
      this._loading = false;

      Elements.browsers.removeChild(notification);
    }
  },

  clampZoomLevel: function clampZoomLevel(aScale) {
    let browser = this._browser;
    let bounded = Util.clamp(aScale, ZoomManager.MIN, ZoomManager.MAX);

    let md = this.metadata;
    if (md && md.minZoom)
      bounded = Math.max(bounded, md.minZoom);
    if (md && md.maxZoom)
      bounded = Math.min(bounded, md.maxZoom);

    bounded = Math.max(bounded, this.getPageZoomLevel());

    let rounded = Math.round(bounded * kBrowserViewZoomLevelPrecision) / kBrowserViewZoomLevelPrecision;
    return rounded || 1.0;
  },

  
  resetZoomLevel: function resetZoomLevel() {
    this._defaultZoomLevel = this._browser.scale;
  },

  scrolledAreaChanged: function scrolledAreaChanged() {
    this.updateDefaultZoomLevel();

    if (!this.useFallbackWidth && this._browser.contentDocumentWidth > kDefaultBrowserWidth)
      this.useFallbackWidth = true;

    this.updateViewportSize();
  },

  



  updateDefaultZoomLevel: function updateDefaultZoomLevel() {
    let browser = this._browser;
    if (!browser)
      return;

    let isDefault = this.isDefaultZoomLevel();
    this._defaultZoomLevel = this.getDefaultZoomLevel();
    if (isDefault)
      browser.scale = this._defaultZoomLevel;
  },

  isDefaultZoomLevel: function isDefaultZoomLevel() {
    return this._browser.scale == this._defaultZoomLevel;
  },

  getDefaultZoomLevel: function getDefaultZoomLevel() {
    let md = this.metadata;
    if (md && md.defaultZoom)
      return this.clampZoomLevel(md.defaultZoom);

    let pageZoom = this.getPageZoomLevel();

    
    let granularity = Services.prefs.getIntPref("browser.ui.zoom.pageFitGranularity");
    let threshold = 1 - 1 / granularity;
    if (threshold < pageZoom && pageZoom < 1)
      pageZoom = 1;

    return this.clampZoomLevel(pageZoom);
  },

  getPageZoomLevel: function getPageZoomLevel() {
    let browserW = this._browser.contentDocumentWidth;
    if (browserW == 0)
      return 1.0;

    return this._browser.getBoundingClientRect().width / browserW;
  },

  get allowZoom() {
    return this.metadata.allowZoom;
  },

  updateThumbnail: function updateThumbnail() {
    let browser = this._browser;

    
    
    if (!browser || this._thumbnailWindowId == browser.contentWindowId)
      return;

    
    
    if (!browser.contentWindowWidth || !browser.contentWindowHeight)
      return;

    this._thumbnailWindowId = browser.contentWindowId;
    this._chromeTab.updateThumbnail(browser, browser.contentWindowWidth, browser.contentWindowHeight);
  },

  set active(aActive) {
    if (!this._browser)
      return;

    let notification = this._notification;
    let browser = this._browser;

    if (aActive) {
      browser.setAttribute("type", "content-primary");
      Elements.browsers.selectedPanel = notification;
      browser.active = true;
      document.getElementById("tabs").selectedTab = this._chromeTab;
    }
    else {
      browser.messageManager.sendAsyncMessage("Browser:Blur", { });
      browser.setAttribute("type", "content");
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

var ViewableAreaObserver = {
  get width() {
    return this._width || window.innerWidth;
  },

  get height() {
    return (this._height || window.innerHeight) - Elements.contentNavigator.getBoundingClientRect().height;
  },

  observe: function va_observe(aSubject, aTopic, aData) {
    let rect = Rect.fromRect(JSON.parse(aData));
    let height = rect.bottom - rect.top;
    let width = rect.right - rect.left;
    if (height == window.innerHeight && width == window.innerWidth) {
      this._height = null;
      this._width = null;
    }
    else {
      this._height = height;
      this._width = width;
    }
    this.update();
  },

  update: function va_update() {
    let oldHeight = parseInt(Browser.styles["viewable-height"].height);
    let oldWidth = parseInt(Browser.styles["viewable-width"].width);

    let newWidth = this.width;
    let newHeight = this.height;
    if (newHeight != oldHeight || newWidth != oldWidth) {
      Browser.styles["viewable-height"].height = newHeight + "px";
      Browser.styles["viewable-height"].maxHeight = newHeight + "px";

      Browser.styles["viewable-width"].width = newWidth + "px";
      Browser.styles["viewable-width"].maxWidth = newWidth + "px";

      for (let i = Browser.tabs.length - 1; i >= 0; i--) {
        let tab = Browser.tabs[i];
        tab.updateViewportSize();

        
        
        if (tab.browser.contentWindowWidth == oldWidth)
          tab.restoreViewportPosition(oldWidth, newWidth);
      }

      
      setTimeout(function() {
        let event = document.createEvent("Events");
        event.initEvent("SizeChanged", true, false);
        Elements.browsers.dispatchEvent(event);
      }, 0);
    }
  }
};
