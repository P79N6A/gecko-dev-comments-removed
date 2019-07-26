



"use strict";

const {Cc, Cu, Ci} = require("chrome");
const promise = require("sdk/core/promise");
const IOService = Cc["@mozilla.org/network/io-service;1"]
  .getService(Ci.nsIIOService);
const {Spectrum} = require("devtools/shared/widgets/Spectrum");
const EventEmitter = require("devtools/shared/event-emitter");
const {colorUtils} = require("devtools/css-color");
const Heritage = require("sdk/core/heritage");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const GRADIENT_RE = /\b(repeating-)?(linear|radial)-gradient\(((rgb|hsl)a?\(.+?\)|[^\)])+\)/gi;
const BORDERCOLOR_RE = /^border-[-a-z]*color$/ig;
const BORDER_RE = /^border(-(top|bottom|left|right))?$/ig;
const BACKGROUND_IMAGE_RE = /url\([\'\"]?(.*?)[\'\"]?\)/;
const XHTML_NS = "http://www.w3.org/1999/xhtml";
const SPECTRUM_FRAME = "chrome://browser/content/devtools/spectrum-frame.xhtml";
const ESCAPE_KEYCODE = Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE;
const ENTER_KEYCODE = Ci.nsIDOMKeyEvent.DOM_VK_RETURN;





























function OptionsStore(defaults, options) {
  this.defaults = defaults || {};
  this.options = options || {};
}

OptionsStore.prototype = {
  





  get: function(name) {
    if (typeof this.options[name] !== "undefined") {
      return this.options[name];
    } else {
      return this.defaults[name];
    }
  }
};




let PanelFactory = {
  






  get: function(doc, options) {
    
    let panel = doc.createElement("panel");
    panel.setAttribute("hidden", true);
    panel.setAttribute("ignorekeys", true);

    
    panel.setAttribute("consumeoutsideclicks", options.get("consumeOutsideClick"));
    panel.setAttribute("noautofocus", options.get("noAutoFocus"));
    panel.setAttribute("type", "arrow");
    panel.setAttribute("level", "top");

    panel.setAttribute("class", "devtools-tooltip theme-tooltip-panel");
    doc.querySelector("window").appendChild(panel);

    return panel;
  }
};








































function Tooltip(doc, options) {
  EventEmitter.decorate(this);

  this.doc = doc;
  this.options = new OptionsStore({
    consumeOutsideClick: false,
    closeOnKeys: [ESCAPE_KEYCODE],
    noAutoFocus: true
  }, options);
  this.panel = PanelFactory.get(doc, this.options);

  
  this.uid = "tooltip-" + Date.now();

  
  for (let event of ["shown", "hidden", "showing", "hiding"]) {
    this["_onPopup" + event] = ((e) => {
      return () => this.emit(e);
    })(event);
    this.panel.addEventListener("popup" + event,
      this["_onPopup" + event], false);
  }

  
  let win = this.doc.querySelector("window");
  this._onKeyPress = event => {
    this.emit("keypress", event.keyCode);
    if (this.options.get("closeOnKeys").indexOf(event.keyCode) !== -1) {
      this.hide();
    }
  };
  win.addEventListener("keypress", this._onKeyPress, false);
}

module.exports.Tooltip = Tooltip;

Tooltip.prototype = {
  defaultPosition: "before_start",
  defaultOffsetX: 0,
  defaultOffsetY: 0,

  









  show: function(anchor,
    position = this.defaultPosition,
    x = this.defaultOffsetX,
    y = this.defaultOffsetY) {
    this.panel.hidden = false;
    this.panel.openPopup(anchor, position, x, y);
  },

  


  hide: function() {
    this.panel.hidden = true;
    this.panel.hidePopup();
  },

  isShown: function() {
    return this.panel.state !== "closed" && this.panel.state !== "hiding";
  },

  


  empty: function() {
    while (this.panel.hasChildNodes()) {
      this.panel.removeChild(this.panel.firstChild);
    }
  },

  


  destroy: function () {
    this.hide();

    for (let event of ["shown", "hidden", "showing", "hiding"]) {
      this.panel.removeEventListener("popup" + event,
        this["_onPopup" + event], false);
    }

    let win = this.doc.querySelector("window");
    win.removeEventListener("keypress", this._onKeyPress, false);

    this.content = null;

    this.doc = null;

    this.panel.parentNode.removeChild(this.panel);
    this.panel = null;

    if (this._basedNode) {
      this.stopTogglingOnHover();
    }
  },

  































  startTogglingOnHover: function(baseNode, targetNodeCb, showDelay = 750) {
    if (this._basedNode) {
      this.stopTogglingOnHover();
    }

    
    
    this._hideOnMouseLeave = !targetNodeCb;

    this._basedNode = baseNode;
    this._showDelay = showDelay;
    this._targetNodeCb = targetNodeCb || (() => true);

    this._onBaseNodeMouseMove = this._onBaseNodeMouseMove.bind(this);
    this._onBaseNodeMouseLeave = this._onBaseNodeMouseLeave.bind(this);

    baseNode.addEventListener("mousemove", this._onBaseNodeMouseMove, false);
    baseNode.addEventListener("mouseleave", this._onBaseNodeMouseLeave, false);
  },

  




  stopTogglingOnHover: function() {
    clearNamedTimeout(this.uid);

    this._basedNode.removeEventListener("mousemove",
      this._onBaseNodeMouseMove, false);
    this._basedNode.removeEventListener("mouseleave",
      this._onBaseNodeMouseLeave, false);

    this._basedNode = null;
    this._targetNodeCb = null;
    this._lastHovered = null;
  },

  _onBaseNodeMouseMove: function(event) {
    if (event.target !== this._lastHovered) {
      this.hide();
      this._lastHovered = null;
      setNamedTimeout(this.uid, this._showDelay, () => {
        this._showOnHover(event.target);
      });
    }
  },

  _showOnHover: function(target) {
    if (this._targetNodeCb(target, this)) {
      this.show(target);
      this._lastHovered = target;
    }
  },

  _onBaseNodeMouseLeave: function() {
    clearNamedTimeout(this.uid);
    this._lastHovered = null;
    if (this._hideOnMouseLeave) {
      this.hide();
    }
  },

  






  set content(content) {
    this.empty();
    if (content) {
      this.panel.appendChild(content);
    }
  },

  get content() {
    return this.panel.firstChild;
  },

  





  setTextContent: function(...messages) {
    let vbox = this.doc.createElement("vbox");
    vbox.className = "devtools-tooltip-simple-text-container";
    vbox.setAttribute("flex", "1");

    for (let text of messages) {
      let description = this.doc.createElement("description");
      description.setAttribute("flex", "1");
      description.className = "devtools-tooltip-simple-text";
      description.textContent = text;
      vbox.appendChild(description);
    }

    this.content = vbox;
  },

  















  setImageContent: function(imageUrl, options={}) {
    
    let vbox = this.doc.createElement("vbox");
    vbox.setAttribute("align", "center")

    
    let image = this.doc.createElement("image");
    image.setAttribute("src", imageUrl);
    if (options.maxDim) {
      image.style.maxWidth = options.maxDim + "px";
      image.style.maxHeight = options.maxDim + "px";
    }
    vbox.appendChild(image);

    
    let label = this.doc.createElement("label");
    label.classList.add("devtools-tooltip-caption");
    label.classList.add("theme-comment");
    label.textContent = l10n.strings.GetStringFromName("previewTooltip.image.brokenImage");
    vbox.appendChild(label);

    this.content = vbox;

    
    let imgObj = new this.doc.defaultView.Image();
    imgObj.src = imageUrl;
    imgObj.onload = () => {
      imgObj.onload = null;

      
      let w = options.naturalWidth || imgObj.naturalWidth;
      let h = options.naturalHeight || imgObj.naturalHeight;
      label.textContent = w + " x " + h;
    }
  },

  



  setCssBackgroundImageContent: function(cssBackground, sheetHref, maxDim=400) {
    let uri = getBackgroundImageUri(cssBackground, sheetHref);
    if (uri) {
      this.setImageContent(uri, {
        maxDim: maxDim
      });
    }
  },

  




  setColorPickerContent: function(color) {
    let def = promise.defer();

    
    let iframe = this.doc.createElementNS(XHTML_NS, "iframe");
    iframe.setAttribute("transparent", true);
    iframe.setAttribute("width", "210");
    iframe.setAttribute("height", "195");
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("class", "devtools-tooltip-iframe");

    let panel = this.panel;
    let xulWin = this.doc.ownerGlobal;

    
    function onLoad() {
      iframe.removeEventListener("load", onLoad, true);
      let win = iframe.contentWindow.wrappedJSObject;

      let container = win.document.getElementById("spectrum");
      let spectrum = new Spectrum(container, color);

      
      panel.addEventListener("popupshown", function shown() {
        panel.removeEventListener("popupshown", shown, true);
        spectrum.show();
        def.resolve(spectrum);
      }, true);
    }
    iframe.addEventListener("load", onLoad, true);
    iframe.setAttribute("src", SPECTRUM_FRAME);

    
    this.content = iframe;

    return def.promise;
  }
};







function SwatchBasedEditorTooltip(doc) {
  
  
  
  
  this.tooltip = new Tooltip(doc, {
    consumeOutsideClick: true,
    closeOnKeys: [ESCAPE_KEYCODE, ENTER_KEYCODE],
    noAutoFocus: false
  });

  
  
  this._onTooltipKeypress = (event, code) => {
    if (code === ESCAPE_KEYCODE) {
      this.revert();
    } else if (code === ENTER_KEYCODE) {
      this.commit();
    }
  };
  this.tooltip.on("keypress", this._onTooltipKeypress);

  
  this.swatches = new Map();

  
  
  
  this.activeSwatch = null;

  this._onSwatchClick = this._onSwatchClick.bind(this);
}

SwatchBasedEditorTooltip.prototype = {
  show: function() {
    if (this.activeSwatch) {
      this.tooltip.show(this.activeSwatch, "topcenter bottomleft");
    }
  },

  hide: function() {
    this.tooltip.hide();
  },

  














  addSwatch: function(swatchEl, callbacks={}, originalValue) {
    if (!callbacks.onPreview) callbacks.onPreview = function() {};
    if (!callbacks.onRevert) callbacks.onRevert = function() {};
    if (!callbacks.onCommit) callbacks.onCommit = function() {};

    this.swatches.set(swatchEl, {
      callbacks: callbacks,
      originalValue: originalValue
    });
    swatchEl.addEventListener("click", this._onSwatchClick, false);
  },

  removeSwatch: function(swatchEl) {
    if (this.swatches.has(swatchEl)) {
      if (this.activeSwatch === swatchEl) {
        this.hide();
        this.activeSwatch = null;
      }
      swatchEl.removeEventListener("click", this._onSwatchClick, false);
      this.swatches.delete(swatchEl);
    }
  },

  _onSwatchClick: function(event) {
    let swatch = this.swatches.get(event.target);
    if (swatch) {
      this.activeSwatch = event.target;
      this.show();
      event.stopPropagation();
    }
  },

  


  preview: function(value) {
    if (this.activeSwatch) {
      let swatch = this.swatches.get(this.activeSwatch);
      swatch.callbacks.onPreview(value);
    }
  },

  


  revert: function() {
    if (this.activeSwatch) {
      let swatch = this.swatches.get(this.activeSwatch);
      swatch.callbacks.onRevert(swatch.originalValue);
    }
  },

  


  commit: function() {
    if (this.activeSwatch) {
      let swatch = this.swatches.get(this.activeSwatch);
      swatch.callbacks.onCommit();
    }
  },

  destroy: function() {
    this.swatches.clear();
    this.activeSwatch = null;
    this.tooltip.off("keypress", this._onTooltipKeypress);
    this.tooltip.destroy();
  }
};










function SwatchColorPickerTooltip(doc) {
  SwatchBasedEditorTooltip.call(this, doc);

  
  
  this.spectrum = this.tooltip.setColorPickerContent([0, 0, 0, 1]);
  this._onSpectrumColorChange = this._onSpectrumColorChange.bind(this);
}

module.exports.SwatchColorPickerTooltip = SwatchColorPickerTooltip;

SwatchColorPickerTooltip.prototype = Heritage.extend(SwatchBasedEditorTooltip.prototype, {
  



  show: function() {
    
    SwatchBasedEditorTooltip.prototype.show.call(this);
    
    if (this.activeSwatch) {
      let swatch = this.swatches.get(this.activeSwatch);
      let color = this.activeSwatch.style.backgroundColor;
      this.spectrum.then(spectrum => {
        spectrum.off("changed", this._onSpectrumColorChange);
        spectrum.rgb = this._colorToRgba(color);
        spectrum.on("changed", this._onSpectrumColorChange);
        spectrum.updateUI();
      });
    }
  },

  _onSpectrumColorChange: function(event, rgba, cssColor) {
    if (this.activeSwatch) {
      this.activeSwatch.style.backgroundColor = cssColor;
      this.activeSwatch.nextSibling.textContent = cssColor;
      this.preview(cssColor);
    }
  },

  _colorToRgba: function(color) {
    color = new colorUtils.CssColor(color);
    let rgba = color._getRGBATuple();
    return [rgba.r, rgba.g, rgba.b, rgba.a];
  },

  destroy: function() {
    SwatchBasedEditorTooltip.prototype.destroy.call(this);
    this.spectrum.then(spectrum => {
      spectrum.off("changed", this._onSpectrumColorChange);
      spectrum.destroy();
    });
  }
});




function isGradientRule(property, value) {
  return (property === "background" || property === "background-image") &&
    value.match(GRADIENT_RE);
}




function isColorOnly(property, value) {
  return property === "background-color" ||
         property === "color" ||
         property.match(BORDERCOLOR_RE);
}




function getBackgroundImageUri(value, sheetHref) {
  let uriMatch = BACKGROUND_IMAGE_RE.exec(value);
  let uri = null;

  if (uriMatch && uriMatch[1]) {
    uri = uriMatch[1];
    if (sheetHref) {
      let sheetUri = IOService.newURI(sheetHref, null, null);
      uri = sheetUri.resolve(uri);
    }
  }

  return uri;
}




function L10N() {}
L10N.prototype = {};

let l10n = new L10N();

loader.lazyGetter(L10N.prototype, "strings", () => {
  return Services.strings.createBundle(
    "chrome://browser/locale/devtools/inspector.properties");
});
