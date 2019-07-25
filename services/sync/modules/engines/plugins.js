

































const EXPORTED_SYMBOLS = ["PluginEngine"];

const Cu = Components.utils;
Cu.import("resource://weave/engines.js");

function PluginEngine() {
  this._init();
}
PluginEngine.prototype = {
  __proto__: SyncEngine.prototype,

  displayName: "Plugins",
  logName: "Plugins",
  name: "plugins",
};
