



"use strict";

const { Cu } = require("chrome");
Cu.import("resource://gre/modules/Services.jsm");
const { gDevTools } = require("resource:///modules/devtools/gDevTools.jsm");

const { defaultTools, defaultThemes } = require("definitions");

defaultTools.forEach(definition => gDevTools.registerTool(definition));
defaultThemes.forEach(definition => gDevTools.registerTheme(definition));



exports.defaultTools = require("definitions").defaultTools;
exports.defaultThemes = require("definitions").defaultThemes;
exports.Tools = require("definitions").Tools;

Object.defineProperty(exports, "Toolbox", {
  get: () => require("devtools/framework/toolbox").Toolbox
});
Object.defineProperty(exports, "TargetFactory", {
  get: () => require("devtools/framework/target").TargetFactory
});

const unloadObserver = {
  observe: function(subject, topic, data) {
    if (subject.wrappedJSObject === require("@loader/unload")) {
      Services.obs.removeObserver(unloadObserver, "sdk:loader:destroy");
      for (let definition of gDevTools.getToolDefinitionArray()) {
        gDevTools.unregisterTool(definition.id);
      }
      for (let definition of gDevTools.getThemeDefinitionArray()) {
        gDevTools.unregisterTheme(definition.id);
      }
    }
  }
};
Services.obs.addObserver(unloadObserver, "sdk:loader:destroy", false);

const events = require("sdk/system/events");
events.emit("devtools-loaded", {});
