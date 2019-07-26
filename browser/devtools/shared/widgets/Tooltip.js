



"use strict";

const {Cc, Cu, Ci} = require("chrome");
const promise = require("sdk/core/promise");
const IOService = Cc["@mozilla.org/network/io-service;1"]
  .getService(Ci.nsIIOService);
const {Spectrum} = require("devtools/shared/widgets/Spectrum");
const EventEmitter = require("devtools/toolkit/event-emitter");
const {colorUtils} = require("devtools/css-color");
const Heritage = require("sdk/core/heritage");
const {CSSTransformPreviewer} = require("devtools/shared/widgets/CSSTransformPreviewer");
const {Eyedropper} = require("devtools/eyedropper/eyedropper");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "setNamedTimeout",
  "resource:///modules/devtools/ViewHelpers.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "clearNamedTimeout",
  "resource:///modules/devtools/ViewHelpers.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "VariablesView",
  "resource:///modules/devtools/VariablesView.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "VariablesViewController",
  "resource:///modules/devtools/VariablesViewController.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

const GRADIENT_RE = /\b(repeating-)?(linear|radial)-gradient\(((rgb|hsl)a?\(.+?\)|[^\)])+\)/gi;
const BORDERCOLOR_RE = /^border-[-a-z]*color$/ig;
const BORDER_RE = /^border(-(top|bottom|left|right))?$/ig;
const XHTML_NS = "http://www.w3.org/1999/xhtml";
const SPECTRUM_FRAME = "chrome://browser/content/devtools/spectrum-frame.xhtml";
const ESCAPE_KEYCODE = Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE;
const RETURN_KEYCODE = Ci.nsIDOMKeyEvent.DOM_VK_RETURN;
const POPUP_EVENTS = ["shown", "hidden", "showing", "hiding"];
const FONT_FAMILY_PREVIEW_TEXT = "(ABCabc123&@%)";





























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
    panel.setAttribute("animate", false);

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
    noAutoFocus: true,
    closeOnEvents: []
  }, options);
  this.panel = PanelFactory.get(doc, this.options);

  
  this.uid = "tooltip-" + Date.now();

  
  for (let event of POPUP_EVENTS) {
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
      if (!this.panel.hidden) {
        event.stopPropagation();
      }
      this.hide();
    }
  };
  win.addEventListener("keypress", this._onKeyPress, false);

  
  this.hide = this.hide.bind(this);
  let closeOnEvents = this.options.get("closeOnEvents");
  for (let {emitter, event, useCapture} of closeOnEvents) {
    for (let add of ["addEventListener", "on"]) {
      if (add in emitter) {
        emitter[add](event, this.hide, useCapture);
        break;
      }
    }
  }
}

module.exports.Tooltip = Tooltip;

