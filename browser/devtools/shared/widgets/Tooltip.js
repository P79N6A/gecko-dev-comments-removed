



"use strict";

const {Cc, Cu, Ci} = require("chrome");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const IOService = Cc["@mozilla.org/network/io-service;1"]
  .getService(Ci.nsIIOService);
const {Spectrum} = require("devtools/shared/widgets/Spectrum");
const {CubicBezierWidget} = require("devtools/shared/widgets/CubicBezierWidget");
const {MdnDocsWidget} = require("devtools/shared/widgets/MdnDocsWidget");
const {CSSFilterEditorWidget} = require("devtools/shared/widgets/FilterWidget");
const EventEmitter = require("devtools/toolkit/event-emitter");
const {colorUtils} = require("devtools/css-color");
const Heritage = require("sdk/core/heritage");
const {Eyedropper} = require("devtools/eyedropper/eyedropper");
const Editor = require("devtools/sourceeditor/editor");
const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

devtools.lazyRequireGetter(this, "beautify", "devtools/jsbeautify");

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
const CUBIC_BEZIER_FRAME = "chrome://browser/content/devtools/cubic-bezier-frame.xhtml";
const MDN_DOCS_FRAME = "chrome://browser/content/devtools/mdn-docs-frame.xhtml";
const FILTER_FRAME = "chrome://browser/content/devtools/filter-frame.xhtml";
const ESCAPE_KEYCODE = Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE;
const RETURN_KEYCODE = Ci.nsIDOMKeyEvent.DOM_VK_RETURN;
const POPUP_EVENTS = ["shown", "hidden", "showing", "hiding"];





























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

  
  for (let eventName of POPUP_EVENTS) {
    this["_onPopup" + eventName] = (name => {
      return e => {
        if (e.target === this.panel) {
          this.emit(name);
        }
      };
    })(eventName);
    this.panel.addEventListener("popup" + eventName,
      this["_onPopup" + eventName], false);
  }

  
  let win = this.doc.querySelector("window");
  this._onKeyPress = event => {
    if (this.panel.hidden) {
      return;
    }

    this.emit("keypress", event.keyCode);
    if (this.options.get("closeOnKeys").indexOf(event.keyCode) !== -1 &&
        this.isShown()) {
      event.stopPropagation();
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
    return this.panel &&
           this.panel.state !== "closed" &&
           this.panel.state !== "hiding";
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

    for (let eventName of POPUP_EVENTS) {
      this.panel.removeEventListener("popup" + eventName,
        this["_onPopup" + eventName], false);
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
    if (!baseNode) {
      
      return;
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

    if (!this._basedNode) {
      return;
    }

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
        }, reason => {
          if (reason === false) {
            
            
            return;
          }
          
          
          console.error("isValidHoverTarget rejected with an unexpected reason:");
          console.error(reason);
        });
      });
    }
  },

  





  isValidHoverTarget: function(target) {
    
    
    let res = this._targetNodeCb(target, this);

    
    
    if (res && res.then) {
      return res.then(arg => {
        return arg instanceof Ci.nsIDOMNode ? arg : target;
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
    this.panel.removeAttribute("clamped-dimensions-no-min-height");
    this.panel.removeAttribute("clamped-dimensions-no-max-or-min-height");
    this.panel.removeAttribute("wide");

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

  








  setEventContent: function({ eventListenerInfos, toolbox }) {
    new EventTooltip(this, eventListenerInfos, toolbox);
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

    if (!options.hideDimensionLabel) {
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
        };
      }

      vbox.appendChild(label);
    }

    this.content = vbox;
  },

  _getImageDimensionLabel: (w, h) => w + " \u00D7 " + h,

  























  setIFrameContent: function({width, height}, url) {

    let def = promise.defer();

    
    let iframe = this.doc.createElementNS(XHTML_NS, "iframe");
    iframe.setAttribute("transparent", true);
    iframe.setAttribute("width", width);
    iframe.setAttribute("height", height);
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("class", "devtools-tooltip-iframe");

    
    function onLoad() {
      iframe.removeEventListener("load", onLoad, true);
      def.resolve(iframe);
    }
    iframe.addEventListener("load", onLoad, true);

    
    iframe.setAttribute("src", url);

    
    this.content = iframe;

    return def.promise;
  },

  




  setColorPickerContent: function(color) {
    let dimensions = {width: "210", height: "216"};
    let panel = this.panel;
    return this.setIFrameContent(dimensions, SPECTRUM_FRAME).then(onLoaded);

    function onLoaded(iframe) {
      let win = iframe.contentWindow.wrappedJSObject;
      let def = promise.defer();
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
      return def.promise;
    }
  },

  




  setCubicBezierContent: function(bezier) {
    let dimensions = {width: "410", height: "360"};
    let panel = this.panel;
    return this.setIFrameContent(dimensions, CUBIC_BEZIER_FRAME).then(onLoaded);

    function onLoaded(iframe) {
      let win = iframe.contentWindow.wrappedJSObject;
      let def = promise.defer();
      let container = win.document.getElementById("container");
      let widget = new CubicBezierWidget(container, bezier);

      
      if (panel.state == "open") {
        def.resolve(widget);
      } else {
        panel.addEventListener("popupshown", function shown() {
          panel.removeEventListener("popupshown", shown, true);
          def.resolve(widget);
        }, true);
      }
      return def.promise;
    }
  },

  




  setFilterContent: function(filter) {
    let dimensions = {width: "350", height: "350"};
    let panel = this.panel;
    return this.setIFrameContent(dimensions, FILTER_FRAME).then(onLoaded);

    function onLoaded(iframe) {
      let win = iframe.contentWindow.wrappedJSObject;
      let doc = win.document.documentElement;
      let def = promise.defer();
      let container = win.document.getElementById("container");
      let widget = new CSSFilterEditorWidget(container, filter);

      iframe.height = doc.offsetHeight;

      widget.on("render", e => {
        iframe.height = doc.offsetHeight;
      });

      
      if (panel.state == "open") {
        def.resolve(widget);
      } else {
        panel.addEventListener("popupshown", function shown() {
          panel.removeEventListener("popupshown", shown, true);
          def.resolve(widget);
        }, true);
      }
      return def.promise;
    }
  },

  










  setFontFamilyContent: Task.async(function*(font, nodeFront) {
    if (!font || !nodeFront) {
      throw "Missing font";
    }

    if (typeof nodeFront.getFontFamilyDataURL === "function") {
      font = font.replace(/"/g, "'");
      font = font.replace("!important", "");
      font = font.trim();

      let fillStyle = (Services.prefs.getCharPref("devtools.theme") === "light") ?
        "black" : "white";

      let {data, size} = yield nodeFront.getFontFamilyDataURL(font, fillStyle);
      let str = yield data.string();
      this.setImageContent(str, { hideDimensionLabel: true, maxDim: size });
    }
  }),

  













  setMdnDocsContent: function() {
    let dimensions = {width: "410", height: "300"};
    return this.setIFrameContent(dimensions, MDN_DOCS_FRAME).then(onLoaded);

    function onLoaded(iframe) {
      let win = iframe.contentWindow.wrappedJSObject;
      
      let widget = new MdnDocsWidget(win.document);
      return widget;
    }
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

      
      
      
      this.tooltip.once("hiding", () => {
        if (!this._reverted && !this.eyedropperOpen) {
          this.commit();
        }
        this._reverted = false;
      });

      
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

  














  addSwatch: function(swatchEl, callbacks={}) {
    if (!callbacks.onPreview) callbacks.onPreview = function() {};
    if (!callbacks.onRevert) callbacks.onRevert = function() {};
    if (!callbacks.onCommit) callbacks.onCommit = function() {};

    this.swatches.set(swatchEl, {
      callbacks: callbacks
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

    if (event.shiftKey) {
      event.stopPropagation();
      return;
    }
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
      swatch.callbacks.onRevert();
      this._reverted = true;
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
  this._openEyeDropper = this._openEyeDropper.bind(this);
}

module.exports.SwatchColorPickerTooltip = SwatchColorPickerTooltip;

SwatchColorPickerTooltip.prototype = Heritage.extend(SwatchBasedEditorTooltip.prototype, {
  



  show: function() {
    
    SwatchBasedEditorTooltip.prototype.show.call(this);
    
    if (this.activeSwatch) {
      this.currentSwatchColor = this.activeSwatch.nextSibling;
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
      this.activeSwatch.parentNode.dataset.color = color;

      color = this._toDefaultType(color);
      this.currentSwatchColor.textContent = color;
      this.preview(color);

      if (this.eyedropperOpen) {
        this.commit();
      }
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
    let dropper = new Eyedropper(chromeWindow, { copyOnSelect: false,
                                                 context: "picker" });

    dropper.once("select", (event, color) => {
      if (toolboxWindow) {
        toolboxWindow.focus();
      }
      this._selectColor(color);
    });

    dropper.once("destroy", () => {
      this.eyedropperOpen = false;
      this.activeSwatch = null;
    });

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

  _toDefaultType: function(color) {
    let colorObj = new colorUtils.CssColor(color);
    return colorObj.toString();
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

function EventTooltip(tooltip, eventListenerInfos, toolbox) {
  this._tooltip = tooltip;
  this._eventListenerInfos = eventListenerInfos;
  this._toolbox = toolbox;
  this._tooltip.eventEditors = new WeakMap();

  this._headerClicked = this._headerClicked.bind(this);
  this._debugClicked = this._debugClicked.bind(this);
  this.destroy = this.destroy.bind(this);

  this._init();
}

EventTooltip.prototype = {
  _init: function() {
    let config = {
      mode: Editor.modes.js,
      lineNumbers: false,
      lineWrapping: false,
      readOnly: true,
      styleActiveLine: true,
      extraKeys: {},
      theme: "mozilla markup-view"
    };

    let doc = this._tooltip.doc;
    let container = doc.createElement("vbox");
    container.setAttribute("id", "devtools-tooltip-events-container");

    for (let listener of this._eventListenerInfos) {
      let phase = listener.capturing ? "Capturing" : "Bubbling";
      let level = listener.DOM0 ? "DOM0" : "DOM2";

      
      let header = doc.createElement("hbox");
      header.className = "event-header devtools-toolbar";
      container.appendChild(header);

      if (!listener.hide.debugger) {
        let debuggerIcon = doc.createElement("image");
        debuggerIcon.className = "event-tooltip-debugger-icon";
        debuggerIcon.setAttribute("src", "chrome://browser/skin/devtools/tool-debugger.svg");
        let openInDebugger = l10n.strings.GetStringFromName("eventsTooltip.openInDebugger");
        debuggerIcon.setAttribute("tooltiptext", openInDebugger);
        header.appendChild(debuggerIcon);
      }

      if (!listener.hide.type) {
        let eventTypeLabel = doc.createElement("label");
        eventTypeLabel.className = "event-tooltip-event-type";
        eventTypeLabel.setAttribute("value", listener.type);
        eventTypeLabel.setAttribute("tooltiptext", listener.type);
        header.appendChild(eventTypeLabel);
      }

      if (!listener.hide.filename) {
        let filename = doc.createElement("label");
        filename.className = "event-tooltip-filename devtools-monospace";
        filename.setAttribute("value", listener.origin);
        filename.setAttribute("tooltiptext", listener.origin);
        filename.setAttribute("crop", "left");
        header.appendChild(filename);
      }

      let attributesContainer = doc.createElement("hbox");
      attributesContainer.setAttribute("class", "event-tooltip-attributes-container");
      header.appendChild(attributesContainer);

      if (!listener.hide.capturing) {
        let attributesBox = doc.createElement("box");
        attributesBox.setAttribute("class", "event-tooltip-attributes-box");
        attributesContainer.appendChild(attributesBox);

        let capturing = doc.createElement("label");
        capturing.className = "event-tooltip-attributes";
        capturing.setAttribute("value", phase);
        capturing.setAttribute("tooltiptext", phase);
        attributesBox.appendChild(capturing);
      }

      if (listener.tags) {
        for (let tag of listener.tags.split(",")) {
          let attributesBox = doc.createElement("box");
          attributesBox.setAttribute("class", "event-tooltip-attributes-box");
          attributesContainer.appendChild(attributesBox);

          let tagBox = doc.createElement("label");
          tagBox.className = "event-tooltip-attributes";
          tagBox.setAttribute("value", tag);
          tagBox.setAttribute("tooltiptext", tag);
          attributesBox.appendChild(tagBox);
        }
      }

      if (!listener.hide.dom0) {
        let attributesBox = doc.createElement("box");
        attributesBox.setAttribute("class", "event-tooltip-attributes-box");
        attributesContainer.appendChild(attributesBox);

        let dom0 = doc.createElement("label");
        dom0.className = "event-tooltip-attributes";
        dom0.setAttribute("value", level);
        dom0.setAttribute("tooltiptext", level);
        attributesBox.appendChild(dom0);
      }

      
      let content = doc.createElement("box");
      let editor = new Editor(config);
      this._tooltip.eventEditors.set(content, {
        editor: editor,
        handler: listener.handler,
        searchString: listener.searchString,
        uri: listener.origin,
        dom0: listener.DOM0,
        appended: false
      });

      content.className = "event-tooltip-content-box";
      container.appendChild(content);

      this._addContentListeners(header);
    }

    this._tooltip.content = container;
    this._tooltip.panel.setAttribute("clamped-dimensions-no-max-or-min-height", "");
    this._tooltip.panel.setAttribute("wide", "");

    this._tooltip.panel.addEventListener("popuphiding", () => {
      this.destroy(container);
    }, false);
  },

  _addContentListeners: function(header) {
    header.addEventListener("click", this._headerClicked);
  },

  _headerClicked: function(event) {
    if (event.target.classList.contains("event-tooltip-debugger-icon")) {
      this._debugClicked(event);
      event.stopPropagation();
      return;
    }

    let doc = this._tooltip.doc;
    let header = event.currentTarget;
    let content = header.nextElementSibling;

    if (content.hasAttribute("open")) {
      content.removeAttribute("open");
    } else {
      let contentNodes = doc.querySelectorAll(".event-tooltip-content-box");

      for (let node of contentNodes) {
        if (node !== content) {
          node.removeAttribute("open");
        }
      }

      content.setAttribute("open", "");

      let eventEditors = this._tooltip.eventEditors.get(content);

      if (eventEditors.appended) {
        return;
      }

      let {editor, handler} = eventEditors;

      let iframe = doc.createElement("iframe");
      iframe.setAttribute("style", "width:100%;");

      editor.appendTo(content, iframe).then(() => {
        let tidied = beautify.js(handler, { indent_size: 2 });

        editor.setText(tidied);

        eventEditors.appended = true;

        let container = header.parentElement.getBoundingClientRect();
        if (header.getBoundingClientRect().top < container.top) {
          header.scrollIntoView(true);
        } else if (content.getBoundingClientRect().bottom > container.bottom) {
          content.scrollIntoView(false);
        }

        this._tooltip.emit("event-tooltip-ready");
      });
    }
  },

  _debugClicked: function(event) {
    let header = event.currentTarget;
    let content = header.nextElementSibling;

    let {uri, searchString, dom0} =
      this._tooltip.eventEditors.get(content);

    if (uri && uri !== "?") {
      
      
      let toolbox = this._toolbox;

      this._tooltip.hide();

      uri = uri.replace(/"/g, "");

      let showSource = ({ DebuggerView }) => {
        let matches = uri.match(/(.*):(\d+$)/);
        let line = 1;

        if (matches) {
          uri = matches[1];
          line = matches[2];
        }

        let item = DebuggerView.Sources.getItemForAttachment(
          a => a.source.url === uri
        );
        if (item) {
          let actor = item.attachment.source.actor;
          DebuggerView.setEditorLocation(actor, line, {noDebug: true}).then(() => {
            if (dom0) {
              let text = DebuggerView.editor.getText();
              let index = text.indexOf(searchString);
              let lastIndex = text.lastIndexOf(searchString);

              
              
              if (index !== -1 && index === lastIndex) {
                text = text.substr(0, index);
                let matches = text.match(/\n/g);

                if (matches) {
                  DebuggerView.editor.setCursor({
                    line: matches.length
                  });
                }
              }
            }
          });
        }
      };

      let debuggerAlreadyOpen = toolbox.getPanel("jsdebugger");
      toolbox.selectTool("jsdebugger").then(({ panelWin: dbg }) => {
        if (debuggerAlreadyOpen) {
          showSource(dbg);
        } else {
          dbg.once(dbg.EVENTS.SOURCES_ADDED, () => showSource(dbg));
        }
      });
    }
  },

  destroy: function(container) {
    if (this._tooltip) {
      this._tooltip.panel.removeEventListener("popuphiding", this.destroy, false);

      let boxes = container.querySelectorAll(".event-tooltip-content-box");

      for (let box of boxes) {
        let {editor} = this._tooltip.eventEditors.get(box);
        editor.destroy();
      }

      this._tooltip.eventEditors.clear();
      this._tooltip.eventEditors = null;
    }

    let headerNodes = container.querySelectorAll(".event-header");

    for (let node of headerNodes) {
      node.removeEventListener("click", this._headerClicked);
    }

    let sourceNodes = container.querySelectorAll(".event-tooltip-debugger-icon");
    for (let node of sourceNodes) {
      node.removeEventListener("click", this._debugClicked);
    }

    this._eventListenerInfos = this._toolbox = this._tooltip = null;
  }
};










function SwatchCubicBezierTooltip(doc) {
  SwatchBasedEditorTooltip.call(this, doc);

  
  
  this.widget = this.tooltip.setCubicBezierContent([0, 0, 1, 1]);
  this._onUpdate = this._onUpdate.bind(this);
}

module.exports.SwatchCubicBezierTooltip = SwatchCubicBezierTooltip;

SwatchCubicBezierTooltip.prototype = Heritage.extend(SwatchBasedEditorTooltip.prototype, {
  



  show: function() {
    
    SwatchBasedEditorTooltip.prototype.show.call(this);
    
    if (this.activeSwatch) {
      this.currentBezierValue = this.activeSwatch.nextSibling;
      let swatch = this.swatches.get(this.activeSwatch);
      this.widget.then(widget => {
        widget.off("updated", this._onUpdate);
        widget.cssCubicBezierValue = this.currentBezierValue.textContent;
        widget.on("updated", this._onUpdate);
      });
    }
  },

  _onUpdate: function(event, bezier) {
    if (!this.activeSwatch) {
      return;
    }

    this.currentBezierValue.textContent = bezier + "";
    this.preview(bezier + "");
  },

  destroy: function() {
    SwatchBasedEditorTooltip.prototype.destroy.call(this);
    this.currentBezierValue = null;
    this.widget.then(widget => {
      widget.off("updated", this._onUpdate);
      widget.destroy();
    });
  }
});






function CssDocsTooltip(doc) {
  this.tooltip = new Tooltip(doc, {
    consumeOutsideClick: true,
    closeOnKeys: [ESCAPE_KEYCODE, RETURN_KEYCODE],
    noAutoFocus: false
  });
  this.widget = this.tooltip.setMdnDocsContent();
}

module.exports.CssDocsTooltip = CssDocsTooltip;

CssDocsTooltip.prototype = {
  



  show: function(anchor, propertyName) {

    function loadCssDocs(widget) {
      return widget.loadCssDocs(propertyName);
    }

    this.widget.then(loadCssDocs);
    this.tooltip.show(anchor, "topcenter bottomleft");
  },

  hide: function() {
    this.tooltip.hide();
  },

  destroy: function() {
    this.tooltip.destroy();
  }
};










function SwatchFilterTooltip(doc) {
  SwatchBasedEditorTooltip.call(this, doc);

  
  
  this.widget = this.tooltip.setFilterContent("none");
  this._onUpdate = this._onUpdate.bind(this);
}

exports.SwatchFilterTooltip = SwatchFilterTooltip;

SwatchFilterTooltip.prototype = Heritage.extend(SwatchBasedEditorTooltip.prototype, {
  show: function() {
    
    SwatchBasedEditorTooltip.prototype.show.call(this);
    
    if (this.activeSwatch) {
      this.currentFilterValue = this.activeSwatch.nextSibling;
      this.widget.then(widget => {
        widget.off("updated", this._onUpdate);
        widget.on("updated", this._onUpdate);
        widget.setCssValue(this.currentFilterValue.textContent);
        widget.render();
      });
    }
  },

  _onUpdate: function(event, filters) {
    if (!this.activeSwatch) {
      return;
    }

    this.currentFilterValue.textContent = filters;
    this.preview();
  },

  destroy: function() {
    SwatchBasedEditorTooltip.prototype.destroy.call(this);
    this.currentFilterValue = null;
    this.widget.then(widget => {
      widget.off("updated", this._onUpdate);
      widget.destroy();
    });
  }
});




function L10N() {}
L10N.prototype = {};

let l10n = new L10N();

loader.lazyGetter(L10N.prototype, "strings", () => {
  return Services.strings.createBundle(
    "chrome://browser/locale/devtools/inspector.properties");
});
