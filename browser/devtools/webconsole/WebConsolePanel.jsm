



"use strict";

this.EXPORTED_SYMBOLS = [ "WebConsolePanel" ];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "HUDService",
    "resource:///modules/HUDService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "EventEmitter",
    "resource:///modules/devtools/EventEmitter.jsm");




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
    let promise = HUDService.openWebConsole(this.target, iframe);

    return promise.then(function onSuccess(aWebConsole) {
      this.hud = aWebConsole;
      this._isReady = true;
      this.emit("ready");
      return this;
    }.bind(this), function onError(aReason) {
      Cu.reportError("WebConsolePanel open failed. " +
                     aReason.error + ": " + aReason.message);
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
    this._destroyer.then(function() {
      this.emit("destroyed");
    }.bind(this));

    return this._destroyer;
  },
};