Tooltip.prototype = {
  defaultPosition: "before_start",
  defaultOffsetX: 0, 
  defaultOffsetY: 0, 
  defaultShowDelay: 50, 

  











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

  setSize: function(width, height) {
    this.panel.sizeTo(width, height);
  },

  


  empty: function() {
    while (this.panel.hasChildNodes()) {
      this.panel.removeChild(this.panel.firstChild);
    }
  },

  



  isHidden: function() {
    return this.panel.state == "closed" || this.panel.state == "hiding";
  },

  



  isEmpty: function() {
    return !this.panel.hasChildNodes();
  },

  


  destroy: function () {
    this.hide();

    for (let event of POPUP_EVENTS) {
      this.panel.removeEventListener("popup" + event,
        this["_onPopup" + event], false);
    }

    let win = this.doc.querySelector("window");
    win.removeEventListener("keypress", this._onKeyPress, false);

    let closeOnEvents = this.options.get("closeOnEvents");
    for (let {emitter, event, useCapture} of closeOnEvents) {
      for (let remove of ["removeEventListener", "off"]) {
        if (remove in emitter) {
          emitter[remove](event, this.hide, useCapture);
          break;
        }
      }
    }

    this.content = null;

    if (this._basedNode) {
      this.stopTogglingOnHover();
    }

    this.doc = null;

    this.panel.remove();
    this.panel = null;
  },

  
































  startTogglingOnHover: function(baseNode, targetNodeCb, showDelay=this.defaultShowDelay) {
    if (this._basedNode) {
      this.stopTogglingOnHover();
    }

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
      this._lastHovered = event.target;
      setNamedTimeout(this.uid, this._showDelay, () => {
        this.isValidHoverTarget(event.target).then(target => {
          this.show(target);
        });
      });
    }
  },

  





  isValidHoverTarget: function(target) {
    
    
    let res = this._targetNodeCb(target, this);

    
    
    if (res && res.then) {
      return res.then(arg => {
        return arg instanceof Ci.nsIDOMNode ? arg : target;
      }, () => {
        return false;
      });
    } else {
      let newTarget = res instanceof Ci.nsIDOMNode ? res : target;
      return res ? promise.resolve(newTarget) : promise.reject(false);
    }
  },

  _onBaseNodeMouseLeave: function() {
    clearNamedTimeout(this.uid);
    this._lastHovered = null;
    this.hide();
  },

  






  set content(content) {
    if (this.content == content) {
      return;
    }

    this.empty();
    this.panel.removeAttribute("clamped-dimensions");

    if (content) {
      this.panel.appendChild(content);
    }
  },

  get content() {
    return this.panel.firstChild;
  },

  











  setTextContent: function(
    {
      messages,
      messagesClass,
      containerClass,
      isAlertTooltip
    },
    extraButtons = []) {
    messagesClass = messagesClass || "default-tooltip-simple-text-colors";
    containerClass = containerClass || "default-tooltip-simple-text-colors";

    let vbox = this.doc.createElement("vbox");
    vbox.className = "devtools-tooltip-simple-text-container " + containerClass;
    vbox.setAttribute("flex", "1");

    for (let text of messages) {
      let description = this.doc.createElement("description");
      description.setAttribute("flex", "1");
      description.className = "devtools-tooltip-simple-text " + messagesClass;
      description.textContent = text;
      vbox.appendChild(description);
    }

    for (let { label, className, command } of extraButtons) {
      let button = this.doc.createElement("button");
      button.className = className;
      button.setAttribute("label", label);
      button.addEventListener("command", command);
      vbox.appendChild(button);
    }

    if (isAlertTooltip) {
      let hbox = this.doc.createElement("hbox");
      hbox.setAttribute("align", "start");

      let alertImg = this.doc.createElement("image");
      alertImg.className = "devtools-tooltip-alert-icon";
      hbox.appendChild(alertImg);
      hbox.appendChild(vbox);
      this.content = hbox;
    } else {
      this.content = vbox;
    }
  },

  




















  setVariableContent: function(
    objectActor,
    viewOptions = {},
    controllerOptions = {},
    relayEvents = {},
    extraButtons = [],
    toolbox = null) {

    let vbox = this.doc.createElement("vbox");
    vbox.className = "devtools-tooltip-variables-view-box";
    vbox.setAttribute("flex", "1");

    let innerbox = this.doc.createElement("vbox");
    innerbox.className = "devtools-tooltip-variables-view-innerbox";
    innerbox.setAttribute("flex", "1");
    vbox.appendChild(innerbox);

    for (let { label, className, command } of extraButtons) {
      let button = this.doc.createElement("button");
      button.className = className;
      button.setAttribute("label", label);
      button.addEventListener("command", command);
      vbox.appendChild(button);
    }

    let widget = new VariablesView(innerbox, viewOptions);

    
    if (toolbox) {
      widget.toolbox = toolbox;
    }

    
    widget.commitHierarchy = () => {};

    for (let e in relayEvents) widget.on(e, relayEvents[e]);
    VariablesViewController.attach(widget, controllerOptions);

    
    widget.searchPlaceholder = viewOptions.searchPlaceholder;
    widget.searchEnabled = viewOptions.searchEnabled;

    
    
    widget.controller.setSingleVariable(
      { objectActor: objectActor }, controllerOptions);

    this.content = vbox;
    this.panel.setAttribute("clamped-dimensions", "");
  },

  






  setRelativeImageContent: Task.async(function*(imageUrl, inspectorFront, maxDim) {
    if (imageUrl.startsWith("data:")) {
      
      this.setImageContent(imageUrl, {maxDim: maxDim});
    } else if (inspectorFront) {
      try {
        let {data, size} = yield inspectorFront.getImageDataFromURL(imageUrl, maxDim);
        size.maxDim = maxDim;
        let str = yield data.string();
        this.setImageContent(str, size);
      } catch (e) {
        this.setBrokenImageContent();
      }
    }
  }),

  


  setBrokenImageContent: function() {
    this.setTextContent({
      messages: [l10n.strings.GetStringFromName("previewTooltip.image.brokenImage")]
    });
  },

  

















  setImageContent: function(imageUrl, options={}) {
    if (!imageUrl) {
      return;
    }

    
    let vbox = this.doc.createElement("vbox");
    vbox.setAttribute("align", "center");

    
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
    if (options.naturalWidth && options.naturalHeight) {
      label.textContent = this._getImageDimensionLabel(options.naturalWidth,
        options.naturalHeight);
    } else {
      
      label.textContent = l10n.strings.GetStringFromName("previewTooltip.image.brokenImage");
      let imgObj = new this.doc.defaultView.Image();
      imgObj.src = imageUrl;
      imgObj.onload = () => {
        imgObj.onload = null;
        label.textContent = this._getImageDimensionLabel(imgObj.naturalWidth,
          imgObj.naturalHeight);
      }
    }
    vbox.appendChild(label);

    this.content = vbox;
  },

  _getImageDimensionLabel: (w, h) => w + " x " + h,

  




  setColorPickerContent: function(color) {
    let def = promise.defer();

    
    let iframe = this.doc.createElementNS(XHTML_NS, "iframe");
    iframe.setAttribute("transparent", true);
    iframe.setAttribute("width", "210");
    iframe.setAttribute("height", "220");
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("class", "devtools-tooltip-iframe");

    let panel = this.panel;
    let xulWin = this.doc.ownerGlobal;

    
    function onLoad() {
      iframe.removeEventListener("load", onLoad, true);
      let win = iframe.contentWindow.wrappedJSObject;

      let container = win.document.getElementById("spectrum");
      let spectrum = new Spectrum(container, color);

      function finalizeSpectrum() {
        spectrum.show();
        def.resolve(spectrum);
      }

      
      if (panel.state == "open") {
        finalizeSpectrum();
      }
      else {
        panel.addEventListener("popupshown", function shown() {
          panel.removeEventListener("popupshown", shown, true);
          finalizeSpectrum();
        }, true);
      }
    }
    iframe.addEventListener("load", onLoad, true);
    iframe.setAttribute("src", SPECTRUM_FRAME);

    
    this.content = iframe;

    return def.promise;
  },

  













  setCssTransformContent: Task.async(function*(transform, pageStyle, node) {
    if (!transform) {
      throw "Missing transform";
    }

    
    
    let styles = yield pageStyle.getComputed(node, {
      filter: "user",
      markMatched: false,
      onlyMatched: false
    });

    let origin = styles["transform-origin"].value;
    let width = parseInt(styles["width"].value);
    let height = parseInt(styles["height"].value);

    let root = this.doc.createElementNS(XHTML_NS, "div");
    let previewer = new CSSTransformPreviewer(root);
    this.content = root;
    if (!previewer.preview(transform, origin, width, height)) {
      throw "Invalid transform";
    }
  }),

  





  setFontFamilyContent: function(font) {
    if (!font) {
      return;
    }

    
    let vbox = this.doc.createElement("vbox");
    vbox.setAttribute("flex", "1");

    
    let previewer = this.doc.createElement("description");
    previewer.setAttribute("flex", "1");
    previewer.style.fontFamily = font;
    previewer.classList.add("devtools-tooltip-font-previewer-text");
    previewer.textContent = FONT_FAMILY_PREVIEW_TEXT;
    vbox.appendChild(previewer);

    this.content = vbox;
  }
};







