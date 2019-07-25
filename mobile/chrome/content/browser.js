

















































let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

function getBrowser() {
  return Browser.selectedBrowser;
}

const kDefaultBrowserWidth = 800;
const kBrowserFormZoomLevelMin = 1.0;
const kBrowserFormZoomLevelMax = 2.0;
const kBrowserViewZoomLevelPrecision = 10000;


window.sizeToContent = function() {
  Components.utils.reportError("window.sizeToContent is not allowed in this window");
}

#ifdef MOZ_CRASH_REPORTER
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
  "@mozilla.org/xre/app-info;1", "nsICrashReporter");
#endif

const endl = '\n';

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

  switch (ev.charCode) {
  case f:
    MemoryObserver.observe();
    dump("Forced a GC\n");
    break;
#ifndef MOZ_PLATFORM_MAEMO
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

var ih = null;

var Browser = {
  _tabs : [],
  _selectedTab : null,
  windowUtils: window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils),
  controlsScrollbox: null,
  controlsScrollboxScroller: null,
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

    let needOverride = Util.needHomepageOverride();
    if (needOverride == "new profile")
      this.initNewProfile();

    let container = document.getElementById("browsers");
    

    
    let inputHandlerOverlay = document.getElementById("inputhandler-overlay");
    inputHandlerOverlay.customClicker = new ContentCustomClicker();
    inputHandlerOverlay.customKeySender = new ContentCustomKeySender();
    inputHandlerOverlay.customDragger = new Browser.MainDragger();

    
    
    
    this.contentScrollbox = container;
    this.contentScrollboxScroller = {
      scrollBy: function(x, y) {
        getBrowser().scrollBy(x, y);
      },

      scrollTo: function(x, y) {
        getBrowser().scrollTo(x, y);
      },

      getPosition: function(scrollX, scrollY) {
        getBrowser().getPosition(scrollX, scrollY);
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
    for each (let style in ["viewport-width", "viewport-height", "window-width", "window-height", "toolbar-height"]) {
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

      let toolbarHeight = Math.round(document.getElementById("toolbar-main").getBoundingClientRect().height);
      let scaledDefaultH = (kDefaultBrowserWidth * (h / w));
      let scaledScreenH = (window.screen.width * (h / w));
      let dpiScale = Services.prefs.getIntPref("zoom.dpiScale") / 100;

      Browser.styles["window-width"].width = w + "px";
      Browser.styles["window-height"].height = h + "px";
      Browser.styles["toolbar-height"].height = toolbarHeight + "px";

      
      BrowserUI.sizeControls(w, h);

      
      Browser.hideSidebars();

      for (let i = Browser.tabs.length - 1; i >= 0; i--)
        Browser.tabs[i].updateViewportSize();

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
    }
    let notifications = document.getElementById("notifications");
    notifications.addEventListener("AlertActive", notificationHandler, false);
    notifications.addEventListener("AlertClose", notificationHandler, false);

    BrowserUI.init();

    
    ih = new InputHandler(inputHandlerOverlay);

    window.controllers.appendController(this);
    window.controllers.appendController(BrowserUI);

    var os = Services.obs;
    os.addObserver(gXPInstallObserver, "addon-install-blocked", false);
    os.addObserver(gSessionHistoryObserver, "browser:purge-session-history", false);

    
    os.addObserver(MemoryObserver, "memory-pressure", false);

    
    os.addObserver(BrowserSearch, "browser-search-engine-modified", false);

    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = new nsBrowserAccess();

    let browsers = document.getElementById("browsers");
    browsers.addEventListener("command", this._handleContentCommand, true);
    browsers.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);

    
    Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
    Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2);

    
    Util.forceOnline();

    
    let whereURI = this.getHomePage();
    if (needOverride == "new profile")
        whereURI = "about:firstrun";

    
    
    if (window.arguments && window.arguments[0]) {
      if (window.arguments[0] instanceof Ci.nsICommandLine) {
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
      else {
        
        whereURI = window.arguments[0];
      }
    } 

    this.addTab(whereURI, true);

    
    if (Services.prefs.getBoolPref("browser.console.showInPanel")){
      let button = document.getElementById("tool-console");
      button.hidden = false;
    }

    
    if (Services.prefs.prefHasUserValue("extensions.disabledAddons")) {
      let addons = Services.prefs.getCharPref("extensions.disabledAddons").split(",");
      if (addons.length > 0) {
        let disabledStrings = Elements.browserBundle.getString("alertAddonsDisabled");
        let label = PluralForm.get(addons.length, disabledStrings).replace("#1", addons.length);

        let alerts = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
        alerts.showAlertNotification(URI_GENERIC_ICON_XPINSTALL, Elements.browserBundle.getString("alertAddons"),
                                     label, false, "", null);
      }
      Services.prefs.clearUserPref("extensions.disabledAddons");
    }

    messageManager.addMessageListener("Browser:ViewportMetadata", this);
    messageManager.addMessageListener("Browser:FormSubmit", this);
    messageManager.addMessageListener("Browser:KeyPress", this);
    messageManager.addMessageListener("Browser:ZoomToPoint:Return", this);
    messageManager.addMessageListener("Browser:MozApplicationManifest", OfflineApps);

    
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

        let messageBase = Elements.browserBundle.getString("tabs.closeWarning");
        let message = PluralForm.get(numTabs, messageBase).replace("#1", numTabs);

        let title = Elements.browserBundle.getString("tabs.closeWarningTitle");
        let closeText = Elements.browserBundle.getString("tabs.closeButton");
        let checkText = Elements.browserBundle.getString("tabs.closeWarningPromptMe");
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

    var os = Services.obs;
    os.removeObserver(gXPInstallObserver, "addon-install-blocked");
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
      url = Services.prefs.getComplexValue("browser.startup.homepage", Ci.nsIPrefLocalizedString).data;
    } catch (e) { }

    return url;
  },

  get browsers() {
    return this._tabs.map(function(tab) { return tab.browser; });
  },

  scrollContentToTop: function scrollContentToTop() {
    this.contentScrollboxScroller.scrollTo(0, 0);
    this.pageScrollboxScroller.scrollTo(0, 0);
  },

  hideSidebars: function scrollSidebarsOffscreen() {
    let container = document.getElementById("browsers");
    let rect = container.getBoundingClientRect();
    this.controlsScrollboxScroller.scrollBy(Math.round(rect.left), 0);
  },

  hideTitlebar: function hideTitlebar() {
    let container = document.getElementById("browsers");
    let rect = container.getBoundingClientRect();
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
      if (currentURI == "about:blank" && !browser.canGoBack && !browser.canGoForward) {
        this.closeTab(oldTab);
        oldTab = null;
      }
      let tab = Browser.addTab(aURI, true, oldTab);
      tab.browser.stop();
    }

    let params = aParams || {};
    let flags = params.flags || Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    getBrowser().loadURIWithFlags(aURI, flags, params.referrerURI, params.charset, params.postData);
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

  addTab: function(uri, bringFront, aOwner) {
    let newTab = new Tab(uri);
    newTab.owner = aOwner || null;
    this._tabs.push(newTab);

    if (bringFront)
      this.selectedTab = newTab;

    let event = document.createEvent("Events");
    event.initEvent("TabOpen", true, false);
    newTab.chromeTab.dispatchEvent(event);
    newTab.browser.messageManager.sendAsyncMessage("Browser:TabOpen");

    return newTab;
  },

  closeTab: function(tab) {
    if (tab instanceof XULElement)
      tab = this.getTabFromChrome(tab);

    if (!tab)
      return;

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

    this.selectedTab = nextTab;

    tab.destroy();
    this._tabs.splice(tabIndex, 1);
  },

  get selectedTab() {
    return this._selectedTab;
  },

  set selectedTab(tab) {
    if (tab instanceof XULElement)
      tab = this.getTabFromChrome(tab);

    if (!tab || this._selectedTab == tab)
      return;

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

    if (oldBrowser) {
      oldBrowser.setAttribute("type", "content");
      oldBrowser.style.display = "none";
      oldBrowser.messageManager.sendAsyncMessage("Browser:Blur", {});
    }

    if (browser) {
      browser.setAttribute("type", "content-primary");
      browser.style.display = "";
      browser.messageManager.sendAsyncMessage("Browser:Focus", {});
    }

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

    if (tab.pageScrollOffset) {
      let { x: pageScrollX, y: pageScrollY } = tab.pageScrollOffset;
      Browser.pageScrollboxScroller.scrollTo(pageScrollX, pageScrollY);
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

  





  _handleContentCommand: function _handleContentCommand(aEvent) {
    
    if (!aEvent.isTrusted)
      return;

    var ot = aEvent.originalTarget;
    var errorDoc = ot.ownerDocument;

    
    
    if (/^about:certerror\?e=nssBadCert/.test(errorDoc.documentURI)) {
      if (ot == errorDoc.getElementById("temporaryExceptionButton") ||
          ot == errorDoc.getElementById("permanentExceptionButton")) {
        try {
          
          let uri = Services.io.newURI(errorDoc.location.href, null, null);
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
        
        var defaultPrefs = Services.prefs.getDefaultBranch(null);
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

    if (leftbarCBR.left > ritebarCBR.left)
      [ritebarCBR, leftbarCBR] = [leftbarCBR, ritebarCBR]; 

    let leftbar = new Rect(Math.round(leftbarCBR.left) - Math.round(dx), 0, Math.round(leftbarCBR.width), 1);
    let ritebar = new Rect(Math.round(ritebarCBR.left) - Math.round(dx), 0, Math.round(ritebarCBR.width), 1);
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
    let tab = this.selectedTab;
    if (!tab.allowZoom)
      return;

    let zoomLevel = getBrowser().scale;
    let zoomValues = ZoomManager.zoomValues;
    let i = zoomValues.indexOf(ZoomManager.snap(zoomLevel)) + (aDirection < 0 ? 1 : -1);
    if (i >= 0 && i < zoomValues.length)
      zoomLevel = zoomValues[i];

    zoomLevel = tab.clampZoomLevel(zoomLevel);

    let centerX = getBrowser().viewportScrollX + window.innerWidth / 2;
    let centerY = getBrowser().viewportScrollY + window.innerHeight / 2;
    this.animatedZoomTo(this._getZoomRectForPoint(centerX, centerY, zoomLevel));
  },

  
  _getZoomLevelForRect: function _getZoomLevelForRect(rect) {
    const margin = 15;
    return this.selectedTab.clampZoomLevel(window.innerWidth / (rect.width + margin * 2));
  },

  



  _getZoomRectForRect: function _getZoomRectForRect(rect, y) {
    let oldZoomLevel = getBrowser().scale;
    let zoomLevel = this._getZoomLevelForRect(rect);
    let zoomRatio = oldZoomLevel / zoomLevel;

    
    
    
    let zoomTolerance = (this.selectedTab.isDefaultZoomLevel()) ? .9 : .6666;
    if (zoomRatio >= zoomTolerance)
      return null;
    else
      return this._getZoomRectForPoint(rect.center().x, y, zoomLevel);
  },

  



  _getZoomRectForPoint: function _getZoomRectForPoint(x, y, zoomLevel) {
    x = x * getBrowser().scale;
    y = y * getBrowser().scale;

    zoomLevel = Math.min(ZoomManager.MAX, zoomLevel);
    let zoomRatio = zoomLevel / getBrowser().scale;
    let newVisW = window.innerWidth / zoomRatio, newVisH = window.innerHeight / zoomRatio;
    let result = new Rect(x - newVisW / 2, y - newVisH / 2, newVisW, newVisH);

    
    return result.translateInside(new Rect(0, 0, getBrowser()._widthInDevicePx, getBrowser()._heightInDevicePx));
  },

  animatedZoomTo: function animatedZoomTo(rect) {
    let zoom = new AnimatedZoom();
    zoom.animateTo(rect);
  },

  setVisibleRect: function setVisibleRect(rect) {
    let zoomRatio = window.innerWidth / rect.width;
    let zoomLevel = getBrowser().scale * zoomRatio;
    let scrollX = rect.left * zoomRatio;
    let scrollY = rect.top * zoomRatio;

    this.hideSidebars();
    this.hideTitlebar();

    getBrowser().setScale(this.selectedTab.clampZoomLevel(zoomLevel));
    getBrowser().scrollTo(scrollX, scrollY);
  },

  zoomToPoint: function zoomToPoint(cX, cY, aRect) {
    let tab = this.selectedTab;
    if (!tab.allowZoom)
      return null;

    let zoomRect = null;
    if (aRect)
      zoomRect = this._getZoomRectForRect(aRect, cY);

    if (!zoomRect && tab.isDefaultZoomLevel())
      zoomRect = this._getZoomRectForPoint(cX, cY, getBrowser().scale * 2);

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

  


  clientToBrowserView: function clientToBrowserView(x, y) {
    let container = document.getElementById("browsers");
    let containerBCR = container.getBoundingClientRect();

    let x0 = Math.round(containerBCR.left);
    let y0;
    if (arguments.length > 1)
      y0 = Math.round(containerBCR.top);

    let scrollX = {}, scrollY = {};
    getBrowser().getPosition(scrollX, scrollY);
    return (arguments.length > 1) ? [x - x0 + scrollX.value, y - y0 + scrollY.value] : (x - x0 + scrollX.value);
  },

  browserViewToClient: function browserViewToClient(x, y) {
    let container = document.getElementById("browsers");
    let containerBCR = container.getBoundingClientRect();

    let x0 = Math.round(-containerBCR.left);
    let y0;
    if (arguments.length > 1)
      y0 = Math.round(-containerBCR.top);

    return (arguments.length > 1) ? [x - x0, y - y0] : (x - x0);
  },

  browserViewToClientRect: function browserViewToClientRect(rect) {
    let container = document.getElementById("browsers");
    let containerBCR = container.getBoundingClientRect();
    return rect.clone().translate(Math.round(containerBCR.left), Math.round(containerBCR.top));
  },

  



  transformClientToBrowser: function transformClientToBrowser(cX, cY) {
    return this.clientToBrowserView(cX, cY).map(function(val) { return val / getBrowser().scale });
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
    switch (aMessage.name) {
      case "Browser:ViewportMetadata":
        let tab = Browser.getTabForBrowser(aMessage.target);
        
        
        if (tab)
          tab.updateViewportMetadata(json);
        break;

      case "Browser:FormSubmit":
        let browser = aMessage.target;
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
        
        let rect = Rect.fromRect(json.rect);
        if (!Browser.zoomToPoint(json.x, json.y, rect))
          Browser.zoomFromPoint(json.x, json.y);
        break;
    }
  }
};


Browser.MainDragger = function MainDragger() {
};

Browser.MainDragger.prototype = {
  isDraggable: function isDraggable(target, scroller) {
    return { x: true, y: true };
  },

  dragStart: function dragStart(clientX, clientY, target, scroller) {
  },

  dragStop: function dragStop(dx, dy, scroller) {
    this.dragMove(Browser.snapSidebars(), 0, scroller);
    Browser.tryUnfloatToolbar();
  },

  dragMove: function dragMove(dx, dy, scroller) {
    let doffset = new Point(dx, dy);

    
    let panOffset = this._panControlsAwayOffset(doffset);

    
    this._panScroller(Browser.contentScrollboxScroller, doffset);

    
    
    doffset.add(panOffset);
    Browser.tryFloatToolbar(doffset.x, 0);
    this._panScroller(Browser.controlsScrollboxScroller, doffset);
    this._panScroller(Browser.pageScrollboxScroller, doffset);

    return !doffset.equals(dx, dy);
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
      browser = Browser.addTab("about:blank", true, Browser.selectedTab).browser;
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

    return browser;
  },

  openURI: function(aURI, aOpener, aWhere, aContext) {
    let browser = this._getBrowser(aURI, aOpener, aWhere, aContext);
    return browser ? browser.contentWindow : null;
  },

  openURIInFrame: function(aURI, aOpener, aWhere, aContext) {
    let browser = this._getBrowser(aURI, aOpener, aWhere, aContext);
    return browser ? browser.QueryInterface(Ci.nsIFrameLoaderOwner) : null;
  },

  isTabContentWindow: function(aWindow) {
    return Browser.browsers.some(function (browser) browser.contentWindow == aWindow);
  }
};


const BrowserSearch = {
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

  get engines() {
    if (this._engines)
      return this._engines;

    let engines = Services.search.getVisibleEngines({ }).map(
      function(item, index, array) {
        return { 
          label: item.name,
          default: (item == Services.search.defaultEngine),
          image: item.iconURI ? item.iconURI.spec : null
        }
    });

    return this._engines = engines;
  },

  updatePageSearchEngines: function updatePageSearchEngines(aNode) {
    let items = Browser.selectedBrowser.searchEngines.filter(this.isPermanentSearchEngine);
    if (!items.length)
      return false;

    
    let engine = items[0];
    aNode.setAttribute("description", engine.title);
    aNode.onclick = function() {
      BrowserSearch.addPermanentSearchEngine(engine);
      PageActions.hideItem(aNode);
    };
    return true;
  },

  addPermanentSearchEngine: function addPermanentSearchEngine(aEngine) {
    let iconURL = BrowserUI._favicon.src;
    Services.search.addEngine(aEngine.href, Ci.nsISearchEngine.DATA_XML, iconURL, false);

    this._engines = null;
  },

  isPermanentSearchEngine: function isPermanentSearchEngine(aEngine) {
    return !BrowserSearch.engines.some(function(item) {
      return aEngine.title == item.name;
    });
  }
};



function ContentCustomClicker() {
}

ContentCustomClicker.prototype = {
  _dispatchMouseEvent: function _dispatchMouseEvent(aName, aX, aY, aModifiers) {
    let aX = aX || 0;
    let aY = aY || 0;
    let aModifiers = aModifiers || null;
    let browser = getBrowser();
    let [x, y] = Browser.transformClientToBrowser(aX, aY);
    browser.messageManager.sendAsyncMessage(aName, { x: x, y: y, modifiers: aModifiers });
  },

  mouseDown: function mouseDown(aX, aY) {
    
    let browser = getBrowser();
    let fl = browser.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
    browser.focus();
    try {
      fl.activateRemoteFrame();
    } catch (e) {
    }
    this._dispatchMouseEvent("Browser:MouseDown", aX, aY);
  },

  mouseUp: function mouseUp(aX, aY) {
  },

  panBegin: function panBegin() {
    TapHighlightHelper.hide();

    this._dispatchMouseEvent("Browser:MouseCancel");
  },

  singleClick: function singleClick(aX, aY, aModifiers) {
    TapHighlightHelper.hide();

    
    if (!ContextHelper.popupState)
      this._dispatchMouseEvent("Browser:MouseUp", aX, aY, aModifiers);
    this._dispatchMouseEvent("Browser:MouseCancel");
  },

  doubleClick: function doubleClick(aX1, aY1, aX2, aY2) {
    TapHighlightHelper.hide();

    this._dispatchMouseEvent("Browser:MouseCancel");

    const kDoubleClickRadius = 32;

    let maxRadius = kDoubleClickRadius * getBrowser().scale;
    let isClickInRadius = (Math.abs(aX1 - aX2) < maxRadius && Math.abs(aY1 - aY2) < maxRadius);
    if (isClickInRadius)
      this._dispatchMouseEvent("Browser:ZoomToPoint", aX1, aY1);
  },

  toString: function toString() {
    return "[ContentCustomClicker] { }";
  }
};



function ContentCustomKeySender() {
}

ContentCustomKeySender.prototype = {
  
  dispatchKeyEvent: function _dispatchKeyEvent(aEvent) {
    let browser = getBrowser();
    if (browser) {
      browser.messageManager.sendAsyncMessage("Browser:KeyEvent", {
        type: aEvent.type,
        keyCode: aEvent.keyCode,
        charCode: aEvent.charCode,
        modifiers: this._parseModifiers(aEvent)
      });
    }
  },

  _parseModifiers: function _parseModifiers(aEvent) {
    const masks = Components.interfaces.nsIDOMNSEvent;
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
    encryption_label: Elements.browserBundle.getString("identity.encrypted2")
  };
  this._staticStrings[this.IDENTITY_MODE_IDENTIFIED] = {
    encryption_label: Elements.browserBundle.getString("identity.encrypted2")
  };
  this._staticStrings[this.IDENTITY_MODE_UNKNOWN] = {
    encryption_label: Elements.browserBundle.getString("identity.unencrypted2")
  };

  
  document.getElementById("browsers").addEventListener("URLChanged", this, true);

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

      
      if (iData.isException)
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

    PageActions.updateSiteMenu();
  },

  show: function ih_show() {
    
    BrowserUI.activePanel = null;
    while (BrowserUI.activeDialog)
      BrowserUI.activeDialog.close();

    this._identityPopup.hidden = false;
    this._identityPopup.top = BrowserUI.toolbarH;
    this._identityPopup.focus();

    this._identityBox.setAttribute("open", "true");

    
    this.setPopupMessages(this._identityBox.getAttribute("mode") || this.IDENTITY_MODE_UNKNOWN);

    BrowserUI.pushPopup(this, [this._identityPopup, this._identityBox, Elements.toolbarContainer]);
    BrowserUI.lockToolbar();
  },

  hide: function ih_hide() {
    this._identityPopup.hidden = true;
    this._identityBox.removeAttribute("open");

    BrowserUI.popPopup();
    BrowserUI.unlockToolbar();
  },

  toggle: function ih_toggle() {
    
    
    if (Elements.urlbarState.getAttribute("mode") == "edit") {
      CommandUpdater.doCommand("cmd_opensearch");
      return;
    }

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
    if (aEvent.type == "URLChanged" && !this._identityPopup.hidden)
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
              callback: function() { gPopupBlockerObserver.allowPopupsForSite(true); }
            },
            {
              label: strings.getString("popupButtonNeverWarn2"),
              accessKey: null,
              callback: function() { gPopupBlockerObserver.allowPopupsForSite(false); }
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

        
        
        
        
        
        if (popupURIspec == "" || popupURIspec == "about:blank" ||
            popupURIspec == uri.spec)
          continue;

        let popupFeatures = pageReport[i].popupWindowFeatures;
        let popupName = pageReport[i].popupWindowName;

        Browser.addTab(popupURIspec, false, Browser.selectedTab);
      }
    }
  }
};

const gXPInstallObserver = {
  observe: function xpi_observer(aSubject, aTopic, aData)
  {
    var brandBundle = document.getElementById("bundle_brand");
    switch (aTopic) {
      case "addon-install-blocked":
        var installInfo = aSubject.QueryInterface(Ci.amIWebInstallInfo);
        var host = installInfo.originatingURI.host;
        var brandShortName = brandBundle.getString("brandShortName");
        var notificationName, messageString, buttons;
        var strings = Elements.browserBundle;
        var enabled = true;
        try {
          enabled = Services.prefs.getBoolPref("xpinstall.enabled");
        }
        catch (e) {}
        if (!enabled) {
          notificationName = "xpinstall-disabled";
          if (Services.prefs.prefIsLocked("xpinstall.enabled")) {
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
                Services.prefs.setBoolPref("xpinstall.enabled", true);
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
    window.QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIDOMWindowUtils).garbageCollect();
    Components.utils.forceGC();
  }
};

function getNotificationBox(aWindow) {
  return Browser.getNotificationBox();
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

var AlertsHelper = {
  _timeoutID: -1,
  _listener: null,
  _cookie: "",
  _clickable: false,
  _container: null,
  get container() {
    if (!this._container) {
      this._container = document.getElementById("alerts-container");
      let self = this;
      this._container.addEventListener("transitionend", function() {
        self.alertTransitionOver();
      }, true);
    }
    return this._container;
  },

  showAlertNotification: function ah_show(aImageURL, aTitle, aText, aTextClickable, aCookie, aListener) {
    this._clickable = aTextClickable || false;
    this._listener = aListener || null;
    this._cookie = aCookie || "";

    document.getElementById("alerts-image").setAttribute("src", aImageURL);
    document.getElementById("alerts-title").value = aTitle;
    document.getElementById("alerts-text").textContent = aText;
    
    let container = this.container;
    container.hidden = false;
    container.height = container.getBoundingClientRect().height;
    container.classList.add("showing");

    let timeout = Services.prefs.getIntPref("alerts.totalOpenTime");
    let self = this;
    if (this._timeoutID)
      clearTimeout(this._timeoutID);
    this._timeoutID = setTimeout(function() { self._timeoutAlert(); }, timeout);
  },
    
  _timeoutAlert: function ah__timeoutAlert() {
    this._timeoutID = -1;
    
    this.container.classList.remove("showing");
    if (this._listener)
      this._listener.observe(null, "alertfinished", this._cookie);
  },
  
  alertTransitionOver: function ah_alertTransitionOver() {
    let container = this.container;
    if (!container.classList.contains("showing")) {
      container.height = 0;
      container.hidden = true;
    }
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
    
    if (aWebProgress.windowId != this._tab.browser.contentWindowId && this._tab.browser.contentWindowId)
      return;

    
    
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_START)
        this._networkStart();
      else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP)
        this._networkStop();
    }

    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_START) {
#ifdef MOZ_CRASH_REPORTER
        if (aRequest instanceof Ci.nsIChannel && CrashReporter.enabled)
          CrashReporter.annotateCrashReport("URL", aRequest.URI.spec);
#endif
      }
      else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
        this._documentStop();
      }
    }
  },

  
  onProgressChange: function onProgressChange(aWebProgress, aRequest, aCurSelf, aMaxSelf, aCurTotal, aMaxTotal) {
    
  },

  
  onLocationChange: function onLocationChange(aWebProgress, aRequest, aLocationURI) {
    
    if (aWebProgress.windowId != this._tab.browser.contentWindowId)
      return;

    let spec = aLocationURI ? aLocationURI.spec : "";
    let location = spec.split("#")[0]; 

    this._hostChanged = true;

    if (location != this.browser.lastLocation) {
      this.browser.lastLocation = location;
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

    if (this._tab == Browser.selectedTab)
      BrowserUI.update(TOOLBARSTATE_LOADED);

    if (this.browser.currentURI.spec != "about:blank")
      this._tab.updateThumbnail();
  },

  _documentStop: function _documentStop() {
      
      
      this._tab.pageScrollOffset = new Point(0, 0);
  }
};

