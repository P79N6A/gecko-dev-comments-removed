





"use strict";

const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");

const promise = devtools.require("sdk/core/promise");

function LayoutView(aInspector, aWindow)
{
  this.inspector = aInspector;

  
  if (this.inspector.target.tab) {
    this.browser = aInspector.target.tab.linkedBrowser;
  }

  this.doc = aWindow.document;
  this.sizeLabel = this.doc.querySelector(".size > span");
  this.sizeHeadingLabel = this.doc.getElementById("element-size");

  this.init();
}

LayoutView.prototype = {
  init: function LV_init() {
    this.update = this.update.bind(this);
    this.onNewNode = this.onNewNode.bind(this);
    this.onNewSelection = this.onNewSelection.bind(this);
    this.onHighlighterLocked = this.onHighlighterLocked.bind(this);
    this.inspector.selection.on("new-node-front", this.onNewSelection);
    this.inspector.sidebar.on("layoutview-selected", this.onNewNode);
    if (this.inspector.highlighter) {
      this.inspector.highlighter.on("locked", this.onHighlighterLocked);
    }

    
    
    
    
    this.map = {
      position: {selector: "#element-position",
                 property: "position",
                 value: undefined},
      marginTop: {selector: ".margin.top > span",
                  property: "margin-top",
                  value: undefined},
      marginBottom: {selector: ".margin.bottom > span",
                  property: "margin-bottom",
                  value: undefined},
      marginLeft: {selector: ".margin.left > span",
                  property: "margin-left",
                  value: undefined},
      marginRight: {selector: ".margin.right > span",
                  property: "margin-right",
                  value: undefined},
      paddingTop: {selector: ".padding.top > span",
                  property: "padding-top",
                  value: undefined},
      paddingBottom: {selector: ".padding.bottom > span",
                  property: "padding-bottom",
                  value: undefined},
      paddingLeft: {selector: ".padding.left > span",
                  property: "padding-left",
                  value: undefined},
      paddingRight: {selector: ".padding.right > span",
                  property: "padding-right",
                  value: undefined},
      borderTop: {selector: ".border.top > span",
                  property: "border-top-width",
                  value: undefined},
      borderBottom: {selector: ".border.bottom > span",
                  property: "border-bottom-width",
                  value: undefined},
      borderLeft: {selector: ".border.left > span",
                  property: "border-left-width",
                  value: undefined},
      borderRight: {selector: ".border.right > span",
                  property: "border-right-width",
                  value: undefined},
    };

    this.onNewNode();
  },

  


  isActive: function LV_isActive() {
    return this.inspector.sidebar.getCurrentTabID() == "layoutview";
  },

  


  destroy: function LV_destroy() {
    this.inspector.sidebar.off("layoutview-selected", this.onNewNode);
    this.inspector.selection.off("new-node-front", this.onNewSelection);
    if (this.browser) {
      this.browser.removeEventListener("MozAfterPaint", this.update, true);
    }
    if (this.inspector.highlighter) {
      this.inspector.highlighter.off("locked", this.onHighlighterLocked);
    }
    this.sizeHeadingLabel = null;
    this.sizeLabel = null;
    this.inspector = null;
    this.doc = null;
  },

  


  onNewSelection: function() {
    let done = this.inspector.updating("layoutview");
    this.onNewNode().then(done, (err) => { console.error(err); done() });
  },

  onNewNode: function LV_onNewNode() {
    if (this.isActive() &&
        this.inspector.selection.isConnected() &&
        this.inspector.selection.isElementNode() &&
        this.inspector.selection.reason != "highlighter") {
      this.undim();
    } else {
      this.dim();
    }
    return this.update();
  },

  


  onHighlighterLocked: function LV_onHighlighterLocked() {
    this.undim();
    this.update();
  },

  


  dim: function LV_dim() {
    if (this.browser) {
      this.browser.removeEventListener("MozAfterPaint", this.update, true);
    }
    this.trackingPaint = false;
    this.doc.body.classList.add("dim");
    this.dimmed = true;
  },

  


  undim: function LV_undim() {
    if (!this.trackingPaint) {
      if (this.browser) {
        this.browser.addEventListener("MozAfterPaint", this.update, true);
      }
      this.trackingPaint = true;
    }
    this.doc.body.classList.remove("dim");
    this.dimmed = false;
  },

  



  update: function LV_update() {
    if (!this.isActive() ||
        !this.inspector.selection.isConnected() ||
        !this.inspector.selection.isElementNode()) {
      return promise.resolve(undefined);
    }

    let node = this.inspector.selection.nodeFront;
    let lastRequest = this.inspector.pageStyle.getLayout(node, {
      autoMargins: !this.dimmed
    }).then(layout => {
      
      if (this._lastRequest != lastRequest) {
        return this._lastRequest;
      }
      this._lastRequest = null;
      let width = layout.width;
      let height = layout.height;
      let newLabel = width + "x" + height;
      if (this.sizeHeadingLabel.textContent != newLabel) {
        this.sizeHeadingLabel.textContent = newLabel;
      }

      
      if (this.dimmed) {
        this.inspector.emit("layoutview-updated");
        return;
      }

      for (let i in this.map) {
        let property = this.map[i].property;
        if (!(property in layout)) {
          
          
          continue;
        }
        let parsedValue = parseInt(layout[property]);
        if (Number.isNaN(parsedValue)) {
          
          
          this.map[i].value = layout[property];
        } else {
          this.map[i].value = parsedValue;
        }
      }

      let margins = layout.autoMargins;
      if ("top" in margins) this.map.marginTop.value = "auto";
      if ("right" in margins) this.map.marginRight.value = "auto";
      if ("bottom" in margins) this.map.marginBottom.value = "auto";
      if ("left" in margins) this.map.marginLeft.value = "auto";

      for (let i in this.map) {
        let selector = this.map[i].selector;
        let span = this.doc.querySelector(selector);
        if (span.textContent.length > 0 &&
            span.textContent == this.map[i].value) {
          continue;
        }
        span.textContent = this.map[i].value;
      }

      width -= this.map.borderLeft.value + this.map.borderRight.value +
               this.map.paddingLeft.value + this.map.paddingRight.value;

      height -= this.map.borderTop.value + this.map.borderBottom.value +
                this.map.paddingTop.value + this.map.paddingBottom.value;

      let newValue = width + "x" + height;
      if (this.sizeLabel.textContent != newValue) {
        this.sizeLabel.textContent = newValue;
      }

      this.inspector.emit("layoutview-updated");
    });
    this._lastRequest = lastRequest;
    return this._lastRequest;
  }
}
