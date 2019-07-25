








































let Ci = Components.interfaces;

const kBrowserFormZoomLevelMin = 1.0;
const kBrowserFormZoomLevelMax = 2.0;
const kBrowserViewZoomLevelPrecision = 10000;

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
    this._visibleRectFactory = visibleRectFactory;
    messageManager.addMessageListener("Browser:MozScrolledAreaChanged", this);
  },

  uninit: function uninit() {
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
