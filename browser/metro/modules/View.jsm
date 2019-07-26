


"use strict";

this.EXPORTED_SYMBOLS = ["View"];

Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
Components.utils.import("resource:///modules/colorUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");





function makeURI(aURL, aOriginCharset, aBaseURI) {
  return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
}







function View(aSet) {
  this._set = aSet;
  this._set.controller = this;

  this.viewStateObserver = {
    observe: (aSubject, aTopic, aData) => this._adjustDOMforViewState(aData)
  };
  Services.obs.addObserver(this.viewStateObserver, "metro_viewstate_changed", false);

  this._adjustDOMforViewState();
}

View.prototype = {
  destruct: function () {
    Services.obs.removeObserver(this.viewStateObserver, "metro_viewstate_changed");
  },

  _adjustDOMforViewState: function _adjustDOMforViewState(aState) {
    if (this._set) {
      if (undefined == aState)
        aState = this._set.getAttribute("viewstate");

      this._set.setAttribute("suppressonselect", (aState == "snapped"));

      if (aState == "portrait") {
        this._set.setAttribute("vertical", true);
      } else {
        this._set.removeAttribute("vertical");
      }

      this._set.arrangeItems();
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
    aItem.iconSrc = aIconUri.spec;
    let faviconURL = (PlacesUtils.favicons.getFaviconLinkForIcon(aIconUri)).spec;
    let xpFaviconURI = makeURI(faviconURL.replace("moz-anno:favicon:",""));
    let successAction = function(foreground, background) {
      aItem.style.color = foreground; 
      aItem.setAttribute("customColor", background);
      let matteColor =  0xffffff; 
      let alpha = 0.04; 
      let [,r,g,b] = background.match(/rgb\((\d+),(\d+),(\d+)/);
      
      let tintColor = ColorUtils.addRgbColors(matteColor, ColorUtils.createDecimalColorWord(r,g,b,alpha));
      aItem.setAttribute("tintColor", ColorUtils.convertDecimalToRgbColor(tintColor));

      if (aItem.refresh) {
        aItem.refresh();
      }
    };
    let failureAction = function() {};
    ColorUtils.getForegroundAndBackgroundIconColors(xpFaviconURI, successAction, failureAction);
  }

};
