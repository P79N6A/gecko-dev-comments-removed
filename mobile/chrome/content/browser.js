











































let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

const FINDSTATE_FIND = 0;
const FINDSTATE_FIND_AGAIN = 1;
const FINDSTATE_FIND_PREVIOUS = 2;

Cu.import("resource://gre/modules/SpatialNavigation.js");

function getBrowser() {
  return Browser.selectedBrowser;
}

const kDefaultTextZoom = 1.2;
const kDefaultBrowserWidth = 1024;

let endl = '\n';

function debug() {
  let bv = Browser._browserView;
  let tc = bv._tileManager._tileCache;
  let scrollbox = document.getElementById("scrollbox")
		.boxObject
		.QueryInterface(Components.interfaces.nsIScrollBoxObject);

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

    let cr = bv._tileManager._criticalRect;
    dump('criticalRect from BV: ' + (cr ? cr.toString() : null) + endl);
    dump('visibleRect from BV : ' + bv._visibleRect.toString() + endl);
    dump('visibleRect from foo: ' + getVisibleRect().toString() + endl);

    dump('batch depth:          ' + bv._batchOps.length + endl);

    dump(endl);

    dump('container width,height from BV: ' + bv._container.style.width + ', '
                                            + bv._container.style.height + endl);
    dump('container width,height via DOM: ' + container.style.width + ', '
                                            + container.style.height + endl);

    dump(endl);

    dump('scrollbox position    : ' + x + ', ' + y + endl);
    dump('scrollbox scrolledsize: ' + w + ', ' + h + endl);


    let sb = document.getElementById("scrollbox");
    dump('container location:     ' + Math.round(container.getBoundingClientRect().left) + " " +
                                      Math.round(container.getBoundingClientRect().top) + endl);



    dump(endl);

    dump('tilecache capacity: ' + bv._tileManager._tileCache.getCapacity() + endl);
    dump('tilecache size    : ' + bv._tileManager._tileCache.size          + endl);
    dump('tilecache numFree : ' + bv._tileManager._tileCache.numFree       + endl);
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
  dump('occupied : ' + tc._isOccupied(i, j) + endl);
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

function onKeyPress(e) {
  let bv = Browser._browserView;

  if (!e.ctrlKey)
    return;

  const a = 97;   
  const c = 99;   
  const d = 100;  
  const f = 102;  
  const i = 105;  
  const l = 108;  
  const m = 109;  
  const r = 114;  
  const t = 116;  

  switch (e.charCode) {
  case r:
    bv.setVisibleRect(getVisibleRect());

  case d:
    debug();

    break;
  case l:
    bv._tileManager.restartLazyCrawl(bv._tileManager._criticalRect);

    break;
  case c:
    let cap = parseInt(window.prompt('new capacity'));
    bv._tileManager._tileCache.setCapacity(cap);

    break;
  case f:
    let noop = function noop() { for (let i = 0; i < 10; ++i); };
    bv._tileManager._tileCache.forEachIntersectingRect(bv._tileManager._criticalRect,
                                                       false, noop, window);

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
    dump('>>>>>> critical rect is ' + (cr ? cr.toString() : cr) + endl);
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
    onMouseUp();
    break;
  default:
    break;
  }
}


function getVisibleRect() {
  let container = document.getElementById("tile-container");
  let containerBCR = container.getBoundingClientRect();

  let x = Math.round(-containerBCR.left);
  let y = Math.round(-containerBCR.top);
  let w = window.innerWidth;
  let h = window.innerHeight;

  return new wsRect(x, y, w, h);
}

function getScrollboxPosition(scrollBoxObject) {
  let x = {};
  let y = {};
  scrollBoxObject.getPosition(x, y);
  return [x.value, y.value];
}

var ih = null;

var Browser = {
  _tabs : [],
  _browsers : [],
  _selectedTab : null,
  windowUtils: window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils),

  startup: function() {
    var self = this;

    dump("begin startup\n");

    let container = document.getElementById("tile-container");
    let bv = this._browserView = new BrowserView(container, getVisibleRect());

    let scrollbox = document.getElementById("scrollbox");

    scrollbox.customDragger = {
      dragStart: function dragStart(scroller) {
        bv.pauseRendering();
      },

      dragStop: function dragStop(dx, dy, scroller) {
        this.dragMove(dx, dy, scroller);
        bv.resumeRendering();
      },

      dragMove: function dragMove(dx, dy, scroller) {
        bv.onBeforeVisibleMove(dx, dy);

        let [x0, y0] = getScrollboxPosition(scroller);

        scroller.scrollBy(dx, dy);

        let [x1, y1] = getScrollboxPosition(scroller);

        let realdx = x1 - x0;
        let realdy = y1 - y0;

        bv.onAfterVisibleMove(realdx, realdy);

        if (realdx != dx || realdy != dy) {
          dump('--> scroll asked for ' + dx + ',' + dy + ' and got ' + realdx + ',' + realdy + '\n');
        }
      }
    };

    
    














    
    bv.beginBatchOperation();

    function panHandler(vr, dx, dy) {
      if (dx) {
        let visibleNow = ws.isWidgetVisible("tabs-container") || ws.isWidgetVisible("browser-controls");
        let isToolbarFrozen = ws.isWidgetFrozen('toolbar-main');
        if (visibleNow && !isToolbarFrozen) {
          BrowserUI.showToolbar(URLBAR_FORCE);
        }
        else if (!visibleNow && isToolbarFrozen) {
          BrowserUI.showToolbar();
        }
      }

      
      
      self.windowUtils.processUpdates();
    }

    

    function resizeHandler(e) {

      if (e.target != window)
        return;

      dump(window.innerWidth + "," + window.innerHeight + "\n");
      
      let w = window.innerWidth;
      let maximize = (document.documentElement.getAttribute("sizemode") == "maximized");
      if (maximize && w > screen.width)
        return;

      bv.beginBatchOperation();

      let h = window.innerHeight;

      
      BrowserUI.sizeControls(w, h);

      
      let browsers = Browser.browsers;
      if (browsers) {
        let scaledH = (kDefaultBrowserWidth * (h / w));
        for (let i=0; i<browsers.length; i++) {
          let browserStyle = browsers[i].style;
          browserStyle.height = scaledH + "px";
        }
      }

      bv.setVisibleRect(getVisibleRect());
      bv.zoomToPage();
      bv.commitBatchOperation();
    }
    window.addEventListener("resize", resizeHandler, false);
    
    function fullscreenHandler() {
      if (!window.fullScreen)
        document.getElementById("toolbar-main").setAttribute("fullscreen", "true");
      else
        document.getElementById("toolbar-main").removeAttribute("fullscreen");      
    }
    window.addEventListener("fullscreen", fullscreenHandler, false);

    
    ih = new InputHandler();

    BrowserUI.init();

    window.controllers.appendController(this);
    window.controllers.appendController(BrowserUI);

    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var styleSheets = Cc["@mozilla.org/content/style-sheet-service;1"].getService(Ci.nsIStyleSheetService);

    
    var hideCursor = gPrefService.getBoolPref("browser.ui.cursor") == false;
    if (hideCursor) {
      window.QueryInterface(Ci.nsIDOMChromeWindow).setCursor("none");

      var styleURI = ios.newURI("chrome://browser/content/cursor.css", null, null);
      styleSheets.loadAndRegisterSheet(styleURI, styleSheets.AGENT_SHEET);
    }

    
    var styleURI = ios.newURI("chrome://browser/content/content.css", null, null);
    styleSheets.loadAndRegisterSheet(styleURI, styleSheets.AGENT_SHEET);

    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.addObserver(gXPInstallObserver, "xpinstall-install-blocked", false);
    os.addObserver(gSessionHistoryObserver, "browser:purge-session-history", false);

    
    

    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = new nsBrowserAccess();

    let browsers = document.getElementById("browsers");
    browsers.addEventListener("command", this._handleContentCommand, false);
    browsers.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);

    
    function panCallback(aElement) {
      if (!aElement)
        return;

      
    }
    
    
    SpatialNavigation.init(browsers, panCallback);

    
    Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);

    
    
    
    if (window.arguments && window.arguments[0]) {
      var whereURI = null;

      try {
        
        var cmdLine = window.arguments[0].QueryInterface(Ci.nsICommandLine);

        try {
          
          whereURI = gPrefService.getCharPref("browser.startup.homepage");
        } catch (e) {}

        
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

      if (whereURI)
        this.addTab(whereURI, true);
    }

    
    if (gPrefService.getBoolPref("browser.console.showInPanel")){
      let tool_console = document.getElementById("tool-console");
      tool_console.hidden = false;
    }

    
    
    if (gPrefService.prefHasUserValue("temporary.disablePlugins")) {
      gPrefService.clearUserPref("temporary.disablePlugins");
      this.setPluginState(true);
    }

    bv.commitBatchOperation();


    dump("end startup\n");
  },

  shutdown: function() {
    this._browserView.setBrowser(null, false);

    BrowserUI.uninit();

    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.removeObserver(gXPInstallObserver, "xpinstall-install-blocked");
    os.removeObserver(gSessionHistoryObserver, "browser:purge-session-history");
    window.controllers.removeController(this);
    window.controllers.removeController(BrowserUI);
  },

  setPluginState: function(enabled)
  {
    var phs = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    var plugins = phs.getPluginTags({ });
    for (var i = 0; i < plugins.length; ++i)
      plugins[i].disabled = !enabled;
  },

  get browsers() {
    return this._browsers;
  },

  _resizeAndPaint: function() {
    
    this._browserView.simulateMozAfterSizeChange();
    

    this._browserView.zoomToPage();
    this._browserView.commitBatchOperation();

    if (this._pageLoading) {
      
      this._browserView.beginBatchOperation();
      this._loadingTimeout = setTimeout(resizeAndPaint, 2000);
    } else {
      delete this._loadingTimeout;
    }
  },

  startLoading: function() {
    if (this._pageLoading)
      throw "!@@!#!";

    this._pageLoading = true;

    if (!this._loadingTimeout) {
      this._browserView.beginBatchOperation();
      this._loadingTimeout = setTimeout(Util.bind(this, this._resizeAndPaint), 2000);
    }
  },

  endLoading: function() {
    if (!this._pageLoading)
      alert("endLoading when page is already done\n");

    this._pageLoading = false;
    clearTimeout(this._loadingTimeout);
    
    
    this._resizeAndPaint();
  },


  


  get selectedBrowser() {
    return this._selectedTab.browser;
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
    this._browsers.push(newTab.browser);

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
    this._browsers.splice(tabIndex, 1);

    
    for (let t = tabIndex; t < this._tabs.length; t++)
      this._tabs[t].updateThumbnail();
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

    let firstTab = this._selectedTab == null;
    this._selectedTab = tab;

    bv.beginBatchOperation();
    this._browserView.setBrowser(this.selectedBrowser, true);
    document.getElementById("tabs").selectedItem = tab.chromeTab;

    if (!firstTab) {
      let webProgress = this.selectedBrowser.webProgress;
      let securityUI = this.selectedBrowser.securityUI;

      try {
        tab._listener.onLocationChange(webProgress, null, this.selectedBrowser.currentURI);
        if (securityUI)
          tab._listener.onSecurityChange(webProgress, null, securityUI.state);
      } catch (e) {
        
        Components.utils.reportError(e);
      }

      let event = document.createEvent("Events");
      event.initEvent("TabSelect", true, false);
      tab.chromeTab.dispatchEvent(event);
    }
    bv.commitBatchOperation(true);
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

  getNotificationBox: function() {
    return document.getElementById("notifications");
  },

  findState: FINDSTATE_FIND,
  openFind: function(aState) {
    this.findState = aState;
    var findbar = document.getElementById("findbar");
    if (!findbar.browser)
      findbar.browser = this.selectedBrowser;

    var panel = document.getElementById("findbar-container");
    if (panel.hidden) {
      panel.hidden = false;
    }
    this.doFind();
  },

  doFind: function() {
    var findbar = document.getElementById("findbar");
    if (Browser.findState == FINDSTATE_FIND)
      findbar.onFindCommand();
    else
      findbar.onFindAgainCommand(Browser.findState == FINDSTATE_FIND_PREVIOUS);

    var panel = document.getElementById("findbar-container");
    panel.top = window.innerHeight - Math.floor(findbar.getBoundingClientRect().height);
  },

  translatePhoneNumbers: function() {
    let doc = getBrowser().contentDocument;
    
    let textnodes = doc.evaluate('//text()[contains(translate(., "0123456789", "^^^^^^^^^^"), "^^^^")]',
                                 doc,
                                 null,
                                 XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE,
                                 null);
    let s, node, lastLastIndex;
    let re = /(\+?1? ?-?\(?\d{3}\)?[ +-\.]\d{3}[ +-\.]\d{4})/;
    for (var i = 0; i < textnodes.snapshotLength; i++) {
      node = textnodes.snapshotItem(i);
      s = node.data;
      if (s.match(re)) {
        s = s.replace(re, "<a href='tel:$1'> $1 </a>");
        try {
          let replacement = doc.createElement("span");
          replacement.innerHTML = s;
          node.parentNode.insertBefore(replacement, node);
          node.parentNode.removeChild(node);
        } catch(e) {
          
        }
      }
    }
  },

  





  _handleContentCommand: function (aEvent) {
    
    if (!aEvent.isTrusted)
      return;

    var ot = aEvent.originalTarget;
    var errorDoc = ot.ownerDocument;

    
    
    if (/^about:neterror\?e=nssBadCert/.test(errorDoc.documentURI)) {
      if (ot == errorDoc.getElementById('exceptionDialogButton')) {
        var params = { exceptionAdded : false };

        try {
          switch (gPrefService.getIntPref("browser.ssl_override_behavior")) {
            case 2 : 
              params.prefetchCert = true;
            case 1 : 
              params.location = errorDoc.location.href;
          }
        } catch (e) {
          Components.utils.reportError("Couldn't get ssl_override pref: " + e);
        }

        window.openDialog('chrome://pippki/content/exceptionDialog.xul',
                          '','chrome,centerscreen,modal', params);

        
        if (params.exceptionAdded)
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
      dump("use -chrome command-line option to load external chrome urls\n");
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
    }
    else {
      if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_NEWTAB)
        newWindow = BrowserUI.newTab().browser.contentWindow;
      else
        newWindow = aOpener ? aOpener.top : browser.contentWindow;
    }

    try {
      var referrer;
      if (aURI) {
        if (aOpener) {
          location = aOpener.location;
          referrer = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService)
                                                            .newURI(location, null, null);
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




function IdentityHandler() {
  this._stringBundle = document.getElementById("bundle_browser");
  this._staticStrings = {};
  this._staticStrings[this.IDENTITY_MODE_DOMAIN_VERIFIED] = {
    encryption_label: this._stringBundle.getString("identity.encrypted2")
  };
  this._staticStrings[this.IDENTITY_MODE_IDENTIFIED] = {
    encryption_label: this._stringBundle.getString("identity.encrypted2")
  };
  this._staticStrings[this.IDENTITY_MODE_UNKNOWN] = {
    encryption_label: this._stringBundle.getString("identity.unencrypted2")
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

  








  checkIdentity: function(state, location) {
    var currentStatus = getBrowser().securityUI
                                .QueryInterface(Ci.nsISSLStatusProvider)
                                .SSLStatus;
    this._lastStatus = currentStatus;
    this._lastLocation = location;

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
    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();

      
      
      var lookupHost = this._lastLocation.host;
      if (lookupHost.indexOf(':') < 0)
        lookupHost += ":443";

      
      if (!this._overrideService)
        this._overrideService = Cc["@mozilla.org/security/certoverride;1"].getService(Ci.nsICertOverrideService);

      
      
      var tooltip = this._stringBundle.getFormattedString("identity.identified.verifier",
                                                          [iData.caOrg]);

      
      
      
      
      if (this._overrideService.hasMatchingOverride(this._lastLocation.hostname,
                                                    (this._lastLocation.port || 443),
                                                    iData.cert, {}, {}))
        tooltip = this._stringBundle.getString("identity.identified.verified_by_you");
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
      tooltip = this._stringBundle.getFormattedString("identity.identified.verifier",
                                                      [iData.caOrg]);
    }
    else {
      tooltip = this._stringBundle.getString("identity.unknown.tooltip");
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
      var owner = this._stringBundle.getString("identity.ownerUnknown2");
      verifier = this._identityBox.tooltipText;
      supplemental = "";
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
      host = this.getEffectiveHost();
      owner = iData.subjectOrg;
      verifier = this._identityBox.tooltipText;

      
      if (iData.city)
        supplemental += iData.city + "\n";
      if (iData.state && iData.country)
        supplemental += this._stringBundle.getFormattedString("identity.identified.state_and_country",
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
  },

  show: function ih_show() {
    this._identityPopup.hidden = false;
    this._identityPopup.top = BrowserUI.toolbarH;
    this._identityPopup.focus();

    
    this.setPopupMessages(this._identityBox.getAttribute("mode") || this.IDENTITY_MODE_UNKNOWN);

    window.addEventListener("blur", this, true);
  },

  hide: function ih_hide() {
    window.removeEventListener("blur", this, true);
    this._identityPopup.hidden = true;
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
  },

  handleEvent: function(event) {
    
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

  onUpdatePageReport: function (aEvent)
  {
    var cBrowser = Browser.selectedBrowser;
    if (aEvent.originalTarget != cBrowser)
      return;

    if (!cBrowser.pageReport)
      return;

    
    
    
    if (!cBrowser.pageReport.reported) {
      if(gPrefService.getBoolPref("privacy.popups.showBrowserMessage")) {
        var bundle_browser = document.getElementById("bundle_browser");
        var brandBundle = document.getElementById("bundle_brand");
        var brandShortName = brandBundle.getString("brandShortName");
        var message;
        var popupCount = cBrowser.pageReport.length;

        if (popupCount > 1)
          message = bundle_browser.getFormattedString("popupWarningMultiple", [brandShortName, popupCount]);
        else
          message = bundle_browser.getFormattedString("popupWarning", [brandShortName]);

        var notificationBox = Browser.getNotificationBox();
        var notification = notificationBox.getNotificationWithValue("popup-blocked");
        if (notification) {
          notification.label = message;
        }
        else {
          var buttons = [
            {
              label: bundle_browser.getString("popupButtonAlwaysAllow"),
              accessKey: bundle_browser.getString("popupButtonAlwaysAllow.accesskey"),
              callback: function() { gPopupBlockerObserver.toggleAllowPopupsForSite(); }
            },
            {
              label: bundle_browser.getString("popupButtonNeverWarn"),
              accessKey: bundle_browser.getString("popupButtonNeverWarn.accesskey"),
              callback: function() { gPopupBlockerObserver.dontShowMessage(); }
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

  toggleAllowPopupsForSite: function (aEvent)
  {
    var currentURI = Browser.selectedBrowser.webNavigation.currentURI;
    var pm = Cc["@mozilla.org/permissionmanager;1"].getService(this._kIPM);
    pm.add(currentURI, "popup", this._kIPM.ALLOW_ACTION);

    Browser.getNotificationBox().removeCurrentNotification();
  },

  dontShowMessage: function ()
  {
    var showMessage = gPrefService.getBoolPref("privacy.popups.showBrowserMessage");
    gPrefService.setBoolPref("privacy.popups.showBrowserMessage", !showMessage);
    Browser.getNotificationBox().removeCurrentNotification();
  }
};

const gXPInstallObserver = {
  observe: function (aSubject, aTopic, aData)
  {
    var brandBundle = document.getElementById("bundle_brand");
    var browserBundle = document.getElementById("bundle_browser");
    switch (aTopic) {
      case "xpinstall-install-blocked":
        var installInfo = aSubject.QueryInterface(Ci.nsIXPIInstallInfo);
        var host = installInfo.originatingURI.host;
        var brandShortName = brandBundle.getString("brandShortName");
        var notificationName, messageString, buttons;
        if (!gPrefService.getBoolPref("xpinstall.enabled")) {
          notificationName = "xpinstall-disabled";
          if (gPrefService.prefIsLocked("xpinstall.enabled")) {
            messageString = browserBundle.getString("xpinstallDisabledMessageLocked");
            buttons = [];
          }
          else {
            messageString = browserBundle.getFormattedString("xpinstallDisabledMessage",
                                                             [brandShortName, host]);
            buttons = [{
              label: browserBundle.getString("xpinstallDisabledButton"),
              accessKey: browserBundle.getString("xpinstallDisabledButton.accesskey"),
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
          messageString = browserBundle.getFormattedString("xpinstallPromptWarning",
                                                           [brandShortName, host]);

          buttons = [{
            label: browserBundle.getString("xpinstallPromptAllowButton"),
            accessKey: browserBundle.getString("xpinstallPromptAllowButton.accesskey"),
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

function getNotificationBox(aWindow) {
  return Browser.getNotificationBox();
}

function showDownloadsManager(aWindowContext, aID, aReason) {
  BrowserUI.show(UIMODE_PANEL);
  BrowserUI.switchPane("downloads-container");
  
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

  show: function had_show(aLauncher) {
    this._launcher = aLauncher;
    document.getElementById("helperapp-target").value = this._launcher.suggestedFileName;

    if (!this._launcher.MIMEInfo.hasDefaultHandler)
      document.getElementById("helperapp-open").disabled = true;

    let toolbar = document.getElementById("toolbar-main");
    let top = toolbar.top + toolbar.boxObject.height;
    let container = document.getElementById("helperapp-container");
    container.hidden = false;

    let rect = container.getBoundingClientRect();
    container.top = top < 0 ? 0 : top;
    container.left = (window.innerWidth - rect.width) / 2;
  },

  save: function had_save() {
    this._launcher.saveToDisk(null, false);
    this.close();
  },

  open: function had_open() {
    this._launcher.launchWithApplication(null, false);
    this.close();
  },

  close: function had_close() {
    document.getElementById("helperapp-target").value = "";
    let container = document.getElementById("helperapp-container");
    container.hidden = true;
  }
};

function ProgressController(tab) {
  this._tab = tab;
}

ProgressController.prototype = {
  get browser() {
    return this._tab.browser;
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
    
    if (aWebProgress.DOMWindow != this._tab.browser.contentWindow)
      return;

    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_START)
        this._networkStart();
      else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP)
        this._networkStop();
    } else if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP)
        this._documentStop();
    }
  },

  
  
  onProgressChange: function(aWebProgress, aRequest, aCurSelf, aMaxSelf, aCurTotal, aMaxTotal) {
  },

  
  onLocationChange: function(aWebProgress, aRequest, aLocationURI) {
    
    var location = aLocationURI ? aLocationURI.spec : "";
    let selectedBrowser = Browser.selectedBrowser;
    let lastURI = selectedBrowser.lastURI;

    
    if (!lastURI && (location == "about:blank" || location == "about:firstrun" ))
      return;

    this._hostChanged = true;

    
    
    
    
    
    
    
    
    selectedBrowser.lastURI = aLocationURI;
    if (lastURI) {
      var oldSpec = lastURI.spec;
      var oldIndexOfHash = oldSpec.indexOf("#");
      if (oldIndexOfHash != -1)
        oldSpec = oldSpec.substr(0, oldIndexOfHash);
      var newSpec = location;
      var newIndexOfHash = newSpec.indexOf("#");
      if (newIndexOfHash != -1)
        newSpec = newSpec.substr(0, newSpec.indexOf("#"));
      if (newSpec != oldSpec) {
        
        

        
        
        
      }
    }

    if (aWebProgress.DOMWindow == selectedBrowser.contentWindow) {
      BrowserUI.setURI();
    }
  },

  
  
  onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage) {
  },

  _networkStart: function() {
    this._tab.setLoading(true);
    if (Browser.selectedBrowser == this.browser) {
      Browser.startLoading();
      BrowserUI.update(TOOLBARSTATE_LOADING);

      
      
      if (this.browser.markupDocumentViewer.textZoom != kDefaultTextZoom)
        this.browser.markupDocumentViewer.textZoom = kDefaultTextZoom;
    }

    
    let event = document.createEvent("Events");
    event.initEvent("URLChanged", true, false);
    this.browser.dispatchEvent(event);
  },

  _networkStop: function _networkStop() {
    this._tab.setLoading(false);

    if (Browser.selectedBrowser == this.browser) {
      BrowserUI.update(TOOLBARSTATE_LOADED);
      this.browser.docShell.isOffScreenBrowser = true;
      Browser.endLoading();
    }

    this._tab.updateThumbnail();
  },

  _documentStop: function() {
    
    Browser.translatePhoneNumbers();

    if (Browser.selectedBrowser == this.browser) {
      
      if (this.browser.currentURI.spec != "about:blank")
        this.browser.contentWindow.focus();
    }
  },

 
  _state: null,
  _host: undefined,
  _hostChanged: false, 

  
  onSecurityChange: function(aWebProgress, aRequest, aState) {
    
    
    if (this._state == aState && !this._hostChanged) {
      return;
    }
    this._state = aState;

    try {
      this._host = getBrowser().contentWindow.location.host;
    }
    catch(ex) {
      this._host = null;
    }

    this._hostChanged = false;

    
    
    
    var location = getBrowser().contentWindow.location;
    var locationObj = {};
    try {
      locationObj.host = location.host;
      locationObj.hostname = location.hostname;
      locationObj.port = location.port;
    }
    catch (ex) {
      
      
      
    }
    getIdentityHandler().checkIdentity(this._state, locationObj);
  },

  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIWebProgressListener) ||
        aIID.equals(Ci.nsISupportsWeakReference) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};


function Tab() {
  this.create();
}

Tab.prototype = {
  _id: null,
  _browser: null,
  _state: null,
  _listener: null,
  _loading: false,
  _chromeTab: null,

  get browser() {
    return this._browser;
  },

  get chromeTab() {
    return this._chromeTab;
  },

  isLoading: function() {
    return this._loading;
  },

  setLoading: function(b) {
    this._loading = b;
  },

  load: function(uri) {
    dump("cb set src\n");
    this._browser.setAttribute("src", uri);
    dump("cb end src\n");
  },

  create: function() {
    this._chromeTab = document.createElement("richlistitem");
    this._chromeTab.setAttribute("type", "documenttab");
    document.getElementById("tabs").addTab(this._chromeTab);

    this._createBrowser();
  },

  destroy: function() {
    this._destroyBrowser();
    document.getElementById("tabs").removeTab(this._chromeTab);
    this._chromeTab = null;
  },

  _createBrowser: function() {
    if (this._browser)
      throw "Browser already exists";

    
    let scaledHeight = kDefaultBrowserWidth * (window.innerHeight / window.innerWidth);
    let browser = this._browser = document.createElement("browser");

    browser.setAttribute("style", "overflow: -moz-hidden-unscrollable; visibility: hidden; width: " + kDefaultBrowserWidth + "px; height: " + scaledHeight + "px;");
    browser.setAttribute("type", "content");

    
    let container = document.getElementById("tile-container");
    browser.setAttribute("contextmenu", container.getAttribute("contextmenu"));
    let autocompletepopup = container.getAttribute("autocompletepopup");
    if (autocompletepopup)
      browser.setAttribute("autocompletepopup", autocompletepopup);

    
    document.getElementById("browsers").appendChild(browser);

    
    browser.stop();

    
    this._listener = new ProgressController(this);
    browser.addProgressListener(this._listener);
  },

  _destroyBrowser: function() {
    document.getElementById("browsers").removeChild(this._browser);
    this._browser = null;
  },

  saveState: function() {
    let state = { };

    this._url = browser.contentWindow.location.toString();
    var browser = this.getBrowserForDisplay(display);
    var doc = browser.contentDocument;
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

    state._scrollX = browser.contentWindow.scrollX;
    state._scrollY = browser.contentWindow.scrollY;

    this._state = state;
  },

  restoreState: function() {
    let state = this._state;
    if (!state)
      return;

    let doc = this._browser.contentDocument;
    for (item in state) {
      var elem = null;
      if (item.charAt(0) == "#") {
        elem = doc.getElementById(item.substring(1));
      }
      else if (item.charAt(0) == "$") {
        var list = doc.getElementsByName(item.substring(1));
        if (list.length)
          elem = list[0];
      }

      if (elem)
        elem.value = state[item];
    }

    this._browser.contentWindow.scrollTo(state._scrollX, state._scrollY);
  },

  updateThumbnail: function() {
    if (!this._browser)
      return;

    
    let srcCanvas = (Browser.selectedBrowser == this._browser) ? document.getElementById("browser-canvas") : null;
    this._chromeTab.updateThumbnail(this._browser, srcCanvas);
  }
};
