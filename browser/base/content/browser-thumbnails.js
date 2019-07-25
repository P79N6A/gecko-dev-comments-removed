#ifdef 0



#endif




let gBrowserThumbnails = {
  _captureDelayMS: 2000,

  


  _timeouts: null,

  


  _pageThumbs: null,

  


  _tabEvents: ["TabClose", "TabSelect"],

  init: function Thumbnails_init() {
    gBrowser.addTabsProgressListener(this);

    this._tabEvents.forEach(function (aEvent) {
      gBrowser.tabContainer.addEventListener(aEvent, this, false);
    }, this);

    this._timeouts = new WeakMap();

    XPCOMUtils.defineLazyModuleGetter(this, "_pageThumbs",
      "resource:///modules/PageThumbs.jsm", "PageThumbs");
  },

  uninit: function Thumbnails_uninit() {
    gBrowser.removeTabsProgressListener(this);

    this._tabEvents.forEach(function (aEvent) {
      gBrowser.tabContainer.removeEventListener(aEvent, this, false);
    }, this);

    this._timeouts = null;
    this._pageThumbs = null;
  },

  handleEvent: function Thumbnails_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "scroll":
        let browser = aEvent.currentTarget;
        if (this._timeouts.has(browser))
          this._delayedCapture(browser);
        break;
      case "TabSelect":
        this._delayedCapture(aEvent.target.linkedBrowser);
        break;
      case "TabClose": {
        this._clearTimeout(aEvent.target.linkedBrowser);
        break;
      }
    }
  },

  


  onStateChange: function Thumbnails_onStateChange(aBrowser, aWebProgress,
                                                   aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK)
      this._delayedCapture(aBrowser);
  },

  _capture: function Thumbnails_capture(aBrowser) {
    if (this._shouldCapture(aBrowser)) {
      this._pageThumbs.capture(aBrowser.contentWindow, function (aInputStream) {
        this._pageThumbs.store(aBrowser.currentURI.spec, aInputStream);
      }.bind(this));
    }
  },

  _delayedCapture: function Thumbnails_delayedCapture(aBrowser) {
    if (this._timeouts.has(aBrowser))
      clearTimeout(this._timeouts.get(aBrowser));
    else
      aBrowser.addEventListener("scroll", this, true);

    let timeout = setTimeout(function () {
      this._clearTimeout(aBrowser);
      this._capture(aBrowser);
    }.bind(this), this._captureDelayMS);

    this._timeouts.set(aBrowser, timeout);
  },

  _shouldCapture: function Thumbnails_shouldCapture(aBrowser) {
    let doc = aBrowser.contentDocument;

    
    
    if (doc instanceof SVGDocument || doc instanceof XMLDocument)
      return false;

    
    if (aBrowser.docShell.busyFlags != Ci.nsIDocShell.BUSY_FLAGS_NONE)
      return false;

    
    if (aBrowser.currentURI.schemeIs("about"))
      return false;

    let channel = aBrowser.docShell.currentDocumentChannel;

    try {
      
      let httpChannel = channel.QueryInterface(Ci.nsIHttpChannel);

      
      return Math.floor(httpChannel.responseStatus / 100) == 2;
    } catch (e) {
      
      return true;
    }
  },

  _clearTimeout: function Thumbnails_clearTimeout(aBrowser) {
    if (this._timeouts.has(aBrowser)) {
      aBrowser.removeEventListener("scroll", this, false);
      clearTimeout(this._timeouts.get(aBrowser));
      this._timeouts.delete(aBrowser);
    }
  }
};
