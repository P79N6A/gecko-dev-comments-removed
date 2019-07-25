









































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
  _drawQ: [],
  
  _maybeZoomToPage: false,
  
  _clippedPageDrawing: true,

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

  setCurrentBrowser: function(browser, skipZoom) {
    let currentBrowser = this._browser;
    if (currentBrowser) {
      
      currentBrowser.removeEventListener("MozAfterPaint", this._paintHandler, false);
      currentBrowser.setAttribute("type", "content");
      currentBrowser.docShell.isOffScreenBrowser = false;
    }

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

  
  addToDrawQ: function addToDrawQ(rect) {
    let q = this._drawQ;
    function resizeAndPaint(self) {
      if (self._maybeZoomToPage) {
        self.zoomToPage();
      }
      
      self.flushDrawQ(self._clippedPageDrawing);
    }
    for(let i = q.length - 1;i>=0;i--) {
      let old = q[i];
      if (!old)
        continue;
      
      if (old.contains(rect)) {
        
        return;
      } else if(rect.contains(old)) {
        
        q[i] = null;
      }
    }

    

    let flushNow = !this._clippedPageDrawing;

    if (this._clippedPageDrawing) {
      if (!this._drawInterval) {
        
        flushNow = true;
        this._maybeZoomToPage = true;
        this._drawInterval = setInterval(resizeAndPaint, 2000, this);
      }
    }

    q.push(rect);

    if (flushNow) {
      resizeAndPaint(this);
    }
  },

  
  
  
  flushDrawQ: function flushDrawQ(justOne) {
    var ctx = this._canvas.getContext("2d");
    ctx.save();
    ctx.scale(this._zoomLevel, this._zoomLevel);
    while (this._drawQ.length) {
      let dest = this._drawQ.pop();
      if (!dest)
        continue;
      ctx.translate(dest.x - this._pageBounds.x, dest.y - this._pageBounds.y);
      ctx.drawWindow(this._browser.contentWindow,
                     dest.x, dest.y,
                     dest.width, dest.height,
                     "white",
                     ctx.DRAWWINDOW_DO_NOT_FLUSH | ctx.DRAWWINDOW_DRAW_CARET);
      if (justOne)
        break;
    }
    ctx.restore();
  },

  clearDrawQ: function clearDrawQ() {
    this._drawQ = [];
  },

  startLoading: function() {
    
    
    
    this.clearDrawQ();
    var ctx = this._canvas.getContext("2d");
    ctx.fillStyle = "rgb(255,255,255)";
    ctx.fillRect(0, 0, this._canvas.width, this._canvas.height);
    this._clippedPageDrawing = true;
  },

  endLoading: function() {
    this._clippedPageDrawing = false;
    this._maybeZoomToPage = false;
    this.zoomToPage();
    this.ensureFullCanvasIsDrawn();

    if (this._drawInterval) {
      clearInterval(this._drawInterval);
      this._drawInterval = null;
    }
  },

  
  ensureFullCanvasIsDrawn: function ensureFullCanvasIsDrawn() {
    if (this._partiallyDrawn) {
      let v = this._visibleBounds
      let r_above = new wsRect(this._pageBounds.x, this._pageBounds.y,
                               this._pageBounds.width, v.y - this._pageBounds.y)
      let r_left = new wsRect(this._pageBounds.x, v.y,
                              v.x - this._pageBounds.x,
                              v.height)
      let r_right = new wsRect(v.x + v.width, v.y,
                               this._pageBounds.width - v.x - v.width,
                               v.height)
      let r_below = new wsRect(this._pageBounds.x, v.y+v.height,
                               this._pageBounds.width,
                               this._pageBounds.height - v.y - v.height)
      this._redrawRect(r_above);
      this._redrawRect(r_left);
      this._redrawRect(r_right);
      this._redrawRect(r_below);
      this._partiallyDrawn = false;
    }
    
    this.flushDrawQ()
  },


  
  
  prepareForPanning: function prepareForPanning() {
    if (!this._clippedPageDrawing) 
      return;

    
    this._maybeZoomToPage = true;

    
    this._clippedPageDrawing = false;
    this.ensureFullCanvasIsDrawn();
  },

  viewportHandler: function(bounds, boundsSizeChanged) {
    let pageBounds = bounds.clone();
    let visibleBounds = ws.viewportVisibleRect;
    pageBounds.top = Math.floor(this._screenToPage(bounds.top));
    pageBounds.left = Math.floor(this._screenToPage(bounds.left));
    pageBounds.bottom = Math.ceil(this._screenToPage(bounds.bottom));
    pageBounds.right = Math.ceil(this._screenToPage(bounds.right));

    
    visibleBounds.top = Math.max(0, Math.floor(this._screenToPage(visibleBounds.top)));
    visibleBounds.left = Math.max(0, Math.floor(this._screenToPage(visibleBounds.left)));
    visibleBounds.bottom = Math.ceil(this._screenToPage(visibleBounds.bottom));
    visibleBounds.right = Math.ceil(this._screenToPage(visibleBounds.right));

    
    
    
    if (!boundsSizeChanged)
      this.flushDrawQ();

    this._visibleBounds = visibleBounds;
    this._pageBounds = pageBounds;

    let dx = this._screenX - bounds.x;
    let dy = this._screenY - bounds.y;
    this._screenX = bounds.x;
    this._screenY = bounds.y;

    if (boundsSizeChanged) {
      
      
      
      this.clearDrawQ();

      
      if (!this._clippedPageDrawing)
        this._partiallyDrawn = false;

      this._redrawRect(pageBounds);
      return;
    }

    
    let srcRect = { x: 0, y: 0,
                    width: this._canvas.width, height: this._canvas.height };
    let dstRect = { x: dx, y: dy,
                    width: this._canvas.width, height: this._canvas.height };

    
    if (srcRect.x == dstRect.x && srcRect.y == dstRect.y &&
        srcRect.width == dstRect.width && srcRect.height == dstRect.height) {
      
      return;
    }

    
    var ctx = this._canvas.getContext("2d");

    ctx.drawImage(this._canvas,
                  srcRect.x, srcRect.y,
                  srcRect.width, srcRect.height,
                  dstRect.x, dstRect.y,
                  dstRect.width, dstRect.height);

    

    
    var rgn = Cc["@mozilla.org/gfx/region;1"].createInstance(Ci.nsIScriptableRegion);
    rgn.setToRect(srcRect.x, srcRect.y, srcRect.width, srcRect.height);
    rgn.subtractRect(dstRect.x, dstRect.y, dstRect.width, dstRect.height);

    let outX = {}; let outY = {}; let outW = {}; let outH = {};
    rgn.getBoundingBox(outX, outY, outW, outH);
    dstRect = { x: outX.value, y: outY.value, width: outW.value, height: outH.value };

    if (dstRect.width > 0 && dstRect.height > 0) {
      dstRect.width += 1;
      dstRect.height += 1;


      

      ctx.save();
      ctx.translate(dstRect.x, dstRect.y);
      ctx.scale(this._zoomLevel, this._zoomLevel);

      let scaledRect = { x: this._pageBounds.x + this._screenToPage(dstRect.x),
                         y: this._pageBounds.y + this._screenToPage(dstRect.y),
                         width: this._screenToPage(dstRect.width),
                         height: this._screenToPage(dstRect.height) };

      

      ctx.drawWindow(this._browser.contentWindow,
                     scaledRect.x, scaledRect.y,
                     scaledRect.width, scaledRect.height,
                     "white",
                     ctx.DRAWWINDOW_DO_NOT_FLUSH | ctx.DRAWWINDOW_DRAW_CARET);

      
      
      

      ctx.restore();
    }
  },

  _handleMozAfterPaint: function(aEvent) {
    let cwin = this._browser.contentWindow;

    for (let i = 0; i < aEvent.clientRects.length; i++) {
      let e = aEvent.clientRects.item(i);
      let r = new wsRect(Math.floor(e.left + cwin.scrollX),
                         Math.floor(e.top + cwin.scrollY),
                         Math.ceil(e.width), Math.ceil(e.height));
      this._redrawRect(r);
    }
  },

  _redrawRect: function(rect) {
    
    
    if (this._clippedPageDrawing)  {
      r2 = this._visibleBounds;
      this._partiallyDrawn = true;
      
      if (rect.bottom > 0 && rect.right > r2.right)
        this._maybeZoomToPage = true;
    } else {
      let [canvasW, canvasH] = this._effectiveCanvasDimensions;
      r2 =  new wsRect(Math.max(this._pageBounds.x,0),
                       Math.max(this._pageBounds.y,0),
                       canvasW, canvasH);
    }

    let dest = rect.intersect(r2);

    if (dest)
      this.addToDrawQ(dest);
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
      this._maybeZoomToPage = false
  },

  zoomToElement: function(aElement) {
    const margin = 15;

    
    let [canvasW, ] = this.canvasDimensions;

    let elRect = this._getPagePosition(aElement);
    let zoomLevel = canvasW / (elRect.width + (2 * margin));
    this.zoomLevel = Math.min(zoomLevel, 10);

    
    ws.panTo(Math.floor(Math.max(this._pageToScreen(elRect.x) - margin, 0)),
             Math.floor(Math.max(this._pageToScreen(elRect.y) - margin, 0)));
  },

  zoomFromElement: function(aElement) {
    let elRect = this._getPagePosition(aElement);

    
    
    this.zoomToPage();

    
    ws.panTo(0, Math.floor(Math.max(0, this._pageToScreen(elRect.y))));
  },

  



  elementFromPoint: function(aX, aY) {
    let [x, y] = this._clientToContentCoords(aX, aY);
    let cwu = this._browser.contentWindow
                  .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                  .getInterface(Components.interfaces.nsIDOMWindowUtils);

    let element = cwu.elementFromPoint(x, y,
                                       true,   
                                       false); 

    return element;
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
