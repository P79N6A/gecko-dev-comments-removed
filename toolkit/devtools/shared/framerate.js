


"use strict";

const { on, once, off, emit } = require("sdk/event/core");
const { Class } = require("sdk/core/heritage");






let Framerate = exports.Framerate = Class({
  initialize: function (tabActor) {
    this.tabActor = tabActor;
    this._contentWin = tabActor.window;
    this._onRefreshDriverTick = this._onRefreshDriverTick.bind(this);
    this._onGlobalCreated = this._onGlobalCreated.bind(this);
    on(this.tabActor, "window-ready", this._onGlobalCreated);
  },
  destroy: function(conn) {
    off(this.tabActor, "window-ready", this._onGlobalCreated);
    this.stopRecording();
  },

  


  startRecording: function () {
    if (this._recording) {
      return;
    }
    this._recording = true;
    this._ticks = [];
    this._startTime = this.tabActor.docShell.now();
    this._rafID = this._contentWin.requestAnimationFrame(this._onRefreshDriverTick);
  },

  


  stopRecording: function (beginAt = 0, endAt = Number.MAX_SAFE_INTEGER) {
    if (!this._recording) {
      return [];
    }
    let ticks = this.getPendingTicks(beginAt, endAt);
    this.cancelRecording();
    return ticks;
  },

  


  cancelRecording: function () {
    this._contentWin.cancelAnimationFrame(this._rafID);
    this._recording = false;
    this._ticks = null;
    this._rafID = -1;
  },

  


  isRecording: function () {
    return !!this._recording;
  },

  


  getPendingTicks: function (beginAt = 0, endAt = Number.MAX_SAFE_INTEGER) {
    if (!this._ticks) {
      return [];
    }
    return this._ticks.filter(e => e >= beginAt && e <= endAt);
  },

  


  _onRefreshDriverTick: function () {
    if (!this._recording) {
      return;
    }
    this._rafID = this._contentWin.requestAnimationFrame(this._onRefreshDriverTick);
    this._ticks.push(this.tabActor.docShell.now() - this._startTime);
  },

  


  _onGlobalCreated: function (win) {
    if (this._recording) {
      this._rafID = this._contentWin.requestAnimationFrame(this._onRefreshDriverTick);
    }
  }
});
