

































const EXPORTED_SYMBOLS = ["ThemeEngine"];

const Cu = Components.utils;
Cu.import("resource://weave/engines.js");

function ThemeEngine() {
  this._init();
}
ThemeEngine.prototype = {
  get enabled() null, 
  __proto__: SyncEngine.prototype,

  displayName: "Themes",
  logName: "Themes",
  name: "themes",
};
