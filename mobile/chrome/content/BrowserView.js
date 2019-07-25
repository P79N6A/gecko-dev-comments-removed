








































let Ci = Components.interfaces;

const kBrowserFormZoomLevelMin = 1.0;
const kBrowserFormZoomLevelMax = 2.0;
const kBrowserViewZoomLevelPrecision = 10000;
























































































function BrowserView(container, visibleRectFactory) {
  Util.bindAll(this);
  this.init(container, visibleRectFactory);
}









BrowserView.Util = {
  visibleRectToCriticalRect: function visibleRectToCriticalRect(visibleRect, browserViewportState) {
    return visibleRect.intersect(browserViewportState.viewportRect);
  },

  createBrowserViewportState: function createBrowserViewportState() {
    return new BrowserView.BrowserViewportState(new Rect(0, 0, 800, 800), 0, 0, 1);
  },

  ensureMozScrolledAreaEvent: function ensureMozScrolledAreaEvent(aBrowser, aWidth, aHeight) {
    let message = {};
    message.target = aBrowser;
    message.name = "Browser:MozScrolledAreaChanged";
    message.json = { width: aWidth, height: aHeight };

    Browser._browserView.updateScrolledArea(message);
  }
};

BrowserView.prototype = {

  
  
  

  init: function init(container, visibleRectFactory) {
    this._container = container;
    this._browser = null;
    this._browserViewportState = null;
    this._visibleRectFactory = visibleRectFactory;
    messageManager.addMessageListener("Browser:MozScrolledAreaChanged", this);
  },

  uninit: function uninit() {
  },

  getVisibleRect: function getVisibleRect() {
    return this._visibleRectFactory();
  },

  getCriticalRect: function getCriticalRect() {
    let bvs = this._browserViewportState;
    let vr = this.getVisibleRect();
    return BrowserView.Util.visibleRectToCriticalRect(vr, bvs);
  },

  


  getViewportDimensions: function getViewportDimensions() {
    let bvs = this._browserViewportState;
    if (!bvs)
      throw "Cannot get viewport dimensions when no browser is set";

    return [bvs.viewportRect.right, bvs.viewportRect.bottom];
  },

  setZoomLevel: function setZoomLevel(zoomLevel) {
    return;

    let bvs = this._browserViewportState;
    if (!bvs)
      return;

    let newZoomLevel = this.clampZoomLevel(zoomLevel);
    if (newZoomLevel != bvs.zoomLevel) {
      let browserW = this.viewportToBrowser(bvs.viewportRect.right);
      let browserH = this.viewportToBrowser(bvs.viewportRect.bottom);
      bvs.zoomLevel = newZoomLevel; 
      bvs.viewportRect.right  = this.browserToViewport(browserW);
      bvs.viewportRect.bottom = this.browserToViewport(browserH);
      this._viewportChanged();

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

  clampZoomLevel: function clampZoomLevel(zl) {
    let bounded = Math.min(Math.max(ZoomManager.MIN, zl), ZoomManager.MAX);

    let bvs = this._browserViewportState;
    if (bvs) {
      let md = bvs.metaData;
      if (md && md.minZoom)
        bounded = Math.max(bounded, md.minZoom);
      if (md && md.maxZoom)
        bounded = Math.min(bounded, md.maxZoom);

      bounded = Math.max(bounded, this.getPageZoomLevel());
    }

    let rounded = Math.round(bounded * kBrowserViewZoomLevelPrecision) / kBrowserViewZoomLevelPrecision;
    return rounded || 1.0;
  },

  


  setBrowser: function setBrowser(browser, browserViewportState) {
    if (browser && !browserViewportState) {
      throw "Cannot set non-null browser with null BrowserViewportState";
    }

    let oldBrowser = this._browser;
    let browserChanged = (oldBrowser !== browser);

    if (oldBrowser) {
      oldBrowser.setAttribute("type", "content");
      oldBrowser.setAttribute("style", "display: none;");
      oldBrowser.messageManager.sendAsyncMessage("Browser:Blur", {});
    }

    this._browser = browser;
    this._browserViewportState = browserViewportState;

    if (browser) {
      browser.setAttribute("type", "content-primary");
      browser.setAttribute("style", "display: block;");
      browser.messageManager.sendAsyncMessage("Browser:Focus", {});
    }
  },

  getBrowser: function getBrowser() {
    return this._browser;
  },

  receiveMessage: function receiveMessage(aMessage) {
    switch (aMessage.name) {
      case "Browser:MozScrolledAreaChanged":
        this.updateScrolledArea(aMessage);
        break;
    }
  },

  updateScrolledArea: function updateScrolledArea(aMessage) {
    let browser = aMessage.target;
    let tab = Browser.getTabForBrowser(browser);
    if (!browser || !tab)
      return;

    let json = aMessage.json;
    let bvs = tab.browserViewportState;

    let vis = this.getVisibleRect();
    let viewport = bvs.viewportRect;
    let oldRight = viewport.right;
    let oldBottom = viewport.bottom;
    viewport.right  = bvs.zoomLevel * json.width;
    viewport.bottom = bvs.zoomLevel * json.height;

    if (browser == this._browser) {
      this._viewportChanged();
      this.updateDefaultZoom();
    }
  },

  
  updateDefaultZoom: function updateDefaultZoom() {
    let bvs = this._browserViewportState;
    if (!bvs)
      return false;

    let isDefault = (bvs.zoomLevel == bvs.defaultZoomLevel);
    bvs.defaultZoomLevel = this.getDefaultZoomLevel();
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

  getDefaultZoomLevel: function getDefaultZoomLevel() {
    let bvs = this._browserViewportState;
    if (!bvs)
      return 0;

    let md = bvs.metaData;
    if (md && md.defaultZoom)
      return this.clampZoomLevel(md.defaultZoom);

    let pageZoom = this.getPageZoomLevel();

    
    let granularity = Services.prefs.getIntPref("browser.ui.zoom.pageFitGranularity");
    let threshold = 1 - 1 / granularity;
    if (threshold < pageZoom && pageZoom < 1)
      pageZoom = 1;

    return this.clampZoomLevel(pageZoom);
  },

  getPageZoomLevel: function getPageZoomLevel() {
    let bvs = this._browserViewportState;  

    
    let browserW = this.viewportToBrowser(bvs.viewportRect.right) || 1.0;
    return this.getVisibleRect().width / browserW;
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

  get allowZoom() {
    let bvs = this._browserViewportState;
    if (!bvs || !bvs.metaData)
      return true;
    return bvs.metaData.allowZoom;
  },

  









  renderToCanvas: function renderToCanvas(destCanvas, destWidth, destHeight, srcRect) {
    return;

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

  _viewportChanged: function() {
    getBrowser().style.MozTransformOrigin = "left top";
    Browser.contentScrollboxScroller.updateTransition();
  },
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
