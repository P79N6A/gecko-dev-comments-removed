





































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

const SERVICE_NOT_SUPPORTED = "Service not supported on this platform";

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
Cu.import("resource://weave/type_records/bookmark.js");

Function.prototype.async = Async.sugar;

function BookmarksEngine() {
  this._init();
}
BookmarksEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "bookmarks",
  displayName: "Bookmarks",
  logName: "Bookmarks",
  _storeObj: BookmarksStore,
  _trackerObj: BookmarksTracker
};

function BookmarksStore() {
  this._init();
}
BookmarksStore.prototype = {
  __proto__: Store.prototype,
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

  get _ms() {
    let ms;
    try {
      ms = Cc["@mozilla.org/microsummary/service;1"].
        getService(Ci.nsIMicrosummaryService);
    } catch (e) {
      ms = null;
      this._log.warn("Could not load microsummary service");
      this._log.debug(e);
    }
    this.__defineGetter__("_ms", function() ms);
    return ms;
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

  _getWeaveIdForItem: function BStore__getWeaveIdForItem(placeId) {
    if (placeId == this._bms.bookmarksMenuFolder)
      return "menu";
    if (placeId == this._bms.toolbarFolder)
      return "toolbar";
    if (placeId == this._bms.unfiledBookmarksFolder)
      return "unfiled";
    return this._bms.getItemGUID(placeId);
  },

  _isToplevel: function BStore__isToplevel(placeId) {
    if (placeId == this._bms.bookmarksMenuFolder)
      return true;
    if (placeId == this._bms.toolbarFolder)
      return true;
    if (placeId == this._bms.unfiledBookmarksFolder)
      return true;
    if (this._bms.getFolderIdForItem(placeId) <= 0)
      return true;
    return false;
  },

  itemExists: function BStore_itemExists(id) {
    return this._getItemIdForGUID(id) > 0;
  },

  create: function BStore_create(record) {
    let newId;
    let parentId = this._getItemIdForGUID(record.parentid);

    if (parentId <= 0) {
      this._log.warn("Creating node with unknown parent -> reparenting to root");
      parentId = this._bms.bookmarksMenuFolder;
    }

    switch (record.cleartext.type) {
    case "query":
    case "bookmark":
    case "microsummary": {
      this._log.debug(" -> creating bookmark \"" + record.cleartext.title + "\"");
      let uri = Utils.makeURI(record.cleartext.uri);
      newId = this._bms.insertBookmark(parentId, uri, record.sortindex,
                                       record.cleartext.title);
      this._ts.untagURI(uri, null);
      this._ts.tagURI(uri, record.cleartext.tags);
      this._bms.setKeywordForBookmark(newId, record.cleartext.keyword);
      if (record.cleartext.description) {
        this._ans.setItemAnnotation(newId, "bookmarkProperties/description",
                                    record.cleartext.description, 0,
                                   this._ans.EXPIRE_NEVER);
      }

      if (record.cleartext.type == "microsummary") {
        this._log.debug("   \-> is a microsummary");
        this._ans.setItemAnnotation(newId, "bookmarks/staticTitle",
                                    record.cleartext.staticTitle || "", 0, this._ans.EXPIRE_NEVER);
        let genURI = Utils.makeURI(record.cleartext.generatorURI);
	if (this._ms) {
          try {
            let micsum = this._ms.createMicrosummary(uri, genURI);
            this._ms.setMicrosummary(newId, micsum);
          }
          catch(ex) {  }
	} else {
	  this._log.warn("Can't create microsummary -- not supported.");
	}
      }
    } break;
    case "folder":
      this._log.debug(" -> creating folder \"" + record.cleartext.title + "\"");
      newId = this._bms.createFolder(parentId,
                                     record.cleartext.title,
                                     record.sortindex);
      
      if ( record.cleartext.outgoingSharedAnno != undefined ) {
	this._ans.setItemAnnotation(newId,
				    OUTGOING_SHARED_ANNO,
                                    record.cleartext.outgoingSharedAnno,
				    0,
				    this._ans.EXPIRE_NEVER);
	this._ans.setItemAnnotation(newId,
				    SERVER_PATH_ANNO,
                                    record.cleartext.serverPathAnno,
				    0,
				    this._ans.EXPIRE_NEVER);

      }
      break;
    case "livemark":
      this._log.debug(" -> creating livemark \"" + record.cleartext.title + "\"");
      newId = this._ls.createLivemark(parentId,
                                      record.cleartext.title,
                                      Utils.makeURI(record.cleartext.siteURI),
                                      Utils.makeURI(record.cleartext.feedURI),
                                      record.sortindex);
      break;
    case "incoming-share":
      


      this._log.debug(" -> creating incoming-share \"" + record.cleartext.title + "\"");
      newId = this._bms.createFolder(parentId,
                                     record.cleartext.title,
                                     record.sortindex);
      this._ans.setItemAnnotation(newId,
				  INCOMING_SHARED_ANNO,
                                  record.cleartext.incomingSharedAnno,
				  0,
				  this._ans.EXPIRE_NEVER);
      this._ans.setItemAnnotation(newId,
				  SERVER_PATH_ANNO,
                                  record.cleartext.serverPathAnno,
				  0,
				  this._ans.EXPIRE_NEVER);
      break;
    case "separator":
      this._log.debug(" -> creating separator");
      newId = this._bms.insertSeparator(parentId, record.sortindex);
      break;
    default:
      this._log.error("_create: Unknown item type: " + record.cleartext.type);
      break;
    }
    if (newId) {
      this._log.trace("Setting GUID of new item " + newId + " to " + record.id);
      let cur = this._bms.getItemGUID(newId);
      if (cur == record.id)
        this._log.warn("Item " + newId + " already has GUID " + record.id);
      else {
        this._bms.setItemGUID(newId, record.id);
        Engines.get("bookmarks")._tracker._all[newId] = record.id; 
      }
    }
  },

  remove: function BStore_remove(record) {
    if (record.id == "menu" ||
        record.id == "toolbar" ||
        record.id == "unfiled") {
      this._log.warn("Attempted to remove root node (" + record.id +
                     ").  Skipping record removal.");
      return;
    }

    var itemId = this._bms.getItemIdForGUID(record.id);
    if (itemId <= 0) {
      this._log.debug("Item " + record.id + " already removed");
      return;
    }
    var type = this._bms.getItemType(itemId);

    switch (type) {
    case this._bms.TYPE_BOOKMARK:
      this._log.debug("  -> removing bookmark " + record.id);
      this._bms.removeItem(itemId);
      break;
    case this._bms.TYPE_FOLDER:
      this._log.debug("  -> removing folder " + record.id);
      this._bms.removeFolder(itemId);
      break;
    case this._bms.TYPE_SEPARATOR:
      this._log.debug("  -> removing separator " + record.id);
      this._bms.removeItem(itemId);
      break;
    default:
      this._log.error("remove: Unknown item type: " + type);
      break;
    }
  },

  update: function BStore_update(record) {
    let itemId = this._getItemIdForGUID(record.id);

    if (record.id == "menu" ||
        record.id == "toolbar" ||
        record.id == "unfiled") {
      this._log.debug("Skipping update for root node.");
      return;
    }
    if (itemId <= 0) {
      this._log.debug("Skipping update for unknown item: " + record.id);
      return;
    }

    this._log.trace("Updating " + record.id + " (" + itemId + ")");

    if ((this._bms.getItemIndex(itemId) != record.sortindex) ||
        (this._bms.getFolderIdForItem(itemId) !=
         this._getItemIdForGUID(record.parentid))) {
      this._log.trace("Moving item (changing folder/index)");
      let parentid = this._getItemIdForGUID(record.parentid);
      this._bms.moveItem(itemId, parentid, record.sortindex);
    }

    for (let key in record.cleartext) {
      switch (key) {
      case "title":
        this._bms.setItemTitle(itemId, record.cleartext.title);
        break;
      case "uri":
        this._bms.changeBookmarkURI(itemId, Utils.makeURI(record.cleartext.uri));
        break;
      case "tags": {
        
        let tags = record.cleartext.tags.filter(function(t) t);
        let tagsURI = this._bms.getBookmarkURI(itemId);
        this._ts.untagURI(tagsURI, null);
        this._ts.tagURI(tagsURI, tags);
      } break;
      case "keyword":
        this._bms.setKeywordForBookmark(itemId, record.cleartext.keyword);
        break;
      case "description":
        this._ans.setItemAnnotation(itemId, "bookmarkProperties/description",
                                    record.cleartext.description, 0,
                                    this._ans.EXPIRE_NEVER);
        break;
      case "generatorURI": {
        let micsumURI = Utils.makeURI(this._bms.getBookmarkURI(itemId));
        let genURI = Utils.makeURI(record.cleartext.generatorURI);
	if (this._ms == SERVICE_NOT_SUPPORTED) {
	  this._log.warn("Can't create microsummary -- not supported.");
	} else {
          let micsum = this._ms.createMicrosummary(micsumURI, genURI);
          this._ms.setMicrosummary(itemId, micsum);
	}
      } break;
      case "siteURI":
        this._ls.setSiteURI(itemId, Utils.makeURI(record.cleartext.siteURI));
        break;
      case "feedURI":
        this._ls.setFeedURI(itemId, Utils.makeURI(record.cleartext.feedURI));
        break;
      case "outgoingSharedAnno":
	this._ans.setItemAnnotation(itemId, OUTGOING_SHARED_ANNO,
				    record.cleartext.outgoingSharedAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      case "incomingSharedAnno":
	this._ans.setItemAnnotation(itemId, INCOMING_SHARED_ANNO,
				    record.cleartext.incomingSharedAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      case "serverPathAnno":
	this._ans.setItemAnnotation(itemId, SERVER_PATH_ANNO,
				    record.cleartext.serverPathAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      }
    }
  },

  changeItemID: function BStore_changeItemID(oldID, newID) {
    var itemId = this._getItemIdForGUID(oldID);
    if (itemId == null) 
      return;
    if (itemId <= 0) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                      newID + ": Item does not exist");
      return;
    }

    var collision = this._getItemIdForGUID(newID);
    if (collision > 0) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                      newID + ": new ID already in use");
      return;
    }

    this._log.debug("Changing GUID " + oldID + " to " + newID);
    this._bms.setItemGUID(itemId, newID);
    Engines.get("bookmarks")._tracker._all[itemId] = newID; 
  },

  _getNode: function BStore__getNode(folder) {
    let query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
  },

  
  _itemDepth: function BStore__itemDepth(id) {
    if (this._isToplevel(id))
      return 0;
    return this._itemDepth(this._bms.getFolderIdForItem(id)) + 1;
  },

  _getTags: function BStore__getTags(uri) {
    try {
      if (typeof(uri) == "string")
        uri = Utils.makeURI(uri);
    } catch(e) {
      this._log.warn("Could not parse URI \"" + uri + "\": " + e);
    }
    return this._ts.getTagsForURI(uri, {});
  },

  _getDescription: function BStore__getDescription(id) {
    try {
      return this._ans.getItemAnnotation(id, "bookmarkProperties/description");
    } catch (e) {
      return undefined;
    }
  },

  _getStaticTitle: function BStore__getStaticTitle(id) {
    try {
      return this._ans.getItemAnnotation(id, "bookmarks/staticTitle");
    } catch (e) {
      return "";
    }
  },

  
  
  
  createRecord: function BStore_createRecord(guid) {
    let record = this.cache.get(guid);
    if (record)
      return record;

    let placeId = this._bms.getItemIdForGUID(guid);
    if (placeId <= 0) { 
      record = new PlacesItem();
      record.cleartext = null;
      return record;
    }

    switch (this._bms.getItemType(placeId)) {
    case this._bms.TYPE_BOOKMARK:
      if (this._ms && this._ms.hasMicrosummary(placeId)) {
        record = new BookmarkMicsum();
        let micsum = this._ms.getMicrosummary(placeId);
        record.generatorURI = micsum.generator.uri; 
        record.staticTitle = this._getStaticTitle(placeId);

      } else {
        record = new Bookmark();
        record.title = this._bms.getItemTitle(placeId);
      }

      record.bmkUri = this._bms.getBookmarkURI(placeId);
      record.tags = this._getTags(record.bmkUri);
      record.keyword = this._bms.getKeywordForBookmark(placeId);
      record.description = this._getDescription(placeId);
      break;

    case this._bms.TYPE_FOLDER:
      if (this._ls.isLivemark(placeId)) {
        record = new Livemark();
        record.siteURI = this._ls.getSiteURI(placeId);
        record.feedURI = this._ls.getFeedURI(placeId);

      } else {
        record = new BookmarkFolder();
        record.title = this._bms.getItemTitle(placeId);
      }
      break;

    case this._bms.TYPE_SEPARATOR:
      record = new BookmarkSeparator();
      break;

    case this._bms.TYPE_DYNAMIC_CONTAINER:
      record = new PlacesItem();
      this._log.warn("Don't know how to serialize dynamic containers yet");
      break;

    default:
      record = new PlacesItem();
      this._log.warn("Unknown item type, cannot serialize: " +
                     this._bms.getItemType(placeId));
    }

    record.parentid = this._getWeaveParentIdForItem(placeId);
    record.depth = this._itemDepth(placeId);
    record.sortindex = this._bms.getItemIndex(placeId);

    this.cache.put(guid, record);
    return record;
  },

  _createMiniRecord: function BStore__createMiniRecord(placesId, depthIndex) {
    let foo = {id: this._bms.getItemGUID(placesId)};
    if (depthIndex) {
      foo.depth = this._itemDepth(placesId);
      foo.sortindex = this._bms.getItemIndex(placesId);
    }
    return foo;
  },

  _getWeaveParentIdForItem: function BStore__getWeaveParentIdForItem(itemId) {
    let parentid = this._bms.getFolderIdForItem(itemId);
    if (parentid == -1) {
      this._log.debug("Found orphan bookmark, reparenting to unfiled");
      parentid = this._bms.unfiledBookmarksFolder;
      this._bms.moveItem(itemId, parentid, -1);
    }
    return this._getWeaveIdForItem(parentid);
  },

  _getChildren: function BStore_getChildren(guid, depthIndex, items) {
    if (typeof(items) == "undefined")
      items = {};
    let node = guid; 
    if (typeof(node) == "string") 
      node = this._getNode(this._getItemIdForGUID(guid));

    if (node.type == node.RESULT_TYPE_FOLDER &&
        !this._ls.isLivemark(node.itemId)) {
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;
      for (var i = 0; i < node.childCount; i++) {
        let child = node.getChild(i);
        let foo = this._createMiniRecord(child.itemId);
        items[foo.id] = foo;
        this._getChildren(child, depthIndex, items);
      }
    }

    return items;
  },

  _getSiblings: function BStore__getSiblings(guid, depthIndex, items) {
    if (typeof(items) == "undefined")
      items = {};

    let parentid = this._bms.getFolderIdForItem(this._getItemIdForGUID(guid));
    let parent = this._getNode(parentid);
    parent.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    parent.containerOpen = true;

    for (var i = 0; i < parent.childCount; i++) {
      let child = parent.getChild(i);
      let foo = this._createMiniRecord(child.itemId);
      items[foo.id] = foo;
    }

    return items;
  },

  getAllIDs: function BStore_getAllIDs() {
    let items = {};
    this._getChildren("menu", true, items);
    this._getChildren("toolbar", true, items);
    this._getChildren("unfiled", true, items);
    return items;
  },

  createMetaRecords: function BStore_createMetaRecords(guid, items) {
    this._getChildren(guid, true, items);
    this._getSiblings(guid, true, items);
    return items;
  },

  wipe: function BStore_wipe() {
    this._bms.removeFolderChildren(this._bms.bookmarksMenuFolder);
    this._bms.removeFolderChildren(this._bms.toolbarFolder);
    this._bms.removeFolderChildren(this._bms.unfiledBookmarksFolder);
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

  get _ls() {
    let ls = Cc["@mozilla.org/browser/livemark-service;2"].
      getService(Ci.nsILivemarkService);
    this.__defineGetter__("_ls", function() ls);
    return ls;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver]),

  _init: function BMT__init() {
    this.__proto__.__proto__._init.call(this);

    
    
    
    
    

    
    let store = new BookmarksStore();
    let all = store.getAllIDs();
    this._all = {};
    for (let guid in all) {
      this._all[this._bms.getItemIdForGUID(guid)] = guid;
    }

    
    
    
    this.ignoreID("menu");
    this.ignoreID("toolbar");
    this.ignoreID("unfiled");
    this.ignoreID(this._all[this._bms.bookmarksMenuFolder]);
    this.ignoreID(this._all[this._bms.toolbarFolder]);
    this.ignoreID(this._all[this._bms.unfiledBookmarksFolder]);

    this._bms.addObserver(this, false);
  },

  
  _upScore: function BMT__upScore() {
    this._score += 10;
  },

  onItemAdded: function BMT_onEndUpdateBatch(itemId, folder, index) {
    if (this._ls.isLivemark(folder))
      return;
    this._log.trace("onItemAdded: " + itemId);
    this._all[itemId] = this._bms.getItemGUID(itemId);
    if (this.addChangedID(this._all[itemId]))
      this._upScore();
  },

  onItemRemoved: function BMT_onItemRemoved(itemId, folder, index) {
    if (this._ls.isLivemark(folder))
      return;
    this._log.trace("onItemRemoved: " + itemId);
    if (this.addChangedID(this._all[itemId]))
      this._upScore();
    delete this._all[itemId];
  },

  onItemChanged: function BMT_onItemChanged(itemId, property, isAnno, value) {
    if (this._ls.isLivemark(this._bms.getFolderIdForItem(itemId))
      return;
    this._log.trace("onItemChanged: " + itemId +
                    (", " + property + (isAnno? " (anno)" : "")) +
                    (value? (" = \"" + value + "\"") : ""));
    
    
    
    if ((itemId in this._all) &&
        (this._bms.getItemGUID(itemId) != this._all[itemId]) &&
        this.addChangedID(this._all[itemId]))
      this._upScore();
  },

  onItemMoved: function BMT_onItemMoved(itemId, oldParent, oldIndex, newParent, newIndex) {
    this._log.trace("onItemMoved: " + itemId);
    if (this.addChangedID(this._all[itemId]))
      this._upScore();
  },

  onBeginUpdateBatch: function BMT_onBeginUpdateBatch() {},
  onEndUpdateBatch: function BMT_onEndUpdateBatch() {},
  onItemVisited: function BMT_onItemVisited(itemId, aVisitID, time) {}
};
