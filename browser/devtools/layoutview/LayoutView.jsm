






































"use strict";

const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/inspector.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");

var EXPORTED_SYMBOLS = ["LayoutView"];

function LayoutView(aOptions)
{
  this.chromeDoc = aOptions.document;
  this.inspector = aOptions.inspector;
  this.browser = this.inspector.chromeWindow.gBrowser;

  this.init();
}

LayoutView.prototype = {
  init: function LV_init() {

    this.update = this.update.bind(this);
    this.onMessage = this.onMessage.bind(this);

    this.isOpen = false;
    this.documentReady = false;

    
    if (!("_layoutViewIsOpen" in this.inspector)) {
      this.inspector._layoutViewIsOpen =
        Services.prefs.getBoolPref("devtools.layoutview.open");
    }

    
    
    
    function onLock() {
      this.undim();
      this.update();
      
      if (!this.trackingPaint) {
        this.browser.addEventListener("MozAfterPaint", this.update, true);
        this.trackingPaint = true;
      }
    }

    function onUnlock() {
      this.browser.removeEventListener("MozAfterPaint", this.update, true);
      this.trackingPaint = false;
      this.dim();
    }

    this.onLock = onLock.bind(this);
    this.onUnlock = onUnlock.bind(this);
    this.inspector.on("locked", this.onLock);
    this.inspector.on("unlocked", this.onUnlock);

    
    this.buildView();

    
    this.inspector.chromeWindow.addEventListener("message", this.onMessage, true);

    
    
    
    
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
  },

  


  destroy: function LV_destroy() {
    this.inspector.removeListener("locked", this.onLock);
    this.inspector.removeListener("unlocked", this.onUnlock);
    this.browser.removeEventListener("MozAfterPaint", this.update, true);
    this.inspector.chromeWindow.removeEventListener("message", this.onMessage, true);
    this.close();
    this.iframe = null;
    this.view.parentNode.removeChild(this.view);
  },

  






  buildView: function LV_buildPanel() {
    this.iframe = this.chromeDoc.createElement("iframe");
    this.iframe.setAttribute("src", "chrome://browser/content/devtools/layoutview/view.xhtml");

    this.view = this.chromeDoc.createElement("vbox");
    this.view.id = "inspector-layoutview-container";
    this.view.appendChild(this.iframe);

    let sidebar = this.chromeDoc.getElementById("devtools-sidebar-box");
    sidebar.appendChild(this.view);
  },

  


  onDocumentReady: function LV_onDocumentReady() {
    this.documentReady = true;
    this.doc = this.iframe.contentDocument;

    
    

    if (this.inspector.locked)
      this.onLock();
    else
      this.onUnlock();

    if (this.inspector._layoutViewIsOpen) {
      this.open();
    } else {
      this.close();
    }

  },

  


  onMessage: function LV_onMessage(e) {
    switch (e.data) {
      case "layoutview-toggle-view":
        this.toggle(true);
        break;
      case "layoutview-ready":
        this.onDocumentReady();
        break;
      default:
        break;
    }
  },

  





  open: function LV_open(aUserAction) {
    this.isOpen = true;
    if (this.documentReady)
      this.doc.body.classList.add("open");
    if (aUserAction) {
      this.inspector._layoutViewIsOpen = true;
      Services.prefs.setBoolPref("devtools.layoutview.open", true);
    }
    this.iframe.setAttribute("open", "true");
    this.update();
  },

  





  close: function LV_close(aUserAction) {
    this.isOpen = false;
    if (this.documentReady)
      this.doc.body.classList.remove("open");
    if (aUserAction) {
      this.inspector._layoutViewIsOpen = false;
      Services.prefs.setBoolPref("devtools.layoutview.open", false);
    }
    this.iframe.removeAttribute("open");
  },

  





  toggle: function LV_toggle(aUserAction) {
    this.isOpen ? this.close(aUserAction):this.open(aUserAction);
  },

  


  dim: function LV_dim() {
    if (!this.documentReady) return;
    this.doc.body.classList.add("dim");
  },

  


  undim: function LV_dim() {
    if (!this.documentReady) return;
    this.doc.body.classList.remove("dim");
  },

  



  update: function LV_update() {
    let node = this.inspector.selection;
    if (!node || !this.documentReady) return;

    
    

    let clientRect = node.getBoundingClientRect();
    let width = Math.round(clientRect.width);
    let height = Math.round(clientRect.height);
    this.doc.querySelector("#element-size").textContent =  width + "x" + height;

    
    if (!this.isOpen) return;

    
    let style = this.browser.contentWindow.getComputedStyle(node);;

    for (let i in this.map) {
      let selector = this.map[i].selector;
      let property = this.map[i].property;
      this.map[i].value = parseInt(style.getPropertyValue(property));
      let span = this.doc.querySelector(selector);
      span.textContent = this.map[i].value;
    }

    width -= this.map.borderLeft.value + this.map.borderRight.value +
             this.map.paddingLeft.value + this.map.paddingRight.value;

    height -= this.map.borderTop.value + this.map.borderBottom.value +
              this.map.paddingTop.value + this.map.paddingBottom.value;

    this.doc.querySelector(".size > span").textContent = width + "x" + height;
  },
}
