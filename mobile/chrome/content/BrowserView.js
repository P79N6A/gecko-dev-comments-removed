







































let Ci = Components.interfaces;

const kBrowserViewZoomLevelMin = 0.2;
const kBrowserViewZoomLevelMax = 4.0;
const kBrowserViewZoomLevelPrecision = 10000;
const kBrowserViewPrefetchBeginIdleWait = 1;    
const kBrowserViewCacheSize = 15;
























































































function BrowserView(container, visibleRectFactory) {
  Util.bindAll(this);
  this.init(container, visibleRectFactory);
}









BrowserView.Util = {
  visibleRectToCriticalRect: function visibleRectToCriticalRect(visibleRect, browserViewportState) {
    return visibleRect.intersect(browserViewportState.viewportRect);
  },

  clampZoomLevel: function clampZoomLevel(zl) {
    let bounded = Math.min(Math.max(kBrowserViewZoomLevelMin, zl), kBrowserViewZoomLevelMax);
    let rounded = Math.round(bounded * kBrowserViewZoomLevelPrecision) / kBrowserViewZoomLevelPrecision;
    return (rounded) ? rounded : 1.0;
  },

  pageZoomLevel: function pageZoomLevel(visibleRect, browserW, browserH) {
    return BrowserView.Util.clampZoomLevel(visibleRect.width / browserW);
  },

  createBrowserViewportState: function createBrowserViewportState() {
    return new BrowserView.BrowserViewportState(new Rect(0, 0, 1, 1), 0, 0, 1);
  },

  getViewportStateFromBrowser: function getViewportStateFromBrowser(browser) {
    return browser.__BrowserView__vps;
  },

  getBrowserDimensions: function getBrowserDimensions(browser) {
    let cdoc = browser.contentDocument;
    if (cdoc instanceof SVGDocument) {
      let rect = cdoc.rootElement.getBoundingClientRect();
      return [Math.ceil(rect.width), Math.ceil(rect.height)];
    }

    
    let body = cdoc.body || {};
    let html = cdoc.documentElement || {};
    let w = Math.max(body.scrollWidth || 1, html.scrollWidth);
    let h = Math.max(body.scrollHeight || 1, html.scrollHeight);

    return [w, h];
  },

  getContentScrollOffset: function getContentScrollOffset(browser) {
    let cwu = BrowserView.Util.getBrowserDOMWindowUtils(browser);
    let scrollX = {};
    let scrollY = {};
    cwu.getScrollXY(false, scrollX, scrollY);

    return new Point(scrollX.value, scrollY.value);
  },

  getBrowserDOMWindowUtils: function getBrowserDOMWindowUtils(browser) {
    return browser.contentWindow
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils);
  },

  getNewBatchOperationState: function getNewBatchOperationState() {
    return {
      viewportSizeChanged: false,
      dirtyAll: false
    };
  },

  initContainer: function initContainer(container, visibleRect) {
    container.style.width    = visibleRect.width  + 'px';
    container.style.height   = visibleRect.height + 'px';
    container.style.overflow = '-moz-hidden-unscrollable';
  },

  resizeContainerToViewport: function resizeContainerToViewport(container, viewportRect) {
    container.style.width  = viewportRect.width  + 'px';
    container.style.height = viewportRect.height + 'px';
  }
};

