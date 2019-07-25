



































const EXPORTED_SYMBOLS = ['Store', 'SnapshotStore',
			  'FormStore',
			  'TabStore'];

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
  _logName: "Store",
  _yieldDuringApply: true,

  __json: null,
  get _json() {
    if (!this.__json)
      this.__json = Cc["@mozilla.org/dom/json;1"].
        createInstance(Ci.nsIJSON);
    return this.__json;
  },

  _init: function Store__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
  },

  applyCommands: function Store_applyCommands(commandList) {
    let self = yield;
    let timer, listener;

    if (this._yieldDuringApply) {
      listener = new Utils.EventListener(self.cb);
      timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }

    for (var i = 0; i < commandList.length; i++) {
      if (this._yieldDuringApply) {
        timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
        yield; 
      }
      var command = commandList[i];
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
    self.done();
  },

  
  wrap: function Store_wrap() {},
  wipe: function Store_wipe() {},
  resetGUIDs: function Store_resetGUIDs() {}
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

  __dirSvc: null,
  get _dirSvc() {
    if (!this.__dirSvc)
      this.__dirSvc = Cc["@mozilla.org/file/directory_service;1"].
        getService(Ci.nsIProperties);
    return this.__dirSvc;
  },

  
  

  _data: {},
  get data() { return this._data; },
  set data(value) { this._data = value; },

  _version: 0,
  get version() { return this._version; },
  set version(value) { this._version = value; },

  _GUID: null,
  get GUID() {
    if (!this._GUID) {
      let uuidgen = Cc["@mozilla.org/uuid-generator;1"].
        getService(Ci.nsIUUIDGenerator);
      this._GUID = uuidgen.generateUUID().toString().replace(/[{}]/g, '');
    }
    return this._GUID;
  },
  set GUID(GUID) {
    this._GUID = GUID;
  },

  _init: function SStore__init(name) {
    this.filename = name;
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
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
      delete this._data[oldGUID]

      for (let GUID in this._data) {
        if (this._data[GUID].parentGUID == oldGUID)
          this._data[GUID].parentGUID = newGUID;
      }
    }
    for (let prop in command.data) {
      if (prop == "GUID")
        continue;
      this._data[command.GUID][prop] = command.data[prop];
    }
  },

  save: function SStore_save() {
    this._log.info("Saving snapshot to disk");

    let file = this._dirSvc.get("ProfD", Ci.nsIFile);
    file.QueryInterface(Ci.nsILocalFile);

    file.append("weave");
    file.append("snapshots");
    file.append(this.filename);
    if (!file.exists())
      file.create(file.NORMAL_FILE_TYPE, PERMS_FILE);

    let out = {version: this.version,
               GUID: this.GUID,
               snapshot: this.data};
    out = this._json.encode(out);

    let [fos] = Utils.open(file, ">");
    fos.writeString(out);
    fos.close();
  },

  load: function SStore_load() {
    let file = this._dirSvc.get("ProfD", Ci.nsIFile);
    file.append("weave");
    file.append("snapshots");
    file.append(this.filename);

    if (!file.exists())
      return;

    try {
      let [is] = Utils.open(file, "<");
      let json = Utils.readStream(is);
      is.close();
      json = this._json.decode(json);

      if (json && 'snapshot' in json && 'version' in json && 'GUID' in json) {
        this._log.info("Read saved snapshot from disk");
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
    json = json.replace(/, parentGUID/g, ",\n\t parentGUID");
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

function FormStore() {
  this._init();
}
FormStore.prototype = {
  _logName: "FormStore",

  __formDB: null,
  get _formDB() {
    if (!this.__formDB) {
      var file = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties).
                 get("ProfD", Ci.nsIFile);
      file.append("formhistory.sqlite");
      var stor = Cc["@mozilla.org/storage/service;1"].
                 getService(Ci.mozIStorageService);
      this.__formDB = stor.openDatabase(file);
    }
    return this.__formDB;
  },

  __formHistory: null,
  get _formHistory() {
    if (!this.__formHistory)
      this.__formHistory = Cc["@mozilla.org/satchel/form-history;1"].
                           getService(Ci.nsIFormHistory2);
    return this.__formHistory;
  },

  _createCommand: function FormStore__createCommand(command) {
    this._log.info("FormStore got createCommand: " + command );
    this._formHistory.addEntry(command.data.name, command.data.value);
  },

  _removeCommand: function FormStore__removeCommand(command) {
    this._log.info("FormStore got removeCommand: " + command );
    this._formHistory.removeEntry(command.data.name, command.data.value);
  },

  _editCommand: function FormStore__editCommand(command) {
    this._log.info("FormStore got editCommand: " + command );
    this._log.warn("Form syncs are expected to only be create/remove!");
  },

  wrap: function FormStore_wrap() {
    var items = [];
    var stmnt = this._formDB.createStatement("SELECT * FROM moz_formhistory");

    while (stmnt.executeStep()) {
      var nam = stmnt.getUTF8String(1);
      var val = stmnt.getUTF8String(2);
      var key = Utils.sha1(nam + val);

      items[key] = { name: nam, value: val };
    }

    return items;
  },

  wipe: function FormStore_wipe() {
    this._formHistory.removeAllEntries();
  },

  resetGUIDs: function FormStore_resetGUIDs() {
    
  }
};
FormStore.prototype.__proto__ = new Store();

function TabStore() {
  this._virtualTabs = {};
  this._init();
}
TabStore.prototype = {
  __proto__: new Store(),

  _logName: "TabStore",

  get _sessionStore() {
    let sessionStore = Cc["@mozilla.org/browser/sessionstore;1"].
		       getService(Ci.nsISessionStore);
    this.__defineGetter__("_sessionStore", function() sessionStore);
    return this._sessionStore;
  },

  get _windowMediator() {
    let windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
			 getService(Ci.nsIWindowMediator);
    this.__defineGetter__("_windowMediator", function() windowMediator);
    return this._windowMediator;
  },

  get _os() {
    let os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    this.__defineGetter__("_os", function() os);
    return this._os;
  },

  get _dirSvc() {
    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    this.__defineGetter__("_dirSvc", function() dirSvc);
    return this._dirSvc;
  },

  










  _virtualTabs: null,

  get virtualTabs() {
    
    
    
    let realTabs = this._wrapRealTabs();
    let virtualTabsChanged = false;
    for (let id in this._virtualTabs) {
      if (id in realTabs) {
        this._log.warn("get virtualTabs: both real and virtual tabs exist for "
                       + id + "; removing virtual one");
        delete this._virtualTabs[id];
        virtualTabsChanged = true;
      }
    }
    if (virtualTabsChanged)
      this._saveVirtualTabs();

    return this._virtualTabs;
  },

  set virtualTabs(newValue) {
    this._virtualTabs = newValue;
    this._saveVirtualTabs();
  },

  
  get _file() {
    let file = this._dirSvc.get("ProfD", Ci.nsILocalFile);
    file.append("weave");
    file.append("store");
    file.append("tabs");
    file.append("virtual.json");
    this.__defineGetter__("_file", function() file);
    return this._file;
  },

  _saveVirtualTabs: function TabStore__saveVirtualTabs() {
    try {
      if (!this._file.exists())
        this._file.create(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
      let out = this._json.encode(this._virtualTabs);
      let [fos] = Utils.open(this._file, ">");
      fos.writeString(out);
      fos.close();
    }
    catch(ex) {
      this._log.warn("could not serialize virtual tabs to disk: " + ex);
    }
  },

  _restoreVirtualTabs: function TabStore__restoreVirtualTabs() {
    try {
      if (this._file.exists()) {
        let [is] = Utils.open(this._file, "<");
        let json = Utils.readStream(is);
        is.close();
        this._virtualTabs = this._json.decode(json);
      }
    }
    catch (ex) {
      this._log.warn("could not parse virtual tabs from disk: " + ex);
    }
  },

  _init: function TabStore__init() {
    this._restoreVirtualTabs();

    this.__proto__.__proto__._init();
  },

  




  applyCommands: function TabStore_applyCommands(commandList) {
    let self = yield;

    this.__proto__.__proto__.applyCommands.async(this, self.cb, commandList);
    yield;

    this._saveVirtualTabs();

    self.done();
  },

  _createCommand: function TabStore__createCommand(command) {
    this._log.debug("_createCommand: " + command.GUID);

    if (command.GUID in this._virtualTabs || command.GUID in this._wrapRealTabs())
      throw "trying to create a tab that already exists; id: " + command.GUID;

    
    this._virtualTabs[command.GUID] = command.data;
    this._os.notifyObservers(null, "weave:store:tabs:virtual:created", null);
  },

  _removeCommand: function TabStore__removeCommand(command) {
    this._log.debug("_removeCommand: " + command.GUID);

    
    
    
    if (command.GUID in this._virtualTabs) {
      delete this._virtualTabs[command.GUID];
      this._os.notifyObservers(null, "weave:store:tabs:virtual:removed", null);
    }
  },

  _editCommand: function TabStore__editCommand(command) {
    this._log.debug("_editCommand: " + command.GUID);

    
    
    

    if (this._virtualTabs[command.GUID])
      this._virtualTabs[command.GUID] = command.data;
  },

  








  wrap: function TabStore_wrap() {
    let items;

    let virtualTabs = this._wrapVirtualTabs();
    let realTabs = this._wrapRealTabs();

    
    
    
    
    items = virtualTabs;
    let virtualTabsChanged = false;
    for (let id in realTabs) {
      
      
      
      
      if (this._virtualTabs[id]) {
        this._log.warn("wrap: both real and virtual tabs exist for " + id +
                       "; removing virtual one");
        delete this._virtualTabs[id];
        virtualTabsChanged = true;
      }

      items[id] = realTabs[id];
    }
    if (virtualTabsChanged)
      this._saveVirtualTabs();

    return items;
  },

  _wrapVirtualTabs: function TabStore__wrapVirtualTabs() {
    let items = {};

    for (let id in this._virtualTabs) {
      let virtualTab = this._virtualTabs[id];

      
      
      
      let item = {};
      for (let property in virtualTab)
        if (property[0] != "_")
          item[property] = virtualTab[property];

      items[id] = item;
    }

    return items;
  },

  _wrapRealTabs: function TabStore__wrapRealTabs() {
    let items = {};

    let session = this._json.decode(this._sessionStore.getBrowserState());

    for (let i = 0; i < session.windows.length; i++) {
      let window = session.windows[i];
      
      
      
      let windowID = i + 1;
      this._log.debug("_wrapRealTabs: window " + windowID);
      for (let j = 0; j < window.tabs.length; j++) {
        let tab = window.tabs[j];

	
	
	let currentEntry = tab.entries[tab.index - 1];

	if (!currentEntry || !currentEntry.url) {
	  this._log.warn("_wrapRealTabs: no current entry or no URL, can't " +
                         "identify " + this._json.encode(tab));
	  continue;
	}

	let tabID = currentEntry.url;
        this._log.debug("_wrapRealTabs: tab " + tabID);

	items[tabID] = {
          
          
	  type: "tab",

          
          
          position: j + 1,

	  windowID: windowID,

	  state: tab
	};
      }
    }

    return items;
  },

  wipe: function TabStore_wipe() {
    
    
    this._virtualTabs = {};
    this._saveVirtualTabs();
  },

  resetGUIDs: function TabStore_resetGUIDs() {
    
  }

};
