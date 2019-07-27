







"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "FormValidationHandler" ];

Cu.import("resource://gre/modules/Services.jsm");

let FormValidationHandler =
{
  _panel: null,
  _anchor: null,

  



  init: function () {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.addMessageListener("FormValidation:ShowPopup", this);
    mm.addMessageListener("FormValidation:HidePopup", this);
  },

  uninit: function () {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.removeMessageListener("FormValidation:ShowPopup", this);
    mm.removeMessageListener("FormValidation:HidePopup", this);
    this._panel = null;
    this._anchor = null;
  },

  hidePopup: function () {
    this._hidePopup();
  },

  



  receiveMessage: function (aMessage) {
    let window = aMessage.target.ownerDocument.defaultView;
    let json = aMessage.json;
    let tabBrowser = window.gBrowser;
    switch (aMessage.name) {
      case "FormValidation:ShowPopup":
        
        
        if (tabBrowser && aMessage.target != tabBrowser.selectedBrowser) {
          return;
        }
        this._showPopup(window, json);
        break;
      case "FormValidation:HidePopup":
        this._hidePopup();
        break;
    }
  },

  observe: function (aSubject, aTopic, aData) {
    this._hidePopup();
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "FullZoomChange":
      case "TextZoomChange":
      case "ZoomChangeUsingMouseWheel":
      case "scroll":
        this._hidePopup();
        break;
      case "popuphiding":
        this._onPopupHiding(aEvent);
        break;
    }
  },

  



  _onPopupHiding: function (aEvent) {
    aEvent.originalTarget.removeEventListener("popuphiding", this, true);
    let tabBrowser = aEvent.originalTarget.ownerDocument.getElementById("content");
    tabBrowser.selectedBrowser.removeEventListener("scroll", this, true);
    tabBrowser.selectedBrowser.removeEventListener("FullZoomChange", this, false);
    tabBrowser.selectedBrowser.removeEventListener("TextZoomChange", this, false);
    tabBrowser.selectedBrowser.removeEventListener("ZoomChangeUsingMouseWheel", this, false);

    this._panel.hidden = true;
    this._panel = null;
    this._anchor.hidden = true;
    this._anchor = null;
  },

  












  _showPopup: function (aWindow, aPanelData) {
    let previouslyShown = !!this._panel;
    this._panel = aWindow.document.getElementById("invalid-form-popup");
    this._panel.firstChild.textContent = aPanelData.message;
    this._panel.hidden = false;

    let tabBrowser = aWindow.gBrowser;
    this._anchor = tabBrowser.formValidationAnchor;
    this._anchor.left = aPanelData.contentRect.left;
    this._anchor.top = aPanelData.contentRect.top;
    this._anchor.width = aPanelData.contentRect.width;
    this._anchor.height = aPanelData.contentRect.height;
    this._anchor.hidden = false;

    
    if (!previouslyShown) {
      
      this._panel.addEventListener("popuphiding", this, true);

      
      tabBrowser.selectedBrowser.addEventListener("scroll", this, true);
      tabBrowser.selectedBrowser.addEventListener("FullZoomChange", this, false);
      tabBrowser.selectedBrowser.addEventListener("TextZoomChange", this, false);
      tabBrowser.selectedBrowser.addEventListener("ZoomChangeUsingMouseWheel", this, false);

      
      this._panel.openPopup(this._anchor, aPanelData.position, 0, 0, false);
    }
  },

  



  _hidePopup: function () {
    if (this._panel) {
      this._panel.hidePopup();
    }
  }
};
