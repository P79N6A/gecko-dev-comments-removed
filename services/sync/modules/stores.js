



































const EXPORTED_SYMBOLS = ['Store',
                          'SnapshotStore'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;






function Store() {
  this._init();
}
Store.prototype = {
  _logName: "BaseClass",

  
  _lookup: null,

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  __json: null,
  get _json() {
    if (!this.__json)
      this.__json = Cc["@mozilla.org/dom/json;1"].
        createInstance(Ci.nsIJSON);
    return this.__json;
  },

  _init: function Store__init() {
    this._log = Log4Moz.repository.getLogger("Store." + this._logName);
  },

  applyIncoming: function BStore_applyIncoming(onComplete, record) {
    let fn = function(rec) {
      let self = yield;
      if (!record.cleartext)
        this.remove(record);
      else if (!this._itemExists(record.id))
        this.create(record);
      else
        this.update(record);
    };
    fn.async(this, onComplete, record);
  },

  
  _applyCommands: function Store__applyCommands(commandList) {
    let self = yield;

    for (var i = 0; i < commandList.length; i++) {
      if (i % 5) {
        Utils.makeTimerForCall(self.cb);
        yield; 
      }

      var command = commandList[i];
      if (this._log.level <= Log4Moz.Level.Trace)
        this._log.trace("Processing command: " + this._json.encode(command));
      switch (command["action"]) {
      case "create":
        this._createCommand(command);
        break;
      case "remove":
        this._removeCommand(command);
        break;
      case "edit":
        this._editCommand(command);
        break;
      default:
        this._log.error("unknown action in command: " + command["action"]);
        break;
      }
    }
  },
  applyCommands: function Store_applyCommands(onComplete, commandList) {
    this._applyCommands.async(this, onComplete, commandList);
  },

  
  _itemExists: function Store__itemExists(GUID) {
    if (GUID in this._lookup)
      return true;
    else
      return false;
  },

  cacheItemsHint: function Store_cacheItemsHint() {
    this._itemCache = this.wrap();
  },

  clearItemCacheHint: function Store_clearItemCacheHint() {
    this._itemCache = null;
  },

  

  wrap: function Store_wrap() {
    throw "override wrap in a subclass";
  },

  wrapItem: function Store_wrapItem() {
    throw "override wrapItem in a subclass";
  },

  changeItemID: function Store_changeItemID(oldID, newID) {
    throw "override changeItemID in a subclass";
  },

  getAllIDs: function Store_getAllIDs() {
    throw "override getAllIDs in a subclass";
  },

  wipe: function Store_wipe() {
    throw "override wipe in a subclass";
  }
};

function SnapshotStore(name) {
  this._init(name);
}
SnapshotStore.prototype = {
  _logName: "SnapStore",

  _filename: null,
  get filename() {
    if (this._filename === null)
      throw "filename is null";
    return this._filename;
  },
  set filename(value) {
    this._filename = value + ".json";
  },

  
  

  _data: {},
  get data() { return this._data; },
  set data(value) { this._data = value; },

  _version: 0,
  get version() { return this._version; },
  set version(value) { this._version = value; },

  _GUID: null,
  get GUID() {
    if (!this._GUID)
      this._GUID = Utils.makeGUID();
    return this._GUID;
  },
  set GUID(GUID) {
    this._GUID = GUID;
  },

  _init: function SStore__init(name) {
    this.filename = name;
    this._log = Log4Moz.repository.getLogger("Service." + this._logName);
  },

  _createCommand: function SStore__createCommand(command) {
    this._data[command.GUID] = Utils.deepCopy(command.data);
  },

  _removeCommand: function SStore__removeCommand(command) {
    delete this._data[command.GUID];
  },

  _editCommand: function SStore__editCommand(command) {
    if ("GUID" in command.data) {
      
      let newGUID = command.data.GUID,
          oldGUID = command.GUID;

      this._data[newGUID] = this._data[oldGUID];
      delete this._data[oldGUID];

      for (let GUID in this._data) {
        if (("parentid" in this._data[GUID]) &&
            (this._data[GUID].parentid == oldGUID))
          this._data[GUID].parentid = newGUID;
      }
    }
    for (let prop in command.data) {
      if (prop == "GUID")
        continue;
      if (command.GUID in this._data)
        this._data[command.GUID][prop] = command.data[prop];
      else
        this._log.warn("Warning! Edit command for unknown item: " + command.GUID);
    }
  },

  save: function SStore_save() {
    this._log.info("Saving snapshot to disk");

    let file = Utils.getProfileFile(
      {path: "weave/snapshots/" + this.filename,
       autoCreate: true}
      );

    let out = {version: this.version,
               GUID: this.GUID,
               snapshot: this.data};
    out = this._json.encode(out);

    let [fos] = Utils.open(file, ">");
    fos.writeString(out);
    fos.close();
  },

  load: function SStore_load() {
    let file = Utils.getProfileFile("weave/snapshots/" + this.filename);
    if (!file.exists())
      return;

    try {
      let [is] = Utils.open(file, "<");
      let json = Utils.readStream(is);
      is.close();
      json = this._json.decode(json);

      if (json && 'snapshot' in json && 'version' in json && 'GUID' in json) {
        this._log.debug("Read saved snapshot from disk");
        this.data = json.snapshot;
        this.version = json.version;
        this.GUID = json.GUID;
      }
    } catch (e) {
      this._log.warn("Could not parse saved snapshot; reverting to initial-sync state");
      this._log.debug("Exception: " + e);
    }
  },

  serialize: function SStore_serialize() {
    let json = this._json.encode(this.data);
    json = json.replace(/:{type/g, ":\n\t{type");
    json = json.replace(/}, /g, "},\n  ");
    json = json.replace(/, parentid/g, ",\n\t parentid");
    json = json.replace(/, index/g, ",\n\t index");
    json = json.replace(/, title/g, ",\n\t title");
    json = json.replace(/, URI/g, ",\n\t URI");
    json = json.replace(/, tags/g, ",\n\t tags");
    json = json.replace(/, keyword/g, ",\n\t keyword");
    return json;
  },

  wrap: function SStore_wrap() {
    return this.data;
  },

  wipe: function SStore_wipe() {
    this.data = {};
    this.version = -1;
    this.GUID = null;
    this.save();
  }
};
SnapshotStore.prototype.__proto__ = new Store();
