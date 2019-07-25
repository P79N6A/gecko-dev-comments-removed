





































const EXPORTED_SYMBOLS = ['BookmarksEngine', 'BookmarksSharingManager'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;


const INCOMING_SHARED_ANNO = "weave/shared-incoming";
const OUTGOING_SHARED_ANNO = "weave/shared-outgoing";
const SERVER_PATH_ANNO = "weave/shared-server-path";

const KEYRING_FILE_NAME = "keyring";
const SHARED_BOOKMARK_FILE_NAME = "shared_bookmarks";

const INCOMING_SHARE_ROOT_ANNO = "weave/mounted-shares-folder";
const INCOMING_SHARE_ROOT_NAME = "Shared Folders";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/notifications.js");
Cu.import("resource://weave/resource.js");

Function.prototype.async = Async.sugar;

function BookmarksEngine(pbeId) {
  this._init(pbeId);
}
BookmarksEngine.prototype = {
  __proto__: SyncEngine.prototype,
  get _super() SyncEngine.prototype,

  get name() "bookmarks",
  get displayName() "Bookmarks",
  get logName() "BmkEngine",
  get serverPrefix() "user-data/bookmarks/",

  get _store() {
    let store = new BookmarksStore();
    this.__defineGetter__("_store", function() store);
    return store;
  },

  get _tracker() {
    let tracker = new BookmarksTracker();
    this.__defineGetter__("_tracker", function() tracker);
    return tracker;
  },

  _recordLike: function SyncEngine__recordLike(a, b) {
    if (a.parentid != b.parentid)
      return false;
    for (let key in a.cleartext) {
      if (key == "index")
        continue;
      if (!Utils.deepEquals(a.cleartext[key], b.cleartext[key]))
        return false;
    }
    for (key in b.cleartext) {
      if (key == "index")
        continue;
      if (!Utils.deepEquals(a.cleartext[key], b.cleartext[key]))
        return false;
    }
    return true;
  },

  _changeRecordRefs: function BmkEngine__changeRecordRefs(oldID, newID) {
    let self = yield;
    for each (let rec in this.outgoing) {
      if (rec.parentid == oldID) {
        rec.parentid = newID;
        rec.cleartext.parentid = newID;
        yield rec.encrypt(self.cb, ID.get('WeaveCryptoID').password);
      }
    }
  },

  _changeItemID: function BmkEngine__changeRecordID(oldID, newID) {
    let self = yield;
    yield this._store._changeItemID.async(this._store, self.cb, oldID, newID);
  }

  
  
};

function BookmarksStore() {
  this._init();
}
BookmarksStore.prototype = {
  __proto__: Store.prototype,
  _logName: "BStore",
  _lookup: null,

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

  applyIncoming: function BStore_applyIncoming(onComplete, record) {
    let fn = function(rec) {
      let self = yield;
      if (!record.cleartext)
        this._removeCommand({GUID: record.id});
      else if (this._getItemIdForGUID(record.id) < 0)
        this._createCommand({GUID: record.id, data: record.cleartext});
      else
        this._editCommand({GUID: record.id, data: record.cleartext});
    };
    fn.async(this, onComplete, record);
  },

  _createCommand: function BStore__createCommand(command) {
    let newId;
    let parentId = this._getItemIdForGUID(command.data.parentid);

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
      if (command.data.description) {
        this._ans.setItemAnnotation(newId, "bookmarkProperties/description",
                                    command.data.description, 0,
                                    this._ans.EXPIRE_NEVER);
      }

      if (command.data.type == "microsummary") {
        this._log.debug("   \-> is a microsummary");
        this._ans.setItemAnnotation(newId, "bookmarks/staticTitle",
                                    command.data.staticTitle || "", 0, this._ans.EXPIRE_NEVER);
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
      
      if ( command.data.outgoingSharedAnno != undefined ) {
	this._ans.setItemAnnotation(newId,
				    OUTGOING_SHARED_ANNO,
                                    command.data.outgoingSharedAnno,
				    0,
				    this._ans.EXPIRE_NEVER);
	this._ans.setItemAnnotation(newId,
				    SERVER_PATH_ANNO,
                                    command.data.serverPathAnno,
				    0,
				    this._ans.EXPIRE_NEVER);

      }
      break;
    case "livemark":
      this._log.debug(" -> creating livemark \"" + command.data.title + "\"");
      newId = this._ls.createLivemark(parentId,
                                      command.data.title,
                                      Utils.makeURI(command.data.siteURI),
                                      Utils.makeURI(command.data.feedURI),
                                      command.data.index);
      break;
    case "incoming-share":
      


      this._log.debug(" -> creating incoming-share \"" + command.data.title + "\"");
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.data.index);
      this._ans.setItemAnnotation(newId,
				  INCOMING_SHARED_ANNO,
                                  command.data.incomingSharedAnno,
				  0,
				  this._ans.EXPIRE_NEVER);
      this._ans.setItemAnnotation(newId,
				  SERVER_PATH_ANNO,
                                  command.data.serverPathAnno,
				  0,
				  this._ans.EXPIRE_NEVER);
      break;
    case "separator":
      this._log.debug(" -> creating separator");
      newId = this._bms.insertSeparator(parentId, command.data.index);
      break;
    default:
      this._log.error("_createCommand: Unknown item type: " + command.data.type);
      break;
    }
    if (newId) {
      this._log.trace("Setting GUID of new item " + newId + " to " + command.GUID);
      let cur = this._bms.getItemGUID(newId);
      if (cur == command.GUID)
        this._log.warn("Item " + newId + " already has GUID " + command.GUID);
      else {
        this._bms.setItemGUID(newId, command.GUID);
        Engines.get("bookmarks")._tracker._all[newId] = command.GUID; 
      }
    }
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
      this._log.debug("Item " + command.GUID + "already removed");
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
      this._log.debug("Attempted to edit root node (" + command.GUID +
                      ").  Skipping command.");
      return;
    }

    var itemId = this._getItemIdForGUID(command.GUID);
    if (itemId < 0) {
      this._log.debug("Item for GUID " + command.GUID + " not found.  Skipping.");
      return;
    }
    this._log.trace("Editing item " + itemId);

    for (let key in command.data) {
      switch (key) {
      case "type":
        
        
        break;
      case "title":
        this._bms.setItemTitle(itemId, command.data.title);
        break;
      case "URI":
        this._bms.changeBookmarkURI(itemId, Utils.makeURI(command.data.URI));
        break;
      case "index":
        let curIdx = this._bms.getItemIndex(itemId);
        if (curIdx != command.data.index) {
          
          if (command.data.parentid &&
              (this._bms.getFolderIdForItem(itemId) !=
               this._getItemIdForGUID(command.data.parentid)))
            break;
          this._log.trace("Moving item (changing index)");
          this._bms.moveItem(itemId, this._bms.getFolderIdForItem(itemId),
                             command.data.index);
        }
        break;
      case "parentid": {
        if (command.data.parentid &&
            (this._bms.getFolderIdForItem(itemId) !=
             this._getItemIdForGUID(command.data.parentid))) {
          this._log.trace("Moving item (changing folder)");
          let index = -1;
          if (command.data.index && command.data.index >= 0)
            index = command.data.index;
          this._bms.moveItem(itemId,
                             this._getItemIdForGUID(command.data.parentid), index);
        }
      } break;
      case "tags": {
        
        let tags = command.data.tags.filter(function(t) t);
        let tagsURI = this._bms.getBookmarkURI(itemId);
        this._ts.untagURI(tagsURI, null);
        this._ts.tagURI(tagsURI, tags);
      } break;
      case "keyword":
        this._bms.setKeywordForBookmark(itemId, command.data.keyword);
        break;
      case "description":
        if (command.data.description) {
          this._ans.setItemAnnotation(itemId, "bookmarkProperties/description",
                                      command.data.description, 0,
                                      this._ans.EXPIRE_NEVER);
        }
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
      case "outgoingSharedAnno":
	this._ans.setItemAnnotation(itemId, OUTGOING_SHARED_ANNO,
				    command.data.outgoingSharedAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      case "incomingSharedAnno":
	this._ans.setItemAnnotation(itemId, INCOMING_SHARED_ANNO,
				    command.data.incomingSharedAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      case "serverPathAnno":
	this._ans.setItemAnnotation(itemId, SERVER_PATH_ANNO,
				    command.data.serverPathAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      default:
        this._log.warn("Can't change item property: " + key);
        break;
      }
    }
  },

  _changeItemID: function BSS__changeItemID(oldID, newID) {
    let self = yield;

    var itemId = this._getItemIdForGUID(oldID);
    if (itemId == null) 
      return;
    if (itemId < 0) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                      newID + ": Item does not exist");
      return;
    }

    var collision = this._getItemIdForGUID(newID);
    if (collision >= 0) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                      newID + ": new ID already in use");
      return;
    }

    this._log.debug("Changing GUID " + oldID + " to " + newID);
    this._bms.setItemGUID(itemId, newID);
    Engines.get("bookmarks")._tracker._all[itemId] = newID; 
  },

  _getNode: function BSS__getNode(folder) {
    let query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
  },

  __wrap: function BSS___wrap(node, items, parentid, index, guidOverride) {
    let GUID, item;

    
    if (guidOverride) {
      GUID = guidOverride;
      item = {};
    } else {
      GUID = this._bms.getItemGUID(node.itemId);
      item = {parentid: parentid, index: index};
    }

    if (node.type == node.RESULT_TYPE_FOLDER) {
      if (this._ls.isLivemark(node.itemId)) {
        item.type = "livemark";
        let siteURI = this._ls.getSiteURI(node.itemId);
        let feedURI = this._ls.getFeedURI(node.itemId);
        item.siteURI = siteURI? siteURI.spec : "";
        item.feedURI = feedURI? feedURI.spec : "";
      } else if (this._ans.itemHasAnnotation(node.itemId, INCOMING_SHARED_ANNO)){
	


	item.type = "incoming-share";
	item.title = node.title;
        item.serverPathAnno = this._ans.getItemAnnotation(node.itemId,
                                                      SERVER_PATH_ANNO);
	item.incomingSharedAnno = this._ans.getItemAnnotation(node.itemId,
                                                      INCOMING_SHARED_ANNO);
      } else {
        item.type = "folder";
        node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
        node.containerOpen = true;
	
	if (this._ans.itemHasAnnotation(node.itemId, OUTGOING_SHARED_ANNO)) {
	  item.outgoingSharedAnno = this._ans.getItemAnnotation(node.itemId,
                                                      OUTGOING_SHARED_ANNO);
	}
	if (this._ans.itemHasAnnotation(node.itemId, SERVER_PATH_ANNO)) {
	  item.serverPathAnno = this._ans.getItemAnnotation(node.itemId,
							    SERVER_PATH_ANNO);
	}

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
        item.staticTitle = "";
        try {
          item.staticTitle = this._ans.getItemAnnotation(node.itemId,
                                                         "bookmarks/staticTitle");
        } catch (e) {}
      } else if (node.type == node.RESULT_TYPE_QUERY) {
        item.type = "query";
        item.title = node.title;
      } else {
        item.type = "bookmark";
        item.title = node.title;
      }

      try {
        item.description =
          this._ans.getItemAnnotation(node.itemId, "bookmarkProperties/description");
      } catch (e) {
        item.description = undefined;
      }

      item.URI = node.uri;

      
      
      
      
      let uri;
      try {
        uri = Utils.makeURI(node.uri);
      }
      catch(e) {
        this._log.error("error parsing URI string <" + node.uri + "> " +
                        "for item " + node.itemId + " (" + node.title + "): " +
                        e);
      }

      if (uri)
        item.tags = this._ts.getTagsForURI(uri, {});

      item.keyword = this._bms.getKeywordForBookmark(node.itemId);

    } else if (node.type == node.RESULT_TYPE_SEPARATOR) {
      item.type = "separator";

    } else {
      this._log.warn("Warning: unknown item type, cannot serialize: " + node.type);
      return;
    }

    items[GUID] = item;
  },

  
  _wrap: function BStore__wrap(node, items, rootName) {
    return this.__wrap(node, items, null, null, rootName);
  },

  _wrapMountOutgoing: function BStore__wrapById( itemId ) {
    let node = this._getNode(itemId);
    if (node.type != node.RESULT_TYPE_FOLDER)
      throw "Trying to wrap a non-folder mounted share";

    let GUID = this._bms.getItemGUID(itemId);
    let snapshot = {};
    node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    node.containerOpen = true;
    for (var i = 0; i < node.childCount; i++) {
      this.__wrap(node.getChild(i), snapshot, GUID, i);
    }

    
    for (let guid in snapshot) {
      
      if (snapshot[guid].type == "incoming-share")
        delete snapshot[guid];
    }
    return snapshot;
  },

  findIncomingShares: function BStore_findIncomingShares() {
    

    let ret = [];
    let a = this._ans.getItemsWithAnnotation(INCOMING_SHARED_ANNO, {});
    for (let i = 0; i < a.length; i++) {
      

      let userId = this._ans.getItemAnnotation(a[i], INCOMING_SHARED_ANNO);
      let node = this._getNode(a[i]);
      let GUID = this._bms.getItemGUID(a[i]);
      let path = this._ans.getItemAnnotation(a[i], SERVER_PATH_ANNO);
      let dat = {rootGUID: GUID, userid: userId, serverPath: path, node: node};
      ret.push(dat);
    }
    return ret;
  },

  wrap: function BStore_wrap() {
    var items = {};
    this._wrap(this._getNode(this._bms.bookmarksMenuFolder), items, "menu");
    this._wrap(this._getNode(this._bms.toolbarFolder), items, "toolbar");
    this._wrap(this._getNode(this._bms.unfiledBookmarksFolder), items, "unfiled");
    this._lookup = items;
    return items;
  },

  
  
  
  wrapItem: function BStore_wrapItem(id) {
    if (this._itemCache)
      return this._itemCache[id];
    let all = this.wrap();
    return all[id];
  },

  
  getAllIDs: function BStore_getAllIDs() {
    let all = this.wrap();
    delete all["unfiled"];
    delete all["toolbar"];
    delete all["menu"];
    return all;
  },

  wipe: function BStore_wipe() {
    this._bms.removeFolderChildren(this._bms.bookmarksMenuFolder);
    this._bms.removeFolderChildren(this._bms.toolbarFolder);
    this._bms.removeFolderChildren(this._bms.unfiledBookmarksFolder);
  },

  __resetGUIDs: function BStore___resetGUIDs(node) {
    let self = yield;

    if (this._ans.itemHasAnnotation(node.itemId, "placesInternal/GUID"))
      this._ans.removeItemAnnotation(node.itemId, "placesInternal/GUID");

    if (node.type == node.RESULT_TYPE_FOLDER &&
        !this._ls.isLivemark(node.itemId)) {
      yield Utils.makeTimerForCall(self.cb); 
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;
      for (var i = 0; i < node.childCount; i++) {
        this.__resetGUIDs(node.getChild(i));
      }
    }
  },

  _resetGUIDs: function BStore__resetGUIDs() {
    let self = yield;
    this.__resetGUIDs(this._getNode(this._bms.bookmarksMenuFolder));
    this.__resetGUIDs(this._getNode(this._bms.toolbarFolder));
    this.__resetGUIDs(this._getNode(this._bms.unfiledBookmarksFolder));
  }
};








