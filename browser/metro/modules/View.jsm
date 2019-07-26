


"use strict";

this.EXPORTED_SYMBOLS = ["View"];

Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
Components.utils.import("resource:///modules/colorUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");





function makeURI(aURL, aOriginCharset, aBaseURI) {
  return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
}







function View(aSet) {
  this._set = aSet;
  this._set.controller = this;
  this._window = aSet.ownerDocument.defaultView;
  this._maxTiles = 8;
  this._tilePrefName = "unknown";

  this.onResize = () => this._adjustDOMforViewState();
  this._window.addEventListener("resize", this.onResize);

  ColorUtils.init();
  this._adjustDOMforViewState();
}

View.prototype = {
  set maxTiles(aVal) {
    this._maxTiles = aVal;
  },

  get maxTiles() {
    return this._maxTiles;
  },

  set showing(aFlag) {
    
    this.vbox.setAttribute("hidden", aFlag ? "false" : "true");
  },

  set tilePrefName(aStr) {
    
    this._tilePrefName = aStr;
    this._maxTiles = Services.prefs.getIntPref(this._tilePrefName);
    Services.prefs.addObserver(this._tilePrefName, this, false);
  },

  destruct: function () {
    this._window.removeEventListener("resize", this.onResize);
    if (this._tilePrefName != "unknown") {
      Services.prefs.removeObserver(this._tilePrefName, this);
    }
  },

  _adjustDOMforViewState: function _adjustDOMforViewState(aState) {
    let grid = this._set;
    if (!grid) {
      return;
    }
    if (!aState) {
      aState = grid.getAttribute("viewstate");
    }
    switch (aState) {
      case "snapped":
        grid.setAttribute("nocontext", true);
        grid.selectNone();
        grid.disableCrossSlide();
        break;
      case "portrait":
        grid.removeAttribute("nocontext");
        grid.setAttribute("vertical", true);
        grid.enableCrossSlide();
        break;
      default:
        grid.removeAttribute("nocontext");
        grid.removeAttribute("vertical");
        grid.enableCrossSlide();
    }
    if ("arrangeItems" in grid) {
      grid.arrangeItems();
    }
  },

  _updateFavicon: function pv__updateFavicon(aItem, aUri) {
    if ("string" == typeof aUri) {
      aUri = makeURI(aUri);
    }
    PlacesUtils.favicons.getFaviconURLForPage(aUri, this._gotIcon.bind(this, aItem));
  },

  _gotIcon: function pv__gotIcon(aItem, aIconUri) {
    if (!aIconUri) {
      aItem.removeAttribute("iconURI");
      if (aItem.refresh) {
        aItem.refresh();
      }
      return;
    }
    if ("string" == typeof aIconUri) {
      aIconUri = makeURI(aIconUri);
    }
    let faviconURL = (PlacesUtils.favicons.getFaviconLinkForIcon(aIconUri)).spec;
    aItem.iconSrc = faviconURL;

    let xpFaviconURI = makeURI(faviconURL.replace("moz-anno:favicon:",""));
    Task.spawn(function() {
      let colorInfo = yield ColorUtils.getForegroundAndBackgroundIconColors(xpFaviconURI);
      if (!(colorInfo && colorInfo.background && colorInfo.foreground)) {
        return;
      }
      let { background, foreground } = colorInfo;
      aItem.style.color = foreground; 
      aItem.setAttribute("customColor", background);
      let matteColor =  0xffffff; 
      let alpha = 0.04; 
      let [,r,g,b] = background.match(/rgb\((\d+),(\d+),(\d+)/);
      
      let tintColor = ColorUtils.addRgbColors(matteColor, ColorUtils.createDecimalColorWord(r,g,b,alpha));
      aItem.setAttribute("tintColor", ColorUtils.convertDecimalToRgbColor(tintColor));
      
      if ('color' in aItem) {
        aItem.color = background;
      }
    });
  },

  refreshView: function () {
  },

  observe: function (aSubject, aTopic, aState) {
    switch (aTopic) {
      case "nsPref:changed": {
        if (aState == this._tilePrefName) {
          let count = Services.prefs.getIntPref(this._tilePrefName);
          this.maxTiles = count;
          this.showing = this.maxTiles > 0;
          this.refreshView();
        }
        break;
      }
    }
  }
};
