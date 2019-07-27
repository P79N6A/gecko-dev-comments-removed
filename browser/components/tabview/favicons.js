



Components.utils.import('resource://gre/modules/PlacesUtils.jsm');

let FavIcons = {
  
  PREF_CHROME_SITE_ICONS: "browser.chrome.site_icons",

  
  PREF_CHROME_FAVICONS: "browser.chrome.favicons",

  
  get _prefSiteIcons() {
    delete this._prefSiteIcons;
    this._prefSiteIcons = Services.prefs.getBoolPref(this.PREF_CHROME_SITE_ICONS);
  },

  
  get _prefFavicons() {
    delete this._prefFavicons;
    this._prefFavicons = Services.prefs.getBoolPref(this.PREF_CHROME_FAVICONS);
  },

  get defaultFavicon() this._favIconService.defaultFavicon.spec,

  init: function FavIcons_init() {
    XPCOMUtils.defineLazyServiceGetter(this, "_favIconService",
      "@mozilla.org/browser/favicon-service;1", "nsIFaviconService");

    Services.prefs.addObserver(this.PREF_CHROME_SITE_ICONS, this, false);
    Services.prefs.addObserver(this.PREF_CHROME_FAVICONS, this, false);
  },

  uninit: function FavIcons_uninit() {
    Services.prefs.removeObserver(this.PREF_CHROME_SITE_ICONS, this);
    Services.prefs.removeObserver(this.PREF_CHROME_FAVICONS, this);
  },

  observe: function FavIcons_observe(subject, topic, data) {
    let value = Services.prefs.getBoolPref(data);

    if (data == this.PREF_CHROME_SITE_ICONS)
      this._prefSiteIcons = value;
    else if (data == this.PREF_CHROME_FAVICONS)
      this._prefFavicons = value;
  },

  
  
  
  getFavIconUrlForTab: function FavIcons_getFavIconUrlForTab(tab, callback) {
    this._isImageDocument(tab, function (isImageDoc) {
      if (isImageDoc) {
        callback(tab.pinned ? tab.image : null);
      } else {
        this._getFavIconForNonImageDocument(tab, callback);
      }
    }.bind(this));
  },

  
  
  
  _getFavIconForNonImageDocument:
    function FavIcons_getFavIconForNonImageDocument(tab, callback) {

    if (tab.image)
      this._getFavIconFromTabImage(tab, callback);
    else if (this._shouldLoadFavIcon(tab))
      this._getFavIconForHttpDocument(tab, callback);
    else
      callback(null);
  },

  
  
  
  _getFavIconFromTabImage:
    function FavIcons_getFavIconFromTabImage(tab, callback) {

    let tabImage = gBrowser.getIcon(tab);

    
    
    if (/^https?:/.test(tabImage)) {
      let tabImageURI = gWindow.makeURI(tabImage);
      tabImage = this._favIconService.getFaviconLinkForIcon(tabImageURI).spec;
    }

    tabImage = PlacesUtils.getImageURLForResolution(window, tabImage);

    callback(tabImage);
  },

  
  
  
  _getFavIconForHttpDocument:
    function FavIcons_getFavIconForHttpDocument(tab, callback) {

    let {currentURI} = tab.linkedBrowser;
    this._favIconService.getFaviconURLForPage(currentURI, function (uri) {
      if (uri) {
        let icon = PlacesUtils.getImageURLForResolution(window,
                     this._favIconService.getFaviconLinkForIcon(uri).spec);
        callback(icon);
      } else {
        callback(this.defaultFavicon);
      }
    }.bind(this));
  },

  
  
  
  _isImageDocument: function UI__isImageDocument(tab, callback) {
    let mm = tab.linkedBrowser.messageManager;
    let message = "Panorama:isImageDocument";

    mm.addMessageListener(message, function onMessage(cx) {
      mm.removeMessageListener(cx.name, onMessage);
      callback(cx.json.isImageDocument);
    });

    mm.sendAsyncMessage(message);
  },

  
  
  
  _shouldLoadFavIcon: function FavIcons_shouldLoadFavIcon(tab) {
    
    if (!this._prefSiteIcons || !this._prefFavicons)
      return false;

    let uri = tab.linkedBrowser.currentURI;

    
    if (!uri || !(uri instanceof Ci.nsIURI))
      return false;

    
    return uri.schemeIs("http") || uri.schemeIs("https");
  }
};
