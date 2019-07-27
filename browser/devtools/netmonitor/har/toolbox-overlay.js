


"use strict";

const { Cu, Ci } = require("chrome");
const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

loader.lazyRequireGetter(this, "HarAutomation", "devtools/netmonitor/har/har-automation", true);



const overlays = new WeakMap();










function ToolboxOverlay(toolbox) {
  this.toolbox = toolbox;

  this.onInit = this.onInit.bind(this);
  this.onDestroy = this.onDestroy.bind(this);

  this.toolbox.on("ready", this.onInit);
  this.toolbox.on("destroy", this.onDestroy);
}

ToolboxOverlay.prototype = {
  


  onInit: function() {
    let autoExport = Services.prefs.getBoolPref(
      "devtools.netmonitor.har.enableAutoExportToFile");

    if (!autoExport) {
      return;
    }

    this.initAutomation();
  },

  


  onDestroy: function(eventId, toolbox) {
    this.destroyAutomation();
  },

  

  initAutomation: function() {
    this.automation = new HarAutomation(this.toolbox);
  },

  destroyAutomation: function() {
    if (this.automation) {
      this.automation.destroy();
    }
  },
};


function register(toolbox) {
  if (overlays.has(toolbox)) {
    throw Error("There is an existing overlay for the toolbox");
  }

  
  let overlay = new ToolboxOverlay(toolbox);
  overlays.set(toolbox, overlay);
}


exports.register = register;
