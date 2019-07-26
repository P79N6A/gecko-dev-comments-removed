




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;

const kBrowserViewZoomLevelPrecision = 10000;


const kTouchTimeout = 300;
const kSetInactiveStateTimeout = 100;

const kDefaultMetadata = { autoSize: false, allowZoom: true, autoScale: true };


window.sizeToContent = function() {
  Cu.reportError("window.sizeToContent is not allowed in this window");
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
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/SelectionHandler.js", true);
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/ContextMenuHandler.js", true);
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/FindHandler.js", true);
      
      
      messageManager.loadFrameScript("chrome://browser/content/contenthandlers/ConsoleAPIObserver.js", true);
      
    } catch (e) {
      
      dump("###########" + e + "\n");
    }

    
    Elements.browsers.customDragger = new Browser.MainDragger();

    
    Elements.browsers.webProgress = WebProgress.init();

    
    
    InputSourceHelper.init();

    TouchModule.init();
    ScrollwheelModule.init(Elements.browsers);
    GestureModule.init();
    BrowserTouchHandler.init();

    
    
    
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
      if (!window.fullScreen)
        Elements.toolbar.setAttribute("fullscreen", "true");
      else
        Elements.toolbar.removeAttribute("fullscreen");
    }
    window.addEventListener("fullscreen", fullscreenHandler, false);

    BrowserUI.init();

    window.controllers.appendController(this);
    window.controllers.appendController(BrowserUI);

    let os = Services.obs;
    os.addObserver(SessionHistoryObserver, "browser:purge-session-history", false);
    os.addObserver(ActivityObserver, "application-background", false);
    os.addObserver(ActivityObserver, "application-foreground", false);
    os.addObserver(ActivityObserver, "system-active", false);
    os.addObserver(ActivityObserver, "system-idle", false);
    os.addObserver(ActivityObserver, "system-display-on", false);
    os.addObserver(ActivityObserver, "system-display-off", false);

    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = new nsBrowserAccess();

    Elements.browsers.addEventListener("DOMUpdatePageReport", PopupBlockerObserver.onUpdatePageReport, false);

    
    Util.forceOnline();

    
    
    
    let commandURL = null;
    if (window.arguments && window.arguments[0])
      commandURL = window.arguments[0];

    
    let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
    if (ss.shouldRestore() || Services.prefs.getBoolPref("browser.startup.sessionRestore")) {
      let bringFront = false;
      
      if (commandURL && commandURL != this.getHomePage()) {
        this.addTab(commandURL, true);
      } else {
        bringFront = true;
        
        
        
        let dummy = this.addTab("about:blank", true);
        let dummyCleanup = {
          observe: function(aSubject, aTopic, aData) {
            Services.obs.removeObserver(dummyCleanup, "sessionstore-windows-restored");
            if (aData == "fail")
              Browser.addTab(commandURL || Browser.getHomePage(), true);
            dummy.chromeTab.ignoreUndo = true;
            Browser.closeTab(dummy, { forceClose: true });
          }
        };
        Services.obs.addObserver(dummyCleanup, "sessionstore-windows-restored", false);
      }
      ss.restoreLastSession(bringFront);
    } else {
      this.addTab(commandURL || this.getHomePage(), true);
    }

    messageManager.addMessageListener("DOMLinkAdded", this);
    messageManager.addMessageListener("MozScrolledAreaChanged", this);
    messageManager.addMessageListener("Browser:ViewportMetadata", this);
    messageManager.addMessageListener("Browser:FormSubmit", this);
    messageManager.addMessageListener("Browser:ZoomToPoint:Return", this);
    messageManager.addMessageListener("Browser:CanUnload:Return", this);
    messageManager.addMessageListener("scroll", this);
    messageManager.addMessageListener("Browser:CertException", this);
    messageManager.addMessageListener("Browser:BlockedSite", this);
    messageManager.addMessageListener("Browser:ErrorPage", this);
    messageManager.addMessageListener("Browser:TapOnSelection", this);
    messageManager.addMessageListener("Browser:PluginClickToPlayClicked", this);

    
    
    InputSourceHelper.fireUpdate();

    
    let event = document.createEvent("Events");
    event.initEvent("UIReady", true, false);
    window.dispatchEvent(event);
  },

  quit: function quit() {
    
    
    if (this.closing()) {
      window.QueryInterface(Ci.nsIDOMChromeWindow).minimize();
      window.close();
    }
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
      if (win != window)
        lastBrowser = false;
    }
    if (!lastBrowser)
      return true;

    
    let closingCancelled = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
    Services.obs.notifyObservers(closingCancelled, "browser-lastwindow-close-requested", null);
    if (closingCancelled.data)
      return false;

    Services.obs.notifyObservers(null, "browser-lastwindow-close-granted", null);
    return true;
  },

  shutdown: function shutdown() {
    BrowserUI.uninit();
    ContentAreaObserver.uninit();

    messageManager.removeMessageListener("MozScrolledAreaChanged", this);
    messageManager.removeMessageListener("Browser:ViewportMetadata", this);
    messageManager.removeMessageListener("Browser:FormSubmit", this);
    messageManager.removeMessageListener("Browser:ZoomToPoint:Return", this);
    messageManager.removeMessageListener("scroll", this);
    messageManager.removeMessageListener("Browser:CertException", this);
    messageManager.removeMessageListener("Browser:BlockedSite", this);
    messageManager.removeMessageListener("Browser:ErrorPage", this);
    messageManager.removeMessageListener("Browser:PluginClickToPlayClicked", this);
    messageManager.removeMessageListener("Browser:TapOnSelection", this);

    var os = Services.obs;
    os.removeObserver(SessionHistoryObserver, "browser:purge-session-history");
    os.removeObserver(ActivityObserver, "application-background", false);
    os.removeObserver(ActivityObserver, "application-foreground", false);
    os.removeObserver(ActivityObserver, "system-active", false);
    os.removeObserver(ActivityObserver, "system-idle", false);
    os.removeObserver(ActivityObserver, "system-display-on", false);
    os.removeObserver(ActivityObserver, "system-display-off", false);

    window.controllers.removeController(this);
    window.controllers.removeController(BrowserUI);
  },

  getHomePage: function getHomePage(aOptions) {
    aOptions = aOptions || { useDefault: false };

    let url = "about:start";
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
    return (this._selectedTab && this._selectedTab.browser);
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

  getBrowserForWindowId: function getBrowserForWindowId(aWindowId) {
    for (let i = 0; i < this.browsers.length; i++) {
      if (this.browsers[i].contentWindowId == aWindowId)
        return this.browsers[i];
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

  closeTab: function closeTab(aTab, aOptions) {
    let tab = aTab instanceof XULElement ? this.getTabFromChrome(aTab) : aTab;
    if (!tab || !this._getNextTab(tab))
      return;

    if (aOptions && "forceClose" in aOptions && aOptions.forceClose) {
      this._doCloseTab(aTab);
      return;
    }

    tab.browser.messageManager.sendAsyncMessage("Browser:CanUnload", {});
  },

  savePage: function() {
    ContentAreaUtils.saveDocument(this.selectedBrowser.contentWindow.document);
  },

  _doCloseTab: function _doCloseTab(aTab) {
    let nextTab = this._getNextTab(aTab);
    if (!nextTab)
       return;

    
    this._tabs.forEach(function(item, index, array) {
      if (item.owner == aTab)
        item.owner = null;
    });

    let event = document.createEvent("Events");
    event.initEvent("TabClose", true, false);
    aTab.chromeTab.dispatchEvent(event);
    aTab.browser.messageManager.sendAsyncMessage("Browser:TabClose");

    let container = aTab.chromeTab.parentNode;
    aTab.destroy();
    this._tabs.splice(this._tabs.indexOf(aTab), 1);

    this.selectedTab = nextTab;

    event = document.createEvent("Events");
    event.initEvent("TabRemove", true, false);
    container.dispatchEvent(event);
  },

  _getNextTab: function _getNextTab(aTab) {
    let tabIndex = this._tabs.indexOf(aTab);
    if (tabIndex == -1)
      return null;

    let nextTab = this._selectedTab;
    if (nextTab == aTab) {
      nextTab = this.getTabAtIndex(tabIndex + 1) || this.getTabAtIndex(tabIndex - 1);

      
      if (aTab.owner && nextTab.owner != aTab.owner)
        nextTab = aTab.owner;

      if (!nextTab)
        return null;
    }

    return nextTab;
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
    let oldBrowser = lastTab ? lastTab._browser : null;
    let browser = tab.browser;

    this._selectedTab = tab;
    
    if (lastTab)
      lastTab.active = false;

    if (tab)
      tab.active = true;

    if (isFirstTab) {
      
      BrowserUI._titleChanged(browser);
    } else {
      
      BrowserUI.updateURI();
      IdentityUI.checkIdentity();

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
          sslExceptions.addPermanentException(uri, errorDoc.defaultView);
        else
          sslExceptions.addTemporaryException(uri, errorDoc.defaultView);
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
        
        try {
          let reportURL = formatter.formatURLPref("browser.safebrowsing.warning.infoURL");
          this.loadURI(reportURL);
        } catch (e) {
          Cu.reportError("Couldn't get phishing info URL: " + e);
        }
        break;
      }
    }
  },

  pinSite: function browser_pinSite() {
    
    let hasher = Cc["@mozilla.org/security/hash;1"].
                 createInstance(Ci.nsICryptoHash);
    hasher.init(Ci.nsICryptoHash.MD5);
    let stringStream = Cc["@mozilla.org/io/string-input-stream;1"].
                       createInstance(Ci.nsIStringInputStream);
    stringStream.data = Browser.selectedBrowser.currentURI.spec;
    hasher.updateFromStream(stringStream, -1);
    let hashASCII = hasher.finish(true);

    
    var file = Components.classes["@mozilla.org/file/directory_service;1"]. 
           getService(Components.interfaces.nsIProperties). 
           get("CurProcD", Components.interfaces.nsIFile);
    
    file = file.parent;
    file.append("tileresources");
    file.append("VisualElements_logo.png");
    var ios = Components.classes["@mozilla.org/network/io-service;1"]. 
              getService(Components.interfaces.nsIIOService); 
    var uriSpec = ios.newFileURI(file).spec;
    MetroUtils.pinTileAsync("FFTileID_" + hashASCII,
                               Browser.selectedBrowser.contentTitle, 
                               Browser.selectedBrowser.contentTitle, 
                               "metrobrowser -url " + Browser.selectedBrowser.currentURI.spec,
                               uriSpec,
                               uriSpec);
  },

  unpinSite: function browser_unpinSite() {
    if (!MetroUtils.immersive)
      return;

    
    let hasher = Cc["@mozilla.org/security/hash;1"].
                 createInstance(Ci.nsICryptoHash);
    hasher.init(Ci.nsICryptoHash.MD5);
    let stringStream = Cc["@mozilla.org/io/string-input-stream;1"].
                       createInstance(Ci.nsIStringInputStream);
    stringStream.data = Browser.selectedBrowser.currentURI.spec;
    hasher.updateFromStream(stringStream, -1);
    let hashASCII = hasher.finish(true);

    MetroUtils.unpinTileAsync("FFTileID_" + hashASCII);
  },

  isSitePinned: function browser_isSitePinned() {
    if (!MetroUtils.immersive)
      return false;

    
    let hasher = Cc["@mozilla.org/security/hash;1"].
                 createInstance(Ci.nsICryptoHash);
    hasher.init(Ci.nsICryptoHash.MD5);
    let stringStream = Cc["@mozilla.org/io/string-input-stream;1"].
                       createInstance(Ci.nsIStringInputStream);
    stringStream.data = Browser.selectedBrowser.currentURI.spec;
    hasher.updateFromStream(stringStream, -1);
    let hashASCII = hasher.finish(true);

    return MetroUtils.isTilePinned("FFTileID_" + hashASCII);
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
    AnimatedZoom.animateTo(rect);
  },

  
  _getZoomLevelForRect: function _getZoomLevelForRect(rect) {
    const margin = 15;
    return this.selectedTab.clampZoomLevel(ContentAreaObserver.width / (rect.width + margin * 2));
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
      AnimatedZoom.animateTo(zoomRect);

    return zoomRect;
  },

  zoomFromPoint: function zoomFromPoint(cX, cY) {
    let tab = this.selectedTab;
    if (tab.allowZoom && !tab.isDefaultZoomLevel()) {
      let zoomLevel = tab.getDefaultZoomLevel();
      let zoomRect = this._getZoomRectForPoint(cX, cY, zoomLevel);
      AnimatedZoom.animateTo(zoomRect);
    }
  },

  
  
  getScaleRatio: function getScaleRatio() {
    let prefValue = Services.prefs.getIntPref("browser.viewport.scaleRatio");
    if (prefValue > 0)
      return prefValue / 100;

    let dpi = Util.displayDPI;
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
      case "MozScrolledAreaChanged": {
        let tab = this.getTabForBrowser(browser);
        if (tab)
          tab.scrolledAreaChanged();
        break;
      }
      case "Browser:ViewportMetadata": {
        let tab = this.getTabForBrowser(browser);
        
        
        if (tab)
          tab.updateViewportMetadata(json);
        break;
      }
      case "Browser:FormSubmit":
        browser.lastLocation = null;
        break;

      case "Browser:CanUnload:Return": {
        if (!json.permit)
          return;

        
        
        setTimeout(function(self) {
          let tab = self.getTabForBrowser(browser);
          self._doCloseTab(tab);
        }, 0, this);
        break;
      }
      case "Browser:ZoomToPoint:Return":
        if (json.zoomTo) {
          let rect = Rect.fromRect(json.zoomTo);
          this.zoomToPoint(json.x, json.y, rect);
        } else {
          this.zoomFromPoint(json.x, json.y);
        }
        break;
      case "Browser:CertException":
        this._handleCertException(aMessage);
        break;
      case "Browser:BlockedSite":
        this._handleBlockedSite(aMessage);
        break;
      case "Browser:ErrorPage":
        break;
      case "Browser:PluginClickToPlayClicked": {
        
        let parent = browser.parentNode;
        let data = browser.__SS_data;
        if (data.entries.length == 0)
          return;

        
        parent.removeChild(browser);

        
        browser.setAttribute("remote", "false");
        parent.appendChild(browser);

        
        browser.__SS_data = data;
        let json = {
          uri: data.entries[data.index - 1].url,
          flags: null,
          entries: data.entries,
          index: data.index
        };
        browser.messageManager.sendAsyncMessage("WebNavigation:LoadURI", json);
        break;
      }

      case "Browser:TapOnSelection":
        if (!InputSourceHelper.isPrecise) {
          if (SelectionHelperUI.isActive) {
            SelectionHelperUI.shutdown();
          }
          if (SelectionHelperUI.canHandle(aMessage)) {
            SelectionHelperUI.openEditSession(aMessage);
          }
        }
        break;
    }
  },

  onAboutPolicyClick: function() {
    FlyoutPanelsUI.hide();
    BrowserUI.newTab(Services.prefs.getCharPref("app.privacyURL"),
                     Browser.selectedTab);
  }

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
    this._scrollScales.x = 0, this._scrollScales.y = 0;
    this._horizontalScrollbar.removeAttribute("panning");
    this._verticalScrollbar.removeAttribute("panning");
    this._horizontalScrollbar.style.MozTransform = "";
    this._verticalScrollbar.style.MozTransform = "";
  }
};