var OfflineApps = {
  offlineAppRequested: function(aRequest) {
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
    let strings = Elements.browserBundle;
    if (notification) {
      notification.documents.push(aRequest);
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
      notification.documents = [aRequest];
    }
  },

  allowSite: function(aRequest) {
    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);
    Services.perms.add(currentURI, "offline-app", Ci.nsIPermissionManager.ALLOW_ACTION);

    
    
    this._startFetching(aRequest);
  },

  disallowSite: function(aRequest) {
    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);
    Services.perms.add(currentURI, "offline-app", Ci.nsIPermissionManager.DENY_ACTION);
  },

  _startFetching: function(aRequest) {
    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);
    let manifestURI = Services.io.newURI(aRequest.manifest, aRequest.charset, currentURI);

    let updateService = Cc["@mozilla.org/offlinecacheupdate-service;1"].getService(Ci.nsIOfflineCacheUpdateService);
    updateService.scheduleUpdate(manifestURI, currentURI);
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (aMessage.name == "Browser:MozApplicationManifest") {
      this.offlineAppRequested(aMessage.json);
    }
  }
};

function Tab(aURI) {
  this._id = null;
  this._browser = null;
  this._state = null;
  this._listener = null;
  this._loading = false;
  this._chromeTab = null;
  this.owner = null;

  
  
  this.lastSelected = 0;

  this.create(aURI);
}

