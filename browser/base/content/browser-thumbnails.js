#ifdef 0



#endif

Cu.import("resource://gre/modules/NewTabUtils.jsm");




let gBrowserThumbnails = {
  


  PREF_DISK_CACHE_SSL: "browser.cache.disk_cache_ssl",

  _captureDelayMS: 1000,

  


  _sslDiskCacheEnabled: null,

  


  _timeouts: null,

  


  _tabEvents: ["TabClose", "TabSelect"],

  init: function Thumbnails_init() {
    
    if (gMultiProcessBrowser)
      return;

    PageThumbs.addExpirationFilter(this);
    gBrowser.addTabsProgressListener(this);
    Services.prefs.addObserver(this.PREF_DISK_CACHE_SSL, this, false);

    this._sslDiskCacheEnabled =
      Services.prefs.getBoolPref(this.PREF_DISK_CACHE_SSL);

    this._tabEvents.forEach(function (aEvent) {
      gBrowser.tabContainer.addEventListener(aEvent, this, false);
    }, this);

    this._timeouts = new WeakMap();
  },

  uninit: function Thumbnails_uninit() {
    
    if (gMultiProcessBrowser)
      return;

    PageThumbs.removeExpirationFilter(this);
    gBrowser.removeTabsProgressListener(this);
    Services.prefs.removeObserver(this.PREF_DISK_CACHE_SSL, this);

    this._tabEvents.forEach(function (aEvent) {
      gBrowser.tabContainer.removeEventListener(aEvent, this, false);
    }, this);
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

  observe: function Thumbnails_observe() {
    this._sslDiskCacheEnabled =
      Services.prefs.getBoolPref(this.PREF_DISK_CACHE_SSL);
  },

  filterForThumbnailExpiration:
  function Thumbnails_filterForThumbnailExpiration(aCallback) {
    aCallback(this._topSiteURLs);
  },

  


  onStateChange: function Thumbnails_onStateChange(aBrowser, aWebProgress,
                                                   aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK)
      this._delayedCapture(aBrowser);
  },

  _capture: function Thumbnails_capture(aBrowser) {
    
    if (this._topSiteURLs.indexOf(aBrowser.currentURI.spec) >= 0 &&
        this._shouldCapture(aBrowser))
      PageThumbs.captureAndStoreIfStale(aBrowser);
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
    
    if (gMultiProcessBrowser)
      return false;

    
    if (aBrowser != gBrowser.selectedBrowser)
      return false;

    
    if (PrivateBrowsingUtils.isWindowPrivate(window))
      return false;

    let doc = aBrowser.contentDocument;

    
    
    if (doc instanceof SVGDocument || doc instanceof XMLDocument)
      return false;

    
    if (aBrowser.docShell.busyFlags != Ci.nsIDocShell.BUSY_FLAGS_NONE)
      return false;

    
    if (aBrowser.currentURI.schemeIs("about"))
      return false;

    let channel = aBrowser.docShell.currentDocumentChannel;

    
    if (!channel)
      return false;

    
    
    let uri = channel.originalURI;
    if (uri.schemeIs("about"))
      return false;

    let httpChannel;
    try {
      httpChannel = channel.QueryInterface(Ci.nsIHttpChannel);
    } catch (e) {  }

    if (httpChannel) {
      
      try {
        if (Math.floor(httpChannel.responseStatus / 100) != 2)
          return false;
      } catch (e) {
        
        
        return false;
      }

      
      if (httpChannel.isNoStoreResponse())
        return false;

      
      if (uri.schemeIs("https") && !this._sslDiskCacheEnabled)
        return false;
    }

    return true;
  },

  get _topSiteURLs() {
    return NewTabUtils.links.getLinks().reduce((urls, link) => {
      if (link)
        urls.push(link.url);
      return urls;
    }, []);
  },

  _clearTimeout: function Thumbnails_clearTimeout(aBrowser) {
    if (this._timeouts.has(aBrowser)) {
      aBrowser.removeEventListener("scroll", this, false);
      clearTimeout(this._timeouts.get(aBrowser));
      this._timeouts.delete(aBrowser);
    }
  }
};
