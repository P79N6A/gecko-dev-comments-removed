





"use strict";

const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");
Cu.import("resource:///modules/devtools/CssLogic.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

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
    this.cssLogic = new CssLogic();

    this.update = this.update.bind(this);
    this.onNewNode = this.onNewNode.bind(this);
    this.onHighlighterLocked = this.onHighlighterLocked.bind(this);
    this.inspector.selection.on("new-node", this.onNewNode);
    this.inspector.sidebar.on("layoutview-selected", this.onNewNode);
    if (this.inspector.highlighter) {
      this.inspector.highlighter.on("locked", this.onHighlighterLocked);
    }

    
    
    
    
    this.map = {
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
    this.inspector.selection.off("new-node", this.onNewNode);
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

  


  onNewNode: function LV_onNewNode() {
    if (this.isActive() &&
        this.inspector.selection.isConnected() &&
        this.inspector.selection.isElementNode() &&
        this.inspector.selection.reason != "highlighter") {
      this.cssLogic.highlight(this.inspector.selection.node);
      this.undim();
    } else {
      this.dim();
    }
    this.update();
  },

  


  onHighlighterLocked: function LV_onHighlighterLocked() {
    this.cssLogic.highlight(this.inspector.selection.node);
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
      return;
    }

    let node = this.inspector.selection.node;

    
    

    let clientRect = node.getBoundingClientRect();
    let width = Math.round(clientRect.width);
    let height = Math.round(clientRect.height);

    let newLabel = width + "x" + height;
    if (this.sizeHeadingLabel.textContent != newLabel) {
      this.sizeHeadingLabel.textContent = newLabel;
    }

    
    if (this.dimmed) return;

    
    let style = node.ownerDocument.defaultView.getComputedStyle(node);

    for (let i in this.map) {
      let property = this.map[i].property;
      this.map[i].value = parseInt(style.getPropertyValue(property));
    }

    let margins = this.processMargins(node);
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
  },

  


  processMargins: function LV_processMargins(node) {
    let margins = {};

    for each (let prop in ["top", "bottom", "left", "right"]) {
      let info = this.cssLogic.getPropertyInfo("margin-" + prop);
      let selectors = info.matchedSelectors;
      if (selectors && selectors.length > 0 && selectors[0].value == "auto") {
        margins[prop] = "auto";
      }
    }

    return margins;
  },
}
