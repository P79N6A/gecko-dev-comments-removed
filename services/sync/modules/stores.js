



































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






function Store() {
  this._init();
}
Store.prototype = {
  _logName: "Store",

  _init: function Store__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
  },

  wrap: function Store_wrap() {
  },

  applyCommands: function Store_applyCommands(commandList) {
    for (var i = 0; i < commandList.length; i++) {
      var command = commandList[i];
      this._log.debug("Processing command: " + uneval(command));
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

  resetGUIDs: function Store_resetGUIDs() {
  }
};

function SnapshotStore(name) {
  this._init(name);
}
SnapshotStore.prototype = {
  _logName: "SStore",

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

  save: function SStore_save() {
    this._log.info("Saving snapshot to disk");

    let file = this._dirSvc.get("ProfD", Ci.nsIFile);
    file.append("weave-snapshots");

    file.QueryInterface(Ci.nsILocalFile);
    if (!file.exists())
      file.create(file.DIRECTORY_TYPE, PERMS_DIRECTORY);
    file.QueryInterface(Ci.nsIFile);

    file.append(this.filename);

    file.QueryInterface(Ci.nsILocalFile);
    if (!file.exists())
      file.create(file.NORMAL_FILE_TYPE, PERMS_FILE);

    let fos = Cc["@mozilla.org/network/file-output-stream;1"].
      createInstance(Ci.nsIFileOutputStream);
    let flags = MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE;
    fos.init(file, flags, PERMS_FILE, 0);

    let out = {version: this.version,
               GUID: this.GUID,
               snapshot: this.data};
    out = uneval(out);
    fos.write(out, out.length);
    fos.close();
  },

  load: function SStore_load() {
    let file = this._dirSvc.get("ProfD", Ci.nsIFile);
    file.append("weave-snapshots");
    file.append(this.filename);

    if (!file.exists())
      return;

    let fis = Cc["@mozilla.org/network/file-input-stream;1"].
      createInstance(Ci.nsIFileInputStream);
    fis.init(file, MODE_RDONLY, PERMS_FILE, 0);
    fis.QueryInterface(Ci.nsILineInputStream);

    let json = "";
    while (fis.available()) {
      let ret = {};
      fis.readLine(ret);
      json += ret.value;
    }
    fis.close();
    json = eval(json);

    if (json && 'snapshot' in json && 'version' in json && 'GUID' in json) {
      this._log.info("Read saved snapshot from disk");
      this.data = json.snapshot;
      this.version = json.version;
      this.GUID = json.GUID;
    }
  },

  serialize: function SStore_serialize() {
    let json = uneval(this.data);
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
  },

  applyCommands: function SStore_applyCommands(commands) {
    for (let i = 0; i < commands.length; i++) {
      
      switch (commands[i].action) {
      case "create":
        this._data[commands[i].GUID] = eval(uneval(commands[i].data));
        break;
      case "edit":
        if ("GUID" in commands[i].data) {
          
          let newGUID = commands[i].data.GUID,
              oldGUID = commands[i].GUID;

          this._data[newGUID] = this._data[oldGUID];
          delete this._data[oldGUID]

          for (let GUID in this._data) {
            if (this._data[GUID].parentGUID == oldGUID)
              this._data[GUID].parentGUID = newGUID;
          }
        }
        for (let prop in commands[i].data) {
          if (prop == "GUID")
            continue;
          this._data[commands[i].GUID][prop] = commands[i].data[prop];
        }
        break;
      case "remove":
        delete this._data[commands[i].GUID];
        break;
      }
    }
    return this._data;
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

  _getFolderNodes: function BSS__getFolderNodes(folder) {
    let query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
  },

  _wrapNode: function BSS__wrapNode(node) {
    var items = {};
    this._wrapNodeInternal(node, items, null, null);
    return items;
  },

  _wrapNodeInternal: function BSS__wrapNodeInternal(node, items, parentGUID, index) {
    let GUID = this._bms.getItemGUID(node.itemId);
    let item = {parentGUID: parentGUID,
                index: index};

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
          this._wrapNodeInternal(node.getChild(i), items, GUID, i);
        }
      }
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
      item.tags = this._ts.getTagsForURI(makeURI(node.uri));
      item.keyword = this._bms.getKeywordForBookmark(node.itemId);
    } else if (node.type == node.RESULT_TYPE_SEPARATOR) {
      item.type = "separator";
    } else {
      this._log.warn("Warning: unknown item type, cannot serialize: " + node.type);
      return;
    }

    items[GUID] = item;
  },

  _getWrappedBookmarks: function BSS__getWrappedBookmarks(folder) {
    return this._wrapNode(this._getFolderNodes(folder));
  },

  _resetGUIDsInt: function BSS__resetGUIDsInt(node) {
    if (this._ans.itemHasAnnotation(node.itemId, "placesInternal/GUID"))
      this._ans.removeItemAnnotation(node.itemId, "placesInternal/GUID");

    if (node.type == node.RESULT_TYPE_FOLDER &&
        !this._ls.isLivemark(node.itemId)) {
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;
      for (var i = 0; i < node.childCount; i++) {
        this._resetGUIDsInt(node.getChild(i));
      }
    }
  },

  _createCommand: function BStore__createCommand(command) {
    let newId;
    let parentId = this._bms.getItemIdForGUID(command.data.parentGUID);

    if (parentId < 0) {
      this._log.warn("Creating node with unknown parent -> reparenting to root");
      parentId = this._bms.bookmarksMenuFolder;
    }

    switch (command.data.type) {
    case "query":
    case "bookmark":
    case "microsummary":
      this._log.info(" -> creating bookmark \"" + command.data.title + "\"");
      let URI = makeURI(command.data.URI);
      newId = this._bms.insertBookmark(parentId,
                                       URI,
                                       command.data.index,
                                       command.data.title);
      this._ts.untagURI(URI, null);
      this._ts.tagURI(URI, command.data.tags);
      this._bms.setKeywordForBookmark(newId, command.data.keyword);

      if (command.data.type == "microsummary") {
        this._log.info("   \-> is a microsummary");
        let genURI = makeURI(command.data.generatorURI);
        try {
          let micsum = this._ms.createMicrosummary(URI, genURI);
          this._ms.setMicrosummary(newId, micsum);
        }
        catch(ex) {  }
      }
      break;
    case "folder":
      this._log.info(" -> creating folder \"" + command.data.title + "\"");
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.data.index);
      break;
    case "livemark":
      this._log.info(" -> creating livemark \"" + command.data.title + "\"");
      newId = this._ls.createLivemark(parentId,
                                      command.data.title,
                                      makeURI(command.data.siteURI),
                                      makeURI(command.data.feedURI),
                                      command.data.index);
      break;
    case "separator":
      this._log.info(" -> creating separator");
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
    var itemId = this._bms.getItemIdForGUID(command.GUID);
    if (itemId < 0) {
      this._log.warn("Attempted to remove item " + command.GUID +
                     ", but it does not exist.  Skipping.");
      return;
    }
    var type = this._bms.getItemType(itemId);

    switch (type) {
    case this._bms.TYPE_BOOKMARK:
      this._log.info("  -> removing bookmark " + command.GUID);
      this._bms.removeItem(itemId);
      break;
    case this._bms.TYPE_FOLDER:
      this._log.info("  -> removing folder " + command.GUID);
      this._bms.removeFolder(itemId);
      break;
    case this._bms.TYPE_SEPARATOR:
      this._log.info("  -> removing separator " + command.GUID);
      this._bms.removeItem(itemId);
      break;
    default:
      this._log.error("removeCommand: Unknown item type: " + type);
      break;
    }
  },

  _editCommand: function BStore__editCommand(command) {
    var itemId = this._bms.getItemIdForGUID(command.GUID);
    if (itemId < 0) {
      this._log.warn("Item for GUID " + command.GUID + " not found.  Skipping.");
      return;
    }

    for (let key in command.data) {
      switch (key) {
      case "GUID":
        var existing = this._bms.getItemIdForGUID(command.data.GUID);
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
        this._bms.changeBookmarkURI(itemId, makeURI(command.data.URI));
        break;
      case "index":
        this._bms.moveItem(itemId, this._bms.getFolderIdForItem(itemId),
                           command.data.index);
        break;
      case "parentGUID":
        let index = -1;
        if (command.data.index && command.data.index >= 0)
          index = command.data.index;
        this._bms.moveItem(
          itemId, this._bms.getItemIdForGUID(command.data.parentGUID), index);
        break;
      case "tags":
        let tagsURI = this._bms.getBookmarkURI(itemId);
        this._ts.untagURI(URI, null);
        this._ts.tagURI(tagsURI, command.data.tags);
        break;
      case "keyword":
        this._bms.setKeywordForBookmark(itemId, command.data.keyword);
        break;
      case "generatorURI":
        let micsumURI = makeURI(this._bms.getBookmarkURI(itemId));
        let genURI = makeURI(command.data.generatorURI);
        let micsum = this._ms.createMicrosummary(micsumURI, genURI);
        this._ms.setMicrosummary(itemId, micsum);
        break;
      case "siteURI":
        this._ls.setSiteURI(itemId, makeURI(command.data.siteURI));
        break;
      case "feedURI":
        this._ls.setFeedURI(itemId, makeURI(command.data.feedURI));
        break;
      default:
        this._log.warn("Can't change item property: " + key);
        break;
      }
    }
  },

  wrap: function BStore_wrap() {
    let filed = this._getWrappedBookmarks(this._bms.bookmarksMenuFolder);
    let toolbar = this._getWrappedBookmarks(this._bms.toolbarFolder);
    let unfiled = this._getWrappedBookmarks(this._bms.unfiledBookmarksFolder);

    for (let guid in unfiled) {
      if (!(guid in filed))
        filed[guid] = unfiled[guid];
    }

    for (let guid in toolbar) {
      if (!(guid in filed))
        filed[guid] = toolbar[guid];
    }

    return filed; 
  },

  resetGUIDs: function BStore_resetGUIDs() {
    this._resetGUIDsInt(this._getFolderNodes(this._bms.bookmarksMenuFolder));
    this._resetGUIDsInt(this._getFolderNodes(this._bms.toolbarFolder));
    this._resetGUIDsInt(this._getFolderNodes(this._bms.unfiledBookmarksFolder));
  },

  applyCommands: function BStore_applyCommands(commandList) {
    for (var i = 0; i < commandList.length; i++) {
      var command = commandList[i];
      this._log.debug("Processing command: " + uneval(command));
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
    if (!this.__hsvc)
      this.__hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                    getService(Ci.nsINavHistoryService);
    return this.__hsvc;
  },

  _createCommand: function HistStore__createCommand(command) {
    this._log.info("  -> creating history entry: " + command.GUID);
    this._hsvc.addVisit(makeURI(command.data.URI), command.data.time,
			0, this._hsvc.TRANSITION_LINK, false, 0);
    this._hsvc.addVisit(makeURI(command.data.URI), command.data.title,
			command.data.accessCount, false, false);
  },

  _removeCommand: function HistStore__removeCommand(command) {
    this._log.info("  -> NOT removing history entry: " + command.GUID);
    
  },

  _editCommand: function HistStore__editCommand(command) {
    this._log.info("  -> FIXME: NOT editing history entry: " + command.GUID);
    
  },

  wrap: function HistStore_wrap() {
    let query = this._hsvc.getNewQuery();
    query.minVisits = 1;
    let root = this._hsvc.executeQuery(query,
				       this._hsvc.getNewQueryOptions()).root;
    root.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    root.containerOpen = true;
    let items = {};
    for (let i = 0; i < root.childCount; i++) {
      let item = root.getChild(i);
      items[item.uri] = {parentGUID: '',
			 title: item.title,
			 URI: item.uri,
			 time: item.time,
			 accessCount: item.accessCount,
			 dateAdded: item.dateAdded,
			};
    }
    return items;
  }
};
HistoryStore.prototype.__proto__ = new Store();
