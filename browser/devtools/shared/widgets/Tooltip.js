



"use strict";

const {Cc, Cu, Ci} = require("chrome");
const promise = require("sdk/core/promise");
const IOService = Cc["@mozilla.org/network/io-service;1"]
  .getService(Ci.nsIIOService);

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const GRADIENT_RE = /\b(repeating-)?(linear|radial)-gradient\(((rgb|hsl)a?\(.+?\)|[^\)])+\)/gi;
const BORDERCOLOR_RE = /^border-[-a-z]*color$/ig;
const BORDER_RE = /^border(-(top|bottom|left|right))?$/ig;
const BACKGROUND_IMAGE_RE = /url\([\'\"]?(.*?)[\'\"]?\)/;

























let PanelFactory = {
  get: function(doc, xulTag="panel") {
    
    let panel = doc.createElement(xulTag);
    panel.setAttribute("hidden", true);

    if (xulTag === "panel") {
      
      panel.setAttribute("consumeoutsideclicks", false);
      panel.setAttribute("type", "arrow");
      panel.setAttribute("level", "top");
    }

    panel.setAttribute("class", "devtools-tooltip devtools-tooltip-" + xulTag);
    doc.querySelector("window").appendChild(panel);

    return panel;
  }
};
























function Tooltip(doc) {
  this.doc = doc;
  this.panel = PanelFactory.get(doc);

  
  this.uid = "tooltip-" + Date.now();
}

module.exports.Tooltip = Tooltip;

Tooltip.prototype = {
  









  show: function(anchor, position="before_start") {
    this.panel.hidden = false;
    this.panel.openPopup(anchor, position);
  },

  


  hide: function() {
    this.panel.hidden = true;
    this.panel.hidePopup();
  },

  


  empty: function() {
    while (this.panel.hasChildNodes()) {
      this.panel.removeChild(this.panel.firstChild);
    }
  },

  


  destroy: function () {
    this.hide();
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
    if (this._targetNodeCb && this._targetNodeCb(target, this)) {
      this.show(target);
      this._lastHovered = target;
    }
  },

  _onBaseNodeMouseLeave: function() {
    clearNamedTimeout(this.uid);
    this._lastHovered = null;
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

  




  setImageContent: function(imageUrl, maxDim=400) {
    
    let vbox = this.doc.createElement("vbox");
    vbox.setAttribute("align", "center")

    
    let tiles = createTransparencyTiles(this.doc, vbox);

    
    let label = this.doc.createElement("label");
    label.classList.add("devtools-tooltip-caption");
    label.textContent = l10n.strings.GetStringFromName("previewTooltip.image.brokenImage");
    vbox.appendChild(label);

    
    let image = this.doc.createElement("image");
    image.setAttribute("src", imageUrl);
    if (maxDim) {
      image.style.maxWidth = maxDim + "px";
      image.style.maxHeight = maxDim + "px";
    }
    tiles.appendChild(image);

    this.content = vbox;

    
    let imgObj = new this.doc.defaultView.Image();
    imgObj.src = imageUrl;
    imgObj.onload = () => {
      imgObj.onload = null;

      
      label.textContent = imgObj.naturalWidth + " x " + imgObj.naturalHeight;
      if (imgObj.naturalWidth > maxDim ||
        imgObj.naturalHeight > maxDim) {
        label.textContent += " *";
      }
    }
  },

  



  setCssBackgroundImageContent: function(cssBackground, sheetHref, maxDim=400) {
    let uri = getBackgroundImageUri(cssBackground, sheetHref);
    if (uri) {
      this.setImageContent(uri, maxDim);
    }
  },

  setCssGradientContent: function(cssGradient) {
    let tiles = createTransparencyTiles(this.doc);

    let gradientBox = this.doc.createElement("box");
    gradientBox.width = "100";
    gradientBox.height = "100";
    gradientBox.style.background = this.cssGradient;
    gradientBox.style.borderRadius = "2px";
    gradientBox.style.boxShadow = "inset 0 0 4px #333";

    tiles.appendChild(gradientBox)

    this.content = tiles;
  },

  _setSimpleCssPropertiesContent: function(properties, width, height) {
    let tiles = createTransparencyTiles(this.doc);

    let box = this.doc.createElement("box");
    box.width = width + "";
    box.height = height + "";
    properties.forEach(({name, value}) => {
      box.style[name] = value;
    });
    tiles.appendChild(box);

    this.content = tiles;
  },

  setCssColorContent: function(cssColor) {
    this._setSimpleCssPropertiesContent([
      {name: "background", value: cssColor},
      {name: "borderRadius", value: "2px"},
      {name: "boxShadow", value: "inset 0 0 4px #333"},
    ], 50, 50);
  },

  setCssBoxShadowContent: function(cssBoxShadow) {
    this._setSimpleCssPropertiesContent([
      {name: "background", value: "white"},
      {name: "boxShadow", value: cssBoxShadow}
    ], 80, 80);
  },

  setCssBorderContent: function(cssBorder) {
    this._setSimpleCssPropertiesContent([
      {name: "background", value: "white"},
      {name: "border", value: cssBorder}
    ], 80, 80);
  }
};





function createTransparencyTiles(doc, parentEl) {
  let tiles = doc.createElement("box");
  tiles.classList.add("devtools-tooltip-tiles");
  if (parentEl) {
    parentEl.appendChild(tiles);
  }
  return tiles;
}




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
