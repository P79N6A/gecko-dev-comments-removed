







































let Ci = Components.interfaces;

const kBrowserViewZoomLevelMin = 0.2;
const kBrowserViewZoomLevelMax = 4.0;
const kBrowserViewZoomLevelPrecision = 10000;

























































































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
    return new BrowserView.BrowserViewportState(new wsRect(0, 0, 1, 1), 0, 0, 1);
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
    let w = Math.max(body.scrollWidth || 0, html.scrollWidth);
    let h = Math.max(body.scrollHeight || 0, html.scrollHeight);

    return [w, h];
  },

  getContentScrollValues: function getContentScrollValues(browser) {
    let cwu = BrowserView.Util.getBrowserDOMWindowUtils(browser);
    let scrollX = {};
    let scrollY = {};
    cwu.getScrollXY(false, scrollX, scrollY);

    return [scrollX.value, scrollY.value];
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
    this._tileManager = new TileManager(this._appendTile, this._removeTile, this);
    this._visibleRectFactory = visibleRectFactory;
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

    let currentBrowser = this._browser;

    let browserChanged = (currentBrowser !== browser);

    if (currentBrowser) {
      currentBrowser.removeEventListener("MozAfterPaint", this.handleMozAfterPaint, false);
      currentBrowser.removeEventListener("scroll", this.handlePageScroll, false);

      
      
      currentBrowser.removeEventListener("FakeMozAfterSizeChange", this.handleMozAfterSizeChange, false);
      

      currentBrowser.setAttribute("type", "content");
      currentBrowser.docShell.isOffScreenBrowser = false;
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

    let [scrollX, scrollY] = BrowserView.Util.getContentScrollValues(browser);
    let clientRects = ev.clientRects;

    let rects = [];
    
    for (let i = clientRects.length - 1; i >= 0; --i) {
      let e = clientRects.item(i);
      let r = new wsRect(e.left + scrollX,
                         e.top + scrollY,
                         e.width, e.height);

      this.browserToViewportRect(r);
      r.round();

      if (r.right < 0 || r.bottom < 0)
        continue;

      try {
        r.restrictTo(vs.viewportRect);
        rects.push(r);
      } catch(ex) { dump("fail\n"); }
    }

    tm.dirtyRects(rects, this.isRendering());
  },

  handlePageScroll: function handlePageScroll(aEvent) {
    if (aEvent.target != this._browser.contentDocument)
      return;

    let [scrollX, scrollY] = BrowserView.Util.getContentScrollValues(this._browser);
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

    let [w, h] = BrowserView.Util.getBrowserDimensions(browser);
    this.setZoomLevel(BrowserView.Util.pageZoomLevel(this.getVisibleRect(), w, h));
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