const OPEN_APPTAB = 100; 

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
    } else if (aWhere == OPEN_APPTAB) {
      Browser.tabs.forEach(function(aTab) {
        if ("appURI" in aTab.browser && aTab.browser.appURI.spec == aURI.spec) {
          Browser.selectedTab = aTab;
          browser = aTab.browser;
        }
      });

      if (!browser) {
        
        let tab = Browser.addTab("about:blank", true, null, { getAttention: true });
        browser = tab.browser;
        browser.appURI = aURI;
      } else {
        
        browser = null;
      }
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

    
    
    BrowserUI.showContent();

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

  zoom: function browser_zoom(aAmount) {
    Browser.zoom(aAmount);
  },

  isTabContentWindow: function(aWindow) {
    return Browser.browsers.some(function (browser) browser.contentWindow == aWindow);
  }
};




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
              isDefault: false,
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

var ActivityObserver = {
  _inBackground : false,
  _notActive : false,
  _isDisplayOff : false,
  _timeoutID: 0,
  observe: function ao_observe(aSubject, aTopic, aData) {
    if (aTopic == "application-background") {
      this._inBackground = true;
    } else if (aTopic == "application-foreground") {
      this._inBackground = false;
    } else if (aTopic == "system-idle") {
      this._notActive = true;
    } else if (aTopic == "system-active") {
      this._notActive = false;
    } else if (aTopic == "system-display-on") {
      this._isDisplayOff = false;
    } else if (aTopic == "system-display-off") {
      this._isDisplayOff = true;
    }
    let activeTabState = !this._inBackground && !this._notActive && !this._isDisplayOff;
    if (this._timeoutID)
      clearTimeout(this._timeoutID);
    if (Browser.selectedTab.active != activeTabState) {
      
      
      
      this._timeoutID = setTimeout(function() { Browser.selectedTab.active = activeTabState; }, activeTabState ? 0 : kSetInactiveStateTimeout);
    }
  }
};

