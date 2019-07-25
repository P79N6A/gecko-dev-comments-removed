

































const EXPORTED_SYMBOLS = ["MicroFormatEngine"];

const Cu = Components.utils;
Cu.import("resource://weave/engines.js");

function MicroFormatEngine() {
  this._init();
}
MicroFormatEngine.prototype = {
  get enabled() null, 
  __proto__: SyncEngine.prototype,

  _displayName: "MicroFormats",
  description: "",
  logName: "MicroFormats",
  name: "microformats",
};
