





"use strict";

const { utils: Cu } = Components;
const DEFAULT_PREVIEW_TEXT = "Abc";
const PREVIEW_UPDATE_DELAY = 150;

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
  init: function() {
    this.update = this.update.bind(this);
    this.onNewNode = this.onNewNode.bind(this);
    this.onThemeChanged = this.onThemeChanged.bind(this);
    this.inspector.selection.on("new-node", this.onNewNode);
    this.inspector.sidebar.on("fontinspector-selected", this.onNewNode);
    this.showAll = this.showAll.bind(this);
    this.showAllButton = this.chromeDoc.getElementById("showall");
    this.showAllButton.addEventListener("click", this.showAll);
    this.previewTextChanged = this.previewTextChanged.bind(this);
    this.previewInput = this.chromeDoc.getElementById("preview-text-input");
    this.previewInput.addEventListener("input", this.previewTextChanged);

    
    gDevTools.on("theme-switched", this.onThemeChanged);

    this.update();
  },

  


  isActive: function() {
    return this.inspector.sidebar &&
           this.inspector.sidebar.getCurrentTabID() == "fontinspector";
  },

  


  destroy: function() {
    this.chromeDoc = null;
    this.inspector.sidebar.off("fontinspector-selected", this.onNewNode);
    this.inspector.selection.off("new-node", this.onNewNode);
    this.showAllButton.removeEventListener("click", this.showAll);
    this.previewInput.removeEventListener("input", this.previewTextChanged);

    gDevTools.off("theme-switched", this.onThemeChanged);

    if (this._previewUpdateTimeout) {
      clearTimeout(this._previewUpdateTimeout);
    }
  },

  


  onNewNode: function() {
    if (this.isActive() &&
        this.inspector.selection.isConnected() &&
        this.inspector.selection.isElementNode()) {
      this.undim();
      this.update();
    } else {
      this.dim();
    }
  },

  




  getPreviewText: function() {
    let inputText = this.previewInput.value.trim();
    if (inputText === "") {
      return DEFAULT_PREVIEW_TEXT;
    }

    return inputText;
  },

  


  previewTextChanged: function() {
    if (this._previewUpdateTimeout) {
      clearTimeout(this._previewUpdateTimeout);
    }

    this._previewUpdateTimeout = setTimeout(() => {
      this.update(this._lastUpdateShowedAllFonts);
    }, PREVIEW_UPDATE_DELAY);
  },

  


  onThemeChanged: function(event, frame) {
    if (frame === this.chromeDoc.defaultView) {
      this.update(this._lastUpdateShowedAllFonts);
    }
  },

  


  dim: function() {
    this.chromeDoc.body.classList.add("dim");
    this.clear();
  },

  


  undim: function() {
    this.chromeDoc.body.classList.remove("dim");
  },

  


  clear: function() {
    this.chromeDoc.querySelector("#all-fonts").innerHTML = "";
  },

 


  update: Task.async(function*(showAllFonts) {
    let node = this.inspector.selection.nodeFront;

    if (!node ||
        !this.isActive() ||
        !this.inspector.selection.isConnected() ||
        !this.inspector.selection.isElementNode() ||
        this.chromeDoc.body.classList.contains("dim")) {
      return;
    }

    this._lastUpdateShowedAllFonts = showAllFonts;

    
    let fillStyle = (Services.prefs.getCharPref("devtools.theme") == "dark") ?
        "white" : "black";

    let options = {
      includePreviews: true,
      previewText: this.getPreviewText(),
      previewFillStyle: fillStyle
    };

    let fonts = [];
    if (showAllFonts) {
      fonts = yield this.pageStyle.getAllUsedFontFaces(options)
                      .then(null, console.error);
    } else {
      fonts = yield this.pageStyle.getUsedFontFaces(node, options)
                      .then(null, console.error);
    }

    if (!fonts || !fonts.length) {
      
      this.clear();
      return;
    }

    for (let font of fonts) {
      font.previewUrl = yield font.preview.data.string();
    }

    
    if (!this.chromeDoc) {
      return;
    }

    
    this.clear();

    for (let font of fonts) {
      this.render(font);
    }

    this.inspector.emit("fontinspector-updated");
  }),

  


  render: function(font) {
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

  


  showAll: function() {
    this.update(true);
  },
};

window.setPanel = function(panel) {
  window.fontInspector = new FontInspector(panel, window);
};

window.onunload = function() {
  if (window.fontInspector) {
    window.fontInspector.destroy();
  }
};
