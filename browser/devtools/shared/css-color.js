



"use strict";

const COLOR_UNIT_PREF = "devtools.defaultColorUnit";
const {Cc, Ci, Cu} = require("chrome");
let {Services} = Cu.import("resource://gre/modules/Services.jsm", {});




































function CssColor(colorValue) {
  this.newColor(colorValue);
}

module.exports.colorUtils = {
  CssColor: CssColor,
  processCSSString: processCSSString
};




CssColor.COLORUNIT = {
  "authored": "authored",
  "hex": "hex",
  "name": "name",
  "rgb": "rgb",
  "hsl": "hsl"
};

CssColor.prototype = {
  authored: null,

  get hasAlpha() {
    if (!this.valid || this.transparent) {
      return false;
    }
    return this._getRGBATuple().a !== 1;
  },

  get valid() {
    return this._validateColor(this.authored);
  },

  get transparent() {
    try {
      let tuple = this._getRGBATuple();
      return tuple === "transparent";
    } catch(e) {
      return false;
    }
  },

  get name() {
    if (!this.valid) {
      return "";
    }
    if (this.authored === "transparent") {
      return "transparent";
    }
    try {
      let tuple = this._getRGBATuple();

      if (tuple === "transparent") {
        return "transparent";
      }
      if (tuple.a !== 1) {
        return this.rgb;
      }
      let {r, g, b} = tuple;
      return DOMUtils.rgbToColorName(r, g, b);
    } catch(e) {
      return this.hex;
    }
  },

  get hex() {
    if (!this.valid) {
      return "";
    }
    if (this.hasAlpha) {
      return this.rgba;
    }
    if (this.transparent) {
      return "transparent";
    }

    let hex = this.longHex;
    if (hex.charAt(1) == hex.charAt(2) &&
        hex.charAt(3) == hex.charAt(4) &&
        hex.charAt(5) == hex.charAt(6)) {
      hex = "#" + hex.charAt(1) + hex.charAt(3) + hex.charAt(5);
    }
    return hex;
  },

  get longHex() {
    if (!this.valid) {
      return "";
    }
    if (this.hasAlpha) {
      return this.rgba;
    }
    if (this.transparent) {
      return "transparent";
    }
    return this.rgb.replace(/\brgb\((\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3})\)/gi, function(_, r, g, b) {
      return "#" + ((1 << 24) + (r << 16) + (g << 8) + (b << 0)).toString(16).substr(-6).toUpperCase();
    });
  },

  get rgb() {
    if (!this.valid) {
      return "";
    }
    if (this.transparent) {
      return "transparent";
    }
    if (!this.hasAlpha) {
      let tuple = this._getRGBATuple();
      return "rgb(" + tuple.r + ", " + tuple.g + ", " + tuple.b + ")";
    }
    return this.rgba;
  },

  get rgba() {
    if (!this.valid) {
      return "";
    }
    if (this.transparent) {
      return "transparent";
    }
    let components = this._getRGBATuple();
    return "rgba(" + components.r + ", " +
                     components.g + ", " +
                     components.b + ", " +
                     components.a + ")";
  },

  get hsl() {
    if (!this.valid) {
      return "";
    }
    if (this.transparent) {
      return "transparent";
    }
    if (this.hasAlpha) {
      return this.hsla;
    }
    return this._hslNoAlpha();
  },

  get hsla() {
    if (!this.valid) {
      return "";
    }
    if (this.transparent) {
      return "transparent";
    }
    
    
    if (this.authored.startsWith("hsla(")) {
      let [, h, s, l, a] = /^\bhsla\(([\d.]+),\s*([\d.]+%),\s*([\d.]+%),\s*([\d.]+|0|1)\)$/gi.exec(this.authored);
      return "hsla(" + h + ", " + s + ", " + l + ", " + a + ")";
    }
    if (this.hasAlpha) {
      let a = this._getRGBATuple().a;
      return this._hslNoAlpha().replace("hsl", "hsla").replace(")", ", " + a + ")");
    }
    return this._hslNoAlpha().replace("hsl", "hsla").replace(")", ", 1)");
  },

  





  newColor: function(color) {
    this.authored = color.toLowerCase();
    return this;
  },

  


  toString: function() {
    let color;
    let defaultUnit = Services.prefs.getCharPref(COLOR_UNIT_PREF);
    let unit = CssColor.COLORUNIT[defaultUnit];

    switch(unit) {
      case CssColor.COLORUNIT.authored:
        color = this.authored;
        break;
      case CssColor.COLORUNIT.hex:
        color = this.hex;
        break;
      case CssColor.COLORUNIT.hsl:
        color = this.hsl;
        break;
      case CssColor.COLORUNIT.name:
        color = this.name;
        break;
      case CssColor.COLORUNIT.rgb:
        color = this.rgb;
        break;
      default:
        color = this.rgb;
    }
    return color;
  },

  



  _getRGBATuple: function() {
    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;
    let span = doc.createElement("span");
    span.style.color = this.authored;
    let computed = win.getComputedStyle(span).color;

    if (computed === "transparent") {
      return "transparent";
    }

    let rgba = /^rgba\((\d+),\s*(\d+),\s*(\d+),\s*(\d+\.\d+|1|0)\)$/gi.exec(computed);

    if (rgba) {
      let [, r, g, b, a] = rgba;
      return {r: r, g: g, b: b, a: a};
    } else {
      let rgb = /^rgb\((\d+),\s*(\d+),\s*(\d+)\)$/gi.exec(computed);
      let [, r, g, b] = rgb;

      return {r: r, g: g, b: b, a: 1};
    }
  },

  _hslNoAlpha: function() {
    let {r, g, b} = this._getRGBATuple();

    
    
    if (this.authored.startsWith("hsl(")) {
      let [, h, s, l] = /^\bhsl\(([\d.]+),\s*([\d.]+%),\s*([\d.]+%)\)$/gi.exec(this.authored);
      return "hsl(" + h + ", " + s + ", " + l + ")";
    }

    r = r / 255;
    g = g / 255;
    b = b / 255;

    let max = Math.max(r, g, b);
    let min = Math.min(r, g, b);
    let h;
    let s;
    let l = (max + min) / 2;

    if(max == min){
      h = s = 0;
    } else {
      let d = max - min;
      s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

      switch(max) {
          case r:
            h = ((g - b) / d) % 6;
            break;
          case g:
            h = (b - r) / d + 2;
            break;
          case b:
            h = (r - g) / d + 4;
            break;
      }
      h *= 60;
      if (h < 0) {
        h += 360;
      }
    }
    return "hsl(" + (Math.round(h * 1000)) / 1000 +
           ", " + Math.round(s * 100) +
           "%, " + Math.round(l * 100) + "%)";
  },

  


  valueOf: function() {
    return this.rgba;
  },

  _validateColor: function(color) {
    if (typeof color !== "string" || color === "") {
      return false;
    }

    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;

    
    let span = doc.createElement("span");
    span.style.color = "rgb(0, 0, 0)";

    
    
    span.style.color = color;
    if (span.style.color !== "rgb(0, 0, 0)") {
      return true;
    }

    
    
    
    span.style.color = "rgb(255, 255, 255)";
    span.style.color = color;
    return span.style.color !== "rgb(255, 255, 255)";
  },
};









function processCSSString(value) {
  if (value && /^""$/.test(value)) {
    return value;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  let colorPattern = /#[0-9a-fA-F]{3}\b|#[0-9a-fA-F]{6}\b|hsl\(.*?\)|hsla\(.*?\)|rgba?\(.*?\)|\b[a-zA-Z-]+\b/g;

  value = value.replace(colorPattern, function(match) {
    let color = new CssColor(match);
    if (color.valid) {
      return color;
    }
    return match;
  });
  return value;
}

loader.lazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
