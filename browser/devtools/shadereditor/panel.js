




"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const promise = require("sdk/core/promise");
const EventEmitter = require("devtools/shared/event-emitter");
const { WebGLFront } = require("devtools/server/actors/webgl");

function ShaderEditorPanel(iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;
  this._destroyer = null;

  EventEmitter.decorate(this);
};

exports.ShaderEditorPanel = ShaderEditorPanel;

ShaderEditorPanel.prototype = {
  open: function() {
    let targetPromise;

    
    if (!this.target.isRemote) {
      targetPromise = this.target.makeRemote();
    } else {
      targetPromise = promise.resolve(this.target);
    }

    return targetPromise
      .then(() => {
        this.panelWin.gTarget = this.target;
        this.panelWin.gFront = new WebGLFront(this.target.client, this.target.form);
        return this.panelWin.startupShaderEditor();
      })
      .then(() => {
        this.isReady = true;
        this.emit("ready");
        return this;
      })
      .then(null, function onError(aReason) {
        Cu.reportError("ShaderEditorPanel open failed. " +
                       aReason.error + ": " + aReason.message);
      });
  },

  

  get target() this._toolbox.target,

  destroy: function() {
    
    if (this._destroyer) {
      return this._destroyer;
    }

    return this._destroyer = this.panelWin.shutdownShaderEditor().then(() => {
      this.emit("destroyed");
    });
  }
};