BrowserView.prototype = {

  
  
  

  init: function init(container, visibleRectFactory) {
    this._batchOps = [];
    this._container = container;
    this._browser = null;
    this._browserViewportState = null;
    this._contentWindow = null;
    this._renderMode = 0;
    
    let cacheSize = kBrowserViewCacheSize;
    try {
      cacheSize = gPrefService.getIntPref("tile.cache.size");
    } catch(e) {}
    this._tileManager = new TileManager(this._appendTile, this._removeTile, this, cacheSize);
    this._visibleRectFactory = visibleRectFactory;

    this._idleServiceObserver = new BrowserView.IdleServiceObserver(this);
    this._idleService = Cc["@mozilla.org/widget/idleservice;1"].getService(Ci.nsIIdleService);
    this._idleService.addIdleObserver(this._idleServiceObserver, kBrowserViewPrefetchBeginIdleWait);
  },
  
  uninit: function uninit() {
    this.setBrowser(null, null, false);
    this._idleService.removeIdleObserver(this._idleServiceObserver, kBrowserViewPrefetchBeginIdleWait);
  },

  getVisibleRect: function getVisibleRect() {
    return this._visibleRectFactory();
  },

  setViewportDimensions: function setViewportDimensions(width, height, causedByZoom) {
    let bvs = this._browserViewportState;

    if (!bvs)
      return;

    bvs.viewportRect.right  = width;
    bvs.viewportRect.bottom = height;

    
    
    
    
    
    

    this._viewportChanged(true, !!causedByZoom);
  },

  setZoomLevel: function setZoomLevel(zl) {
    let bvs = this._browserViewportState;

    if (!bvs)
      return;

    let newZL = BrowserView.Util.clampZoomLevel(zl);

    if (newZL != bvs.zoomLevel) {
      let browserW = this.viewportToBrowser(bvs.viewportRect.right);
      let browserH = this.viewportToBrowser(bvs.viewportRect.bottom);
      bvs.zoomLevel = newZL; 
      this.setViewportDimensions(this.browserToViewport(browserW),
                                 this.browserToViewport(browserH),
                                 true);
      this.zoomChanged = true;
    }
  },

  getZoomLevel: function getZoomLevel() {
    let bvs = this._browserViewportState;
    if (!bvs)
      return undefined;

    return bvs.zoomLevel;
  },

  beginBatchOperation: function beginBatchOperation() {
    this._batchOps.push(BrowserView.Util.getNewBatchOperationState());
    this.pauseRendering();
  },

  commitBatchOperation: function commitBatchOperation() {
    let bops = this._batchOps;

    if (bops.length == 0)
      return;

    let opState = bops.pop();
    this._viewportChanged(opState.viewportSizeChanged, opState.dirtyAll);
    this.resumeRendering();
  },

  discardBatchOperation: function discardBatchOperation() {
    let bops = this._batchOps;
    bops.pop();
    this.resumeRendering();
  },

  discardAllBatchOperations: function discardAllBatchOperations() {
    let bops = this._batchOps;
    while (bops.length > 0)
      this.discardBatchOperation();
  },

  



  pauseRendering: function pauseRendering() {
    this._renderMode++;
  },

  



  resumeRendering: function resumeRendering(renderNow) {
    if (this._renderMode > 0)
      this._renderMode--;

    if (renderNow || this._renderMode == 0)
      this.renderNow();

    if (this._renderMode == 0)
      this._idleServiceObserver.resumeCrawls();
  },

  


  renderNow: function renderNow() {
    this._tileManager.criticalRectPaint();
  },

  isRendering: function isRendering() {
    return (this._renderMode == 0);
  },

  



  onBeforeVisibleMove: function onBeforeVisibleMove(dx, dy) {
    let vs = this._browserViewportState;
    let vr = this.getVisibleRect();

    let destCR = BrowserView.Util.visibleRectToCriticalRect(vr.translate(dx, dy), vs);

    this._tileManager.beginCriticalMove(destCR);
  },

  onAfterVisibleMove: function onAfterVisibleMove() {
    let vs = this._browserViewportState;
    let vr = this.getVisibleRect();

    vs.visibleX = vr.left;
    vs.visibleY = vr.top;

    let cr = BrowserView.Util.visibleRectToCriticalRect(vr, vs);

    this._tileManager.endCriticalMove(cr, this.isRendering());
  },

  


  setBrowser: function setBrowser(browser, browserViewportState, doZoom) {
    if (browser && !browserViewportState) {
      throw "Cannot set non-null browser with null BrowserViewportState";
    }

    let browserChanged = (this._browser !== browser);

    if (this._browser) {
      this._browser.removeEventListener("MozAfterPaint", this.handleMozAfterPaint, false);
      this._browser.removeEventListener("scroll", this.handlePageScroll, false);

      
      
      this._browser.removeEventListener("FakeMozAfterSizeChange", this.handleMozAfterSizeChange, false);
      

      this._browser.setAttribute("type", "content");
      this._browser.docShell.isOffScreenBrowser = false;
    }

    this._browser = browser;
    this._contentWindow = (browser) ? browser.contentWindow : null;
    this._browserViewportState = browserViewportState;

    if (browser) {
      browser.setAttribute("type", "content-primary");

      this.beginBatchOperation();

      browser.addEventListener("MozAfterPaint", this.handleMozAfterPaint, false);
      browser.addEventListener("scroll", this.handlePageScroll, false);

      
      
      browser.addEventListener("FakeMozAfterSizeChange", this.handleMozAfterSizeChange, false);
      

      if (doZoom) {
        browser.docShell.isOffScreenBrowser = true;
        this.zoomToPage();
      }

      this._viewportChanged(browserChanged, browserChanged);

      this.commitBatchOperation();
    }
  },

  getBrowser: function getBrowser() {
    return this._browser;
  },

  handleMozAfterPaint: function handleMozAfterPaint(ev) {
    let browser = this._browser;
    let tm = this._tileManager;
    let vs = this._browserViewportState;

    let { x: scrollX, y: scrollY } = BrowserView.Util.getContentScrollOffset(browser);
    let clientRects = ev.clientRects;

    let rects = [];
    
    for (let i = clientRects.length - 1; i >= 0; --i) {
      let e = clientRects.item(i);
      let r = new Rect(e.left + scrollX,
                            e.top + scrollY,
                            e.width, e.height);

      r = this.browserToViewportRect(r);
      r.expandToIntegers();

      if (r.right < 0 || r.bottom < 0)
        continue;

      r.restrictTo(vs.viewportRect);
      if (!r.isEmpty())
        rects.push(r);
    }

    tm.dirtyRects(rects, this.isRendering());
  },

  
  handlePageScroll: function handlePageScroll(aEvent) {
    if (aEvent.target != this._browser.contentDocument)
      return;

    let { x: scrollX, y: scrollY } = BrowserView.Util.getContentScrollOffset(this._browser);
    Browser.contentScrollboxScroller.scrollTo(this.browserToViewport(scrollX), 
                                              this.browserToViewport(scrollY));
    this.onAfterVisibleMove();
  },

  
  simulateMozAfterSizeChange: function simulateMozAfterSizeChange() {
    let [w, h] = BrowserView.Util.getBrowserDimensions(this._browser);
    let ev = document.createEvent("MouseEvents");
    ev.initMouseEvent("FakeMozAfterSizeChange", false, false, window, 0, w, h, 0, 0, false, false, false, false, 0, null);
    this._browser.dispatchEvent(ev);
  },
  

  handleMozAfterSizeChange: function handleMozAfterSizeChange(ev) {
    
    
    
    
    let w = ev.screenX;
    let h = ev.screenY;
    
    this.setViewportDimensions(this.browserToViewport(w), this.browserToViewport(h));
  },

  zoomToPage: function zoomToPage() {
    let browser = this._browser;

    if (!browser)
      return;

    var windowUtils = browser.contentWindow
                             .QueryInterface(Ci.nsIInterfaceRequestor)
                             .getInterface(Ci.nsIDOMWindowUtils);
    var handheldFriendly = windowUtils.getDocumentMetadata("HandheldFriendly");
    
    if (handheldFriendly == "true") {
      browser.className = "browser-handheld";
      this.setZoomLevel(1);
      browser.markupDocumentViewer.textZoom = 1;
    } else {
      browser.className = "browser";
      let [w, h] = BrowserView.Util.getBrowserDimensions(browser);
      this.setZoomLevel(BrowserView.Util.pageZoomLevel(this.getVisibleRect(), w, h));
    }
  },

  zoom: function zoom(aDirection) {
    let bvs = this._browserViewportState;

    if (!bvs)
      throw "No browser is set";

    if (aDirection == 0)
      return;

    var zoomDelta = 0.05; 
    if (aDirection >= 0)
      zoomDelta *= -1;

    this.setZoomLevel(bvs.zoomLevel + zoomDelta);
  },

  









  renderToCanvas: function renderToCanvas(destCanvas, destWidth, destHeight, srcRect) {
    let bvs = this._browserViewportState;
    if (!bvs) {
      throw "Browser viewport state null in call to renderToCanvas (probably no browser set on BrowserView).";
    }

    if (!srcRect) {
      let vr = this.getVisibleRect();
      let cr = BrowserView.Util.visibleRectToCriticalRect(vr, bvs);
      vr.x = cr.left;
      vr.y = cr.top;
      srcRect = vr;
    }

    let scalex = (destWidth / srcRect.width) || 1;
    let scaley = (destHeight / srcRect.height) || 1;

    srcRect.restrictTo(bvs.viewportRect);
    this._tileManager.renderRectToCanvas(srcRect, destCanvas, scalex, scaley);
  },

  viewportToBrowser: function viewportToBrowser(x) {
    let bvs = this._browserViewportState;

    if (!bvs)
      throw "No browser is set";

    return x / bvs.zoomLevel;
  },

  browserToViewport: function browserToViewport(x) {
    let bvs = this._browserViewportState;

    if (!bvs)
      throw "No browser is set";

    return x * bvs.zoomLevel;
  },

  viewportToBrowserRect: function viewportToBrowserRect(rect) {
    let f = this.viewportToBrowser(1.0);
    return rect.scale(f, f);
  },

  browserToViewportRect: function browserToViewportRect(rect) {
    let f = this.browserToViewport(1.0);
    return rect.scale(f, f);
  },

  browserToViewportCanvasContext: function browserToViewportCanvasContext(ctx) {
    let f = this.browserToViewport(1.0);
    ctx.scale(f, f);
  },

  forceContainerResize: function forceContainerResize() {
    let bvs = this._browserViewportState;
    if (bvs)
      BrowserView.Util.resizeContainerToViewport(this._container, bvs.viewportRect);
  },

  
  
  

  _viewportChanged: function _viewportChanged(viewportSizeChanged, dirtyAll) {
    let bops = this._batchOps;

    if (bops.length > 0) {
      let opState = bops[bops.length - 1];

      if (viewportSizeChanged)
        opState.viewportSizeChanged = viewportSizeChanged;

      if (dirtyAll)
        opState.dirtyAll = dirtyAll;

      return;
    }

    let bvs = this._browserViewportState;

    if (bvs) {
      BrowserView.Util.resizeContainerToViewport(this._container, bvs.viewportRect);

      if (dirtyAll) {
        
        
        BrowserView.Util.getBrowserDOMWindowUtils(this._browser).clearMozAfterPaintEvents();
      }

      this._tileManager.viewportChangeHandler(bvs.viewportRect,
                                              BrowserView.Util.visibleRectToCriticalRect(this.getVisibleRect(), bvs),
                                              viewportSizeChanged,
                                              dirtyAll);
    }
  },

  _appendTile: function _appendTile(tile) {
    let canvas = tile.getContentImage();

    
    
    

    
    
    
    
    
    
    
    
    canvas.setAttribute("style", "position: absolute; left: " + tile.boundRect.left + "px; " + "top: " + tile.boundRect.top + "px;");

    this._container.appendChild(canvas);
  },

  _removeTile: function _removeTile(tile) {
    let canvas = tile.getContentImage();

    this._container.removeChild(canvas);
  }

};













