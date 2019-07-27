








"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

let HTMLInputElement = Ci.nsIDOMHTMLInputElement;
let HTMLTextAreaElement = Ci.nsIDOMHTMLTextAreaElement;
let HTMLSelectElement = Ci.nsIDOMHTMLSelectElement;
let HTMLButtonElement = Ci.nsIDOMHTMLButtonElement;

this.EXPORTED_SYMBOLS = [ "FormSubmitObserver" ];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/BrowserUtils.jsm");

function FormSubmitObserver(aWindow, aTabChildGlobal) {
  this.init(aWindow, aTabChildGlobal);
}

FormSubmitObserver.prototype =
{
  _validationMessage: "",
  _content: null,
  _element: null,

  



  init: function(aWindow, aTabChildGlobal)
  {
    this._content = aWindow;
    this._tab = aTabChildGlobal;
    this._mm = 
      this._content.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDocShell)
                   .sameTypeRootTreeItem
                   .QueryInterface(Ci.nsIDocShell)
                   .QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIContentFrameMessageManager);

    
    
    Services.obs.addObserver(this, "invalidformsubmit", false);
    this._tab.addEventListener("pageshow", this, false);
    this._tab.addEventListener("unload", this, false);
  },

  uninit: function()
  {
    Services.obs.removeObserver(this, "invalidformsubmit");
    this._content.removeEventListener("pageshow", this, false);
    this._content.removeEventListener("unload", this, false);
    this._mm = null;
    this._element = null;
    this._content = null;
    this._tab = null;
  },

  



  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "pageshow":
        if (this._isRootDocumentEvent(aEvent)) {
          this._hidePopup();
        }
        break;
      case "unload":
        this.uninit();
        break;
      case "input":
        this._onInput(aEvent);
        break;
      case "blur":
        this._onBlur(aEvent);
        break;
    }
  },

  



  notifyInvalidSubmit : function (aFormElement, aInvalidElements)
  {
    
    
    
    if (!aInvalidElements.length) {
      return;
    }
    
    
    
    if (this._content != aFormElement.ownerDocument.defaultView.top.document.defaultView) {
      return;
    }

    let element = aInvalidElements.queryElementAt(0, Ci.nsISupports);
    if (!(element instanceof HTMLInputElement ||
          element instanceof HTMLTextAreaElement ||
          element instanceof HTMLSelectElement ||
          element instanceof HTMLButtonElement)) {
      return;
    }

    
    if (this._element == element) {
      this._showPopup(element);
      return;
    }
    this._element = element;

    element.focus();

    this._validationMessage = element.validationMessage;

    
    element.addEventListener("input", this, false);

    
    
    element.addEventListener("blur", this, false);

    this._showPopup(element);
  },

  


  
  




  _onInput: function (aEvent) {
    let element = aEvent.originalTarget;

    
    if (element.validity.valid) {
      this._hidePopup();
      return;
    }

    
    
    if (this._validationMessage != element.validationMessage) {
      this._validationMessage = element.validationMessage;
      this._showPopup(element);
    }
  },

  



  _onBlur: function (aEvent) {
    aEvent.originalTarget.removeEventListener("input", this, false);
    aEvent.originalTarget.removeEventListener("blur", this, false);
    this._element = null;
    this._hidePopup();
  },

  




  _showPopup: function (aElement) {
    
    let panelData = {};

    panelData.message = this._validationMessage;

    
    
    panelData.contentRect = this._msgRect(aElement);

    
    
    let offset = 0;
    let position = "";

    if (aElement.tagName == 'INPUT' &&
        (aElement.type == 'radio' || aElement.type == 'checkbox')) {
      panelData.position = "bottomcenter topleft";
    } else {
      let win = aElement.ownerDocument.defaultView;
      let style = win.getComputedStyle(aElement, null);
      if (style.direction == 'rtl') {
        offset = parseInt(style.paddingRight) + parseInt(style.borderRightWidth);
      } else {
        offset = parseInt(style.paddingLeft) + parseInt(style.borderLeftWidth);
      }
      let zoomFactor = this._getWindowUtils().fullZoom;
      panelData.offset = Math.round(offset * zoomFactor);
      panelData.position = "after_start";
    }
    this._mm.sendAsyncMessage("FormValidation:ShowPopup", panelData);
  },

  _hidePopup: function () {
    this._mm.sendAsyncMessage("FormValidation:HidePopup", {});
  },

  _getWindowUtils: function () {
    return this._content.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
  },

  _isRootDocumentEvent: function (aEvent) {
    if (this._content == null) {
      return true;
    }
    let target = aEvent.originalTarget;
    return (target == this._content.document ||
            (target.ownerDocument && target.ownerDocument == this._content.document));
  },

  



  _msgRect: function (aElement) {
    let domRect = aElement.getBoundingClientRect();
    let zoomFactor = this._getWindowUtils().fullZoom;
    let { offsetX, offsetY } = BrowserUtils.offsetToTopLevelWindow(this._content, aElement);
    return {
      left: (domRect.left + offsetX) * zoomFactor,
      top: (domRect.top + offsetY) * zoomFactor,
      width: domRect.width * zoomFactor,
      height: domRect.height * zoomFactor
    };
  },

  QueryInterface : XPCOMUtils.generateQI([Ci.nsIFormSubmitObserver])
};
