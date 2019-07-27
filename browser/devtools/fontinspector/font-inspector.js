





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;
const DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");

function FontInspector(inspector, window)
{
  this.inspector = inspector;
  this.pageStyle = this.inspector.pageStyle;
  this.chromeDoc = window.document;
  this.init();
}

FontInspector.prototype = {
  init: function FI_init() {
    this.update = this.update.bind(this);
    this.onNewNode = this.onNewNode.bind(this);
    this.inspector.selection.on("new-node", this.onNewNode);
    this.inspector.sidebar.on("fontinspector-selected", this.onNewNode);
    this.showAll = this.showAll.bind(this);
    this.showAllButton = this.chromeDoc.getElementById("showall");
    this.showAllButton.addEventListener("click", this.showAll);
    this.update();
  },

  


  isActive: function FI_isActive() {
    return this.inspector.sidebar &&
           this.inspector.sidebar.getCurrentTabID() == "fontinspector";
  },

  


  destroy: function FI_destroy() {
    this.chromeDoc = null;
    this.inspector.sidebar.off("fontinspector-selected", this.onNewNode);
    this.inspector.selection.off("new-node", this.onNewNode);
    this.showAllButton.removeEventListener("click", this.showAll);
  },

  


  onNewNode: function FI_onNewNode() {
    if (this.isActive() &&
        this.inspector.selection.isConnected() &&
        this.inspector.selection.isElementNode()) {
      this.undim();
      this.update();
    } else {
      this.dim();
    }
  },

  


  dim: function FI_dim() {
    this.chromeDoc.body.classList.add("dim");
    this.chromeDoc.querySelector("#all-fonts").innerHTML = "";
  },

  


  undim: function FI_undim() {
    this.chromeDoc.body.classList.remove("dim");
  },

 


  update: Task.async(function*() {
    let node = this.inspector.selection.nodeFront;

    if (!node ||
        !this.isActive() ||
        !this.inspector.selection.isConnected() ||
        !this.inspector.selection.isElementNode() ||
        this.chromeDoc.body.classList.contains("dim")) {
      return;
    }

    this.chromeDoc.querySelector("#all-fonts").innerHTML = "";

    let fillStyle = (Services.prefs.getCharPref("devtools.theme") == "light") ?
        "black" : "white";
    let options = {
      includePreviews: true,
      previewFillStyle: fillStyle
    }

    let fonts = yield this.pageStyle.getUsedFontFaces(node, options)
                      .then(null, console.error);
    if (!fonts) {
      return;
    }

    for (let font of fonts) {
      font.previewUrl = yield font.preview.data.string();
    }

    
    if (!this.chromeDoc) {
      return;
    }

    
    this.chromeDoc.querySelector("#all-fonts").innerHTML = "";

    for (let font of fonts) {
      this.render(font);
    }

    this.inspector.emit("fontinspector-updated");
  }),

  


  render: function FI_render(font) {
    let s = this.chromeDoc.querySelector("#template > section");
    s = s.cloneNode(true);

    s.querySelector(".font-name").textContent = font.name;
    s.querySelector(".font-css-name").textContent = font.CSSFamilyName;

    if (font.URI) {
      s.classList.add("is-remote");
    } else {
      s.classList.add("is-local");
    }

    let formatElem = s.querySelector(".font-format");
    if (font.format) {
      formatElem.textContent = font.format;
    } else {
      formatElem.hidden = true;
    }

    s.querySelector(".font-url").value = font.URI;

    if (font.rule) {
      
      let cssText = font.ruleText;

      s.classList.add("has-code");
      s.querySelector(".font-css-code").textContent = cssText;
    }
    let preview = s.querySelector(".font-preview");
    preview.src = font.previewUrl;

    this.chromeDoc.querySelector("#all-fonts").appendChild(s);
  },

  


  showAll: function FI_showAll() {
    if (!this.isActive() ||
        !this.inspector.selection.isConnected() ||
        !this.inspector.selection.isElementNode()) {
      return;
    }

    
    let walker = this.inspector.walker;

    walker.getRootNode().then(root => walker.querySelector(root, "body")).then(body => {
      this.inspector.selection.setNodeFront(body, "fontinspector");
    });
  },
}

window.setPanel = function(panel) {
  window.fontInspector = new FontInspector(panel, window);
}

window.onunload = function() {
  if (window.fontInspector) {
    window.fontInspector.destroy();
  }
}
