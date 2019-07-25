









































function CanvasBrowser(canvas) {
  this._canvas = canvas;
  this._rgnPage = Cc["@mozilla.org/gfx/region;1"].createInstance(Ci.nsIScriptableRegion);
  this._pageBounds = new wsRect(0,0,0,0);
  this._visibleBounds = new wsRect(0,0,0,0);
}

CanvasBrowser.prototype = {
  _canvas: null,
  _zoomLevel: 1,
  _browser: null,
  _pageBounds: null,
  _screenX: 0,
  _screenY: 0,
  _visibleBounds: null,

  
  _lazyWidthChanged: false,
  _lazyHeightChanged: false,

  
  _pageLoading: true,

  
  _rgnPage: null,

  
  
  _isPanning: false,

  
  
  _drawTimeout: 0,

  
  
  
  _maxRight: 0,
  _maxBottom: 0,

  
  _needToPanToTop: false,

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
    this._paintHandler = function(ev) { self._handleMozAfterPaint(ev); };

    browser.addEventListener("MozAfterPaint", this._paintHandler, false);

    this._browser = browser;

    
    if (!skipZoom) {
      self.zoomToPage();
    }
  },

  
  
  
  
  flushRegion: function flushRegion(viewingBoundsOnly) {
    let rgn = this._rgnPage;

    let clearRegion = false;
    let drawls = [];
    let updateBounds = null;
    let pixelsInRegion = 0;
    let inRects = rgn.getRects();
    if (!inRects)
      return;

    for (let i = 0; i < inRects.length; i+=4) {
      let rect = new wsRect(inRects[i], inRects[i+1],
                            inRects[i+2], inRects[i+3]);
      if (viewingBoundsOnly) {
        
        rect = rect.intersect(this._visibleBounds);
        if (!rect)
          continue;
      } else {
        clearRegion = true;
      }
      drawls.push(rect);

      if (updateBounds == null) {
        updateBounds = rect.clone();
      } else {
        updateBounds = updateBounds.union(rect);
      }

      pixelsInRegion += rect.width * rect.height;
    }

    
    
    
    if (clearRegion)
      this.clearRegion();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (drawls.length > 1 &&
        pixelsInRegion > (updateBounds.width * updateBounds.height * 0.90))
    {
      drawls = [updateBounds];
    }
    this._drawRects(drawls, !clearRegion);
  },

  _drawRects: function _drawRects(drawls, subtractRects) {
    let oldX = 0;
    let oldY = 0;
    var ctx = this._canvas.getContext("2d");
    ctx.save();
    ctx.scale(this._zoomLevel, this._zoomLevel);

    
    for each (let rect in drawls) {
      if (subtractRects)
        this._rgnPage.subtractRect(rect.left, rect.top, rect.width, rect.height);

      
      rect.round(this._zoomLevel);
      let x = rect.x - this._pageBounds.x;
      let y = rect.y - this._pageBounds.y;

      
      
      
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
    this._maxRight = 0;
    this._maxBottom = 0;
    this._pageLoading = true;
    this._needToPanToTop = true;
  },

  endLoading: function() {
    this._pageLoading = false;
    this._lazyWidthChanged = false;
    this._lazyHeightChanged = false;
    this.zoomToPage();
    
    
    this.flushRegion();

    if (this._drawTimeout) {
      clearTimeout(this._drawTimeout);
      this._drawTimeout = 0;
    }
  },

  
  
  startPanning: function startPanning() {
    this.flushRegion();

    
    this._isPanning = true;
  },

  endPanning: function endPanning() {
    this.flushRegion();

    this._isPanning = false;
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
      this._redrawRects([pageBounds]);
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

    let rects = rgn.getRects();
    if (!rects)
      return;

    let rectsToDraw = [];
    for (let i = 0; i < rects.length; i+=4) {
      rectsToDraw.push(new wsRect(this._pageBounds.x + this._screenToPage(rects[i]),
                                  this._pageBounds.y + this._screenToPage(rects[i+1]),
                                  this._screenToPage(rects[i+2]),
                                  this._screenToPage(rects[i+3])));
    }
    this._redrawRects(rectsToDraw);
  },

  _handleMozAfterPaint: function(aEvent) {
    let [scrollX, scrollY] = this.contentScrollValues;
    let clientRects = aEvent.clientRects;

    let rects = [];
    
    for (let i = clientRects.length - 1; i >= 0; --i) {
      let e = clientRects.item(i);
      let r = new wsRect(e.left + scrollX,
                         e.top + scrollY,
                         e.width, e.height);
      rects.push(r);
    }

    this._redrawRects(rects);
  },

  _redrawRects: function(rects) {
    
    if (!this._pageLoading && rects.length == 1
        && this._visibleBounds.contains(rects[0])) {
      this._drawRects(rects, false);
      return;
    }

    
    
    let realRectCount = 0;

    let zeroPageBounds = this._pageBounds.clone();
    zeroPageBounds.left = Math.max(zeroPageBounds.left, 0);
    zeroPageBounds.top = Math.max(zeroPageBounds.top, 0);

    
    
    for each (var rect in rects) {
      if (this._pageLoading)  {
        
        
        if (rect.right > this._maxRight) {
          this._lazyWidthChanged = true;
          this._maxRight = rect.right;
        }
        if (rect.bottom > this._maxBottom) {
          this._lazyHeightChanged = true;
          this._maxBottom = rect.bottom;
        }
      }

      rect = rect.intersect(zeroPageBounds);

      if (!rect)
        continue;

      rect.round(1);

      this._rgnPage.unionRect(rect.x, rect.y, rect.width, rect.height);

      realRectCount++;
    }

    
    if (realRectCount == 0)
      return;

    
    
    function resizeAndPaint(self) {
      if (self._lazyWidthChanged) {
        
        
        
        
        
        let contentW = self._maxRight;
        let [canvasW, ] = self.canvasDimensions;

        if (contentW > canvasW)
          this.zoomLevel = canvasW / contentW;

        self._lazyWidthChanged = false;
      } else if (self._lazyHeightChanged) {

        
        
        Browser.updateViewportSize();
        self._lazyHeightChanged = false;
      }

      
      if (!self._isPanning)
        self.flushRegion(true);

      if (self._pageLoading) {
        
        self._drawTimeout = setTimeout(resizeAndPaint, 2000, self);
      } else {
        self._drawTimeout = 0;
      }
    }

    let flushNow = !this._pageLoading;

    
    
    if (this._pageLoading && !this._drawTimeout) {
      
      flushNow = true;
      this._lazyWidthChanged = true;
      this._drawTimeout = setTimeout(resizeAndPaint, 2000, this);
    }

    if (flushNow) {
      resizeAndPaint(this);
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
    let needToPanToTop = this._needToPanToTop;
    
    
    if (needToPanToTop) {
      ws.beginUpdateBatch();
      this._needToPanToTop = false;
      ws.panTo(0, -60);
    }
    
    
    let [contentW, ] = this._contentAreaDimensions;
    let [canvasW, ] = this.canvasDimensions;

    if (contentW > canvasW)
      this.zoomLevel = canvasW / contentW;

    if (needToPanToTop)
      ws.endUpdateBatch();
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
    let [scrollX, scrollY] = this.contentScrollValues;
    let r = aElement.getBoundingClientRect();

    return {
      width: r.width,
      height: r.height,
      x: r.left + scrollX,
      y: r.top + scrollY
    };
  },

  


  _clientToContentCoords: function(aClientX, aClientY) {
    
    
    

    let canvasRect = this._canvas.getBoundingClientRect();
    let clickOffsetX = this._screenToPage(aClientX - canvasRect.left) + this._pageBounds.x;
    let clickOffsetY = this._screenToPage(aClientY - canvasRect.top) + this._pageBounds.y;

    
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

  
  panToElement: function(aElement) {
    var elRect = this._getPagePosition(aElement);

    this.panTo(elRect.x, elRect.y);
  },

  panTo: function(x, y) {
    ws.panTo(x, y);
  }
};