BrowserView.BrowserViewportState = function(viewportRect,
                                            visibleX,
                                            visibleY,
                                            zoomLevel) {

  this.init(viewportRect, visibleX, visibleY, zoomLevel);
};

BrowserView.BrowserViewportState.prototype = {

  init: function init(viewportRect, visibleX, visibleY, zoomLevel) {
    this.viewportRect = viewportRect;
    this.visibleX     = visibleX;
    this.visibleY     = visibleY;
    this.zoomLevel    = zoomLevel;
    this.zoomChanged  = false;
  },

  clone: function clone() {
    return new BrowserView.BrowserViewportState(this.viewportRect,
                                                this.visibleX,
                                                this.visibleY,
                                                                                    this.zoomLevel);
  },

  toString: function toString() {
    let props = ['\tviewportRect=' + this.viewportRect.toString(),
                 '\tvisibleX='     + this.visibleX,
                 '\tvisibleY='     + this.visibleY,
                 '\tzoomLevel='    + this.zoomLevel];

    return '[BrowserViewportState] {\n' + props.join(',\n') + '\n}';
  }

};







BrowserView.IdleServiceObserver = function IdleServiceObserver(browserView) {
  this._browserView = browserView;
  this._crawlStarted = false;
  this._crawlPause = false;
  this._idleState = false;
};

BrowserView.IdleServiceObserver.prototype = {

  isIdle: function isIdle() {
    return this._idleState;
  },

  observe: function observe(aSubject, aTopic, aUserIdleTime) {
    let bv = this._browserView;

    if (aTopic == "idle")
      this._idleState = true;
    else
      this._idleState = false;

    if (this._idleState && !this._crawlStarted) {
      if (bv.isRendering()) {
        bv._tileManager.restartPrefetchCrawl();
        this._crawlStarted = true;
        this._crawlPause = false;
      } else {
        this._crawlPause = true;
      }
    } else if (!this._idleState && this._crawlStarted) {
      this._crawlStarted = false;
      this._crawlPause = false;
      bv._tileManager.stopPrefetchCrawl();
    }
  },

  resumeCrawls: function resumeCrawls() {
    if (this._crawlPause) {
      this._browserView._tileManager.restartPrefetchCrawl();
      this._crawlStarted = true;
      this._crawlPause = false;
    }
  }
};
