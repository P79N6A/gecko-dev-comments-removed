let noop = function() {};
Browser = {
  updateViewportSize: noop
    
















};
let ws = {
  beginUpdateBatch: noop,
  panTo: noop,
  endUpdateBatch: noop
};
let Ci = Components.interfaces;
let bv = null;
let endl = "\n";


function BrowserView() {
  this.init();
  bindAll(this);
}

BrowserView.prototype = {

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  get tileManager() { return this._tileManager; },
  get scrollbox() { return this._scrollbox; },

  init: function init() {
    let scrollbox = document.getElementById("scrollbox")
			.boxObject
			.QueryInterface(Components.interfaces.nsIScrollBoxObject);
    this._scrollbox = scrollbox;

    let leftbar  = document.getElementById("left_sidebar");
    let rightbar = document.getElementById("right_sidebar");
    let topbar   = document.getElementById("top_urlbar");
    this._leftbar = leftbar;
    this._rightbar = rightbar;
    this._topbar = topbar;

    scrollbox.scrollTo(Math.round(leftbar.getBoundingClientRect().right), 0);

    let tileContainer = document.getElementById("tile_container");
    tileContainer.addEventListener("mousedown", onMouseDown, true);
    tileContainer.addEventListener("mouseup",   onMouseUp,   true);
    tileContainer.addEventListener("mousemove", onMouseMove, true);
    this._tileContainer = tileContainer;

    let tileManager = new TileManager(this.appendTile, this.removeTile, window.innerWidth);
    this._tileManager = tileManager;

    let browser = document.getElementById("googlenews");
    this.setCurrentBrowser(browser, false);    

    let cdoc = browser.contentDocument;

    
    let body = cdoc.body || {};
    let html = cdoc.documentElement || {};

    let w = Math.max(body.scrollWidth || 0, html.scrollWidth);
    let h = Math.max(body.scrollHeight || 0, html.scrollHeight);

    let viewportRect = new wsRect(0, 0, w, h);
    this._viewportRect = viewportRect;

    let viewportInnerBoundsRect = this.getViewportInnerBoundsRect();
    this._viewportInnerBoundsRect = viewportInnerBoundsRect;

    tileManager.viewportHandler(viewportRect,
				window.innerWidth,
				viewportInnerBoundsRect,
				true);
  },

  resizeTileContainer: function resizeTileContainer() {

  },

  scrollboxToViewportRect: function scrollboxToViewportRect(rect, clip) {
    let leftbar  = this._leftbar.getBoundingClientRect();
    let rightbar = this._rightbar.getBoundingClientRect();
    let topbar   = this._topbar.getBoundingClientRect();

    let xtrans = -leftbar.width;
    let ytrans = -topbar.height;
    let x = rect.x + xtrans;
    let y = rect.y + ytrans;

    
    
    rect.x = (clip) ? Math.max(x, 0) : x;
    rect.y = (clip) ? Math.max(y, 0) : y;

    return rect;
  },

  getScrollboxPosition: function getScrollboxPosition() {
    let x = {};
    let y = {};
    this._scrollbox.getPosition(x, y);
    return [x.value, y.value];
  },

  getViewportInnerBoundsRect: function getViewportInnerBoundsRect(dx, dy) {
    if (!dx) dx = 0;
    if (!dy) dy = 0;

    let w = window.innerWidth;
    let h = window.innerHeight;

    let leftbar  = this._leftbar.getBoundingClientRect();
    let rightbar = this._rightbar.getBoundingClientRect();
    let topbar   = this._topbar.getBoundingClientRect();

    let leftinner  = Math.max(leftbar.right - dx, 0);
    let rightinner = Math.min(rightbar.left - dx, w);
    let topinner   = Math.max(topbar.bottom - dy, 0);

    let [x, y] = this.getScrollboxPosition();

    return this.scrollboxToViewportRect(new wsRect(x + dx, y + dy, rightinner - leftinner, h - topinner),
				        true);
  },

  appendTile: function appendTile(tile) {
    let canvas = tile.contentImage;

    canvas.style.position = "absolute";
    canvas.style.left = tile.x + "px";
    canvas.style.top  = tile.y + "px";

    let tileContainer = document.getElementById("tile_container");
    tileContainer.appendChild(canvas);

    dump('++ ' + tile.toString() + endl);
  },

  removeTile: function removeTile(tile) {
    let canvas = tile.contentImage;

    let tileContainer = document.getElementById("tile_container");
    tileContainer.removeChild(canvas);

    dump('-- ' + tile.toString() + endl);
  },

  scrollBy: function scrollBy(dx, dy) {
    
    this.onBeforeScroll();
    this.onAfterScroll();
  },

  
  
  
  
  onBeforeScroll: function onBeforeScroll(x, y, dx, dy) {
    this.tileManager.onBeforeScroll(this.getViewportInnerBoundsRect(dx, dy));

    
    let sidebars = document.getElementsByClassName("sidebar");
    for (let i = 0; i < sidebars.length; i++) {
      let sidebar = sidebars[i];
      sidebar.style.margin = (y + dy) + "px 0px 0px 0px";
    }

    let urlbar = document.getElementById("top_urlbar");
    urlbar.style.margin = "0px 0px 0px " + (x + dx) + "px";
  },

  onAfterScroll: function onAfterScroll(x, y, dx, dy) {
    this.tileManager.onAfterScroll(this.getViewportInnerBoundsRect());
  },

  setCurrentBrowser: function setCurrentBrowser(browser, skipZoom) {
    let currentBrowser = this._browser;
    if (currentBrowser) {
      
      currentBrowser.mZoomLevel = this.zoomLevel;
      currentBrowser.mPanX = ws._viewingRect.x;
      currentBrowser.mPanY = ws._viewingRect.y;

      
      currentBrowser.removeEventListener("MozAfterPaint", this.handleMozAfterPaint, false);
      currentBrowser.setAttribute("type", "content");
      currentBrowser.docShell.isOffScreenBrowser = false;
    }

    browser.setAttribute("type", "content-primary");
    if (!skipZoom)
      browser.docShell.isOffScreenBrowser = true;

    
    browser.addEventListener("MozAfterPaint", this.handleMozAfterPaint, false);

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

    this.tileManager.browser = browser;
  },

  handleMozAfterPaint: function handleMozAfterPaint(ev) {
    this.tileManager.handleMozAfterPaint(ev);
  },

  zoomToPage: function zoomToPage() {
    


















  }

};


