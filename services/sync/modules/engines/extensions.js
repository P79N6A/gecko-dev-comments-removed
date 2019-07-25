

































const EXPORTED_SYMBOLS = ["ExtensionEngine"];

const Cu = Components.utils;
Cu.import("resource://weave/engines.js");

function ExtensionEngine() {
  this._init();
}
ExtensionEngine.prototype = {
  get enabled() null, 
  __proto__: SyncEngine.prototype,

  _displayName: "Extensions",
  description: "",
  logName: "Extensions",
  name: "extensions",
};
