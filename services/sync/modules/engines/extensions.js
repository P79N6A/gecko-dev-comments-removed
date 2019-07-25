

































const EXPORTED_SYMBOLS = ["ExtensionEngine"];

const Cu = Components.utils;
Cu.import("resource://weave/engines.js");

function ExtensionEngine() {
  this._init();
}
ExtensionEngine.prototype = {
  __proto__: SyncEngine.prototype,

  displayName: "Extensions",
  logName: "Extensions",
  name: "extensions",
};
