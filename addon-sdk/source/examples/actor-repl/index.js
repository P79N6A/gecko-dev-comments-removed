


"use strict";

const { Panel } = require("dev/panel");
const { Tool } = require("dev/toolbox");
const { Class } = require("sdk/core/heritage");


const REPLPanel = Class({
  extends: Panel,
  label: "Actor REPL",
  tooltip: "Firefox debugging protocol REPL",
  icon: "./robot.png",
  url: "./index.html",
  setup: function({debuggee}) {
    this.debuggee = debuggee;
  },
  dispose: function() {
    this.debuggee = null;
  },
  onReady: function() {
    console.log("repl panel document is interactive");
    this.debuggee.start();
    this.postMessage("RDP", [this.debuggee]);
  },
  onLoad: function() {
    console.log("repl panel document is fully loaded");
  }
});
exports.REPLPanel = REPLPanel;


const replTool = new Tool({
  panels: { repl: REPLPanel }
});
