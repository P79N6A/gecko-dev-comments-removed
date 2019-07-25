










































function CanvasBrowser(canvas) {
  
  this._canvas = canvas;

  
  this._rgnPage = Cc["@mozilla.org/gfx/region;1"].createInstance(Ci.nsIScriptableRegion);

  
  this._pageBounds = new wsRect(0,0,0,0);

  
  this._visibleBounds = new wsRect(0,0,0,0);

  
  this._zoomLevel = 1.0;

  
  this._browser = null;

  
  this._screenX = 0;
  this._screenY = 0;

   
  this._lazyWidthChanged = false;
  this._lazyHeightChanged = false;

   
  this._pageLoading = true;

   
   
  this._isPanning = false;

   
   
  this._drawTimeout = 0;

   
   
   
  this._maxRight = 0;
  this._maxBottom = 0;

  
  this._needToPanToTop = false;
  
  this._eventHandler.cb = this;
}

CanvasBrowser.prototype = {
  
  get canvasDimensions() {
    if (!this._canvasRect) {
      let canvasRect = this._canvas.getBoundingClientRect();
      this._canvasRect = {
        width: canvasRect.width,
        height: canvasRect.height
      };
    }
    return [this._canvasRect.width, this._canvasRect.height];
  },

  get _effectiveCanvasDimensions() {
    return this.canvasDimensions.map(this._screenToPage, this);
  },

  get contentDOMWindowUtils() {
    if (!this._contentDOMWindowUtils) {
      this._contentDOMWindowUtils = this._browser.contentWindow
        .QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils);
    }
    return this._contentDOMWindowUtils;
  },

  _eventHandler: ({
    QueryInterface: function EH_QueryInterface(aIID) {
      if (aIID.equals(Ci.nsISupportsWeakReference) ||
          aIID.equals(Ci.nsIDOMEventListener) ||
          aIID.equals(Ci.nsISupports))
        return this;
    
      throw Cr.NS_ERROR_NO_INTERFACE;
    },

    handleEvent: function EH_handleEvent(aEvent) {
      switch (aEvent.type) {
        case "MozAfterPaint":
          this.cb._handleMozAfterPaint(aEvent);
          break;
        case "scroll":
          this.cb._handlePageScroll(aEvent);
          break;
      }
    }
  }),

  setCurrentBrowser: function setCurrentBrowser(browser, skipZoom) {
    let currentBrowser = this._browser;
    if (currentBrowser) {
      
      currentBrowser.mZoomLevel = this.zoomLevel;
      currentBrowser.mPanX = ws._viewingRect.x;
      currentBrowser.mPanY = ws._viewingRect.y;
      
      
      currentBrowser.removeEventListener("MozAfterPaint", this._eventHandler, false);
      currentBrowser.removeEventListener("scroll", this._eventHandler, false);
      currentBrowser.setAttribute("type", "content");
      currentBrowser.docShell.isOffScreenBrowser = false;
    }

    this._contentDOMWindowUtils = null;
    
    if (!browser)
      return;

    browser.setAttribute("type", "content-primary");
    if (!skipZoom)
      browser.docShell.isOffScreenBrowser = true;

    
    browser.addEventListener("MozAfterPaint", this._eventHandler, false);
    
    browser.addEventListener("scroll", this._eventHandler, false);

    this._browser = browser;

    
    if (!skipZoom) {
      this.zoomToPage();
    }

    if ("mZoomLevel" in browser) {
      
      ws.beginUpdateBatch();
      ws.panTo(browser.mPanX, browser.mPanY);
      this.zoomLevel = browser.mZoomLevel;
      ws.endUpdateBatch(true);

      
      delete browser.mZoomLevel;
      delete browser.mPanX;
      delete browser.mPanY;
    }
  },

  
  
  
  



































































































  startLoading: function startLoading() {
    this._maxRight = 0;
    this._maxBottom = 0;
    this._pageLoading = true;
    this._needToPanToTop = true;
  },

  endLoading: function endLoading() {
    dump("*** done loading\n");

    this._pageLoading = false;
    this._lazyWidthChanged = false;
    this._lazyHeightChanged = false;
    this.zoomToPage();
    
    
    this._criticalRegionPaint();

    if (this._drawTimeout) {
      clearTimeout(this._drawTimeout);
      this._drawTimeout = 0;
    }
  },

  
  
  startPanning: function startPanning() {
    this._criticalRegionPaint();

    
    this._isPanning = true;
  },

  endPanning: function endPanning() {
    this._criticalRegionPaint();

    this._isPanning = false;
  },

  
  
  viewportHandler: function viewportHandler(viewportBoundsRect,
                                            viewportInnerBoundsRect,
                                            viewportVisibleRect,
                                            boundsSizeChanged) {
    this._isPanning = false;

    this._viewportRect = viewportBoundsRect;
    this._canvasCoordsInViewport = [viewportInnerBoundsRect.x, viewportInnerBoundsRect.y];
    this._visibleRect = viewportVisibleRect;

    
    
    
    
    

    
    
    
    

    if (boundsSizeChanged) {

      
      
      this.contentDOMWindowUtils.clearMozAfterPaintEvents();
      this._redrawRects([this._viewportRect.clone()]);

    } else {
      this._criticalRegionPaint();
    }










































































































































































  _clampZoomLevel: function _clampZoomLevel(aZoomLevel) {
    const min = 0.2;
    const max = 4.0;

    return Math.min(Math.max(min, aZoomLevel), max);
  },

  _safeSetZoomLevel: function _safeSetZoomLevel(zl) {
    this._zoomLevel = this._clampZoomLevel(zl);
  },

  set zoomLevel(val) {
    this._safeSetZoomLevel(val);
    Browser.updateViewportSize();
  },

  get zoomLevel() {
    return this._zoomLevel;
  },

  zoom: function zoom(aDirection) {
    if (aDirection == 0)
      return;

    var zoomDelta = 0.05; 
    if (aDirection >= 0)
      zoomDelta *= -1;

    this.zoomLevel = this._zoomLevel + zoomDelta;
  },

  zoomToPage: function zoomToPage() {
    let needToPanToTop = this._needToPanToTop;
    
    
    if (needToPanToTop) {
      ws.beginUpdateBatch();
      this._needToPanToTop = false;
      ws.panTo(0, -BrowserUI.toolbarH);
    }
    
    
    let [contentW, ] = this._contentAreaDimensions;
    let [canvasW, ] = this.canvasDimensions;

    if (contentW > canvasW)
      this.zoomLevel = canvasW / contentW;

    if (needToPanToTop)
      ws.endUpdateBatch();
  },

  zoomToElement: function zoomToElement(aElement) {
    const margin = 15;

    let elRect = this._getPagePosition(aElement);
    let elWidth = elRect.width;
    let visibleViewportWidth = this._visibleRect.width;
    

    let zoomLevel = visibleViewportWidth / (elWidth + (2 * margin));
    ws.beginUpdateBatch();

    this.zoomLevel = zoomLevel;

    
    
    

    



    let xpadding = Math.max(margin, visibleViewportWidth - this._pageToScreen(elWidth));

    
    ws.panTo(Math.floor(Math.max(this._pageToScreen(elRect.x) - xpadding, 0)),
             Math.floor(Math.max(this._pageToScreen(elRect.y) - margin, 0)));

    ws.endUpdateBatch();
  },

  zoomFromElement: function zoomFromElement(aElement) {
    let elRect = this._getPagePosition(aElement);

    ws.beginUpdateBatch();

    
    
    this.zoomToPage();

    
    ws.panTo(0, Math.floor(Math.max(0, this._pageToScreen(elRect.y))));

    ws.endUpdateBatch();
  },

  



  elementFromPoint: function elementFromPoint(aX, aY) {
    let [x, y] = this._clientToContentCoords(aX, aY);
    let cwu = this.contentDOMWindowUtils;
    return cwu.elementFromPoint(x, y,
                                true,   
                                false); 
  },

  



  _getPagePosition: function _getPagePosition(aElement) {
    let [scrollX, scrollY] = this.contentScrollValues;
    let r = aElement.getBoundingClientRect();

    return new wsRect(r.left + scrollX,
                      r.top + scrollY,
                      r.width, r.height);
    
    
    
    
    
    
  },

  


  _clientToContentCoords: function _clientToContentCoords(aClientX, aClientY) {
    
    
    

    let [canvasX, canvasY] = this._canvasCoordsInViewport;
    let canvasRect = this._canvas.getBoundingClientRect();
    let clickOffsetX = this._screenToPage(aClientX - canvasRect.left + canvasX);
    let clickOffsetY = this._screenToPage(aClientY - canvasRect.top + canvasY);

    
    let [scrollX, scrollY] = this.contentScrollValues;
    return [clickOffsetX - scrollX,
            clickOffsetY - scrollY];
  },

  get contentScrollValues() {
    let cwu = this.contentDOMWindowUtils;
    let scrollX = {}, scrollY = {};
    cwu.getScrollXY(false, scrollX, scrollY);

    return [scrollX.value, scrollY.value];
  },

  get _effectiveContentAreaDimensions() {
    return this._contentAreaDimensions.map(this._pageToScreen, this);
  },

  
  get _contentAreaDimensions() {
    var cdoc = this._browser.contentDocument;

    if (cdoc instanceof SVGDocument) {
      let rect = cdoc.rootElement.getBoundingClientRect();
      return [rect.width, rect.height];
    }

    
    var body = cdoc.body || {};
    var html = cdoc.documentElement || {};

    var w = Math.max(body.scrollWidth || 0, html.scrollWidth);
    var h = Math.max(body.scrollHeight || 0, html.scrollHeight);

    if (w == 0 || h == 0)
      return [this._canvas.width, this._canvas.height];

    return [w, h];
  },

  _screenToPage: function _screenToPage(aValue) {
    return aValue / this._zoomLevel;
  },

  _pageToScreen: function _pageToScreen(aValue) {
    return aValue * this._zoomLevel;
  },

  
  ensureElementIsVisible: function ensureElementIsVisible(aElement) {
    let elRect = this._getPagePosition(aElement);
    let curRect = this._visibleBounds;
    let newx = curRect.x;
    let newy = curRect.y;

    if (elRect.x < curRect.x || elRect.width > curRect.width) {
      newx = elRect.x;
    } else if (elRect.x + elRect.width > curRect.x + curRect.width) {
      newx = elRect.x - curRect.width + elRect.width;
    }

    if (elRect.y < curRect.y || elRect.height > curRect.height) {
      newy = elRect.y;
    } else if (elRect.y + elRect.height > curRect.y + curRect.height) {
      newy = elRect.y - curRect.height + elRect.height;
    }

    ws.panTo(this._pageToScreen(newx), this._pageToScreen(newy));
  },

  
  panToElement: function panToElement(aElement) {
    var elRect = this._getPagePosition(aElement);

    ws.panTo(elRect.x, elRect.y);
  }
};
