



































let EXPORTED_SYMBOLS = ["LightweightThemeConsumer"];

function LightweightThemeConsumer(aDocument) {
  this._doc = aDocument;
  this._footerId = aDocument.documentElement.getAttribute("lightweightthemesfooter");

  Components.classes["@mozilla.org/observer-service;1"]
            .getService(Components.interfaces.nsIObserverService)
            .addObserver(this, "lightweight-theme-changed", false);

  var temp = {};
  Components.utils.import("resource://gre/modules/LightweightThemeManager.jsm", temp);
  this._update(temp.LightweightThemeManager.currentTheme);
}

LightweightThemeConsumer.prototype = {
  observe: function (aSubject, aTopic, aData) {
    if (aTopic != "lightweight-theme-changed")
      return;

    this._update(JSON.parse(aData));
  },

  destroy: function () {
    Components.classes["@mozilla.org/observer-service;1"]
              .getService(Components.interfaces.nsIObserverService)
              .removeObserver(this, "lightweight-theme-changed");

    this._doc = null;
  },

  _update: function (aData) {
    if (!aData)
      aData = { headerURL: "", footerURL: "", textColor: "", dominantColor: "" };

    var root = this._doc.documentElement;
    var active = !!aData.headerURL;

    if (active) {
      root.style.color = aData.textColor || "black";
      let [r, g, b] = _parseRGB(this._doc.defaultView.getComputedStyle(root, "").color);
      let brightness = (r + g + b) / 3;
      root.setAttribute("lwthemetextcolor", brightness <= 127 ? "dark" : "bright");
      root.setAttribute("lwtheme", "true");
    } else {
      root.style.color = "";
      root.removeAttribute("lwthemetextcolor");
      root.removeAttribute("lwtheme");
    }

    _setImage(root, active, aData.headerURL);
    if (this._footerId) {
      let footer = this._doc.getElementById(this._footerId);
      _setImage(footer, active, aData.footerURL);
      if (active && aData.footerURL)
        footer.setAttribute("lwthemefooter", "true");
      else
        footer.removeAttribute("lwthemefooter");
    }

    if (root.hasAttribute("activetitlebarcolor")) {
      if (!root.hasAttribute("originalactivetitlebarcolor")) {
        root.setAttribute("originalactivetitlebarcolor",
                          root.getAttribute("activetitlebarcolor"));
        root.setAttribute("originalinactivetitlebarcolor",
                          root.getAttribute("inactivetitlebarcolor"));
      }
      root.setAttribute("activetitlebarcolor", aData.dominantColor
                          || root.getAttribute("originalactivetitlebarcolor"));
      root.setAttribute("inactivetitlebarcolor", aData.dominantColor
                          || root.getAttribute("originalinactivetitlebarcolor"));
    }
  }
}

function _setImage(aElement, aActive, aURL) {
  aElement.style.backgroundImage =
    (aActive && aURL) ? 'url("' + aURL.replace('"', '\\"', "g") + '")' : "";
}

function _parseRGB(aColorString) {
  var rgb = aColorString.match(/^rgba?\((\d+), (\d+), (\d+)/);
  rgb.shift();
  return rgb.map(function (x) parseInt(x));
}
