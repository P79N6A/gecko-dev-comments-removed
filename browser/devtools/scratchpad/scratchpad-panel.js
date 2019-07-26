




"use strict";

const EventEmitter = require("devtools/shared/event-emitter");
const promise = require("sdk/core/promise");


function ScratchpadPanel(iframeWindow, toolbox) {
  let { Scratchpad } = iframeWindow;
  this._toolbox = toolbox;
  this.panelWin = iframeWindow;
  this.scratchpad = Scratchpad;
  
  Scratchpad.target = this.target;
  Scratchpad.hideMenu();

  let deferred = promise.defer();
  this._readyObserver = deferred.promise;
  Scratchpad.addObserver({
    onReady: function() {
      Scratchpad.removeObserver(this);
      deferred.resolve();
    }
  });

  EventEmitter.decorate(this);
}
exports.ScratchpadPanel = ScratchpadPanel;

ScratchpadPanel.prototype = {
  



  open: function() {
    return this._readyObserver.then(() => {
      this.isReady = true;
      this.emit("ready");
      return this;
    });
  },

  get target() {
    return this._toolbox.target;
  },

  destroy: function() {
    this.emit("destroyed");
    return promise.resolve();
  }
};
