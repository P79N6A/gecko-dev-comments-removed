



"use strict";

const {Cc, Ci, Cu} = require("chrome");

loader.lazyGetter(this, "promise", () => require("sdk/core/promise"));
loader.lazyGetter(this, "HUDService", () => require("devtools/webconsole/hudservice"));
loader.lazyGetter(this, "EventEmitter", () => require("devtools/shared/event-emitter"));




function WebConsolePanel(iframeWindow, toolbox)
{
  this._frameWindow = iframeWindow;
  this._toolbox = toolbox;
  EventEmitter.decorate(this);
}
exports.WebConsolePanel = WebConsolePanel;

WebConsolePanel.prototype = {
  hud: null,

  





  open: function WCP_open()
  {
    let parentDoc = this._toolbox.doc;
    let iframe = parentDoc.getElementById("toolbox-panel-iframe-webconsole");
    iframe.className = "web-console-frame";

    
    let deferredIframe = promise.defer();
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
      promiseTarget = promise.resolve(this.target);
    }

    
    
    
    return deferredIframe.promise
      .then(() => promiseTarget)
      .then((aTarget) => {
        this._frameWindow._remoteTarget = aTarget;

        let webConsoleUIWindow = iframe.contentWindow.wrappedJSObject;
        let chromeWindow = iframe.ownerDocument.defaultView;
        return HUDService.openWebConsole(this.target, webConsoleUIWindow,
                                         chromeWindow);
      })
      .then((aWebConsole) => {
        this.hud = aWebConsole;
        this._isReady = true;
        this.emit("ready");
        return this;
      }, (aReason) => {
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
