






































let Ci = Components.interfaces;


let noop = function() {};
let endl = '\n';


function BrowserView(container, visibleRect) {
  bindAll(this);
  this.init(container, visibleRect);
}
























































































BrowserView.prototype = (
function() {

  
  
  

  const kZoomLevelMin = 0.2;
  const kZoomLevelMax = 4.0;
  const kZoomLevelPrecision = 10000;

  function visibleRectToCriticalRect(visibleRect, browserViewportState) {
    return visibleRect.intersect(browserViewportState.viewportRect);
  }

  function clampZoomLevel(zl) {
    let bounded = Math.min(Math.max(kZoomLevelMin, zl), kZoomLevelMax);
    return Math.round(bounded * kZoomLevelPrecision) / kZoomLevelPrecision;
  }

  function pageZoomLevel(visibleRect, browserW, browserH) {
    return clampZoomLevel(visibleRect.width / browserW);
  }

  function seenBrowser(browser) {
    return !!(browser.__BrowserView__vps);
  }

  function initBrowserState(browser, visibleRect) {
    let [browserW, browserH] = getBrowserDimensions(browser);

    let zoomLevel = pageZoomLevel(visibleRect, browserW, browserH);
    let viewportRect = (new wsRect(0, 0, browserW, browserH)).scale(zoomLevel, zoomLevel);

    dump('--- initing browser to ---' + endl);
    browser.__BrowserView__vps = new BrowserView.BrowserViewportState(viewportRect,
                                                                      visibleRect.x,
                                                                      visibleRect.y,
                                                                      zoomLevel);
    dump(browser.__BrowserView__vps.toString() + endl);
    dump('--------------------------' + endl);
  }

  function getViewportStateFromBrowser(browser) {
    return browser.__BrowserView__vps;
  }

  function getBrowserDimensions(browser) {
    let cdoc = browser.contentDocument;

    
    let body = cdoc.body || {};
    let html = cdoc.documentElement || {};
    let w = Math.max(body.scrollWidth || 0, html.scrollWidth);
    let h = Math.max(body.scrollHeight || 0, html.scrollHeight);

    return [w, h];
  }

  function getContentScrollValues(browser) {
    let cwu = getBrowserDOMWindowUtils(browser);
    let scrollX = {};
    let scrollY = {};
    cwu.getScrollXY(false, scrollX, scrollY);

    return [scrollX.value, scrollY.value];
  }

  function getBrowserDOMWindowUtils(browser) {
    return browser.contentWindow
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils);
  }

  function getNewBatchOperationState() {
    return {
      viewportSizeChanged: false,
      dirtyAll: false
    };
  }

  function clampViewportWH(width, height, visibleRect) {
    let minW = visibleRect.width;
    let minH = visibleRect.height;
    return [Math.max(width, minW), Math.max(height, minH)];
  }

  function initContainer(container, visibleRect) {
    container.style.width    = visibleRect.width  + 'px';
    container.style.height   = visibleRect.height + 'px';
    container.style.overflow = '-moz-hidden-unscrollable';
  }

  function resizeContainerToViewport(container, viewportRect) {
    container.style.width  = viewportRect.width  + 'px';
    container.style.height = viewportRect.height + 'px';
  }

  
  function simulateMozAfterSizeChange(browser, width, height) {
    let ev = document.createElement("MouseEvents");
    ev.initEvent("FakeMozAfterSizeChange", false, false, window, 0, width, height);
    browser.dispatchEvent(ev);
  }
  

  


  
  return {

    
    
    

    init: function init(container, visibleRect) {
      this._batchOps = [];
      this._container = container;
      this._browserViewportState = null;
      this._renderMode = 0;
      this._tileManager = new TileManager(this._appendTile, this._removeTile, this);
      this.setVisibleRect(visibleRect);

      
      
      this._resizeHack = {
        maxSeenW: 0,
        maxSeenH: 0
      };
      
    },

    setVisibleRect: function setVisibleRect(r) {
      let bvs = this._browserViewportState;
      let vr  = this._visibleRect;

      if (!vr)
        this._visibleRect = vr = r.clone();
      else
        vr.copyFrom(r);

      if (bvs) {
        bvs.visibleX = vr.left;
        bvs.visibleY = vr.top;

        
        
      } else
        this._viewportChanged(false, false);
    },

    getVisibleRect: function getVisibleRect() {
      return this._visibleRect.clone();
    },

    getVisibleRectX: function getVisibleRectX() { return this._visibleRect.x; },
    getVisibleRectY: function getVisibleRectY() { return this._visibleRect.y; },
    getVisibleRectWidth: function getVisibleRectWidth() { return this._visibleRect.width; },
    getVisibleRectHeight: function getVisibleRectHeight() { return this._visibleRect.height; },

    setViewportDimensions: function setViewportDimensions(width, height, causedByZoom) {
      let bvs = this._browserViewportState;
      let vis = this._visibleRect;

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

      let newZL = clampZoomLevel(zl);

      if (newZL != bvs.zoomLevel) {
        let browserW = this.viewportToBrowser(bvs.viewportRect.right);
        let browserH = this.viewportToBrowser(bvs.viewportRect.bottom);
        bvs.zoomLevel = newZL; 
        this.setViewportDimensions(this.browserToViewport(browserW),
                                   this.browserToViewport(browserH));
      }
    },

    getZoomLevel: function getZoomLevel() {
      let bvs = this._browserViewportState;
      if (!bvs)
        return undefined;

      return bvs.zoomLevel;
    },

    beginBatchOperation: function beginBatchOperation() {
      this._batchOps.push(getNewBatchOperationState());
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

    moveVisibleBy: function moveVisibleBy(dx, dy) {
      let vr = this._visibleRect;
      let vs = this._browserViewportState;

      this.onBeforeVisibleMove(dx, dy);
      this.onAfterVisibleMove(dx, dy);
    },

    moveVisibleTo: function moveVisibleTo(x, y) {
      let visibleRect = this._visibleRect;
      let dx = x - visibleRect.x;
      let dy = y - visibleRect.y;
      this.moveBy(dx, dy);
    },

    



    pauseRendering: function pauseRendering() {
      this._renderMode++;
    },

    



    resumeRendering: function resumeRendering(renderNow) {
      if (this._renderMode > 0)
        this._renderMode--;

      if (renderNow || this._renderMode == 0)
        this._tileManager.criticalRectPaint();
    },

    isRendering: function isRendering() {
      return (this._renderMode == 0);
    },

    



    onBeforeVisibleMove: function onBeforeVisibleMove(dx, dy) {
      let vs = this._browserViewportState;
      let vr = this._visibleRect;

      let destCR = visibleRectToCriticalRect(vr.clone().translate(dx, dy), vs);

      this._tileManager.beginCriticalMove(destCR);
    },

    



    onAfterVisibleMove: function onAfterVisibleMove(dx, dy) {
      let vs = this._browserViewportState;
      let vr = this._visibleRect;

      vr.translate(dx, dy);
      vs.visibleX = vr.left;
      vs.visibleY = vr.top;

      let cr = visibleRectToCriticalRect(vr, vs);

      this._tileManager.endCriticalMove(cr, this.isRendering());
    },

    setBrowser: function setBrowser(browser, skipZoom) {
      let currentBrowser = this._browser;

      let browserChanged = (currentBrowser !== browser);

      if (currentBrowser) {
        currentBrowser.removeEventListener("MozAfterPaint", this.handleMozAfterPaint, false);

        
        
        currentBrowser.removeEventListener("FakeMozAfterSizeChange", this.handleMozAfterSizeChange, false);
        

        this.discardAllBatchOperations();

        currentBrowser.setAttribute("type", "content");
        currentBrowser.docShell.isOffScreenBrowser = false;
      }

      this._restoreBrowser(browser);

      browser.setAttribute("type", "content-primary");

      this.beginBatchOperation();

      browser.addEventListener("MozAfterPaint", this.handleMozAfterPaint, false);

      
      
      browser.addEventListener("FakeMozAfterSizeChange", this.handleMozAfterSizeChange, false);
      

      if (!skipZoom) {
        browser.docShell.isOffScreenBrowser = true;
        this.zoomToPage();
      }

      this._viewportChanged(browserChanged, browserChanged);

      this.commitBatchOperation();
    },

    handleMozAfterPaint: function handleMozAfterPaint(ev) {
      let browser = this._browser;
      let tm = this._tileManager;
      let vs = this._browserViewportState;

      let [scrollX, scrollY] = getContentScrollValues(browser);
      let clientRects = ev.clientRects;

      
      
      let hack = this._resizeHack;
      let hackSizeChanged = false;
      

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

        
        
        
        
        if (r.right > hack.maxW) {
          hack.maxW = rect.right;
          hackSizeChanged = true;
        }
        if (r.bottom > hack.maxH) {
          hack.maxH = rect.bottom;
          hackSizeChanged = true;
        }
        

        r.restrictTo(vs.viewportRect);
        rects.push(r);
      }

      
      
      if (hackSizeChanged)
        simulateMozAfterSizeChange(browser, hack.maxW, hack.maxH);
      

      tm.dirtyRects(rects, this.isRendering());
    },

    handleMozAfterSizeChange: function handleMozAfterPaint(ev) {
      
      
      
      
      let w = ev.screenX;
      let h = ev.screenY;
      

      this.setViewportDimensions(w, h);
    },

    zoomToPage: function zoomToPage() {
      let browser = this._browser;

      if (!browser)
        return;

      let [w, h] = getBrowserDimensions(browser);
      this.setZoomLevel(pageZoomLevel(this._visibleRect, w, h));
    },

    zoom: function zoom(aDirection) {
      if (aDirection == 0)
        return;

      var zoomDelta = 0.05; 
      if (aDirection >= 0)
        zoomDelta *= -1;

      this.zoomLevel = this._zoomLevel + zoomDelta;
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


    
    
    

    _restoreBrowser: function _restoreBrowser(browser) {
      let vr = this._visibleRect;

      if (!seenBrowser(browser))
        initBrowserState(browser, vr);

      let bvs = getViewportStateFromBrowser(browser);

      this._contentWindow = browser.contentWindow;
      this._browser = browser;
      this._browserViewportState = bvs;
      vr.left = bvs.visibleX;
      vr.top  = bvs.visibleY;
      this._tileManager.setBrowser(browser);
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
      let vis = this._visibleRect;

      
      
      
      
      






      

      if (bvs) {
        resizeContainerToViewport(this._container, bvs.viewportRect);

        this._tileManager.viewportChangeHandler(bvs.viewportRect,
                                                visibleRectToCriticalRect(vis, bvs),
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

}
)();






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

