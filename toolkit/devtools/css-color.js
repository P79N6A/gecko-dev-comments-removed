



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

const COLOR_UNIT_PREF = "devtools.defaultColorUnit";

const REGEX_JUST_QUOTES  = /^""$/;
const REGEX_HSL_3_TUPLE  = /^\bhsl\(([\d.]+),\s*([\d.]+%),\s*([\d.]+%)\)$/i;















const REGEX_ALL_COLORS = /#[0-9a-fA-F]{3}\b|#[0-9a-fA-F]{6}\b|hsl\(.*?\)|hsla\(.*?\)|rgba?\(.*?\)|\b[a-zA-Z-]+\b/g;

const SPECIALVALUES = new Set([
  "currentcolor",
  "initial",
  "inherit",
  "transparent",
  "unset"
]);





































function CssColor(colorValue) {
  this.newColor(colorValue);
}

module.exports.colorUtils = {
  CssColor: CssColor,
  processCSSString: processCSSString,
  rgbToHsl: rgbToHsl,
  setAlpha: setAlpha
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
    if (!this.valid) {
      return false;
    }
    return this._getRGBATuple().a !== 1;
  },

  get valid() {
    return DOMUtils.isValidCSSColor(this.authored);
  },

  


  get transparent() {
    try {
      let tuple = this._getRGBATuple();
      return !(tuple.r || tuple.g || tuple.b || tuple.a);
    } catch(e) {
      return false;
    }
  },

  get specialValue() {
    return SPECIALVALUES.has(this.authored) ? this.authored : null;
  },

  get name() {
    let invalidOrSpecialValue = this._getInvalidOrSpecialValue();
    if (invalidOrSpecialValue !== false) {
      return invalidOrSpecialValue;
    }

    try {
      let tuple = this._getRGBATuple();

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
    let invalidOrSpecialValue = this._getInvalidOrSpecialValue();
    if (invalidOrSpecialValue !== false) {
      return invalidOrSpecialValue;
    }
    if (this.hasAlpha) {
      return this.rgba;
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
    let invalidOrSpecialValue = this._getInvalidOrSpecialValue();
    if (invalidOrSpecialValue !== false) {
      return invalidOrSpecialValue;
    }
    if (this.hasAlpha) {
      return this.rgba;
    }
    return this.rgb.replace(/\brgb\((\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3})\)/gi, function(_, r, g, b) {
      return "#" + ((1 << 24) + (r << 16) + (g << 8) + (b << 0)).toString(16).substr(-6).toUpperCase();
    });
  },

  get rgb() {
    let invalidOrSpecialValue = this._getInvalidOrSpecialValue();
    if (invalidOrSpecialValue !== false) {
      return invalidOrSpecialValue;
    }
    if (!this.hasAlpha) {
      if (this.authored.startsWith("rgb(")) {
        
        return this.authored;
      }
      let tuple = this._getRGBATuple();
      return "rgb(" + tuple.r + ", " + tuple.g + ", " + tuple.b + ")";
    }
    return this.rgba;
  },

  get rgba() {
    let invalidOrSpecialValue = this._getInvalidOrSpecialValue();
    if (invalidOrSpecialValue !== false) {
      return invalidOrSpecialValue;
    }
    if (this.authored.startsWith("rgba(")) {
      
        return this.authored;
    }
    let components = this._getRGBATuple();
    return "rgba(" + components.r + ", " +
                     components.g + ", " +
                     components.b + ", " +
                     components.a + ")";
  },

  get hsl() {
    let invalidOrSpecialValue = this._getInvalidOrSpecialValue();
    if (invalidOrSpecialValue !== false) {
      return invalidOrSpecialValue;
    }
    if (this.authored.startsWith("hsl(")) {
      
      return this.authored;
    }
    if (this.hasAlpha) {
      return this.hsla;
    }
    return this._hslNoAlpha();
  },

  get hsla() {
    let invalidOrSpecialValue = this._getInvalidOrSpecialValue();
    if (invalidOrSpecialValue !== false) {
      return invalidOrSpecialValue;
    }
    if (this.authored.startsWith("hsla(")) {
      
      return this.authored;
    }
    if (this.hasAlpha) {
      let a = this._getRGBATuple().a;
      return this._hslNoAlpha().replace("hsl", "hsla").replace(")", ", " + a + ")");
    }
    return this._hslNoAlpha().replace("hsl", "hsla").replace(")", ", 1)");
  },

  










  _getInvalidOrSpecialValue: function() {
    if (this.specialValue) {
      return this.specialValue;
    }
    if (!this.valid) {
      return "";
    }
    return false;
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
    let tuple = DOMUtils.colorToRGBA(this.authored);

    tuple.a = parseFloat(tuple.a.toFixed(1));

    return tuple;
  },

  _hslNoAlpha: function() {
    let {r, g, b} = this._getRGBATuple();

    if (this.authored.startsWith("hsl(")) {
      
      
      let [, h, s, l] = this.authored.match(REGEX_HSL_3_TUPLE);
      return "hsl(" + h + ", " + s + ", " + l + ")";
    }

    let [h,s,l] = rgbToHsl([r,g,b]);

    return "hsl(" + h + ", " + s + "%, " + l + "%)";
  },

  


  valueOf: function() {
    return this.rgba;
  },
};









function processCSSString(value) {
  if (value && REGEX_JUST_QUOTES.test(value)) {
    return value;
  }

  let colorPattern = REGEX_ALL_COLORS;

  value = value.replace(colorPattern, function(match) {
    let color = new CssColor(match);
    if (color.valid) {
      return color;
    }
    return match;
  });
  return value;
}









function rgbToHsl([r,g,b]) {
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

  return [Math.round(h), Math.round(s * 100), Math.round(l * 100)];
}













function setAlpha(colorValue, alpha) {
  let color = new CssColor(colorValue);

  
  if (!color.valid) {
    throw new Error("Invalid color.");
  }

  
  if (!(alpha >= 0 && alpha <= 1)) {
    alpha = 1;
  }

  let { r, g, b } = color._getRGBATuple();
  return "rgba(" + r + ", " + g + ", " + b + ", " + alpha + ")";
}

loader.lazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