Tab.prototype = {
  get browser() {
    return this._browser;
  },

  get chromeTab() {
    return this._chromeTab;
  },

  
  updateViewportMetadata: function updateViewportMetadata(metaData) {
    let browser = this._browser;
    if (!browser)
      return;

    this.metaData = metaData;
    this.updateViewportSize();
  },

  
  updateViewportSize: function updateViewportSize() {
    let browser = this._browser;
    if (!browser)
      return;

    let metaData = this.metaData || {};
    if (!metaData.autoSize) {
      let screenW = window.innerWidth;
      let screenH = window.innerHeight;
      let viewportW = metaData.width;
      let viewportH = metaData.height;

      
      let maxInitialZoom = metaData.defaultZoom || metaData.maxZoom;
      if (maxInitialZoom && viewportW)
        viewportW = Math.max(viewportW, screenW / maxInitialZoom);

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

      browser.setCssViewportSize(viewportW, viewportH);
    }
    else {
      let browserBCR = browser.getBoundingClientRect();
      let w = browserBCR.width;
      let h = browserBCR.height;
      if (metaData.defaultZoom != 1.0) {
        let dpiScale = Services.prefs.getIntPref("zoom.dpiScale") / 100;
        w /= dpiScale;
        h /= dpiScale;
      }

      browser.setCssViewportSize(w, h);
    }

    
    





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

  create: function create(aURI) {
    this._chromeTab = document.getElementById("tabs").addTab();
    this._createBrowser(aURI);
  },

  destroy: function destroy() {
    document.getElementById("tabs").removeTab(this._chromeTab);
    this._chromeTab = null;
    this._destroyBrowser();
  },

  _createBrowser: function _createBrowser(aURI) {
    if (this._browser)
      throw "Browser already exists";

    
    let browser = this._browser = document.createElement("browser");
    browser.setAttribute("class", "window-width window-height");
    this._chromeTab.linkedBrowser = browser;

    browser.setAttribute("type", "content");

    let useRemote = Services.prefs.getBoolPref("browser.tabs.remote");
    let useLocal = Util.isLocalScheme(aURI);
    browser.setAttribute("remote", (!useLocal && useRemote) ? "true" : "false");

    
    document.getElementById("browsers").appendChild(browser);

    
    browser.stop();

    
    let flags = Ci.nsIWebProgress.NOTIFY_LOCATION |
                Ci.nsIWebProgress.NOTIFY_SECURITY |
                Ci.nsIWebProgress.NOTIFY_STATE_NETWORK |
                Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT;
    this._listener = new ProgressController(this);
    browser.webProgress.addProgressListener(this._listener, flags);

    browser.setAttribute("src", aURI);

    let self = this;
    browser.messageManager.addMessageListener("MozScrolledAreaChanged", function() {
      self.updateDefaultZoomLevel();
    });
  },

  _destroyBrowser: function _destroyBrowser() {
    if (this._browser) {
      var browser = this._browser;
      browser.removeProgressListener(this._listener);

      this._browser = null;
      this._listener = null;
      this._loading = false;

      Util.executeSoon(function() {
        document.getElementById("browsers").removeChild(browser);
      });
    }
  },

  clampZoomLevel: function clampZoomLevel(zl) {
    let browser = this._browser;
    let bounded = Math.min(Math.max(ZoomManager.MIN, zl), ZoomManager.MAX);

    let md = this.metaData;
    if (md && md.minZoom)
      bounded = Math.max(bounded, md.minZoom);
    if (md && md.maxZoom)
      bounded = Math.min(bounded, md.maxZoom);

    bounded = Math.max(bounded, this.getPageZoomLevel());

    let rounded = Math.round(bounded * kBrowserViewZoomLevelPrecision) / kBrowserViewZoomLevelPrecision;
    return rounded || 1.0;
  },

  


  resetZoomLevel: function resetZoomLevel() {
    let browser = this._browser;
    this._defaultZoomLevel = browser.scale;
  },

  


  updateDefaultZoomLevel: function updateDefaultZoomLevel() {
    let browser = this._browser;
    let isDefault = (browser.scale == this._defaultZoomLevel);
    this._defaultZoomLevel = this.getDefaultZoomLevel();
    if (isDefault)
      browser.setScale(this._defaultZoomLevel);
  },

  isDefaultZoomLevel: function isDefaultZoomLevel() {
    return getBrowser().scale == this._defaultZoomLevel;
  },

  getDefaultZoomLevel: function getDefaultZoomLevel() {
    let md = this.metaData;
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
    let browserW = this._browser.viewportWidthInCSSPx;
    return this._browser.getBoundingClientRect().width / browserW;
  },

  get allowZoom() {
    return this.metaData.allowZoom;
  },

  updateThumbnail: function updateThumbnail() {
    return;

    if (!this._browser)
      return;

    
    
    
    this._chromeTab.updateThumbnail(this._browser, 800, 500);
  },

  toString: function() {
    return "[Tab " + (this._browser ? this._browser.currentURI.spec : "(no browser)") + "]";
  }
};
