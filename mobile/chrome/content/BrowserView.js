








































let Ci = Components.interfaces;

function BrowserView(container, visibleRectFactory) {
  Util.bindAll(this);
  this.init(container, visibleRectFactory);
}









BrowserView.Util = {
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
    let oldBrowser = this._browser;
    let browserChanged = (oldBrowser !== browser);

    if (oldBrowser) {
      oldBrowser.setAttribute("type", "content");
      oldBrowser.style.display = "none";
      oldBrowser.messageManager.sendAsyncMessage("Browser:Blur", {});
    }

    this._browser = browser;
    this._browserViewportState = browserViewportState;

    if (browser) {
      browser.setAttribute("type", "content-primary");
      browser.style.display = "";
      browser.messageManager.sendAsyncMessage("Browser:Focus", {});
    }
  },

  getBrowser: function getBrowser() {
    return this._browser;
  }
};