function BookmarksTracker() {
  this._init();
}
BookmarksTracker.prototype = {
  __proto__: Tracker.prototype,
  _logName: "BmkTracker",
  file: "bookmarks",

  get _bms() {
    let bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
      getService(Ci.nsINavBookmarksService);
    this.__defineGetter__("_bms", function() bms);
    return bms;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver]),

  _init: function BMT__init() {
    this.__proto__.__proto__._init.call(this);

    
    
    
    
    

    
    let store = new BookmarksStore();
    let all = store.wrap();
    this._all = {};
    for (let guid in all) {
      this._all[this._bms.getItemIdForGUID(guid)] = guid;
    }

    this._bms.addObserver(this, false);
  },

  
  _upScore: function BMT__upScore() {
    if (!this.enabled)
      return;
    this._score += 10;
  },

  onItemAdded: function BMT_onEndUpdateBatch(itemId, folder, index) {
    this._log.trace("onItemAdded: " + itemId);

    this._all[itemId] = this._bms.getItemGUID(itemId);
    this.addChangedID(this._all[itemId]);

    this._upScore();
  },

  onItemRemoved: function BMT_onItemRemoved(itemId, folder, index) {
    this._log.trace("onItemRemoved: " + itemId);

    this.addChangedID(this._all[itemId]);
    delete this._all[itemId];

    this._upScore();
  },

  onItemChanged: function BMT_onItemChanged(itemId, property, isAnnotationProperty, value) {
    this._log.trace("onItemChanged: " + itemId + ", property: " + property +
                    ", isAnno: " + isAnnotationProperty + ", value: " + value);

    
    
    
    
    
    
    
    let guid = this._bms.getItemGUID(itemId);
    if (guid != this._all[itemId])
      this._log.trace("GUID change, ignoring");
    else
      this.addChangedID(this._all[itemId]); 

    this._upScore();
  },

  onItemMoved: function BMT_onItemMoved(itemId, oldParent, oldIndex, newParent, newIndex) {
    this._log.trace("onItemMoved: " + itemId);
    this.addChangedID(this._all[itemId]);
    this._upScore();
  },

  onBeginUpdateBatch: function BMT_onBeginUpdateBatch() {},
  onEndUpdateBatch: function BMT_onEndUpdateBatch() {},
  onItemVisited: function BMT_onItemVisited(itemId, aVisitID, time) {}
};
