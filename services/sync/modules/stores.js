



































const EXPORTED_SYMBOLS = ['Store', 'SnapshotStore', 'BookmarksStore',
			  'HistoryStore'];

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
    let self = yield, timer, listener;

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
      this._log.debug("Processing command: " + this._json.encode(command));
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

function BookmarksStore() {
  this._init();
}
BookmarksStore.prototype = {
  _logName: "BStore",

  __bms: null,
  get _bms() {
    if (!this.__bms)
      this.__bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                   getService(Ci.nsINavBookmarksService);
    return this.__bms;
  },

  __hsvc: null,
  get _hsvc() {
    if (!this.__hsvc)
      this.__hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                    getService(Ci.nsINavHistoryService);
    return this.__hsvc;
  },

  __ls: null,
  get _ls() {
    if (!this.__ls)
      this.__ls = Cc["@mozilla.org/browser/livemark-service;2"].
        getService(Ci.nsILivemarkService);
    return this.__ls;
  },

  __ms: null,
  get _ms() {
    if (!this.__ms)
      this.__ms = Cc["@mozilla.org/microsummary/service;1"].
        getService(Ci.nsIMicrosummaryService);
    return this.__ms;
  },

  __ts: null,
  get _ts() {
    if (!this.__ts)
      this.__ts = Cc["@mozilla.org/browser/tagging-service;1"].
                  getService(Ci.nsITaggingService);
    return this.__ts;
  },

  __ans: null,
  get _ans() {
    if (!this.__ans)
      this.__ans = Cc["@mozilla.org/browser/annotation-service;1"].
                   getService(Ci.nsIAnnotationService);
    return this.__ans;
  },

  _getItemIdForGUID: function BStore__getItemIdForGUID(GUID) {
    switch (GUID) {
    case "menu":
      return this._bms.bookmarksMenuFolder;
    case "toolbar":
      return this._bms.toolbarFolder;
    case "unfiled":
      return this._bms.unfiledBookmarksFolder;
    default:
      return this._bms.getItemIdForGUID(GUID);
    }
    return null;
  },

  _createCommand: function BStore__createCommand(command) {
    let newId;
    let parentId = this._getItemIdForGUID(command.data.parentGUID);

    if (parentId < 0) {
      this._log.warn("Creating node with unknown parent -> reparenting to root");
      parentId = this._bms.bookmarksMenuFolder;
    }

    switch (command.data.type) {
    case "query":
    case "bookmark":
    case "microsummary": {
      this._log.debug(" -> creating bookmark \"" + command.data.title + "\"");
      let URI = Utils.makeURI(command.data.URI);
      newId = this._bms.insertBookmark(parentId,
                                       URI,
                                       command.data.index,
                                       command.data.title);
      this._ts.untagURI(URI, null);
      this._ts.tagURI(URI, command.data.tags);
      this._bms.setKeywordForBookmark(newId, command.data.keyword);

      if (command.data.type == "microsummary") {
        this._log.debug("   \-> is a microsummary");
        let genURI = Utils.makeURI(command.data.generatorURI);
        try {
          let micsum = this._ms.createMicrosummary(URI, genURI);
          this._ms.setMicrosummary(newId, micsum);
        }
        catch(ex) {  }
      }
    } break;
    case "folder":
      this._log.debug(" -> creating folder \"" + command.data.title + "\"");
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.data.index);
      break;
    case "livemark":
      this._log.debug(" -> creating livemark \"" + command.data.title + "\"");
      newId = this._ls.createLivemark(parentId,
                                      command.data.title,
                                      Utils.makeURI(command.data.siteURI),
                                      Utils.makeURI(command.data.feedURI),
                                      command.data.index);
      break;
    case "separator":
      this._log.debug(" -> creating separator");
      newId = this._bms.insertSeparator(parentId, command.data.index);
      break;
    default:
      this._log.error("_createCommand: Unknown item type: " + command.data.type);
      break;
    }
    if (newId)
      this._bms.setItemGUID(newId, command.GUID);
  },

  _removeCommand: function BStore__removeCommand(command) {
    if (command.GUID == "menu" ||
        command.GUID == "toolbar" ||
        command.GUID == "unfiled") {
      this._log.warn("Attempted to remove root node (" + command.GUID +
                     ").  Skipping command.");
      return;
    }

    var itemId = this._bms.getItemIdForGUID(command.GUID);
    if (itemId < 0) {
      this._log.warn("Attempted to remove item " + command.GUID +
                     ", but it does not exist.  Skipping.");
      return;
    }
    var type = this._bms.getItemType(itemId);

    switch (type) {
    case this._bms.TYPE_BOOKMARK:
      this._log.debug("  -> removing bookmark " + command.GUID);
      this._bms.removeItem(itemId);
      break;
    case this._bms.TYPE_FOLDER:
      this._log.debug("  -> removing folder " + command.GUID);
      this._bms.removeFolder(itemId);
      break;
    case this._bms.TYPE_SEPARATOR:
      this._log.debug("  -> removing separator " + command.GUID);
      this._bms.removeItem(itemId);
      break;
    default:
      this._log.error("removeCommand: Unknown item type: " + type);
      break;
    }
  },

  _editCommand: function BStore__editCommand(command) {
    if (command.GUID == "menu" ||
        command.GUID == "toolbar" ||
        command.GUID == "unfiled") {
      this._log.warn("Attempted to edit root node (" + command.GUID +
                     ").  Skipping command.");
      return;
    }

    var itemId = this._bms.getItemIdForGUID(command.GUID);
    if (itemId < 0) {
      this._log.warn("Item for GUID " + command.GUID + " not found.  Skipping.");
      return;
    }

    for (let key in command.data) {
      switch (key) {
      case "GUID":
        var existing = this._getItemIdForGUID(command.data.GUID);
        if (existing < 0)
          this._bms.setItemGUID(itemId, command.data.GUID);
        else
          this._log.warn("Can't change GUID " + command.GUID +
                         " to " + command.data.GUID + ": GUID already exists.");
        break;
      case "title":
        this._bms.setItemTitle(itemId, command.data.title);
        break;
      case "URI":
        this._bms.changeBookmarkURI(itemId, Utils.makeURI(command.data.URI));
        break;
      case "index":
        this._bms.moveItem(itemId, this._bms.getFolderIdForItem(itemId),
                           command.data.index);
        break;
      case "parentGUID": {
        let index = -1;
        if (command.data.index && command.data.index >= 0)
          index = command.data.index;
        this._bms.moveItem(
          itemId, this._getItemIdForGUID(command.data.parentGUID), index);
      } break;
      case "tags": {
        let tagsURI = this._bms.getBookmarkURI(itemId);
        this._ts.untagURI(URI, null);
        this._ts.tagURI(tagsURI, command.data.tags);
      } break;
      case "keyword":
        this._bms.setKeywordForBookmark(itemId, command.data.keyword);
        break;
      case "generatorURI": {
        let micsumURI = Utils.makeURI(this._bms.getBookmarkURI(itemId));
        let genURI = Utils.makeURI(command.data.generatorURI);
        let micsum = this._ms.createMicrosummary(micsumURI, genURI);
        this._ms.setMicrosummary(itemId, micsum);
      } break;
      case "siteURI":
        this._ls.setSiteURI(itemId, Utils.makeURI(command.data.siteURI));
        break;
      case "feedURI":
        this._ls.setFeedURI(itemId, Utils.makeURI(command.data.feedURI));
        break;
      default:
        this._log.warn("Can't change item property: " + key);
        break;
      }
    }
  },

  _getNode: function BSS__getNode(folder) {
    let query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
  },

  __wrap: function BSS__wrap(node, items, parentGUID, index, guidOverride) {
    let GUID, item;

    
    if (guidOverride) {
      GUID = guidOverride;
      item = {};
    } else {
      GUID = this._bms.getItemGUID(node.itemId);
      item = {parentGUID: parentGUID, index: index};
    }

    if (node.type == node.RESULT_TYPE_FOLDER) {
      if (this._ls.isLivemark(node.itemId)) {
        item.type = "livemark";
        let siteURI = this._ls.getSiteURI(node.itemId);
        let feedURI = this._ls.getFeedURI(node.itemId);
        item.siteURI = siteURI? siteURI.spec : "";
        item.feedURI = feedURI? feedURI.spec : "";
      } else {
        item.type = "folder";
        node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
        node.containerOpen = true;
        for (var i = 0; i < node.childCount; i++) {
          this.__wrap(node.getChild(i), items, GUID, i);
        }
      }
      if (!guidOverride)
        item.title = node.title; 

    } else if (node.type == node.RESULT_TYPE_URI ||
               node.type == node.RESULT_TYPE_QUERY) {
      if (this._ms.hasMicrosummary(node.itemId)) {
        item.type = "microsummary";
        let micsum = this._ms.getMicrosummary(node.itemId);
        item.generatorURI = micsum.generator.uri.spec; 
      } else if (node.type == node.RESULT_TYPE_QUERY) {
        item.type = "query";
        item.title = node.title;
      } else {
        item.type = "bookmark";
        item.title = node.title;
      }
      item.URI = node.uri;
      item.tags = this._ts.getTagsForURI(Utils.makeURI(node.uri), {});
      item.keyword = this._bms.getKeywordForBookmark(node.itemId);

    } else if (node.type == node.RESULT_TYPE_SEPARATOR) {
      item.type = "separator";

    } else {
      this._log.warn("Warning: unknown item type, cannot serialize: " + node.type);
      return;
    }

    items[GUID] = item;
  },

  
  _wrap: function BStore_wrap(node, items, rootName) {
    return this.__wrap(node, items, null, null, rootName);
  },

  _resetGUIDs: function BSS__resetGUIDs(node) {
    if (this._ans.itemHasAnnotation(node.itemId, "placesInternal/GUID"))
      this._ans.removeItemAnnotation(node.itemId, "placesInternal/GUID");

    if (node.type == node.RESULT_TYPE_FOLDER &&
        !this._ls.isLivemark(node.itemId)) {
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;
      for (var i = 0; i < node.childCount; i++) {
        this._resetGUIDs(node.getChild(i));
      }
    }
  },

  wrap: function BStore_wrap() {
    var items = {};
    this._wrap(this._getNode(this._bms.bookmarksMenuFolder), items, "menu");
    this._wrap(this._getNode(this._bms.toolbarFolder), items, "toolbar");
    this._wrap(this._getNode(this._bms.unfiledBookmarksFolder), items, "unfiled");
    return items;
  },

  wipe: function BStore_wipe() {
    this._bms.removeFolderChildren(this._bms.bookmarksMenuFolder);
    this._bms.removeFolderChildren(this._bms.toolbarFolder);
    this._bms.removeFolderChildren(this._bms.unfiledBookmarksFolder);
  },

  resetGUIDs: function BStore_resetGUIDs() {
    this._resetGUIDs(this._getNode(this._bms.bookmarksMenuFolder));
    this._resetGUIDs(this._getNode(this._bms.toolbarFolder));
    this._resetGUIDs(this._getNode(this._bms.unfiledBookmarksFolder));
  }
};
BookmarksStore.prototype.__proto__ = new Store();

