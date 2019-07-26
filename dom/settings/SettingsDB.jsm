



let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = ["SettingsDB", "SETTINGSDB_NAME", "SETTINGSSTORE_NAME"];

function debug(s) {
  
}

this.SETTINGSDB_NAME = "settings";
this.SETTINGSDB_VERSION = 1;
this.SETTINGSSTORE_NAME = "settings";

Cu.import("resource://gre/modules/IndexedDBHelper.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

this.SettingsDB = function SettingsDB() {}

SettingsDB.prototype = {

  __proto__: IndexedDBHelper.prototype,

  upgradeSchema: function upgradeSchema(aTransaction, aDb, aOldVersion, aNewVersion) {
    let objectStore = aDb.createObjectStore(SETTINGSSTORE_NAME,
                                            { keyPath: "settingName" });
    objectStore.createIndex("settingValue", "settingValue", { unique: false });
    debug("Created object stores and indexes");

    if (aOldVersion != 0) {
      return;
    }

    
    
    
    let settingsFile = FileUtils.getFile("DefRt", ["settings.json"], false);
    if (!settingsFile || (settingsFile && !settingsFile.exists())) {
      
      
      settingsFile = FileUtils.getFile("ProfD", ["settings.json"], false);
      if (!settingsFile || (settingsFile && !settingsFile.exists())) {
        return;
      }
    }

    let chan = NetUtil.newChannel(settingsFile);
    let stream = chan.open();
    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let rawstr = converter.ConvertToUnicode(NetUtil.readInputStreamToString(
                                            stream,
                                            stream.available()) || "");
    let settings;
    try {
      settings = JSON.parse(rawstr);
    } catch(e) {
      debug("Error parsing " + settingsFile.path + " : " + e);
      return;
    }

    for (let setting in settings) {
      debug("Adding setting " + setting);
      objectStore.put({ settingName: setting,
                        settingValue: settings[setting] });
    }
  },

  init: function init(aGlobal) {
    this.initDBHelper(SETTINGSDB_NAME, SETTINGSDB_VERSION,
                      SETTINGSSTORE_NAME, aGlobal);
  }
}
