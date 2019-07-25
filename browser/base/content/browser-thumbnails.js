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
      case "TabSelect":
        this._delayedCapture(aEvent.target.linkedBrowser);
        break;
      case "TabClose": {
        let browser = aEvent.target.linkedBrowser;
        if (this._timeouts.has(browser)) {
          clearTimeout(this._timeouts.get(browser));
          this._timeouts.delete(browser);
        }
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
      let canvas = this._pageThumbs.capture(aBrowser.contentWindow);
      this._pageThumbs.store(aBrowser.currentURI.spec, canvas);
    }
  },

  _delayedCapture: function Thumbnails_delayedCapture(aBrowser) {
    if (this._timeouts.has(aBrowser))
      clearTimeout(this._timeouts.get(aBrowser));

    let timeout = setTimeout(function () {
      this._timeouts.delete(aBrowser);
      this._capture(aBrowser);
    }.bind(this), this._captureDelayMS);

    this._timeouts.set(aBrowser, timeout);
  },

  _shouldCapture: function Thumbnails_shouldCapture(aBrowser) {
    
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
  }
};
