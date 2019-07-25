







































let Ci = Components.interfaces;

const kBrowserViewZoomLevelMin = 0.2;
const kBrowserViewZoomLevelMax = 4.0;
const kBrowserFormZoomLevelMin = 1.0;
const kBrowserFormZoomLevelMax = 2.0;
const kBrowserViewZoomLevelPrecision = 10000;
const kBrowserViewZoomLevelIncrement = 0.1;
const kBrowserViewZoomLevelPageFitAdjust = 0.2;
const kBrowserViewPrefetchBeginIdleWait = 1;    
const kBrowserViewPrefetchBeginIdleWaitLoading = 10;    
const kBrowserViewCacheSize = 6;
























































































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

  
  adjustZoomLevel: function adjustZoomLevel(zl, threshold) {
    return (Math.abs(1.0 - zl) < threshold) ? 1.0 : zl;
  },

  pageZoomLevel: function pageZoomLevel(visibleRect, browserW, browserH) {
    let zl = BrowserView.Util.clampZoomLevel(visibleRect.width / browserW);
    return BrowserView.Util.adjustZoomLevel(zl, kBrowserViewZoomLevelPageFitAdjust);
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
    this._offscreenDepth = 0;

    let cacheSize = kBrowserViewCacheSize;
    try {
      cacheSize = gPrefService.getIntPref("tile.cache.size");
    } catch(e) {}

    if (cacheSize == -1) {
      let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
      let device = sysInfo.get("device");
      switch (device) {
#ifdef MOZ_PLATFORM_MAEMO
        case "Nokia N900":
          cacheSize = 26;
          break;
        case "Nokia N8xx":
          
          cacheSize = 10;
          break;
#endif
        default:
          
          cacheSize = 6;
      }
    }

    this._tileManager = new TileManager(this._appendTile, this._removeTile, this, cacheSize);
    this._visibleRectFactory = visibleRectFactory;

    this._idleServiceObserver = new BrowserView.IdleServiceObserver(this);
    this._idleService = Cc["@mozilla.org/widget/idleservice;1"].getService(Ci.nsIIdleService);
    this._idleService.addIdleObserver(this._idleServiceObserver, kBrowserViewPrefetchBeginIdleWait);
    this._idleServiceWait = kBrowserViewPrefetchBeginIdleWait;

    let browsers = document.getElementById("browsers");
    browsers.addEventListener("MozScrolledAreaChanged", this.handleMozScrolledAreaChanged, false);
  },

  uninit: function uninit() {
    this.setBrowser(null, null);
    this._idleService.removeIdleObserver(this._idleServiceObserver, this._idleServiceWait);
  },

  
  setAggressive: function setAggressive(aggro) {
    let wait = aggro ? kBrowserViewPrefetchBeginIdleWait : kBrowserViewPrefetchBeginIdleWaitLoading;
    this._idleService.removeIdleObserver(this._idleServiceObserver, this._idleServiceWait);
    this._idleService.addIdleObserver(this._idleServiceObserver, wait);
    this._idleServiceWait = wait;
  },

  getVisibleRect: function getVisibleRect() {
    return this._visibleRectFactory();
  },

  


  getViewportDimensions: function getViewportDimensions() {
    let bvs = this._browserViewportState;
    if (!bvs)
      throw "Cannot get viewport dimensions when no browser is set";

    return [bvs.viewportRect.right, bvs.viewportRect.bottom];
  },

  setZoomLevel: function setZoomLevel(zoomLevel) {
    let bvs = this._browserViewportState;
    if (!bvs)
      return;

    let newZoomLevel = BrowserView.Util.clampZoomLevel(zoomLevel);
    if (newZoomLevel != bvs.zoomLevel) {
      let browserW = this.viewportToBrowser(bvs.viewportRect.right);
      let browserH = this.viewportToBrowser(bvs.viewportRect.bottom);
      bvs.zoomLevel = newZoomLevel; 
      bvs.viewportRect.right  = this.browserToViewport(browserW);
      bvs.viewportRect.bottom = this.browserToViewport(browserH);
      this._viewportChanged(true, true);

      if (this._browser) {
        let event = document.createEvent("Events");
        event.initEvent("ZoomChanged", true, false);
        this._browser.dispatchEvent(event);
      }
    }
  },

  getZoomLevel: function getZoomLevel() {
    let bvs = this._browserViewportState;
    if (!bvs)
      return undefined;

    return bvs.zoomLevel;
  },

  beginOffscreenOperation: function beginOffscreenOperation(rect) {
    if (this._offscreenDepth == 0) {
      let vis = this.getVisibleRect();
      rect = rect || vis;
      let zoomRatio = vis.width / rect.width;
      let viewBuffer = Elements.viewBuffer;
      viewBuffer.width = vis.width;
      viewBuffer.height = vis.height;

      this._tileManager.renderRectToCanvas(rect, viewBuffer, zoomRatio, zoomRatio, false);
      viewBuffer.style.display = "block";
      window.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils).processUpdates();
      this.pauseRendering();
    }
    this._offscreenDepth++;
  },

  commitOffscreenOperation: function commitOffscreenOperation() {
    this._offscreenDepth--;
    if (this._offscreenDepth == 0) {
      this.resumeRendering();
      Elements.viewBuffer.style.display = "none";
    }
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
    if (this._renderMode == 1 && this._browser) {
      let event = document.createEvent("Events");
      event.initEvent("RenderStateChanged", true, false);
      event.isRendering = false;
      this._browser.dispatchEvent(event);
    }
  },

  



  resumeRendering: function resumeRendering(renderNow) {
    if (this._renderMode > 0)
      this._renderMode--;

    if (renderNow || this._renderMode == 0)
      this.renderNow();

    if (this._renderMode == 0 && this._browser) {
      let event = document.createEvent("Events");
      event.initEvent("RenderStateChanged", true, false);
      event.isRendering = true;
      this._browser.dispatchEvent(event);
    }
  },

  


  renderNow: function renderNow() {
    this._tileManager.criticalRectPaint();
  },

  isRendering: function isRendering() {
    return (this._renderMode == 0);
  },

  onAfterVisibleMove: function onAfterVisibleMove() {
    let vs = this._browserViewportState;
    let vr = this.getVisibleRect();

    vs.visibleX = vr.left;
    vs.visibleY = vr.top;

    let cr = BrowserView.Util.visibleRectToCriticalRect(vr, vs);

    this._tileManager.criticalMove(cr, this.isRendering());
  },

  


  setBrowser: function setBrowser(browser, browserViewportState) {
    if (browser && !browserViewportState) {
      throw "Cannot set non-null browser with null BrowserViewportState";
    }

    let oldBrowser = this._browser;
    let browserChanged = (oldBrowser !== browser);

    if (oldBrowser) {
      oldBrowser.removeEventListener("MozAfterPaint", this.handleMozAfterPaint, false);
      oldBrowser.removeEventListener("scroll", this.handlePageScroll, false);
      oldBrowser.setAttribute("type", "content");
      oldBrowser.docShell.isOffScreenBrowser = false;
    }

    this._browser = browser;
    this._contentWindow = (browser) ? browser.contentWindow : null;
    this._browserViewportState = browserViewportState;

    if (browser) {
      browser.setAttribute("type", "content-primary");

      this.beginBatchOperation();

      browser.addEventListener("MozAfterPaint", this.handleMozAfterPaint, false);
      browser.addEventListener("scroll", this.handlePageScroll, false);

      browser.docShell.isOffScreenBrowser = true;

      if (browserChanged)
        this._viewportChanged(true, true);

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

    tm.dirtyRects(rects, this.isRendering(), true);
  },

  
  handlePageScroll: function handlePageScroll(aEvent) {
    if (aEvent.target != this._browser.contentDocument || this._ignorePageScroll)
      return;
    
    Browser.scrollContentToBrowser();
  },

  _ignorePageScroll: false,
  ignorePageScroll: function ignorePageScroll(aIgnoreScroll) {
    this._ignorePageScroll = aIgnoreScroll;
  },

  handleMozScrolledAreaChanged: function handleMozScrolledAreaChanged(ev) {
    let tab = Browser.getTabForDocument(ev.originalTarget) ||
             (ev.target.contentDocument && Browser.getTabForDocument(ev.target.contentDocument));
    if (!tab)
      return;

    let browser = tab.browser;
    let bvs = tab.browserViewportState;
    let { x: scrollX, y: scrollY } = BrowserView.Util.getContentScrollOffset(browser);
    let x = ev.x + scrollX;
    let y = ev.y + scrollY;
    let w = ev.width;
    let h = ev.height;

    
    
    
    if (x < 0) w += x;
    if (y < 0) h += y;

    let vis = this.getVisibleRect();
    let viewport = bvs.viewportRect;
    let oldRight = viewport.right;
    let oldBottom = viewport.bottom;
    viewport.right  = bvs.zoomLevel * w;
    viewport.bottom = bvs.zoomLevel * h;

    if (browser == this._browser) {
      
      let sizeChanged = oldRight != viewport.right || oldBottom != viewport.bottom;
      this._viewportChanged(sizeChanged, false);
      this.updateDefaultZoom();
      if (vis.right > viewport.right || vis.bottom > viewport.bottom) {
        
        
        Browser.contentScrollboxScroller.scrollBy(0, 0);
        this.onAfterVisibleMove();
      }
    }
  },

  
  updateDefaultZoom: function updateDefaultZoom() {
    let bvs = this._browserViewportState;
    if (!bvs)
      return false;

    let isDefault = (bvs.zoomLevel == bvs.defaultZoomLevel);
    bvs.defaultZoomLevel = this.getZoomForPage();
    if (isDefault)
      this.setZoomLevel(bvs.defaultZoomLevel);
    return isDefault;
  },

  isDefaultZoom: function isDefaultZoom() {
    let bvs = this._browserViewportState;
    if (!bvs)
      return true;
    return bvs.zoomLevel == bvs.defaultZoomLevel;
  },

  zoomToPage: function zoomToPage() {
    let bvs = this._browserViewportState;
    if (bvs) {
      this.setZoomLevel(this.getZoomForPage());
      bvs.defaultZoomLevel = bvs.zoomLevel;  
    }
  },

  getZoomForPage: function getZoomForPage() {
    let browser = this._browser;
    if (!browser)
      return 0;

    let metaData = Util.contentIsHandheld(browser);
    if (metaData.reason == "handheld" || metaData.reason == "doctype")
      return 1;
    else if (metaData.reason == "viewport" && metaData.scale > 0)
      return metaData.scale;

    let bvs = this._browserViewportState;  
    let w = this.viewportToBrowser(bvs.viewportRect.right);
    let h = this.viewportToBrowser(bvs.viewportRect.bottom);
    return BrowserView.Util.pageZoomLevel(this.getVisibleRect(), w, h);
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  







  invalidateEntireView: function invalidateEntireView() {
    if (this._browserViewportState) {
      this._viewportChanged(false, true);
    }
  },

  









  renderToCanvas: function renderToCanvas(destCanvas, destWidth, destHeight, srcRect) {
    let bvs = this._browserViewportState;
    if (!bvs) {
      throw "Browser viewport state null in call to renderToCanvas (probably no browser set on BrowserView).";
    }

    if (!srcRect) {
      let vr = this.getVisibleRect();
      vr.x = bvs.viewportRect.left;
      vr.y = bvs.viewportRect.top;
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

  



  forceViewportChange: function forceViewportChange() {
    let bops = this._batchOps;
    if (bops.length > 0) {
      let opState = bops[bops.length - 1];
      this._applyViewportChanges(opState.viewportSizeChanged, opState.dirtyAll);
      opState.viewportSizeChanged = false;
      opState.dirtyAll = false;
    }
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

    this._applyViewportChanges(viewportSizeChanged, dirtyAll);
  },

  _applyViewportChanges: function _applyViewportChanges(viewportSizeChanged, dirtyAll) {
    let bvs = this._browserViewportState;
    if (bvs) {
      BrowserView.Util.resizeContainerToViewport(this._container, bvs.viewportRect);

      if (dirtyAll) {
        
        
        BrowserView.Util.getBrowserDOMWindowUtils(this._browser).clearMozAfterPaintEvents();
      }

      let vr = this.getVisibleRect();
      this._tileManager.viewportChangeHandler(bvs.viewportRect,
                                              BrowserView.Util.visibleRectToCriticalRect(vr, bvs),
                                              viewportSizeChanged,
                                              dirtyAll);

      let rects = vr.subtract(bvs.viewportRect);
      this._tileManager.clearRects(rects);
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













BrowserView.BrowserViewportState = function(viewportRect, visibleX, visibleY, zoomLevel) {
  this.init(viewportRect, visibleX, visibleY, zoomLevel);
};

BrowserView.BrowserViewportState.prototype = {

  init: function init(viewportRect, visibleX, visibleY, zoomLevel) {
    this.viewportRect = viewportRect;
    this.visibleX     = visibleX;
    this.visibleY     = visibleY;
    this.zoomLevel    = zoomLevel;
    this.defaultZoomLevel = 1;
  },

  toString: function toString() {
    let props = ["\tviewportRect=" + this.viewportRect.toString(),
                 "\tvisibleX="     + this.visibleX,
                 "\tvisibleY="     + this.visibleY,
                 "\tzoomLevel="    + this.zoomLevel];

    return "[BrowserViewportState] {\n" + props.join(",\n") + "\n}";
  }

};







BrowserView.IdleServiceObserver = function IdleServiceObserver(browserView) {
  this._browserView = browserView;
  this._idle = false;
  this._paused = false;
};

BrowserView.IdleServiceObserver.prototype = {
  
  pause: function pause() {
    this._paused = true;
    this._updateTileManager();
  },

  
  resume: function resume() {
    this._paused = false;
    this._updateTileManager();
  },

  
  observe: function observe(aSubject, aTopic, aUserIdleTime) {
    this._idle = (aTopic == "idle") ? true : false;
    this._updateTileManager();
  },

  _updateTileManager: function _updateTileManager() {
    let bv = this._browserView;
    bv._tileManager.setPrefetch(this._idle && !this._paused);
  }
};
