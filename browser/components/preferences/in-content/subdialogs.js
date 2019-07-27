



"use strict";

let gSubDialog = {
  _closingCallback: null,
  _frame: null,
  _overlay: null,
  _box: null,
  _injectedStyleSheets: ["chrome://mozapps/content/preferences/preferences.css",
                         "chrome://browser/skin/preferences/preferences.css",
                         "chrome://global/skin/in-content/common.css",
                         "chrome://browser/skin/preferences/in-content/preferences.css",
                         "chrome://browser/skin/preferences/in-content/dialog.css"],

  init: function() {
    this._frame = document.getElementById("dialogFrame");
    this._overlay = document.getElementById("dialogOverlay");
    this._box = document.getElementById("dialogBox");

    
    let dialogClose = document.getElementById("dialogClose");
    dialogClose.addEventListener("command", this.close.bind(this));

    
    let chromeBrowser = window.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebNavigation)
                              .QueryInterface(Ci.nsIDocShell)
                              .chromeEventHandler;
    chromeBrowser.addEventListener("DOMTitleChanged", this.updateTitle, true);

    
    window.addEventListener("DOMFrameContentLoaded", this._onContentLoaded.bind(this), true);

    
    
    this._frame.addEventListener("load", this._onLoad.bind(this));
  },

  uninit: function() {
    let chromeBrowser = window.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebNavigation)
                              .QueryInterface(Ci.nsIDocShell)
                              .chromeEventHandler;
    chromeBrowser.removeEventListener("DOMTitleChanged", gSubDialog.updateTitle, true);
  },

  updateTitle: function(aEvent) {
    if (aEvent.target != gSubDialog._frame.contentDocument)
      return;
    document.getElementById("dialogTitle").textContent = gSubDialog._frame.contentDocument.title;
  },

  injectXMLStylesheet: function(aStylesheetURL) {
    let contentStylesheet = this._frame.contentDocument.createProcessingInstruction(
      'xml-stylesheet',
      'href="' + aStylesheetURL + '" type="text/css"'
    );
    this._frame.contentDocument.insertBefore(contentStylesheet,
                                             this._frame.contentDocument.documentElement);
  },

  open: function(aURL, aFeatures = null, aParams = null, aClosingCallback = null) {
    let features = (!!aFeatures ? aFeatures + "," : "") + "resizable,dialog=no,centerscreen";
    let dialog = window.openDialog(aURL, "dialogFrame", features, aParams);
    if (aClosingCallback) {
      this._closingCallback = aClosingCallback.bind(dialog);
    }
    features = features.replace(/,/g, "&");
    let featureParams = new URLSearchParams(features.toLowerCase());
    this._box.setAttribute("resizable", featureParams.has("resizable") &&
                                        featureParams.get("resizable") != "no" &&
                                        featureParams.get("resizable") != "0");
    return dialog;
  },

  close: function(aEvent = null) {
    if (this._closingCallback) {
      try {
        this._closingCallback.call(null, aEvent);
      } catch (ex) {
        Cu.reportError(ex);
      }
      this._closingCallback = null;
    }

    this._overlay.style.visibility = "";
    
    this._frame.removeAttribute("style");

    setTimeout(() => {
      
      
      this._frame.loadURI("about:blank");
    }, 0);
  },

  

  _onContentLoaded: function(aEvent) {
    if (aEvent.target != this._frame || aEvent.target.contentWindow.location == "about:blank")
      return;

    for (let styleSheetURL of this._injectedStyleSheets) {
      this.injectXMLStylesheet(styleSheetURL);
    }

    
    let oldClose = this._frame.contentWindow.close;
    this._frame.contentWindow.close = function() {
      var closingEvent = new CustomEvent("dialogclosing", {
        bubbles: true,
        detail: { button: null },
      });
      gSubDialog._frame.contentWindow.dispatchEvent(closingEvent);

      oldClose.call(gSubDialog._frame.contentWindow);
    };

    
    
    
    this._overlay.style.visibility = "visible";
    this._overlay.style.opacity = "0.01";

    this._frame.contentWindow.addEventListener("dialogclosing", function closingDialog(aEvent) {
      gSubDialog._frame.contentWindow.removeEventListener("dialogclosing", closingDialog);
      gSubDialog.close(aEvent);
    });
  },

  _onLoad: function(aEvent) {
    if (aEvent.target.contentWindow.location == "about:blank")
      return;

    
    let docEl = this._frame.contentDocument.documentElement;

    
    
    let paddingBottom = parseFloat(this._frame.contentWindow.getComputedStyle(docEl).paddingBottom);

    this._frame.style.width = docEl.style.width || docEl.scrollWidth + "px";
    this._frame.style.height = docEl.style.height || (docEl.scrollHeight + paddingBottom) + "px";

    this._overlay.style.visibility = "visible";
    this._frame.focus();
    this._overlay.style.opacity = ""; 
  },
};
