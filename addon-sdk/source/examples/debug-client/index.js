


"use strict";

const { Panel } = require("dev/panel");
const { Tool } = require("dev/toolbox");
const { Class } = require("sdk/core/heritage");


const LadybugPanel = Class({
  extends: Panel,
  label: "Ladybug",
  tooltip: "Debug client example",
  icon: "./plugin.png",
  url: "./index.html",
  setup: function({debuggee}) {
    this.debuggee = debuggee;
  },
  dispose: function() {
    delete this.debuggee;
  },
  onReady: function() {
    this.debuggee.start();
    this.postMessage("RDP", [this.debuggee]);
  },
});
exports.LadybugPanel = LadybugPanel;


const ladybug = new Tool({
  panels: { ladybug: LadybugPanel }
});
