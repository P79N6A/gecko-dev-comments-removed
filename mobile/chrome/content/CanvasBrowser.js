









































function CanvasBrowser(canvas) {
  this._canvas = canvas;
}

CanvasBrowser.prototype = {
  _canvas: null,
  _zoomLevel: 1,
  _browser: null,
  _pageX: 0,
  _pageY: 0,
  _screenX: 0,
  _screenY: 0,

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

  get _effectiveViewportDimensions() {
    
  },

  setCurrentBrowser: function(browser, skipZoom) {
    let currentBrowser = this._browser;
    if (currentBrowser) {
      
      currentBrowser.removeEventListener("MozAfterPaint", this._paintHandler, false);
      currentBrowser.setAttribute("type", "content");
      currentBrowser.docShell.isOffScreenBrowser = false;
    }

    browser.setAttribute("type", "content-primary");
    browser.docShell.isOffScreenBrowser = true;

    
    var self = this;
    this._paintHandler = function(ev) { self._handleMozAfterPaint(ev); }

    browser.addEventListener("MozAfterPaint", this._paintHandler, false);

    this._browser = browser;
    
    
    if (!skipZoom) {
      self.zoomToPage();
    }
  },

  startLoading: function() {
    
    
    

    var ctx = this._canvas.getContext("2d");
    ctx.fillStyle = "rgb(255,255,255)";
    ctx.fillRect(0, 0, this._canvas.width, this._canvas.height);

    this._resizeInterval = setInterval(function(self) { self.zoomToPage(); }, 2000, this);
  },

  endLoading: function() {
    clearInterval(this._resizeInterval);
    this.zoomToPage();
  },

  viewportHandler: function(bounds, oldBounds) {
    let pageBounds = bounds.clone();
    pageBounds.top = Math.floor(this._screenToPage(bounds.top));
    pageBounds.left = Math.floor(this._screenToPage(bounds.left));
    pageBounds.bottom = Math.ceil(this._screenToPage(bounds.bottom));
    pageBounds.right = Math.ceil(this._screenToPage(bounds.right));

    if (0) {
      if (true ) {
        this._pageX = pageBounds.x;
        this._pageY = pageBounds.y;

        var ctx = this._canvas.getContext("2d");

        ctx.save();
        ctx.scale(this._zoomLevel, this._zoomLevel);

        try {
          dump("drawWindow: " + pageBounds.x + " " + pageBounds.y + " " + pageBounds.width + " " + pageBounds.height + "\n");
          ctx.drawWindow(this._browser.contentWindow,
                         pageBounds.x, pageBounds.y, pageBounds.width, pageBounds.height,
                         "white",
                         ctx.DRAWWINDOW_DO_NOT_FLUSH | ctx.DRAWWINDOW_DRAW_CARET);
        } catch (e) {
          dump("DRAWWINDOW FAILED\n");
        }

        ctx.restore();
        return;
      }
    }

    let dx = this._screenX - bounds.x;
    let dy = this._screenY - bounds.y;
    this._screenX = bounds.x;
    this._screenY = bounds.y;
    this._pageX = pageBounds.x;
    this._pageY = pageBounds.y;
    
    if (!oldBounds) {
      
      this._redrawRect(pageBounds.x, pageBounds.y,
                       pageBounds.width, pageBounds.height);
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

      let [offX, offY] = this._pageOffset;
      let scaledRect = { x: offX + this._screenToPage(dstRect.x),
                         y: offY + this._screenToPage(dstRect.y),
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
      
      
      
      this._redrawRect(Math.floor(e.left + cwin.scrollX),
                       Math.floor(e.top + cwin.scrollY),
                       Math.ceil(e.width), Math.ceil(e.height));
    }
  },

  _redrawRect: function(x, y, width, height) {
    function intersect(r1, r2) {
      let xmost1 = r1.x + r1.width;
      let ymost1 = r1.y + r1.height;
      let xmost2 = r2.x + r2.width;
      let ymost2 = r2.y + r2.height;

      let x = Math.max(r1.x, r2.x);
      let y = Math.max(r1.y, r2.y);

      let temp = Math.min(xmost1, xmost2);
      if (temp <= x)
        return null;

      let width = temp - x;

      temp = Math.min(ymost1, ymost2);
      if (temp <= y)
        return null;

      let height = temp - y;

      return { x: x,
               y: y,
               width: width,
               height: height };
    }

    let r1 = { x : x,
               y : y,
               width : width,
               height: height };

    
    let [canvasW, canvasH] = this._effectiveCanvasDimensions;
    let r2 = { x : Math.max(this._pageX,0),
               y : Math.max(this._pageY,0),
               width : canvasW,
               height: canvasH };

    let dest = intersect(r1, r2);

    if (!dest)
      return;

    

    var ctx = this._canvas.getContext("2d");

    ctx.save();
    ctx.scale(this._zoomLevel, this._zoomLevel);

    let [offX, offY] = this._pageOffset;
    ctx.translate(dest.x - offX, dest.y - offY);

    
    ctx.drawWindow(this._browser.contentWindow,
                   dest.x, dest.y,
                   dest.width, dest.height,
                   "white",
                   ctx.DRAWWINDOW_DO_NOT_FLUSH | ctx.DRAWWINDOW_DRAW_CARET);

    ctx.restore();
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
  },

  zoomToElement: function(aElement) {
    const margin = 15;

    
    
    
    ws.panTo(0, 0);

    
    let [canvasW, ] = this.canvasDimensions;

    let elRect = this._getPagePosition(aElement);
    let zoomLevel = canvasW / (elRect.width + (2 * margin));
    this.zoomLevel = Math.min(zoomLevel, 10);

    
    ws.panTo(Math.floor(Math.max(this._pageToScreen(elRect.x) - margin, 0)),
             Math.floor(Math.max(this._pageToScreen(elRect.y) - margin, 0)));
  },

  zoomFromElement: function(aElement) {
    let elRect = this._getPagePosition(aElement);

    
    
    
    ws.panTo(0, 0);

    
    
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

  get _pageOffset() {
    
    
    return [this._pageX, this._pageY];
  },

  


  _clientToContentCoords: function(aClientX, aClientY) {
    
    
    

    let canvasRect = this._canvas.getBoundingClientRect();
    let clickOffsetX = this._screenToPage(aClientX - canvasRect.left) + this._pageX;
    let clickOffsetY = this._screenToPage(aClientY - canvasRect.top) + this._pageY;

    
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
    
    
    return;

    let elRect = this._getPagePosition(aElement);
    let [viewportW, viewportH] = this._effectiveViewportDimensions;
    let curRect = {
      x: this._pageX,
      y: this._pageY,
      width: viewportW,
      height: viewportH
    };

    
    
    let browserRect = this._currentBrowser.getBoundingClientRect();
    curRect.height -= this._screenToPage(Math.abs(browserRect.top));
    if (browserRect.top < 0)
      curRect.y -= this._screenToPage(browserRect.top);
    curRect.width -= this._screenToPage(Math.abs(browserRect.left));
    if (browserRect.left < 0)
      curRect.x -= this._screenToPage(browserRect.left);

    let newx = curRect.x;
    let newy = curRect.y;

    if (elRect.x + elRect.width > curRect.x + curRect.width) {
      newx = curRect.x + ((elRect.x + elRect.width)-(curRect.x + curRect.width));
    } else if (elRect.x < curRect.x) {
      newx = elRect.x;
    }

    if (elRect.y + elRect.height > curRect.y + curRect.height) {
      newy = curRect.y + ((elRect.y + elRect.height)-(curRect.y + curRect.height));
    } else if (elRect.y < curRect.y) {
      newy = elRect.y;
    }

    this.panTo(newx, newy);
  },

  
  panToElement: function(aElement) {
    var elRect = this._getPagePosition(aElement);

    this.panTo(elRect.x, elRect.y);
  },

  panTo: function(x, y) {
    ws.panTo(x, y);
  }
};