function onResize(e) {
  let browser = document.getElementById("googlenews");
  let cdoc = browser.contentDocument;

  
  var body = cdoc.body || {};
  var html = cdoc.documentElement || {};

  var w = Math.max(body.scrollWidth || 0, html.scrollWidth);
  var h = Math.max(body.scrollHeight || 0, html.scrollHeight);

  if (bv)
    bv.tileManager.viewportHandler(new wsRect(0, 0, w, h),
				   window.innerWidth,
				   bv.getViewportInnerBoundsRect(),
				   true);
}

function onMouseDown(e) {
  window._isDragging = true;
  window._dragStart = {x: e.clientX, y: e.clientY};

  bv.tileManager.startPanning();
}

function onMouseUp() {
  window._isDragging = false;

  bv.tileManager.endPanning();
}

function onMouseMove(e) {
  if (window._isDragging) {
    let scrollbox = bv.scrollbox;

    let x = {};
    let y = {};
    let w = {};
    let h = {};
    scrollbox.getPosition(x, y);
    scrollbox.getScrolledSize(w, h);

    let dx = window._dragStart.x - e.clientX;
    let dy = window._dragStart.y - e.clientY;

    
    let newX = Math.max(x.value + dx, 0);
    let newY = Math.max(y.value + dy, 0);

    if (newX < w.value || newY < h.value) {
      
      dx = Math.max(dx, -x.value);
      dy = Math.max(dy, -y.value);

      bv.onBeforeScroll(x.value, y.value, dx, dy);

      






      scrollbox.scrollBy(dx, dy);

      



      bv.onAfterScroll();
    }
  }

  window._dragStart = {x: e.clientX, y: e.clientY};
}

function onLoad() {
  bv = new BrowserView();
}
