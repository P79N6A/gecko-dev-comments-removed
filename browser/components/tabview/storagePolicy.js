










































let StoragePolicy = {
  
  PREF_DISK_CACHE_SSL: "browser.cache.disk_cache_ssl",

  
  _enablePersistentHttpsCaching: null,

  
  _deniedBrowsers: [],

  
  
  
  toString: function StoragePolicy_toString() {
    return "[StoragePolicy]";
  },

  
  
  
  init: function StoragePolicy_init() {
    
    this._enablePersistentHttpsCaching =
      Services.prefs.getBoolPref(this.PREF_DISK_CACHE_SSL);

    Services.prefs.addObserver(this.PREF_DISK_CACHE_SSL, this, false);

    
    
    if (!this._enablePersistentHttpsCaching)
      Array.forEach(gBrowser.browsers, this._initializeBrowser.bind(this));

    
    this._onTabClose = this._onTabClose.bind(this);
    gBrowser.tabContainer.addEventListener("TabClose", this._onTabClose, false);

    let mm = gWindow.messageManager;

    
    this._onGranted = this._onGranted.bind(this);
    mm.addMessageListener("Panorama:StoragePolicy:granted", this._onGranted);

    
    this._onDenied = this._onDenied.bind(this);
    mm.addMessageListener("Panorama:StoragePolicy:denied", this._onDenied);
  },

  
  
  
  
  _initializeBrowser: function StoragePolicy__initializeBrowser(browser) {
    let self = this;

    function checkExclusion() {
      if (browser.currentURI.schemeIs("https"))
        self._deniedBrowsers.push(browser);
    }

    function waitForDocumentLoad() {
      let mm = browser.messageManager;

      mm.addMessageListener("Panorama:DOMContentLoaded", function onLoad(cx) {
        mm.removeMessageListener(cx.name, onLoad);
        checkExclusion(browser);
      });
    }

    this._isDocumentLoaded(browser, function (isLoaded) {
      if (isLoaded)
        checkExclusion();
      else
        waitForDocumentLoad();
    });
  },

  
  
  
  _isDocumentLoaded: function StoragePolicy__isDocumentLoaded(browser, callback) {
    let mm = browser.messageManager;
    let message = "Panorama:isDocumentLoaded";

    mm.addMessageListener(message, function onMessage(cx) {
      mm.removeMessageListener(cx.name, onMessage);
      callback(cx.json.isLoaded);
    });

    mm.sendAsyncMessage(message);
  },

  
  
  
  uninit: function StoragePolicy_uninit() {
    Services.prefs.removeObserver(this.PREF_DISK_CACHE_SSL, this);
    gBrowser.removeTabsProgressListener(this);
    gBrowser.tabContainer.removeEventListener("TabClose", this._onTabClose, false);

    let mm = gWindow.messageManager;

    
    mm.removeMessageListener("Panorama:StoragePolicy:granted", this._onGranted);
    mm.removeMessageListener("Panorama:StoragePolicy:denied", this._onDenied);
  },

  
  
  
  
  _onGranted: function StoragePolicy__onGranted(cx) {
    let index = this._deniedBrowsers.indexOf(cx.target);

    if (index > -1)
      this._deniedBrowsers.splice(index, 1);
  },

  
  
  
  
  _onDenied: function StoragePolicy__onDenied(cx) {
    
    
    
    if ("https" == cx.json.reason && this._enablePersistentHttpsCaching)
      return;

    let browser = cx.target;

    if (this._deniedBrowsers.indexOf(browser) == -1)
      this._deniedBrowsers.push(browser);
  },

  
  
  
  _onTabClose: function StoragePolicy__onTabClose(event) {
    let browser = event.target.linkedBrowser;
    let index = this._deniedBrowsers.indexOf(browser);

    if (index > -1)
      this._deniedBrowsers.splice(index, 1);
  },

  
  
  
  canStoreThumbnailForTab: function StoragePolicy_canStoreThumbnailForTab(tab) {
    
    if (gPrivateBrowsing.privateBrowsingEnabled &&
        UI._privateBrowsing.transitionMode != "enter")
      return false;

    return (this._deniedBrowsers.indexOf(tab.linkedBrowser) == -1);
  },

  
  
  
  observe: function StoragePolicy_observe(subject, topic, data) {
    this._enablePersistentHttpsCaching =
      Services.prefs.getBoolPref(this.PREF_DISK_CACHE_SSL);
  }
};