function HistoryStore() {
  this._init();
}
HistoryStore.prototype = {
  _logName: "HistStore",

  __hsvc: null,
  get _hsvc() {
    if (!this.__hsvc) {
      this.__hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                    getService(Ci.nsINavHistoryService);
      this.__hsvc.QueryInterface(Ci.nsIGlobalHistory2);
      this.__hsvc.QueryInterface(Ci.nsIBrowserHistory);
    }
    return this.__hsvc;
  },

  _createCommand: function HistStore__createCommand(command) {
    this._log.debug("  -> creating history entry: " + command.GUID);
    try {
      let uri = Utils.makeURI(command.GUID);
      this._hsvc.addVisit(uri, command.data.time, null,
                          this._hsvc.TRANSITION_TYPED, false, null);
      this._hsvc.setPageTitle(uri, command.data.title);
    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));
    }
  },

  _removeCommand: function HistStore__removeCommand(command) {
    this._log.debug("  -> NOT removing history entry: " + command.GUID);
    
    
    
    
  },

  _editCommand: function HistStore__editCommand(command) {
    this._log.debug("  -> FIXME: NOT editing history entry: " + command.GUID);
    
  },

  _historyRoot: function HistStore__historyRoot() {
    let query = this._hsvc.getNewQuery(),
        options = this._hsvc.getNewQueryOptions();

    query.minVisits = 1;
    options.maxResults = 500;
    options.resultType = options.RESULTS_AS_FULL_VISIT;
    options.sortingMode = query.SORT_BY_LASTMODIFIED_DESCENDING;
    options.queryType = options.QUERY_TYPE_HISTORY;

    let root = this._hsvc.executeQuery(query, options).root;
    root.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    return root;
  },

  wrap: function HistStore_wrap() {
    let root = this._historyRoot();
    root.containerOpen = true;
    let items = {};
    for (let i = 0; i < root.childCount; i++) {
      let item = root.getChild(i);
      items[item.uri] = {parentGUID: '',
			 title: item.title,
			 URI: item.uri,
			 time: item.time
			};
      
    }
    return items;
  },

  wipe: function HistStore_wipe() {
    this._hsvc.removeAllPages();
  }
};
HistoryStore.prototype.__proto__ = new Store();
