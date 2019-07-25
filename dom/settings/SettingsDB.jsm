



let EXPORTED_SYMBOLS = ["SettingsDB", "SETTINGSDB_NAME", "SETTINGSSTORE_NAME"];


let DEBUG = 0;
if (DEBUG) {
  debug = function (s) { dump("-*- SettingsDB: " + s + "\n"); }
} else {
  debug = function (s) {}
}

const SETTINGSDB_NAME = "settings";
const SETTINGSDB_VERSION = 1;
const SETTINGSSTORE_NAME = "settings";

Components.utils.import("resource://gre/modules/IndexedDBHelper.jsm");

function SettingsDB() {}

SettingsDB.prototype = {

  __proto__: IndexedDBHelper.prototype,

  createSchema: function createSchema(aDb) {
    let objectStore = aDb.createObjectStore(SETTINGSSTORE_NAME, { keyPath: "settingName" });
    objectStore.createIndex("settingValue", "settingValue", { unique: false });
    debug("Created object stores and indexes");
  },

  init: function init(aGlobal) {
      this.initDBHelper(SETTINGSDB_NAME, SETTINGSDB_VERSION, SETTINGSSTORE_NAME, aGlobal);
  }
}
