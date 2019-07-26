



"use strict";

this.EXPORTED_SYMBOLS = [ "WebConsolePanel" ];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
    "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "HUDService",
    "resource:///modules/HUDService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "EventEmitter",
    "resource:///modules/devtools/shared/event-emitter.js");




function WebConsolePanel(iframeWindow, toolbox) {
  this._frameWindow = iframeWindow;
  this._toolbox = toolbox;
  EventEmitter.decorate(this);
}

WebConsolePanel.prototype = {
  hud: null,

  





  open: function WCP_open()
  {
    let parentDoc = this._toolbox.doc;
    let iframe = parentDoc.getElementById("toolbox-panel-iframe-webconsole");
    iframe.className = "web-console-frame";

    
    let deferredIframe = Promise.defer();
    let win, doc;
    if ((win = iframe.contentWindow) &&
        (doc = win.document) &&
        doc.readyState == "complete") {
      deferredIframe.resolve(null);
    }
    else {
      iframe.addEventListener("load", function onIframeLoad() {
        iframe.removeEventListener("load", onIframeLoad, true);
        deferredIframe.resolve(null);
      }, true);
    }

    
    let promiseTarget;
    if (!this.target.isRemote) {
      promiseTarget = this.target.makeRemote();
    }
    else {
      promiseTarget = Promise.resolve(this.target);
    }

    
    
    
    return deferredIframe.promise
      .then(() => promiseTarget)
      .then((aTarget) => {
        this._frameWindow._remoteTarget = aTarget;

        let webConsoleUIWindow = iframe.contentWindow.wrappedJSObject;
        let chromeWindow = iframe.ownerDocument.defaultView;
        dump("a\n");
        return HUDService.openWebConsole(this.target, webConsoleUIWindow,
                                         chromeWindow);
      })
      .then((aWebConsole) => {
        dump("b\n");
        this.hud = aWebConsole;
        this._isReady = true;
        this.emit("ready");
        return this;
      }, (aReason) => {
        dump(aReason.stack + "\n");
        let msg = "WebConsolePanel open failed. " +
                  aReason.error + ": " + aReason.message;
        dump(msg + "\n");
        Cu.reportError(msg);
      });
  },

  get target() this._toolbox.target,

  _isReady: false,
  get isReady() this._isReady,

  destroy: function WCP_destroy()
  {
    if (this._destroyer) {
      return this._destroyer;
    }

    this._destroyer = this.hud.destroy();
    this._destroyer.then(() => this.emit("destroyed"));

    return this._destroyer;
  },
};