function SwatchBasedEditorTooltip(doc) {
  
  
  
  
  this.tooltip = new Tooltip(doc, {
    consumeOutsideClick: true,
    closeOnKeys: [ESCAPE_KEYCODE, RETURN_KEYCODE],
    noAutoFocus: false
  });

  
  
  this._onTooltipKeypress = (event, code) => {
    if (code === ESCAPE_KEYCODE) {
      this.revert();
    } else if (code === RETURN_KEYCODE) {
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
      this.tooltip.once("hidden", () => {
        if (!this.eyedropperOpen) {
          this.activeSwatch = null;
        }
      });
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
      let newValue = swatch.callbacks.onCommit();
      if (typeof newValue !== "undefined") {
        swatch.originalValue = newValue;
      }
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
  this._openEyeDropper = this._openEyeDropper.bind(this);
}

module.exports.SwatchColorPickerTooltip = SwatchColorPickerTooltip;

SwatchColorPickerTooltip.prototype = Heritage.extend(SwatchBasedEditorTooltip.prototype, {
  



  show: function() {
    
    SwatchBasedEditorTooltip.prototype.show.call(this);
    
    if (this.activeSwatch) {
      this.currentSwatchColor = this.activeSwatch.nextSibling;
      let swatch = this.swatches.get(this.activeSwatch);
      let color = this.activeSwatch.style.backgroundColor;
      this.spectrum.then(spectrum => {
        spectrum.off("changed", this._onSpectrumColorChange);
        spectrum.rgb = this._colorToRgba(color);
        spectrum.on("changed", this._onSpectrumColorChange);
        spectrum.updateUI();
      });
    }

    let tooltipDoc = this.tooltip.content.contentDocument;
    let eyeButton = tooltipDoc.querySelector("#eyedropper-button");
    eyeButton.addEventListener("click", this._openEyeDropper);
  },

  _onSpectrumColorChange: function(event, rgba, cssColor) {
    this._selectColor(cssColor);
  },

  _selectColor: function(color) {
    if (this.activeSwatch) {
      this.activeSwatch.style.backgroundColor = color;
      this.currentSwatchColor.textContent = color;
      this.preview(color);
    }
  },

 _openEyeDropper: function() {
    let chromeWindow = this.tooltip.doc.defaultView.top;
    let windowType = chromeWindow.document.documentElement
                     .getAttribute("windowtype");
    let toolboxWindow;
    if (windowType != "navigator:browser") {
      
      
      toolboxWindow = chromeWindow;
      chromeWindow = Services.wm.getMostRecentWindow("navigator:browser");
      chromeWindow.focus();
    }
    let dropper = new Eyedropper(chromeWindow, { copyOnSelect: false });

    dropper.once("select", (event, color) => {
      if (toolboxWindow) {
        toolboxWindow.focus();
      }
      this._selectColor(color);
    });

    dropper.once("destroy", () => {
      this.eyedropperOpen = false;
      this.activeSwatch = null;
    })

    dropper.open();
    this.eyedropperOpen = true;

    
    this.hide();

    this.tooltip.emit("eyedropper-opened", dropper);
  },

  _colorToRgba: function(color) {
    color = new colorUtils.CssColor(color);
    let rgba = color._getRGBATuple();
    return [rgba.r, rgba.g, rgba.b, rgba.a];
  },

  destroy: function() {
    SwatchBasedEditorTooltip.prototype.destroy.call(this);
    this.currentSwatchColor = null;
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




function L10N() {}
L10N.prototype = {};

let l10n = new L10N();

loader.lazyGetter(L10N.prototype, "strings", () => {
  return Services.strings.createBundle(
    "chrome://browser/locale/devtools/inspector.properties");
});
