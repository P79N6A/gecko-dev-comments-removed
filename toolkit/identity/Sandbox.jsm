



"use strict";

this.EXPORTED_SYMBOLS = ["Sandbox"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const XHTML_NS = "http://www.w3.org/1999/xhtml";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
                                  "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");















this.Sandbox = function Sandbox(aURL, aCallback) {
  
  this._url = Services.io.newURI(aURL, null, null).spec;
  this._log("Creating sandbox for:", this._url);
  this._createFrame();
  this._createSandbox(aCallback);
};

this.Sandbox.prototype = {

  


  get id() {
    return this._frame.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  },

  



  reload: function Sandbox_reload(aCallback) {
    this._log("reload:", this.id, ":", this._url);
    this._createSandbox(function createdSandbox(aSandbox) {
      this._log("reloaded sandbox id:", aSandbox.id);
      aCallback(aSandbox);
    }.bind(this));
  },

  


  free: function Sandbox_free() {
    this._log("free:", this.id);
    this._container.removeChild(this._frame);
    this._frame = null;
    this._container = null;
    this._url = null;
  },

  



  _createFrame: function Sandbox__createFrame() {
    let hiddenWindow = Services.appShell.hiddenDOMWindow;
    let doc = hiddenWindow.document;

    
    let frame = doc.createElementNS(XHTML_NS, "iframe");
    frame.setAttribute("mozframetype", "content");
    frame.sandbox = "allow-forms allow-scripts allow-same-origin";
    frame.style.visibility = "collapse";
    doc.documentElement.appendChild(frame);

    let docShell = frame.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                      .getInterface(Ci.nsIWebNavigation)
                                      .QueryInterface(Ci.nsIInterfaceRequestor)
                                      .getInterface(Ci.nsIDocShell);

    
    docShell.stop(Ci.nsIWebNavigation.STOP_NETWORK);

    
    docShell.allowAuth = false;
    docShell.allowPlugins = false;
    docShell.allowImages = false;
    docShell.allowMedia = false;
    docShell.allowWindowControl = false;

    
    let markupDocViewer = docShell.contentViewer
                                  .QueryInterface(Ci.nsIMarkupDocumentViewer);
    markupDocViewer.authorStyleDisabled = true;

    
    this._frame = frame;
    this._container = doc.documentElement;
  },

  _createSandbox: function Sandbox__createSandbox(aCallback) {
    let self = this;
    function _makeSandboxContentLoaded(event) {
      self._log("_makeSandboxContentLoaded:", self.id,
                event.target.location.toString());
      if (event.target != self._frame.contentDocument) {
        return;
      }
      self._frame.removeEventListener(
        "DOMWindowCreated", _makeSandboxContentLoaded, true
      );

      aCallback(self);
    };

    this._frame.addEventListener("DOMWindowCreated",
                                 _makeSandboxContentLoaded,
                                 true);

    
    let webNav = this._frame.contentWindow
                            .QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIWebNavigation);

    webNav.loadURI(
      this._url,
      Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE,
      null, 
      null, 
      null  
    );

  },

  _log: function Sandbox__log(...aMessageArgs) {
    Logger.log.apply(Logger, ["sandbox"].concat(aMessageArgs));
  },

};