function getNotificationBox(aBrowser) {
  return Browser.getNotificationBox(aBrowser);
}

function showDownloadManager(aWindowContext, aID, aReason) {
  PanelUI.show("downloads-container");
  
}

function Tab(aURI, aParams) {
  this._id = null;
  this._browser = null;
  this._notification = null;
  this._loading = false;
  this._chromeTab = null;
  this._metadata = null;

  this.owner = null;

  this.hostChanged = false;
  this.state = null;

  
  
  this.lastSelected = 0;

  
  
  this.create(aURI, aParams || {});

  
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

  get metadata() {
    return this._metadata || kDefaultMetadata;
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

  


  updateViewportSize: function updateViewportSize(width, height) {
    













































  },

  restoreViewportPosition: function restoreViewportPosition(aOldWidth, aNewWidth) {
    let browser = this._browser;

    
    let oldScale = browser.scale;
    let newScale = this.clampZoomLevel(oldScale * aNewWidth / aOldWidth);
    let scaleRatio = newScale / oldScale;

    let view = browser.getRootView();
    let pos = view.getPosition();
    browser.fuzzyZoom(newScale, pos.x * scaleRatio, pos.y * scaleRatio);
    browser.finishFuzzyZoom();
  },

  startLoading: function startLoading() {
    if (this._loading) throw "Already Loading!";
    this._loading = true;
  },

  endLoading: function endLoading() {
    if (!this._loading) throw "Not Loading!";
    this._loading = false;
    this.updateFavicon();
  },

  isLoading: function isLoading() {
    return this._loading;
  },

  create: function create(aURI, aParams) {
    this._chromeTab = Elements.tabList.addTab();
    this._id = Browser.createTabId();
    let browser = this._createBrowser(aURI, null);

    
    if ("delayLoad" in aParams && aParams.delayLoad)
      return;

    try {
      let flags = aParams.flags || Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
      let postData = ("postData" in aParams && aParams.postData) ? aParams.postData.value : null;
      let referrerURI = "referrerURI" in aParams ? aParams.referrerURI : null;
      let charset = "charset" in aParams ? aParams.charset : null;
      browser.loadURIWithFlags(aURI, flags, referrerURI, charset, postData);
    } catch(e) {
      dump("Error: " + e + "\n");
    }
  },

  destroy: function destroy() {
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

  _createBrowser: function _createBrowser(aURI, aInsertBefore) {
    if (this._browser)
      throw "Browser already exists";

    
    
    
    let notification = this._notification = document.createElement("notificationbox");

    let browser = this._browser = document.createElement("browser");
    browser.id = "browser-" + this._id;
    browser.setAttribute("class", "viewable-width viewable-height");
    this._chromeTab.linkedBrowser = browser;

    browser.setAttribute("type", "content");

    let useRemote = Services.prefs.getBoolPref("browser.tabs.remote");
    let useLocal = Util.isLocalScheme(aURI);
    browser.setAttribute("remote", (!useLocal && useRemote) ? "true" : "false");

    
    notification.appendChild(browser);
    Elements.browsers.insertBefore(notification, aInsertBefore);

    
    browser.stop();

    let fl = browser.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
    fl.renderMode = Ci.nsIFrameLoader.RENDER_MODE_ASYNC_SCROLL;

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

  





  clampZoomLevel: function clampZoomLevel(aScale, aPageZoomLevel) {
    let md = this.metadata;
    if (!this.allowZoom) {
      return (md && md.defaultZoom)
        ? md.defaultZoom
        : (aPageZoomLevel || this.getPageZoomLevel());
    }

    let browser = this._browser;
    let bounded = Util.clamp(aScale, ZoomManager.MIN, ZoomManager.MAX);

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

  scrolledAreaChanged: function scrolledAreaChanged(firstPaint) {
    if (!this._browser)
      return;

    if (firstPaint) {
      
      this.updateViewportSize();
    }

    this.updateDefaultZoomLevel();
  },

  



  updateDefaultZoomLevel: function updateDefaultZoomLevel() {
    let browser = this._browser;
    if (!browser || !this._firstPaint)
      return;

    let isDefault = this.isDefaultZoomLevel();
    this._defaultZoomLevel = this.getDefaultZoomLevel();
    if (isDefault) {
      if (browser.scale != this._defaultZoomLevel) {
        browser.scale = this._defaultZoomLevel;
      } else {
        
        
        
        
        
        
        browser.getRootView()._updateCacheViewport();
      }
    } else {
      
      
      browser.getRootView()._updateCacheViewport();
    }
  },

  isDefaultZoomLevel: function isDefaultZoomLevel() {
    return this._browser.scale == this._defaultZoomLevel;
  },

  getDefaultZoomLevel: function getDefaultZoomLevel() {
    let md = this.metadata;
    if (md && md.defaultZoom)
      return this.clampZoomLevel(md.defaultZoom);

    let browserWidth = this._browser.getBoundingClientRect().width;
    let defaultZoom = browserWidth / this._browser.contentWindowWidth;
    return this.clampZoomLevel(defaultZoom);
  },

  




  getPageZoomLevel: function getPageZoomLevel(aScreenWidth) {
    let browserW = this._browser.contentDocumentWidth;
    if (browserW == 0)
      return 1.0;

    let screenW = aScreenWidth || this._browser.getBoundingClientRect().width;
    return screenW / browserW;
  },

  get allowZoom() {
    return this.metadata.allowZoom && !Util.isURLEmpty(this.browser.currentURI.spec);
  },

  updateThumbnailSource: function updateThumbnailSource() {
    this._chromeTab.updateThumbnailSource(this._browser);
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
      browser.setAttribute("type", "content-primary");
      Elements.browsers.selectedPanel = notification;
      browser.active = true;
      Elements.tabList.selectedTab = this._chromeTab;
      browser.focus();
    } else {
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

var ContentAreaObserver = {
  styles: {},
  
  
  
  get width() { return window.innerWidth || 1366; },
  get height() { return window.innerHeight || 768; },

  get contentHeight() {
    return this._getContentHeightForWindow(this.height);
  },

  get contentTop () {
    return Elements.toolbar.getBoundingClientRect().bottom;
  },

  get viewableHeight() {
    return this._getViewableHeightForContent(this.contentHeight);
  },

  get isKeyboardOpened() { return MetroUtils.keyboardVisible; },
  get hasVirtualKeyboard() { return true; },

  init: function cao_init() {
    window.addEventListener("resize", this, false);

    let os = Services.obs;
    os.addObserver(this, "metro_softkeyboard_shown", false);
    os.addObserver(this, "metro_softkeyboard_hidden", false);

    
    
    
    
    
    
    
    
    
    
    
    let stylesheet = document.styleSheets[0];
    for (let style of ["window-width", "window-height",
                       "content-height", "content-width",
                       "viewable-height", "viewable-width"]) {
      let index = stylesheet.insertRule("." + style + " {}", stylesheet.cssRules.length);
      this.styles[style] = stylesheet.cssRules[index].style;
    }
    this.update();
  },

  uninit: function cao_uninit() {
    let os = Services.obs;
    os.removeObserver(this, "metro_softkeyboard_shown");
    os.removeObserver(this, "metro_softkeyboard_hidden");
  },

  update: function cao_update (width, height) {
    let oldWidth = parseInt(this.styles["window-width"].width);
    let oldHeight = parseInt(this.styles["window-height"].height);

    let newWidth = width || this.width;
    let newHeight = height || this.height;

    if (newHeight == oldHeight && newWidth == oldWidth)
      return;

    this.styles["window-width"].width = newWidth + "px";
    this.styles["window-width"].maxWidth = newWidth + "px";
    this.styles["window-height"].height = newHeight + "px";
    this.styles["window-height"].maxHeight = newHeight + "px";

    let isStartup = !oldHeight && !oldWidth;
    for (let i = Browser.tabs.length - 1; i >=0; i--) {
      let tab = Browser.tabs[i];
      let oldContentWindowWidth = tab.browser.contentWindowWidth;
      tab.updateViewportSize(newWidth, newHeight); 
      
      
      if (!isStartup) {
        
        
        if (tab.browser.contentWindowWidth == oldContentWindowWidth)
          tab.restoreViewportPosition(oldWidth, newWidth);

        tab.updateDefaultZoomLevel();
      }
    }

    
    if (!isStartup) {
      let currentElement = document.activeElement;
      let [scrollbox, scrollInterface] = ScrollUtils.getScrollboxFromElement(currentElement);
      if (scrollbox && scrollInterface && currentElement && currentElement != scrollbox) {
        
        while (currentElement && currentElement.parentNode != scrollbox)
          currentElement = currentElement.parentNode;
  
        setTimeout(function() { scrollInterface.ensureElementIsVisible(currentElement) }, 0);
      }
    }

    this.updateContentArea(newWidth, this._getContentHeightForWindow(newHeight));
    this._fire("SizeChanged");
  },

  updateContentArea: function cao_updateContentArea (width, height) {
    let oldHeight = parseInt(this.styles["content-height"].height);
    let oldWidth = parseInt(this.styles["content-width"].width);

    let newWidth = width || this.width;
    let newHeight = height || this.contentHeight;

    if (newHeight == oldHeight && newWidth == oldWidth)
      return;

    this.styles["content-height"].height = newHeight + "px";
    this.styles["content-height"].maxHeight = newHeight + "px";
    this.styles["content-width"].width = newWidth + "px";
    this.styles["content-width"].maxWidth = newWidth + "px";

    this.updateViewableArea(newWidth, this._getViewableHeightForContent(newHeight));
    this._fire("ContentSizeChanged");
  },

  updateViewableArea: function cao_updateViewableArea (width, height) {
    let oldHeight = parseInt(this.styles["viewable-height"].height);
    let oldWidth = parseInt(this.styles["viewable-width"].width);

    let newWidth = width || this.width;
    let newHeight = height || this.viewableHeight;

    if (newHeight == oldHeight && newWidth == oldWidth)
      return;

    this.styles["viewable-height"].height = newHeight + "px";
    this.styles["viewable-height"].maxHeight = newHeight + "px";
    this.styles["viewable-width"].width = newWidth + "px";
    this.styles["viewable-width"].maxWidth = newWidth + "px";

    this._fire("ViewableSizeChanged");
  },

  observe: function cao_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "metro_softkeyboard_shown":
      case "metro_softkeyboard_hidden": {
        let event = document.createEvent("UIEvents");
        let eventDetails = {
          opened: aTopic == "metro_softkeyboard_shown",
          details: aData
        };

        event.initUIEvent("KeyboardChanged", true, false, window, eventDetails);
        window.dispatchEvent(event);

        this.updateViewableArea();
        break;
      }
    };
  },

  handleEvent: function cao_handleEvent(anEvent) {
    switch (anEvent.type) {
      case 'resize':
        if (anEvent.target != window)
          return;

        ContentAreaObserver.update();
        break;
    }
  },

  _fire: function (aName) {
    
    setTimeout(function() {
      let event = document.createEvent("Events");
      event.initEvent(aName, true, false);
      Elements.browsers.dispatchEvent(event);
    }, 0);
  },

  _getContentHeightForWindow: function (windowHeight) {
    let contextUIHeight = BrowserUI.isTabsOnly ? Elements.toolbar.getBoundingClientRect().bottom : 0;
    return windowHeight - contextUIHeight;
  },

  _getViewableHeightForContent: function (contentHeight) {
    let keyboardHeight = MetroUtils.keyboardHeight;
    return contentHeight - keyboardHeight;
  }
};
