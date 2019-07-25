













































let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

const FINDSTATE_FIND = 0;
const FINDSTATE_FIND_AGAIN = 1;
const FINDSTATE_FIND_PREVIOUS = 2;

const endl = '\n';

Cu.import("resource://gre/modules/SpatialNavigation.js");

function getBrowser() {
  return Browser.selectedBrowser;
}

const kDefaultTextZoom = 1.2;
const kDefaultBrowserWidth = 1024;

function debug() {
  let bv = Browser._browserView;
  let tc = bv._tileManager._tileCache;
  let scrollbox = document.getElementById("tile-container-container")
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

    dump('location from Browser: ' + Browser.selectedBrowser.contentWindow.location + endl);
    dump('location from BV     : ' + bv.getBrowser().contentWindow.location + endl);

    dump(endl + endl);

    let cr = bv._tileManager._criticalRect;
    dump('criticalRect from BV: ' + (cr ? cr.toString() : null) + endl);
    dump('visibleRect from BV : ' + bv._visibleRect + endl);
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


    let sb = document.getElementById("tile-container-container");
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

function onKeyPress(ev) {
  let bv = Browser._browserView;

  if (!ev.ctrlKey)
    return;

  const a = 97;   
  const b = 98;   
  const c = 99;   
  const d = 100;  
  const e = 101;
  const f = 102;
  const g = 103;
  const h = 104;
  const i = 105;  
  const j = 106;
  const k = 107;
  const l = 108;  
  const m = 109;  
  const n = 110;
  const o = 111;
  const p = 112;  
  const q = 113;
  const r = 114;  
  const s = 115;
  const t = 116;  
  const u = 117;
  const v = 118;
  const w = 119;
  const x = 120;
  const y = 121;
  const z = 122;  

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
  case r:
    bv.onAfterVisibleMove();
    

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
  _browsers : [],
  _selectedTab : null,
  windowUtils: window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils),
  contentScrollbox: null,
  contentScrollboxScroller: null,
  controlsScrollbox: null,
  controlsScrollboxScroller: null,

  startup: function() {
    var self = this;

    dump("begin startup\n");

    let container = document.getElementById("tile-container");
    let bv = this._browserView = new BrowserView(container, Browser.getVisibleRect);

    
    container.customClicker = this._createContentCustomClicker(bv);

    
    let contentScrollbox = this.contentScrollbox = document.getElementById("tile-container-container");
    this.contentScrollboxScroller = contentScrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    contentScrollbox.customDragger = new Browser.MainDragger(bv);

    
    let controlsScrollbox = this.controlsScrollbox = document.getElementById("scrollbox");
    this.controlsScrollboxScroller = controlsScrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    controlsScrollbox.customDragger = {
      dragStart: function dragStart(cx, cy, target, scroller) {},
      dragStop: function dragStop(dx, dy, scroller) { return false; },
      dragMove: function dragMove(dx, dy, scroller) { return false; }
    };

    
    bv.beginBatchOperation();

    function resizeHandler(e) {

      if (e.target != window)
        return;

      dump(window.innerWidth + "," + window.innerHeight + "\n");
      
      let w = window.innerWidth;
      let h = window.innerHeight;
      let maximize = (document.documentElement.getAttribute("sizemode") == "maximized");
      if (maximize && w > screen.width)
        return;

      bv.beginBatchOperation();

      contentScrollbox.style.width  = w + 'px';
      contentScrollbox.style.height = h + 'px';

      controlsScrollbox.style.width  = w + 'px';
      controlsScrollbox.style.height = h + 'px';

      let toolbarHeight = Math.round(document.getElementById("toolbar-main").getBoundingClientRect().height);
      let spacers = document.getElementsByClassName("sidebar-spacer");
      for (let i = 0, len = spacers.length; i < len; i++) spacers[i].style.height = toolbarHeight + 'px';

      let toolbarContainer = document.getElementById("toolbar-container");
      let stackToolbarContainer = document.getElementById("stack-toolbar-container");
      toolbarContainer.style.width = w + 'px';
      toolbarContainer.style.height = toolbarHeight + 'px';
      stackToolbarContainer.style.width = w + 'px';
      stackToolbarContainer.style.height = toolbarHeight + 'px';

      
      BrowserUI.sizeControls(w, h);

      
      let browsers = Browser.browsers;
      if (browsers) {
        let scaledH = (kDefaultBrowserWidth * (h / w));
        for (let i=0; i<browsers.length; i++) {
          let browserStyle = browsers[i].style;
          browserStyle.height = scaledH + 'px';
        }
      }

      bv.onAfterVisibleMove();
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

    
    ih = new InputHandler(container);

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
#ifdef WINCE
    os.addObserver(SoftKeyboardObserver, "softkb-change", false);
#endif

    
    

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
#ifdef WINCE
    os.removeObserver(SoftKeyboardObserver, "softkb-change");
#endif

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
      this._loadingTimeout = setTimeout(Util.bind(Browser._resizeAndPaint, Browser), 2000);
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
      this._loadingTimeout = setTimeout(Util.bind(Browser._resizeAndPaint, Browser), 2000);
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
    bv.setBrowser(this.selectedBrowser, true);
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

  





  _handleContentCommand: function _handleContentCommand(aEvent) {
    
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
  },

  _createContentCustomClicker: function _createContentCustomClicker(browserView) {
    
    

    function dispatchContentClick(browser, x, y) {
      let cwu = BrowserView.Util.getBrowserDOMWindowUtils(browser);
      let scrollX = { value: 0 };
      let scrollY = { value: 0 };
      cwu.getScrollXY(false, scrollX, scrollY);
      cwu.sendMouseEvent("mousedown", x - scrollX.value, y - scrollY.value, 0, 1, 0, true);
      cwu.getScrollXY(false, scrollX, scrollY);
      cwu.sendMouseEvent("mouseup",   x - scrollX.value, y - scrollY.value, 0, 1, 0, true);
    }

    return {
      
      
      
      zoomIn: false,

      singleClick: function singleClick(cX, cY) {
        let browser = browserView.getBrowser();
        if (browser) {
          let [x, y] = Browser.transformClientToBrowser(cX, cY);
          dispatchContentClick(browser, x, y);
        }
      },

      doubleClick: function doubleClick(cX1, cY1, cX2, cY2) {
        let [x, y] = Browser.transformClientToBrowser(cX2, cY2);
        let zoomElement = Browser.elementFromPoint(x, y);

        if (zoomElement) {
          dump('@@@ zoomElement is ' + zoomElement + ' :: ' + zoomElement.id + ' :: ' + zoomElement.name + '\n');
          this.zoomIn = !this.zoomIn;

          if (this.zoomIn)
            Browser.zoomToElement(zoomElement);
          else
            Browser.zoomFromElement(zoomElement);
        }
      },

      toString: function toString() {
        return "[ContentCustomClicker] { zoomed=" + this.zoomIn + " }";
      }
    };
  },

  




  computeSidebarVisibility: function computeSidebarVisibility() {
    
    function visibility(bar, visrect) {
      try {
        let w = bar.width;
        let h = bar.height;
        bar.restrictTo(visrect); 
        return bar.width / w;
      } catch (e) {
        return 0;
      }
    }

    let leftbarCBR = document.getElementById('tabs-container').getBoundingClientRect();
    let ritebarCBR = document.getElementById('browser-controls').getBoundingClientRect();

    let leftbar = new wsRect(leftbarCBR.left, 0, leftbarCBR.width, 1);
    let ritebar = new wsRect(ritebarCBR.left, 0, ritebarCBR.width, 1);
    let leftw = leftbar.width;
    let ritew = ritebar.width;

    let visrect = new wsRect(0, 0, window.innerWidth, 1);

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

      snappedX = Math.round(snappedX);
    }
    else if (ritevis != 0 && ritevis != 1) {
      if (ritevis >= 0.6666) {
        snappedX = (1 - ritevis) * ritew;
      } else {
        snappedX = -ritevis * ritew;
      }

      snappedX = Math.round(snappedX);
    }

    return snappedX;
  },

  tryFloatToolbar: function tryFloatToolbar() {
    let stackToolbarContainer = document.getElementById("stack-toolbar-container");
    let toolbarMain = document.getElementById("toolbar-main");

    if (toolbarMain.parentNode == stackToolbarContainer)
      return true;

    let [leftvis, ritevis, leftw, ritew] = Browser.computeSidebarVisibility();

    
    if (leftvis > 0.002 || ritevis > 0.002) {
      let toolbarContainer = document.getElementById("toolbar-container");

      
      dump("moving toolbar to stack\n");
      stackToolbarContainer.appendChild(toolbarMain);
      stackToolbarContainer.setAttribute("hidden", false);

      return true;
    }
    return false;
  },

  tryUnfloatToolbar: function tryUnfloatToolbar() {
    let toolbarContainer = document.getElementById("toolbar-container");
    let toolbarMain = document.getElementById("toolbar-main");

    if (toolbarMain.parentNode == toolbarContainer)
      return true;

    let [leftvis, ritevis, leftw, ritew] = Browser.computeSidebarVisibility();

    if (leftvis <= 0.002 && ritevis <= 0.002) {
      let stackToolbarContainer = document.getElementById("stack-toolbar-container");

      dump("moving toolbar to scrollbox\n");
      toolbarContainer.appendChild(toolbarMain);
      stackToolbarContainer.setAttribute("hidden", true);
      return true;
    }
    return false;
  },

  zoomToElement: function zoomToElement(aElement) {
    const margin = 15;

    let bv = Browser._browserView;
    let vis = bv.getVisibleRect();
    let scroller = Browser.contentScrollboxScroller;

    let elRect = Browser.getBoundingContentRect(aElement);
    let elWidth = elRect.width;
    let vrWidth = vis.width;
    

    let zoomLevel = vrWidth / (elWidth + (2 * margin));

    bv.beginBatchOperation();

    bv.setZoomLevel(zoomLevel);

    



    let screenW = vrWidth - bv.browserToViewport(elWidth);
    let xpadding = Math.max(margin, screenW);

    let x0 = vis.left;
    let y0 = vis.top;

    let x = bv.browserToViewport(elRect.left) - xpadding;
    let y = bv.browserToViewport(elRect.top) - margin;

    x = Math.floor(Math.max(x, 0));
    y = Math.floor(Math.max(y, 0));

    bv.forceContainerResize();
    Browser.forceChromeReflow();

    dump('zoom to element at ' + x + ', ' + y + ' by dragging ' + (x - x0) + ', ' + (y - y0) + '\n');

    Browser.contentScrollbox.customDragger.dragMove(x - x0, y - y0, scroller);

    bv.commitBatchOperation();
  },

  zoomFromElement: function zoomFromElement(aElement) {
    let bv = Browser._browserView;
    let vis = bv.getVisibleRect();
    let scroller = Browser.contentScrollboxScroller;

    let elRect = Browser.getBoundingContentRect(aElement);

    bv.beginBatchOperation();

    bv.zoomToPage();

    let x0 = vis.left;
    let y0 = vis.top;

    let x = bv.browserToViewport(elRect.left);
    let y = bv.browserToViewport(elRect.top);

    x = Math.floor(Math.max(x, 0));
    y = Math.floor(Math.max(y, 0));

    bv.forceContainerResize();
    Browser.forceChromeReflow();

    dump('zoom from element at ' + x + ', ' + y + ' by dragging ' + (x - x0) + ', ' + (y - y0) + '\n');

    Browser.contentScrollbox.customDragger.dragMove(x - x0, y - y0, scroller);

    bv.commitBatchOperation();
  },

  getBoundingContentRect: function getBoundingContentRect(contentElem) {
    let browser = Browser._browserView.getBrowser();

    if (!browser)
      return null;

    let scrollX = { value: 0 };
    let scrollY = { value: 0 };

    let cwu = BrowserView.Util.getBrowserDOMWindowUtils(browser);
    cwu.getScrollXY(false, scrollX, scrollY);

    let r = contentElem.getBoundingClientRect();

    dump('getBoundingContentRect: clientRect is at ' + r.left + ', ' + r.top + '; scrolls are ' + scrollX.value + ', ' + scrollY.value + '\n');

    return new wsRect(r.left + scrollX.value,
                      r.top + scrollY.value,
                      r.width, r.height);
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

  



  transformClientToBrowser: function transformClientToBrowser(cX, cY) {
    return this.clientToBrowserView(cX, cY).map(this._browserView.viewportToBrowser);
  },

  



  elementFromPoint: function elementFromPoint(x, y) {
    Util.dumpLn("*** elementFromPoint: page ", x, ",", y);

    let browser = this._browserView.getBrowser();
    if (!browser) return null;

    let cwu = BrowserView.Util.getBrowserDOMWindowUtils(browser);

    let scrollX = { value: 0 }, scrollY = { value: 0 };
    cwu.getScrollXY(false, scrollX, scrollY);
    x = x - scrollX.value;
    y = y - scrollY.value;

    let elem = cwu.elementFromPoint(x, y,
                                    true,   
                                    false); 

    
    while (elem && (elem instanceof HTMLIFrameElement || elem instanceof HTMLFrameElement)) {
      let frameWin = elem.ownerDocument.defaultView;
      let frameUtils = frameWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
      frameUtils.getScrollXY(false, scrollX, scrollY);

      x = x - elem.offsetLeft + scrollX.value;
      y = y - elem.offsetTop + scrollY.value;
      elem = elem.contentDocument.elementFromPoint(x, y);
    }

    return elem;
  },

  



  getVisibleRect: function getVisibleRect() {
    let container = document.getElementById("tile-container");
    let containerBCR = container.getBoundingClientRect();

    let x = Math.round(-containerBCR.left);
    let y = Math.round(-containerBCR.top);
    let w = window.innerWidth;
    let h = window.innerHeight;

    return new wsRect(x, y, w, h);
  },

  






  getScrollboxPosition: function getScrollboxPosition(scroller) {
    let x = {};
    let y = {};
    scroller.getPosition(x, y);
    return [x.value, y.value];
  },

  forceChromeReflow: function forceChromeReflow() {
    let dummy = getComputedStyle(document.documentElement, "").width;
  }

};

Browser.MainDragger = function MainDragger(browserView) {
  this.scrollingOuterX = true;
  this.bv = browserView;
  this.floatedWhileDragging = false;
  this.draggedFrame = null;
};

Browser.MainDragger.prototype = {
  _targetIsContent: function _targetIsContent(target) {
    let tileBox = document.getElementById("tile-container");
    while (target) {
      if (target === window)
        return false;
      if (target === tileBox)
        return true;

      target = target.parentNode;
    }
    return false;
  },

  dragStart: function dragStart(clientX, clientY, target, scroller) {
    this.draggedFrame = null;

    if (this._targetIsContent(target)) {
      
      
      let [x, y] = Browser.transformClientToBrowser(clientX, clientY);
      let element = Browser.elementFromPoint(x, y);
      if (element && element.ownerDocument != Browser.selectedBrowser.contentDocument) {
        Util.dumpLn("*** dragStart got element ", element, " ownerDoc ", element.ownerDocument,
                    " selectedBrowser.contentDoc ", Browser.selectedBrowser.contentDocument);
        this.draggedFrame = element.ownerDocument.defaultView;
      }
    }

    this.bv.pauseRendering();
    this.floatedWhileDragging = false;
  },

  _panFrame: function _panFrame(dx, dy) {
    if (this.draggedFrame === null)
      return false;

    if (dx == 0 && dy == 0)
      return true;

    let panned = false;
    let elem = this.draggedFrame;

    
    
    
    while (elem && elem !== elem.parent.document.defaultView) {
      let windowUtils = elem.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);

      let origX = {}, origY = {};
      windowUtils.getScrollXY(false, origX, origY);

      elem.scrollBy(dx, dy);

      let newX = {}, newY = {};
      windowUtils.getScrollXY(false, newX, newY);

      panned = (origX.value != newX.value) || (origY.value != newY.value);

      if (panned) {
        
        
        this.bv.renderNow();
        break;
      }

      elem = elem.parent.document.defaultView;
    }

    return panned;
  },

  dragStop: function dragStop(dx, dy, scroller) {
    let dx = this.dragMove(dx, dy, scroller, true);

    dx += this.dragMove(Browser.snapSidebars(), 0, scroller, true);

    Browser.tryUnfloatToolbar();

    this.bv.resumeRendering();
    this.floatedWhileDragging = false;

    return (dx != 0) || (dy != 0);
  },

  dragMove: function dragMove(dx, dy, scroller, doReturnDX) {
    let outrv = 0;

    
    if (this._panFrame(dx, dy))
      return true;

    if (this.scrollingOuterX) {
      let odx = 0;
      let ody = 0;

      if (dx > 0) {
        let contentleft = Math.floor(Browser.contentScrollbox.getBoundingClientRect().left);
        odx = (contentleft > 0) ? Math.min(contentleft, dx) : dx;
      } else if (dx < 0) {
        let contentright = Math.ceil(Browser.contentScrollbox.getBoundingClientRect().right);
        let w = window.innerWidth;
        odx = (contentright < w) ? Math.max(contentright - w, dx) : dx;
      }

      if (odx) {
        outrv = this.outerDragMove(odx, ody, Browser.controlsScrollboxScroller, doReturnDX);
      }

      if (odx != dx || ody != dy) {
        this.scrollingOuterX = false;
        dx -= odx;
        dy -= ody;
      } else {
        return outrv;
      }
    }

    this.bv.onBeforeVisibleMove(dx, dy);

    if (!this.floatedWhileDragging)
      this.floatedWhileDragging = Browser.tryFloatToolbar();

    let [x0, y0] = Browser.getScrollboxPosition(scroller);
    scroller.scrollBy(dx, dy);
    let [x1, y1] = Browser.getScrollboxPosition(scroller);

    let realdx = x1 - x0;
    let realdy = y1 - y0;

    this.bv.onAfterVisibleMove(realdx, realdy);

    if (realdx != dx) {
      let restdx = dx - realdx;

      dump("--> restdx: " + restdx + "\n");

      this.scrollingOuterX = true;
      this.dragMove(restdx, 0, scroller, doReturnDX);
    }

    return (doReturnDX) ? (outrv + realdx) : (outrv || realdx != 0 || realdy != 0);
  },

  outerDragMove: function outerDragMove(dx, dy, scroller, doReturnDX) {
    this.bv.onBeforeVisibleMove(dx, dy);

    let [x0, y0] = Browser.getScrollboxPosition(scroller);
    scroller.scrollBy(dx, dy);
    let [x1, y1] = Browser.getScrollboxPosition(scroller);

    let realdx = x1 - x0;
    let realdy = y1 - y0;

    this.bv.onAfterVisibleMove(realdx, realdy);

    return (doReturnDX) ? realdx : (realdx != 0 || realdy != 0);
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

    this._identityBox.setAttribute("open", "true");

    
    this.setPopupMessages(this._identityBox.getAttribute("mode") || this.IDENTITY_MODE_UNKNOWN);
  },

  hide: function ih_hide() {
    this._identityPopup.hidden = true;

    this._identityBox.removeAttribute("open");
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

  toggleAllowPopupsForSite: function toggleAllowPopupsForSite(aEvent)
  {
    var currentURI = Browser.selectedBrowser.webNavigation.currentURI;
    var pm = Cc["@mozilla.org/permissionmanager;1"].getService(this._kIPM);
    pm.add(currentURI, "popup", this._kIPM.ALLOW_ACTION);

    Browser.getNotificationBox().removeCurrentNotification();
  },

  dontShowMessage: function dontShowMessage()
  {
    var showMessage = gPrefService.getBoolPref("privacy.popups.showBrowserMessage");
    gPrefService.setBoolPref("privacy.popups.showBrowserMessage", !showMessage);
    Browser.getNotificationBox().removeCurrentNotification();
  }
};

const gXPInstallObserver = {
  observe: function xpi_observer(aSubject, aTopic, aData)
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

#ifdef WINCE


var SoftKeyboardObserver = {
  observe: function sko_observe(subject, topic, data) {
    if (topic === "softkb-change") {
      
      
      let rect = JSON.parse(data);
      if (rect) {
        let height = rect.bottom - rect.top;
        let width = rect.right - rect.left;
        window.resizeTo(width, height);
      }
    }
  }
};
#endif

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

  onStateChange: function onStateChange(aWebProgress, aRequest, aStateFlags, aStatus) {
    
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

  _networkStart: function _networkStart() {
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
    this._browser.setAttribute("src", uri);
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
