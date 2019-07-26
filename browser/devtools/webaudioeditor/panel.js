




"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const EventEmitter = require("devtools/toolkit/event-emitter");
const { WebAudioFront } = require("devtools/server/actors/webaudio");
let Promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;

function WebAudioEditorPanel (iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;
  this._destroyer = null;

  EventEmitter.decorate(this);
}

exports.WebAudioEditorPanel = WebAudioEditorPanel;

WebAudioEditorPanel.prototype = {
  open: function() {
    let targetPromise;

    
    if (!this.target.isRemote) {
      targetPromise = this.target.makeRemote();
    } else {
      targetPromise = Promise.resolve(this.target);
    }

    return targetPromise
      .then(() => {
        this.panelWin.gToolbox = this._toolbox;
        this.panelWin.gTarget = this.target;
        this.panelWin.gFront = new WebAudioFront(this.target.client, this.target.form);
        return this.panelWin.startupWebAudioEditor();
      })
      .then(() => {
        this.isReady = true;
        this.emit("ready");
        return this;
      })
      .then(null, function onError(aReason) {
        Cu.reportError("WebAudioEditorPanel open failed. " +
                       aReason.error + ": " + aReason.message);
      });
  },

  

  get target() this._toolbox.target,

  destroy: function() {
    
    if (this._destroyer) {
      return this._destroyer;
    }

    return this._destroyer = this.panelWin.shutdownWebAudioEditor().then(() => {
      this.emit("destroyed");
    });
  }
};
