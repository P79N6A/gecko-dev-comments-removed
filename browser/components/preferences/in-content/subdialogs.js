



"use strict";

let gSubDialog = {
  _closingCallback: null,
  _closingEvent: null,
  _isClosing: false,
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
    this._addDialogEventListeners();

    let features = (!!aFeatures ? aFeatures + "," : "") + "resizable,dialog=no,centerscreen";
    let dialog = window.openDialog(aURL, "dialogFrame", features, aParams);
    if (aClosingCallback) {
      this._closingCallback = aClosingCallback.bind(dialog);
    }

    this._closingEvent = null;
    this._isClosing = false;

    features = features.replace(/,/g, "&");
    let featureParams = new URLSearchParams(features.toLowerCase());
    this._box.setAttribute("resizable", featureParams.has("resizable") &&
                                        featureParams.get("resizable") != "no" &&
                                        featureParams.get("resizable") != "0");
    return dialog;
  },

  close: function(aEvent = null) {
    if (this._isClosing) {
      return;
    }
    this._isClosing = true;

    if (this._closingCallback) {
      try {
        this._closingCallback.call(null, aEvent);
      } catch (ex) {
        Cu.reportError(ex);
      }
      this._closingCallback = null;
    }

    this._removeDialogEventListeners();

    this._overlay.style.visibility = "";
    
    this._frame.removeAttribute("style");
    
    this._box.removeAttribute("width");
    this._box.removeAttribute("height");
    this._box.style.removeProperty("min-height");
    this._box.style.removeProperty("min-width");

    setTimeout(() => {
      
      
      this._frame.loadURI("about:blank");
    }, 0);
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "command":
        this.close(aEvent);
        break;
      case "dialogclosing":
        this._onDialogClosing(aEvent);
        break;
      case "DOMTitleChanged":
        this.updateTitle(aEvent);
        break;
      case "DOMFrameContentLoaded":
        this._onContentLoaded(aEvent);
        break;
      case "load":
        this._onLoad(aEvent);
        break;
      case "unload":
        this._onUnload(aEvent);
        break;
    }
  },

  

  _onUnload: function(aEvent) {
    if (aEvent.target.location.href != "about:blank") {
      this.close(this._closingEvent);
    }
  },

  _onContentLoaded: function(aEvent) {
    if (aEvent.target != this._frame || aEvent.target.contentWindow.location == "about:blank")
      return;

    for (let styleSheetURL of this._injectedStyleSheets) {
      this.injectXMLStylesheet(styleSheetURL);
    }

    
    this._frame.contentDocument.documentElement.setAttribute("subdialog", "true");

    this._frame.contentWindow.addEventListener("dialogclosing", this);

    
    let oldClose = this._frame.contentWindow.close;
    this._frame.contentWindow.close = function() {
      var closingEvent = gSubDialog._closingEvent;
      if (!closingEvent) {
        closingEvent = new CustomEvent("dialogclosing", {
          bubbles: true,
          detail: { button: null },
        });

        gSubDialog._frame.contentWindow.dispatchEvent(closingEvent);
      }

      gSubDialog.close(closingEvent);
      oldClose.call(gSubDialog._frame.contentWindow);
    };

    
    
    
    this._overlay.style.visibility = "visible";
    this._overlay.style.opacity = "0.01";
  },

  _onLoad: function(aEvent) {
    if (aEvent.target.contentWindow.location == "about:blank")
      return;

    
    let docEl = this._frame.contentDocument.documentElement;

    let groupBoxTitle = document.getAnonymousElementByAttribute(this._box, "class", "groupbox-title");
    let groupBoxTitleHeight = groupBoxTitle.clientHeight +
                              parseFloat(getComputedStyle(groupBoxTitle).borderBottomWidth);

    let groupBoxBody = document.getAnonymousElementByAttribute(this._box, "class", "groupbox-body");
    let boxVerticalPadding = 2 * parseFloat(getComputedStyle(groupBoxBody).paddingTop);
    let boxHorizontalPadding = 2 * parseFloat(getComputedStyle(groupBoxBody).paddingLeft);
    let frameWidth = docEl.style.width || docEl.scrollWidth + "px";
    let frameHeight = docEl.style.height || docEl.scrollHeight + "px";
    let boxVerticalBorder = 2 * parseFloat(getComputedStyle(this._box).borderTopWidth);
    let boxHorizontalBorder = 2 * parseFloat(getComputedStyle(this._box).borderLeftWidth);

    let frameRect = this._frame.getBoundingClientRect();
    let boxRect = this._box.getBoundingClientRect();
    let frameSizeDifference = (frameRect.top - boxRect.top) + (boxRect.bottom - frameRect.bottom);

    
    
    let maxHeight = window.innerHeight - frameSizeDifference - 30;
    if (frameHeight > maxHeight) {
      
      frameHeight = maxHeight;
      let containers = this._frame.contentDocument.querySelectorAll('.largeDialogContainer');
      for (let container of containers) {
        container.classList.add("doScroll");
      }
    }

    this._frame.style.width = frameWidth;
    this._frame.style.height = frameHeight;
    this._box.style.minHeight = "calc(" +
                                (boxVerticalBorder + groupBoxTitleHeight + boxVerticalPadding) +
                                "px + " + frameHeight + ")";
    this._box.style.minWidth = "calc(" +
                               (boxHorizontalBorder + boxHorizontalPadding) +
                               "px + " + frameWidth + ")";

    this._overlay.style.visibility = "visible";
    this._frame.focus();
    this._overlay.style.opacity = ""; 
  },

  _onDialogClosing: function(aEvent) {
    this._frame.contentWindow.removeEventListener("dialogclosing", this);
    this._closingEvent = aEvent;
  },

  _addDialogEventListeners: function() {
    
    let dialogClose = document.getElementById("dialogClose");
    dialogClose.addEventListener("command", this);

    
    let chromeBrowser = window.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebNavigation)
                              .QueryInterface(Ci.nsIDocShell)
                              .chromeEventHandler;
    chromeBrowser.addEventListener("DOMTitleChanged", this, true);

    
    window.addEventListener("DOMFrameContentLoaded", this, true);

    
    
    this._frame.addEventListener("load", this);

    chromeBrowser.addEventListener("unload", this, true);
  },

  _removeDialogEventListeners: function() {
    let chromeBrowser = window.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebNavigation)
                              .QueryInterface(Ci.nsIDocShell)
                              .chromeEventHandler;
    chromeBrowser.removeEventListener("DOMTitleChanged", this, true);
    chromeBrowser.removeEventListener("unload", this, true);

    let dialogClose = document.getElementById("dialogClose");
    dialogClose.removeEventListener("command", this);

    window.removeEventListener("DOMFrameContentLoaded", this, true);
    this._frame.removeEventListener("load", this);
    this._frame.contentWindow.removeEventListener("dialogclosing", this);
  }
};
