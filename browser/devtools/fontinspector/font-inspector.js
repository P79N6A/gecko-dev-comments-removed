





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;
const DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);

function FontInspector(inspector, window)
{
  this.inspector = inspector;
  this.chromeDoc = window.document;
  this.init();
}

FontInspector.prototype = {
  init: function FI_init() {
    this.update = this.update.bind(this);
    this.onNewNode = this.onNewNode.bind(this);
    this.onHighlighterLocked = this.onHighlighterLocked.bind(this);
    this.inspector.selection.on("new-node", this.onNewNode);
    this.inspector.sidebar.on("fontinspector-selected", this.onNewNode);
    if (this.inspector.highlighter) {
      this.inspector.highlighter.on("locked", this.onHighlighterLocked);
    }
    this.update();
  },

  


  isActive: function FI_isActive() {
    return this.inspector.sidebar &&
           this.inspector.sidebar.getCurrentTabID() == "fontinspector";
  },

  


  destroy: function FI_destroy() {
    this.chromeDoc = null;
    this.inspector.sidebar.off("layoutview-selected", this.onNewNode);
    this.inspector.selection.off("new-node", this.onNewNode);
    if (this.inspector.highlighter) {
      this.inspector.highlighter.off("locked", this.onHighlighterLocked);
    }
  },

  


  onNewNode: function FI_onNewNode() {
    if (this.isActive() &&
        this.inspector.selection.isConnected() &&
        this.inspector.selection.isElementNode() &&
        this.inspector.selection.reason != "highlighter") {
      this.undim();
      this.update();
    } else {
      this.dim();
    }
  },

  


  onHighlighterLocked: function FI_onHighlighterLocked() {
    this.undim();
    this.update();
  },

  


  dim: function FI_dim() {
    this.chromeDoc.body.classList.add("dim");
    this.chromeDoc.querySelector("#all-fonts").innerHTML = "";
  },

  


  undim: function FI_undim() {
    this.chromeDoc.body.classList.remove("dim");
  },

  



  update: function FI_update() {
    if (!this.isActive() ||
        !this.inspector.selection.isConnected() ||
        !this.inspector.selection.isElementNode() ||
        this.chromeDoc.body.classList.contains("dim")) {
      return;
    }

    let node = this.inspector.selection.node;
    let contentDocument = node.ownerDocument;

    
    let rng = contentDocument.createRange();
    rng.selectNode(node);
    let fonts = DOMUtils.getUsedFontFaces(rng);
    let fontsArray = [];
    for (let i = 0; i < fonts.length; i++) {
      fontsArray.push(fonts.item(i));
    }
    fontsArray = fontsArray.sort(function(a, b) {
      return a.srcIndex < b.srcIndex;
    });
    this.chromeDoc.querySelector("#all-fonts").innerHTML = "";
    for (let f of fontsArray) {
      this.render(f, contentDocument);
    }
  },

  


  render: function FI_render(font, document) {
    let s = this.chromeDoc.querySelector("#template > section");
    s = s.cloneNode(true);

    s.querySelector(".font-name").textContent = font.name;
    s.querySelector(".font-css-name").textContent = font.CSSFamilyName;
    s.querySelector(".font-format").textContent = font.format;

    if (font.srcIndex == -1) {
      s.classList.add("is-local");
    } else {
      s.classList.add("is-remote");
    }

    s.querySelector(".font-url").value = font.URI;

    let iframe = s.querySelector(".font-preview");
    if (font.rule) {
      
      let cssText = font.rule.style.parentRule.cssText;

      s.classList.add("has-code");
      s.querySelector(".font-css-code").textContent = cssText;

      
      
      
      
      let origin = font.rule.style.parentRule.parentStyleSheet.href;
      if (!origin) { 
        origin = document.location.href;
      }
      
      let base = origin.replace(/\/[^\/]*$/,"/")

      
      this.buildPreview(iframe, font.CSSFamilyName, cssText, base);
    } else {
      this.buildPreview(iframe, font.CSSFamilyName, "", "");
    }

    this.chromeDoc.querySelector("#all-fonts").appendChild(s);
  },

  


  buildPreview: function FI_buildPreview(iframe, name, cssCode, base) {
    










    let extraCSS = "* {padding:0;margin:0}";
    extraCSS += ".theme-dark {color: white}";
    extraCSS += "p {font-family: '" + name + "';}";
    extraCSS += "p {font-size: 40px;line-height:60px;padding:0 10px;margin:0;}";
    cssCode += extraCSS;
    let src = "data:text/html;charset=utf-8,<!DOCTYPE HTML><head><base></base></head><style></style><p contenteditable>Abc</p>";
    iframe.addEventListener("load", function onload() {
      iframe.removeEventListener("load", onload, true);
      let doc = iframe.contentWindow.document;
      
      
      doc.querySelector("base").href = base;
      doc.querySelector("style").textContent = cssCode;
      
      doc.documentElement.className = document.documentElement.className;
    }, true);
    iframe.src = src;
  },

  


  showAll: function FI_showAll() {
    if (!this.isActive() ||
        !this.inspector.selection.isConnected() ||
        !this.inspector.selection.isElementNode()) {
      return;
    }
    let node = this.inspector.selection.node;
    let contentDocument = node.ownerDocument;
    let root = contentDocument.documentElement;
    if (contentDocument.body) {
      root = contentDocument.body;
    }
    this.inspector.selection.setNode(root, "fontinspector");
  },
}


window.setPanel = function(panel) {
  window.fontInspector = new FontInspector(panel, window);
}

window.onunload = function() {
  window.fontInspector.destroy();
}
