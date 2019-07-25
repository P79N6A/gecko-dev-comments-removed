









































function CanvasBrowser(canvas) {
  this._canvas = canvas;
}

CanvasBrowser.prototype = {
  _canvas: null,
  _zoomLevel: 1,
  _browser: null,
  _pageBounds: new wsRect(0,0,0,0),
  _screenX: 0,
  _screenY: 0,
  _visibleBounds:new wsRect(0,0,0,0),
  
  _maybeZoomToPage: false,
  
  _pageLoading: true,
  
  _rgnPage: Cc["@mozilla.org/gfx/region;1"].createInstance(Ci.nsIScriptableRegion),
  
  
  _isPanning: false,
  
  get canvasDimensions() {
    if (!this._canvasRect) {
      let canvasRect = this._canvas.getBoundingClientRect();
      this._canvasRect = {
        width: canvasRect.width,
        height: canvasRect.height
      }
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
  
  setCurrentBrowser: function(browser, skipZoom) {
    let currentBrowser = this._browser;
    if (currentBrowser) {
      
      currentBrowser.removeEventListener("MozAfterPaint", this._paintHandler, false);
      currentBrowser.setAttribute("type", "content");
      currentBrowser.docShell.isOffScreenBrowser = false;
    }
    this._contentDOMWindowUtils = null;

    browser.setAttribute("type", "content-primary");
    if (!skipZoom)
      browser.docShell.isOffScreenBrowser = true;

    
    var self = this;
    this._paintHandler = function(ev) { self._handleMozAfterPaint(ev); }

    browser.addEventListener("MozAfterPaint", this._paintHandler, false);

    this._browser = browser;

    
    if (!skipZoom) {
      self.zoomToPage();
    }
  },

  


  addToRegion: function addToRegion(rect) {
    this._rgnPage.unionRect(rect.x, rect.y, rect.width, rect.height);

    function resizeAndPaint(self) {
      if (self._maybeZoomToPage) {
        self.zoomToPage();
      }
      
      if (!self._isPanning)
        self.flushRegion(true);
    }
    
    let flushNow = !this._pageLoading && rect.intersects(this._visibleBounds);
    
    
    if (this._pageLoading && !this._drawInterval) {
      
      flushNow = true;
      this._maybeZoomToPage = true;
      this._drawInterval = setInterval(resizeAndPaint, 2000, this);
    }

    if (flushNow) {
      resizeAndPaint(this);
    }
  },

  
  
  
  
  flushRegion: function flushRegion(viewingBoundsOnly) {
    let rgn = this._rgnPage;

    let clearRegion = false;
    let drawls = [];
    let outX = {}; let outY = {}; let outW = {}; let outH = {};
    let numRects = rgn.numRects;
    for (let i=0;i<numRects;i++) {
      rgn.getRect(i, outX, outY, outW, outH);
      let rect = new wsRect(outX.value, outY.value,
                            outW.value, outH.value);
      if (viewingBoundsOnly) {
        
        rect = rect.intersect(this._visibleBounds)
        if (!rect)
          continue;
      } else {
        clearRegion = true;
      }
      drawls.push(rect)
    }

    if (clearRegion)
      this.clearRegion();

    let oldX = 0;
    let oldY = 0;
    var ctx = this._canvas.getContext("2d");
    ctx.save();
    ctx.scale(this._zoomLevel, this._zoomLevel);

    
    for each(let rect in drawls) {
      
      if (!clearRegion)
        rgn.subtractRect(rect.left, rect.top,
                         rect.width, rect.height);
      
      rect.round(this._zoomLevel);
      let x = rect.x - this._pageBounds.x
      let y = rect.y - this._pageBounds.y
      
      ctx.translate(x - oldX, y - oldY);
      oldX = x;
      oldY = y;
      ctx.drawWindow(this._browser.contentWindow,
                     rect.x, rect.y,
                     rect.width, rect.height,
                     "white",
                     (ctx.DRAWWINDOW_DO_NOT_FLUSH | ctx.DRAWWINDOW_DRAW_CARET));
    }
    
    ctx.restore();
  },

  clearRegion: function clearRegion() {
    
    
    this._rgnPage.setToRect(0,0,0,0);
  },

  startLoading: function startLoading() {
    
    
    
    this.clearRegion();
    var ctx = this._canvas.getContext("2d");
    ctx.fillStyle = "rgb(255,255,255)";
    ctx.fillRect(0, 0, this._canvas.width, this._canvas.height);
    this._pageLoading = true;
  },

  endLoading: function() {
    this._pageLoading = false;
    this._maybeZoomToPage = false;
    this.zoomToPage();
    
    this.flushRegion();
    
    if (this._drawInterval) {
      clearInterval(this._drawInterval);
      this._drawInterval = null;
    }
  },

  
  
  prepareForPanning: function prepareForPanning() {
    this.flushRegion();
    
    
    this._isPanning = true;
  },

  viewportHandler: function viewportHandler(bounds, boundsSizeChanged) {
    this._isPanning = false;
    let pageBounds = bounds.clone();
    let visibleBounds = ws.viewportVisibleRect;

    

    
    pageBounds.top = this._screenToPage(pageBounds.top);
    pageBounds.left = this._screenToPage(pageBounds.left);
    pageBounds.bottom = Math.ceil(this._screenToPage(pageBounds.bottom));
    pageBounds.right = Math.ceil(this._screenToPage(pageBounds.right));
        
    visibleBounds.top = Math.max(0, this._screenToPage(visibleBounds.top));
    visibleBounds.left = Math.max(0, this._screenToPage(visibleBounds.left));
    visibleBounds.bottom = Math.ceil(this._screenToPage(visibleBounds.bottom));
    visibleBounds.right = Math.ceil(this._screenToPage(visibleBounds.right));

    
    
    
    if (boundsSizeChanged) {
      this.clearRegion();
      
      
      this.contentDOMWindowUtils.clearMozAfterPaintEvents();
    } else
      this.flushRegion();
    
    this._visibleBounds = visibleBounds;
    this._pageBounds = pageBounds;

    let dx = this._screenX - bounds.x;
    let dy = this._screenY - bounds.y;
    this._screenX = bounds.x;
    this._screenY = bounds.y;

    if (boundsSizeChanged) {
      this._redrawRect(pageBounds);
      return;
    }

    
    
    if (!dx && !dy) {
      
      return;
    }

    
    var ctx = this._canvas.getContext("2d");
    let cWidth = this._canvas.width;
    let cHeight = this._canvas.height;
    
    ctx.drawImage(this._canvas,
                  0, 0, cWidth, cHeight,
                  dx, dy, cWidth, cHeight);

    

    
    let rgn = this._rgnScratch;
    if (!rgn) {
      rgn = Cc["@mozilla.org/gfx/region;1"].createInstance(Ci.nsIScriptableRegion);
      this._rgnScratch = rgn;
    }
    rgn.setToRect(0, 0, cWidth, cHeight);
    rgn.subtractRect(dx, dy, cWidth, cHeight);
    
    let outX = {}; let outY = {}; let outW = {}; let outH = {};
    let rectCount = rgn.numRects
    for (let i = 0;i < rectCount;i++) {
      rgn.getRect(i, outX, outY, outW, outH);
      if (outW.value > 0 && outH.value > 0) {
        this._redrawRect(new wsRect(Math.floor(this._pageBounds.x +this._screenToPage(outX.value)),
                                    Math.floor(this._pageBounds.y +this._screenToPage(outY.value)),
                                    Math.ceil(this._screenToPage(outW.value)),
                                    Math.ceil(this._screenToPage(outH.value))));
      }
    }
  },

  _handleMozAfterPaint: function(aEvent) {
    let cwin = this._browser.contentWindow;

    for (let i = 0; i < aEvent.clientRects.length; i++) {
      let e = aEvent.clientRects.item(i);
      let r = new wsRect(e.left + cwin.scrollX,
                         e.top + cwin.scrollY,
                         e.width, e.height);
      this._redrawRect(r);
    }
  },

  _redrawRect: function(rect) {
    
    
    if (this._pageLoading)  {
      if (rect.bottom > 0 && rect.right > this._visibleBounds.right)
        this._maybeZoomToPage = true;
    } 
    
    let r2 = this._pageBounds.clone();
    r2.left = Math.max(r2.left, 0);
    r2.top = Math.max(r2.top, 0);
    let dest = rect.intersect(r2);
    
    if (dest) {
      dest.round(1);
      this.addToRegion(dest);
    }
  },

  _clampZoomLevel: function(aZoomLevel) {
    const min = 0.2;
    const max = 2.0;

    return Math.min(Math.max(min, aZoomLevel), max);
  },

  set zoomLevel(val) {
    this._zoomLevel = this._clampZoomLevel(val);
    Browser.updateViewportSize();
  },

  get zoomLevel() {
    return this._zoomLevel;
  },

  zoom: function(aDirection) {
    if (aDirection == 0)
      return;

    var zoomDelta = 0.05; 
    if (aDirection >= 0)
      zoomDelta *= -1;

    this.zoomLevel = this._zoomLevel + zoomDelta;
  },

  zoomToPage: function() {
    
    
    
    let [contentW, ] = this._contentAreaDimensions;
    let [canvasW, ] = this.canvasDimensions;

    if (contentW > canvasW)
      this.zoomLevel = canvasW / contentW;

    if (this._clippedPageDrawing)
      this._maybeZoomToPage = false;
  },

  zoomToElement: function(aElement) {
    const margin = 15;

    let elRect = this._getPagePosition(aElement);
    let elWidth = elRect.width;
    let visibleViewportWidth = this._pageToScreen(this._visibleBounds.width);
    

    let zoomLevel = visibleViewportWidth / (elWidth + (2 * margin));
    ws.beginUpdateBatch();

    this.zoomLevel = zoomLevel;
    
    



    let xpadding = Math.max(margin, visibleViewportWidth - this._pageToScreen(elWidth));

    
    ws.panTo(Math.floor(Math.max(this._pageToScreen(elRect.x) - xpadding, 0)),
             Math.floor(Math.max(this._pageToScreen(elRect.y) - margin, 0)));

    ws.endUpdateBatch();
  },

  zoomFromElement: function(aElement) {
    let elRect = this._getPagePosition(aElement);

    ws.beginUpdateBatch();

    
    
    this.zoomToPage();

    
    ws.panTo(0, Math.floor(Math.max(0, this._pageToScreen(elRect.y))));

    ws.endUpdateBatch();
  },

  



  elementFromPoint: function(aX, aY) {
    let [x, y] = this._clientToContentCoords(aX, aY);
    let cwu = this.contentDOMWindowUtils;
    return cwu.elementFromPoint(x, y,
                                true,   
                                false); 
  },

  



  _getPagePosition: function(aElement) {
    let r = aElement.getBoundingClientRect();
    let cwin = this._browser.contentWindow;
    let retVal = {
      width: r.width,
      height: r.height,
      x: r.left + cwin.scrollX,
      y: r.top + cwin.scrollY
    };

    return retVal;
  },

  


  _clientToContentCoords: function(aClientX, aClientY) {
    
    
    

    let canvasRect = this._canvas.getBoundingClientRect();
    let clickOffsetX = this._screenToPage(aClientX - canvasRect.left) + this._pageBounds.x;
    let clickOffsetY = this._screenToPage(aClientY - canvasRect.top) + this._pageBounds.y;

    
    let cwin = this._browser.contentWindow;
    return [clickOffsetX - cwin.scrollX,
            clickOffsetY - cwin.scrollY];
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

    var w = Math.max(body.scrollWidth, html.scrollWidth);
    var h = Math.max(body.scrollHeight, html.scrollHeight);

    if (isNaN(w) || isNaN(h) || w == 0 || h == 0)
      return [this._canvas.width, this._canvas.height];

    return [w, h];
  },

  _screenToPage: function(aValue) {
    return aValue / this._zoomLevel;
  },

  _pageToScreen: function(aValue) {
    return aValue * this._zoomLevel;
  },

  
  ensureElementIsVisible: function(aElement) {
    let elRect = this._getPagePosition(aElement);
    let curRect = this._visibleBounds
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

  
  panToElement: function(aElement) {
    var elRect = this._getPagePosition(aElement);

    this.panTo(elRect.x, elRect.y);
  },

  panTo: function(x, y) {
    ws.panTo(x, y);
  }
};
