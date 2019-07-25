








































const EXPORTED_SYMBOLS = ['BookmarksEngine', "PlacesItem", "Bookmark",
                          "BookmarkFolder", "BookmarkQuery",
                          "Livemark", "BookmarkSeparator"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const ALLBOOKMARKS_ANNO    = "AllBookmarks";
const DESCRIPTION_ANNO     = "bookmarkProperties/description";
const SIDEBAR_ANNO         = "bookmarkProperties/loadInSidebar";
const FEEDURI_ANNO         = "livemark/feedURI";
const SITEURI_ANNO         = "livemark/siteURI";
const MOBILEROOT_ANNO      = "mobile/bookmarksRoot";
const MOBILE_ANNO          = "MobileBookmarks";
const EXCLUDEBACKUP_ANNO   = "places/excludeFromBackup";
const SMART_BOOKMARKS_ANNO = "Places/SmartBookmark";
const PARENT_ANNO          = "sync/parent";
const ANNOS_TO_TRACK = [DESCRIPTION_ANNO, SIDEBAR_ANNO,
                        FEEDURI_ANNO, SITEURI_ANNO];

const SERVICE_NOT_SUPPORTED = "Service not supported on this platform";
const FOLDER_SORTINDEX = 1000000;

Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/util.js");

Cu.import("resource://services-sync/main.js");      

function PlacesItem(collection, id, type) {
  CryptoWrapper.call(this, collection, id);
  this.type = type || "item";
}
PlacesItem.prototype = {
  decrypt: function PlacesItem_decrypt(keyBundle) {
    
    let clear = CryptoWrapper.prototype.decrypt.call(this, keyBundle);

    
    if (!this.deleted)
      this.__proto__ = this.getTypeObject(this.type).prototype;

    return clear;
  },

  getTypeObject: function PlacesItem_getTypeObject(type) {
    switch (type) {
      case "bookmark":
      case "microsummary":
        return Bookmark;
      case "query":
        return BookmarkQuery;
      case "folder":
        return BookmarkFolder;
      case "livemark":
        return Livemark;
      case "separator":
        return BookmarkSeparator;
      case "item":
        return PlacesItem;
    }
    throw "Unknown places item object type: " + type;
  },

  __proto__: CryptoWrapper.prototype,
  _logName: "Record.PlacesItem",
};

Utils.deferGetSet(PlacesItem, "cleartext", ["hasDupe", "parentid", "parentName",
                                            "type"]);

function Bookmark(collection, id, type) {
  PlacesItem.call(this, collection, id, type || "bookmark");
}
Bookmark.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Bookmark",
};

Utils.deferGetSet(Bookmark, "cleartext", ["title", "bmkUri", "description",
  "loadInSidebar", "tags", "keyword"]);

function BookmarkQuery(collection, id) {
  Bookmark.call(this, collection, id, "query");
}
BookmarkQuery.prototype = {
  __proto__: Bookmark.prototype,
  _logName: "Record.BookmarkQuery",
};

Utils.deferGetSet(BookmarkQuery, "cleartext", ["folderName",
                                               "queryId"]);

function BookmarkFolder(collection, id, type) {
  PlacesItem.call(this, collection, id, type || "folder");
}
BookmarkFolder.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Folder",
};

Utils.deferGetSet(BookmarkFolder, "cleartext", ["description", "title",
                                                "children"]);

function Livemark(collection, id) {
  BookmarkFolder.call(this, collection, id, "livemark");
}
Livemark.prototype = {
  __proto__: BookmarkFolder.prototype,
  _logName: "Record.Livemark",
};

Utils.deferGetSet(Livemark, "cleartext", ["siteUri", "feedUri"]);

function BookmarkSeparator(collection, id) {
  PlacesItem.call(this, collection, id, "separator");
}
BookmarkSeparator.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Separator",
};

Utils.deferGetSet(BookmarkSeparator, "cleartext", "pos");


function archiveBookmarks() {
  
  try {
    PlacesUtils.archiveBookmarksFile(null, true);
  }
  catch(ex) {}
}

let kSpecialIds = {

  
  
  
  guids: ["menu", "places", "tags", "toolbar", "unfiled", "mobile"],

  
  createMobileRoot: function createMobileRoot() {
    let root = Svc.Bookmark.placesRoot;
    let mRoot = Svc.Bookmark.createFolder(root, "mobile", -1);
    Svc.Annos.setItemAnnotation(mRoot, MOBILEROOT_ANNO, 1, 0,
                                Svc.Annos.EXPIRE_NEVER);
    return mRoot;
  },

  findMobileRoot: function findMobileRoot(create) {
    
    let root = Svc.Annos.getItemsWithAnnotation(MOBILEROOT_ANNO, {});
    if (root.length != 0)
      return root[0];

    if (create)
      return this.createMobileRoot();

    return null;
  },

  
  isSpecialGUID: function isSpecialGUID(g) {
    return this.guids.indexOf(g) != -1;
  },

  specialIdForGUID: function specialIdForGUID(guid, create) {
    if (guid == "mobile") {
      return this.findMobileRoot(create);
    }
    return this[guid];
  },

  
  specialGUIDForId: function specialGUIDForId(id) {
    for each (let guid in this.guids)
      if (this.specialIdForGUID(guid, false) == id)
        return guid;
    return null;
  },

  get menu()    Svc.Bookmark.bookmarksMenuFolder,
  get places()  Svc.Bookmark.placesRoot,
  get tags()    Svc.Bookmark.tagsFolder,
  get toolbar() Svc.Bookmark.toolbarFolder,
  get unfiled() Svc.Bookmark.unfiledBookmarksFolder,
  get mobile()  this.findMobileRoot(true),
};

function BookmarksEngine() {
  SyncEngine.call(this, "Bookmarks");
}
BookmarksEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _recordObj: PlacesItem,
  _storeObj: BookmarksStore,
  _trackerObj: BookmarksTracker,
  version: 2,

  _sync: Utils.batchSync("Bookmark", SyncEngine),

  _syncStartup: function _syncStart() {
    SyncEngine.prototype._syncStartup.call(this);

    
    if (this.lastSync == 0)
      archiveBookmarks();

    
    this.__defineGetter__("_lazyMap", function() {
      delete this._lazyMap;

      let lazyMap = {};
      for (let guid in this._store.getAllIDs()) {
        
        let key;
        let id = this._store.idForGUID(guid);
        switch (Svc.Bookmark.getItemType(id)) {
          case Svc.Bookmark.TYPE_BOOKMARK:

            
            let queryId;
            try {
              queryId = Svc.Annos.getItemAnnotation(id, SMART_BOOKMARKS_ANNO);
            } catch(ex) {}
            
            if (queryId)
              key = "q" + queryId;
            else
              key = "b" + Svc.Bookmark.getBookmarkURI(id).spec + ":" +
                    Svc.Bookmark.getItemTitle(id);
            break;
          case Svc.Bookmark.TYPE_FOLDER:
            key = "f" + Svc.Bookmark.getItemTitle(id);
            break;
          case Svc.Bookmark.TYPE_SEPARATOR:
            key = "s" + Svc.Bookmark.getItemIndex(id);
            break;
          default:
            continue;
        }

        
        let parent = Svc.Bookmark.getFolderIdForItem(id);
        if (parent <= 0)
          continue;

        let parentName = Svc.Bookmark.getItemTitle(parent);
        if (lazyMap[parentName] == null)
          lazyMap[parentName] = {};

        
        let entry = new String(guid);
        entry.hasDupe = lazyMap[parentName][key] != null;

        
        lazyMap[parentName][key] = entry;
        this._log.trace("Mapped: " + [parentName, key, entry, entry.hasDupe]);
      }

      
      return this._lazyMap = function(item) {
        
        let key;
        let altKey;
        switch (item.type) {
          case "query":
            
            
            
            if (item.queryId) {
              key = "q" + item.queryId;
              altKey = "b" + item.bmkUri + ":" + item.title;
              break;
            }
            
          case "bookmark":
          case "microsummary":
            key = "b" + item.bmkUri + ":" + item.title;
            break;
          case "folder":
          case "livemark":
            key = "f" + item.title;
            break;
          case "separator":
            key = "s" + item.pos;
            break;
          default:
            return;
        }

        
        this._log.trace("Finding mapping: " + item.parentName + ", " + key);
        let parent = lazyMap[item.parentName];
        
        if (!parent) {
          this._log.trace("No parent => no dupe.");
          return undefined;
        }
          
        let dupe = parent[key];
        
        if (dupe) {
          this._log.trace("Mapped dupe: " + dupe);
          return dupe;
        }
        
        if (altKey) {
          dupe = parent[altKey];
          if (dupe) {
            this._log.trace("Mapped dupe using altKey " + altKey + ": " + dupe);
            return dupe;
          }
        }
        
        this._log.trace("No dupe found for key " + key + "/" + altKey + ".");
        return undefined;
      };
    });

    this._store._childrenToOrder = {};
  },

  _processIncoming: function _processIncoming() {
    try {
      SyncEngine.prototype._processIncoming.call(this);
    } finally {
      
      this._tracker.ignoreAll = true;
      this._store._orderChildren();
      this._tracker.ignoreAll = false;
      delete this._store._childrenToOrder;
    }
  },

  _syncFinish: function _syncFinish() {
    SyncEngine.prototype._syncFinish.call(this);
    delete this._lazyMap;
    this._tracker._ensureMobileQuery();
  },

  _createRecord: function _createRecord(id) {
    
    let record = SyncEngine.prototype._createRecord.call(this, id);
    let entry = this._lazyMap(record);
    if (entry != null && entry.hasDupe)
      record.hasDupe = true;
    return record;
  },

  _findDupe: function _findDupe(item) {
    
    if (item.hasDupe)
      return;
    return this._lazyMap(item);
  },

  _handleDupe: function _handleDupe(item, dupeId) {
    
    this._store.changeItemID(dupeId, item.id);
    this._deleteId(dupeId);
    this._tracker.addChangedID(item.id, 0);
    if (item.parentid) {
      this._tracker.addChangedID(item.parentid, 0);
    }
  }
};

function BookmarksStore(name) {
  Store.call(this, name);

  
  Svc.Obs.add("places-shutdown", function() {
    this.__bms = null;
    this.__hsvc = null;
    this.__ls = null;
    this.__ts = null;
    for each ([query, stmt] in Iterator(this._stmts))
      stmt.finalize();
  }, this);
}
BookmarksStore.prototype = {
  __proto__: Store.prototype,

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
                    getService(Ci.nsINavHistoryService).
                    QueryInterface(Ci.nsPIPlacesDatabase);
    return this.__hsvc;
  },

  __ls: null,
  get _ls() {
    if (!this.__ls)
      this.__ls = Cc["@mozilla.org/browser/livemark-service;2"].
                  getService(Ci.nsILivemarkService);
    return this.__ls;
  },

  __ts: null,
  get _ts() {
    if (!this.__ts)
      this.__ts = Cc["@mozilla.org/browser/tagging-service;1"].
                  getService(Ci.nsITaggingService);
    return this.__ts;
  },


  itemExists: function BStore_itemExists(id) {
    return this.idForGUID(id, true) > 0;
  },
  
  




  preprocessTagQuery: function preprocessTagQuery(record) {
    if (record.type != "query" ||
        record.bmkUri == null ||
        record.folderName == null)
      return;
    
    
    let uri           = record.bmkUri
    let queriesRef    = {};
    let queryCountRef = {};
    let optionsRef    = {};
    Svc.History.queryStringToQueries(uri, queriesRef, queryCountRef, optionsRef);
    
    
    if (optionsRef.value.resultType != optionsRef.value.RESULTS_AS_TAG_CONTENTS)
      return;
    
    
    let tag = record.folderName;
    let dummyURI = Utils.makeURI("about:weave#BStore_preprocess");
    this._ts.tagURI(dummyURI, [tag]);

    
    let tags = this._getNode(this._bms.tagsFolder);
    if (!(tags instanceof Ci.nsINavHistoryQueryResultNode)) {
      this._log.debug("tags isn't an nsINavHistoryQueryResultNode; aborting.");
      return;
    }

    tags.containerOpen = true;
    try {
      for (let i = 0; i < tags.childCount; i++) {
        let child = tags.getChild(i);
        if (child.title == tag) {
          
          this._log.debug("Tag query folder: " + tag + " = " + child.itemId);
          
          this._log.trace("Replacing folders in: " + uri);
          for each (let q in queriesRef.value)
            q.setFolders([child.itemId], 1);
          
          record.bmkUri = Svc.History.queriesToQueryString(queriesRef.value,
                                                           queryCountRef.value,
                                                           optionsRef.value);
          return;
        }
      }
    }
    finally {
      tags.containerOpen = false;
    }
  },
  
  applyIncoming: function BStore_applyIncoming(record) {
    
    if (record.deleted) {
      Store.prototype.applyIncoming.call(this, record);
      return;
    }

    
    if ((record.id in kSpecialIds) && record.children) {
      this._log.debug("Processing special node: " + record.id);
      
      this._childrenToOrder[record.id] = record.children;
      return;
    }

    
    this.preprocessTagQuery(record);

    
    let parentGUID = record.parentid;
    if (!parentGUID) {
      throw "Record " + record.id + " has invalid parentid: " + parentGUID;
    }

    let parentId = this.idForGUID(parentGUID);
    if (parentId > 0) {
      
      record._parent = parentId;
      record._orphan = false;
    } else {
      this._log.trace("Record " + record.id +
                      " is an orphan: could not find parent " + parentGUID);
      record._orphan = true;
    }

    
    Store.prototype.applyIncoming.call(this, record);

    
    let itemId = this.idForGUID(record.id);
    if (itemId > 0) {
      
      if (record.type == "folder") {
        this._reparentOrphans(itemId);
        
        if (record.children)
          this._childrenToOrder[record.id] = record.children;
      }

      
      if (record._orphan) {
        Svc.Annos.setItemAnnotation(itemId, PARENT_ANNO, parentGUID, 0,
                                    Svc.Annos.EXPIRE_NEVER);
      }
    }
  },

  


  _findAnnoItems: function BStore__findAnnoItems(anno, val) {
    return Svc.Annos.getItemsWithAnnotation(anno, {}).filter(function(id)
      Svc.Annos.getItemAnnotation(id, anno) == val);
  },

  


  _reparentOrphans: function _reparentOrphans(parentId) {
    
    let parentGUID = this.GUIDForId(parentId);
    let orphans = this._findAnnoItems(PARENT_ANNO, parentGUID);

    this._log.debug("Reparenting orphans " + orphans + " to " + parentId);
    orphans.forEach(function(orphan) {
      
      if (this._reparentItem(orphan, parentId)) {
        Svc.Annos.removeItemAnnotation(orphan, PARENT_ANNO);
      }
    }, this);
  },

  _reparentItem: function _reparentItem(itemId, parentId) {
    this._log.trace("Attempting to move item " + itemId + " to new parent " +
                    parentId);
    try {
      if (parentId > 0) {
        Svc.Bookmark.moveItem(itemId, parentId, Svc.Bookmark.DEFAULT_INDEX);
        return true;
      }
    } catch(ex) {
      this._log.debug("Failed to reparent item. " + Utils.exceptionStr(ex));
    }
    return false;
  },

  
  
  _recordType: function _recordType(itemId) {
    let bms  = this._bms;
    let type = bms.getItemType(itemId);

    switch (type) {
      case bms.TYPE_FOLDER:
        if (this._ls.isLivemark(itemId))
          return "livemark";
        return "folder";

      case bms.TYPE_BOOKMARK:
        let bmkUri = bms.getBookmarkURI(itemId).spec;
        if (bmkUri.search(/^place:/) == 0)
          return "query";
        return "bookmark";

      case bms.TYPE_SEPARATOR:
        return "separator";

      default:
        return null;
    }
  },

  create: function BStore_create(record) {
    
    
    
    
    
    
    if (!(record._parent > 0)) {
      this._log.debug("Parent is " + record._parent + "; reparenting to unfiled.");
      record._parent = kSpecialIds.unfiled;
    }

    let newId;
    switch (record.type) {
    case "bookmark":
    case "query":
    case "microsummary": {
      let uri = Utils.makeURI(record.bmkUri);
      newId = this._bms.insertBookmark(record._parent, uri,
                                       Svc.Bookmark.DEFAULT_INDEX, record.title);
      this._log.debug("created bookmark " + newId + " under " + record._parent
                      + " as " + record.title + " " + record.bmkUri);

      
      if (record.queryId) {
        Svc.Annos.setItemAnnotation(newId, SMART_BOOKMARKS_ANNO, record.queryId,
                                    0, Svc.Annos.EXPIRE_NEVER);
      }

      if (Utils.isArray(record.tags)) {
        this._tagURI(uri, record.tags);
      }
      this._bms.setKeywordForBookmark(newId, record.keyword);
      if (record.description) {
        Svc.Annos.setItemAnnotation(newId, DESCRIPTION_ANNO,
                                    record.description, 0,
                                    Svc.Annos.EXPIRE_NEVER);
      }

      if (record.loadInSidebar) {
        Svc.Annos.setItemAnnotation(newId, SIDEBAR_ANNO, true, 0,
                                    Svc.Annos.EXPIRE_NEVER);
      }

    } break;
    case "folder":
      newId = this._bms.createFolder(record._parent, record.title,
                                     Svc.Bookmark.DEFAULT_INDEX);
      this._log.debug("created folder " + newId + " under " + record._parent
                      + " as " + record.title);

      if (record.description) {
        Svc.Annos.setItemAnnotation(newId, DESCRIPTION_ANNO, record.description,
                                    0, Svc.Annos.EXPIRE_NEVER);
      }

      
      break;
    case "livemark":
      let siteURI = null;
      if (!record.feedUri) {
        this._log.debug("No feed URI: skipping livemark record " + record.id);
        return;
      }
      if (this._ls.isLivemark(record._parent)) {
        this._log.debug("Invalid parent: skipping livemark record " + record.id);
        return;
      }

      if (record.siteUri != null)
        siteURI = Utils.makeURI(record.siteUri);

      
      
      newId = this._ls.createLivemarkFolderOnly(record._parent, record.title,
                                                siteURI,
                                                Utils.makeURI(record.feedUri),
                                                Svc.Bookmark.DEFAULT_INDEX);
      this._log.debug("Created livemark " + newId + " under " + record._parent +
                      " as " + record.title + ", " + record.siteUri + ", " + 
                      record.feedUri + ", GUID " + record.id);
      break;
    case "separator":
      newId = this._bms.insertSeparator(record._parent,
                                        Svc.Bookmark.DEFAULT_INDEX);
      this._log.debug("created separator " + newId + " under " + record._parent);
      break;
    case "item":
      this._log.debug(" -> got a generic places item.. do nothing?");
      return;
    default:
      this._log.error("_create: Unknown item type: " + record.type);
      return;
    }

    this._log.trace("Setting GUID of new item " + newId + " to " + record.id);
    this._setGUID(newId, record.id);
  },

  
  
  removeById: function removeById(itemId, guid) {
    let type = this._bms.getItemType(itemId);

    switch (type) {
    case this._bms.TYPE_BOOKMARK:
      this._log.debug("  -> removing bookmark " + guid);
      this._ts.untagURI(this._bms.getBookmarkURI(itemId), null);
      this._bms.removeItem(itemId);
      break;
    case this._bms.TYPE_FOLDER:
      this._log.debug("  -> removing folder " + guid);
      Svc.Bookmark.removeItem(itemId);
      break;
    case this._bms.TYPE_SEPARATOR:
      this._log.debug("  -> removing separator " + guid);
      this._bms.removeItem(itemId);
      break;
    default:
      this._log.error("remove: Unknown item type: " + type);
      break;
    }
  },

  remove: function BStore_remove(record) {
    let itemId = this.idForGUID(record.id);
    if (itemId <= 0) {
      this._log.debug("Item " + record.id + " already removed");
      return;
    }
    this.removeById(itemId, record.id);
  },

  update: function BStore_update(record) {
    let itemId = this.idForGUID(record.id);

    if (itemId <= 0) {
      this._log.debug("Skipping update for unknown item: " + record.id);
      return;
    }

    
    
    
    
    
    
    let localItemType    = this._recordType(itemId);
    let remoteRecordType = record.type;
    this._log.trace("Local type: " + localItemType + ". " +
                    "Remote type: " + remoteRecordType + ".");

    if (localItemType != remoteRecordType) {
      this._log.debug("Local record and remote record differ in type. " +
                      "Deleting and recreating.");
      this.removeById(itemId, record.id);
      this.create(record);
      return;
    }

    this._log.trace("Updating " + record.id + " (" + itemId + ")");

    
    if (record._parent > 0 &&
        Svc.Bookmark.getFolderIdForItem(itemId) != record._parent) {
      this._reparentItem(itemId, record._parent);
    }

    for (let [key, val] in Iterator(record.cleartext)) {
      switch (key) {
      case "title":
        val = val || "";
        this._bms.setItemTitle(itemId, val);
        break;
      case "bmkUri":
        this._bms.changeBookmarkURI(itemId, Utils.makeURI(val));
        break;
      case "tags":
        if (Utils.isArray(val)) {
          this._tagURI(this._bms.getBookmarkURI(itemId), val);
        }
        break;
      case "keyword":
        this._bms.setKeywordForBookmark(itemId, val);
        break;
      case "description":
        val = val || "";
        Svc.Annos.setItemAnnotation(itemId, DESCRIPTION_ANNO, val, 0,
                                    Svc.Annos.EXPIRE_NEVER);
        break;
      case "loadInSidebar":
        if (val) {
          Svc.Annos.setItemAnnotation(itemId, SIDEBAR_ANNO, true, 0,
                                      Svc.Annos.EXPIRE_NEVER);
        } else {
          Svc.Annos.removeItemAnnotation(itemId, SIDEBAR_ANNO);
        }
        break;
      case "queryId":
        Svc.Annos.setItemAnnotation(itemId, SMART_BOOKMARKS_ANNO, val, 0,
                                    Svc.Annos.EXPIRE_NEVER);
        break;
      case "siteUri":
        this._ls.setSiteURI(itemId, Utils.makeURI(val));
        break;
      case "feedUri":
        this._ls.setFeedURI(itemId, Utils.makeURI(val));
        break;
      }
    }
  },

  _orderChildren: function _orderChildren() {
    for (let [guid, children] in Iterator(this._childrenToOrder)) {
      
      
      let delta = 0;
      let parent = null;
      for (let idx = 0; idx < children.length; idx++) {
        let itemid = this.idForGUID(children[idx]);
        if (itemid == -1) {
          delta += 1;
          this._log.trace("Could not locate record " + children[idx]);
          continue;
        }
        try {
          
          
          
          if (!parent)
            parent = Svc.Bookmark.getFolderIdForItem(itemid);
          Svc.Bookmark.moveItem(itemid, parent, idx - delta);
        } catch (ex) {
          this._log.debug("Could not move item " + children[idx] + ": " + ex);
        }
      }
    }
  },

  changeItemID: function BStore_changeItemID(oldID, newID) {
    this._log.debug("Changing GUID " + oldID + " to " + newID);

    
    let itemId = this.idForGUID(oldID);
    if (itemId <= 0)
      return;

    this._setGUID(itemId, newID);
  },

  _getNode: function BStore__getNode(folder) {
    let query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
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
      return Svc.Annos.getItemAnnotation(id, DESCRIPTION_ANNO);
    } catch (e) {
      return null;
    }
  },

  _isLoadInSidebar: function BStore__isLoadInSidebar(id) {
    return Svc.Annos.itemHasAnnotation(id, SIDEBAR_ANNO);
  },

  get _childGUIDsStm() {
    return this._getStmt(
      "SELECT id AS item_id, guid " +
      "FROM moz_bookmarks " +
      "WHERE parent = :parent " +
      "ORDER BY position");
  },
  _childGUIDsCols: ["item_id", "guid"],

  _getChildGUIDsForId: function _getChildGUIDsForId(itemid) {
    let stmt = this._childGUIDsStm;
    stmt.params.parent = itemid;
    let rows = Utils.queryAsync(stmt, this._childGUIDsCols);
    return rows.map(function (row) {
      if (row.guid) {
        return row.guid;
      }
      
      return this.GUIDForId(row.item_id);
    }, this);
  },

  
  createRecord: function createRecord(id, collection) {
    let placeId = this.idForGUID(id);
    let record;
    if (placeId <= 0) { 
      record = new PlacesItem(collection, id);
      record.deleted = true;
      return record;
    }

    let parent = Svc.Bookmark.getFolderIdForItem(placeId);
    switch (this._bms.getItemType(placeId)) {
    case this._bms.TYPE_BOOKMARK:
      let bmkUri = this._bms.getBookmarkURI(placeId).spec;
      if (bmkUri.search(/^place:/) == 0) {
        record = new BookmarkQuery(collection, id);

        
        let folder = bmkUri.match(/[:&]folder=(\d+)/);
        try {
          
          if (folder != null) {
            folder = folder[1];
            record.folderName = this._bms.getItemTitle(folder);
            this._log.trace("query id: " + folder + " = " + record.folderName);
          }
        }
        catch(ex) {}
        
        
        try {
          let anno = Svc.Annos.getItemAnnotation(placeId, SMART_BOOKMARKS_ANNO);
          if (anno != null) {
            this._log.trace("query anno: " + SMART_BOOKMARKS_ANNO +
                            " = " + anno);
            record.queryId = anno;
          }
        }
        catch(ex) {}
      }
      else {
        record = new Bookmark(collection, id);
      }
      record.title = this._bms.getItemTitle(placeId);

      record.parentName = Svc.Bookmark.getItemTitle(parent);
      record.bmkUri = bmkUri;
      record.tags = this._getTags(record.bmkUri);
      record.keyword = this._bms.getKeywordForBookmark(placeId);
      record.description = this._getDescription(placeId);
      record.loadInSidebar = this._isLoadInSidebar(placeId);
      break;

    case this._bms.TYPE_FOLDER:
      if (this._ls.isLivemark(placeId)) {
        record = new Livemark(collection, id);

        let siteURI = this._ls.getSiteURI(placeId);
        if (siteURI != null)
          record.siteUri = siteURI.spec;
        record.feedUri = this._ls.getFeedURI(placeId).spec;

      } else {
        record = new BookmarkFolder(collection, id);
      }

      if (parent > 0)
        record.parentName = Svc.Bookmark.getItemTitle(parent);
      record.title = this._bms.getItemTitle(placeId);
      record.description = this._getDescription(placeId);
      record.children = this._getChildGUIDsForId(placeId);
      break;

    case this._bms.TYPE_SEPARATOR:
      record = new BookmarkSeparator(collection, id);
      if (parent > 0)
        record.parentName = Svc.Bookmark.getItemTitle(parent);
      
      record.pos = Svc.Bookmark.getItemIndex(placeId);
      break;

    case this._bms.TYPE_DYNAMIC_CONTAINER:
      record = new PlacesItem(collection, id);
      this._log.warn("Don't know how to serialize dynamic containers yet");
      break;

    default:
      record = new PlacesItem(collection, id);
      this._log.warn("Unknown item type, cannot serialize: " +
                     this._bms.getItemType(placeId));
    }

    record.parentid = this.GUIDForId(parent);
    record.sortindex = this._calculateIndex(record);

    return record;
  },

  _stmts: {},
  _getStmt: function(query) {
    if (query in this._stmts)
      return this._stmts[query];

    this._log.trace("Creating SQL statement: " + query);
    return this._stmts[query] = this._hsvc.DBConnection
                                    .createAsyncStatement(query);
  },

  get _frecencyStm() {
    return this._getStmt(
        "SELECT frecency " +
        "FROM moz_places " +
        "WHERE url = :url " +
        "LIMIT 1");
  },
  _frecencyCols: ["frecency"],

  get _setGUIDStm() {
    return this._getStmt(
      "UPDATE moz_bookmarks " +
      "SET guid = :guid " +
      "WHERE id = :item_id");
  },

  
  _setGUID: function _setGUID(id, guid) {
    if (!guid)
      guid = Utils.makeGUID();

    let stmt = this._setGUIDStm;
    stmt.params.guid = guid;
    stmt.params.item_id = id;
    Utils.queryAsync(stmt);
    return guid;
  },

  get _guidForIdStm() {
    return this._getStmt(
      "SELECT guid " +
      "FROM moz_bookmarks " +
      "WHERE id = :item_id");
  },
  _guidForIdCols: ["guid"],

  GUIDForId: function GUIDForId(id) {
    let special = kSpecialIds.specialGUIDForId(id);
    if (special)
      return special;

    let stmt = this._guidForIdStm;
    stmt.params.item_id = id;

    
    let result = Utils.queryAsync(stmt, this._guidForIdCols)[0];
    if (result && result.guid)
      return result.guid;

    
    return this._setGUID(id);
  },

  get _idForGUIDStm() {
    return this._getStmt(
      "SELECT id AS item_id " +
      "FROM moz_bookmarks " +
      "WHERE guid = :guid");
  },
  _idForGUIDCols: ["item_id"],

  
  
  idForGUID: function idForGUID(guid, noCreate) {
    if (kSpecialIds.isSpecialGUID(guid))
      return kSpecialIds.specialIdForGUID(guid, !noCreate);

    let stmt = this._idForGUIDStm;
    
    stmt.params.guid = guid.toString();

    let results = Utils.queryAsync(stmt, this._idForGUIDCols);
    this._log.trace("Number of rows matching GUID " + guid + ": "
                    + results.length);
    
    
    let result = results[0];
    
    if (!result)
      return -1;
    
    return result.item_id;
  },

  _calculateIndex: function _calculateIndex(record) {
    
    if (record.type == "folder")
      return FOLDER_SORTINDEX;

    
    
    let index = 0;
    if (record.parentid == "toolbar")
      index += 150;

    
    if (record.bmkUri != null) {
      this._frecencyStm.params.url = record.bmkUri;
      let result = Utils.queryAsync(this._frecencyStm, this._frecencyCols);
      if (result.length)
        index += result[0].frecency;
    }

    return index;
  },

  _getChildren: function BStore_getChildren(guid, items) {
    let node = guid; 
    if (typeof(node) == "string") { 
      let nodeID = this.idForGUID(guid, true);
      if (!nodeID) {
        this._log.debug("No node for GUID " + guid + "; returning no children.");
        return items;
      }
      node = this._getNode(nodeID);
    }
    
    if (node.type == node.RESULT_TYPE_FOLDER &&
        !this._ls.isLivemark(node.itemId)) {
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;
      try {
        
        for (let i = 0; i < node.childCount; i++) {
          let child = node.getChild(i);
          items[this.GUIDForId(child.itemId)] = true;
          this._getChildren(child, items);
        }
      }
      finally {
        node.containerOpen = false;
      }
    }

    return items;
  },

  _tagURI: function BStore_tagURI(bmkURI, tags) {
    
    tags = tags.filter(function(t) t);

    
    let dummyURI = Utils.makeURI("about:weave#BStore_tagURI");
    this._ts.tagURI(dummyURI, tags);
    this._ts.untagURI(bmkURI, null);
    this._ts.tagURI(bmkURI, tags);
    this._ts.untagURI(dummyURI, null);
  },

  getAllIDs: function BStore_getAllIDs() {
    let items = {"menu": true,
                 "toolbar": true};
    for each (let guid in kSpecialIds.guids) {
      if (guid != "places" && guid != "tags")
        this._getChildren(guid, items);
    }
    return items;
  },

  wipe: function BStore_wipe() {
    
    archiveBookmarks();

    for each (let guid in kSpecialIds.guids)
      if (guid != "places") {
        let id = kSpecialIds.specialIdForGUID(guid);
        if (id)
          this._bms.removeFolderChildren(id);
      }
  }
};

function BookmarksTracker(name) {
  Tracker.call(this, name);

  Svc.Obs.add("places-shutdown", this);
  Svc.Obs.add("weave:engine:start-tracking", this);
  Svc.Obs.add("weave:engine:stop-tracking", this);
}
BookmarksTracker.prototype = {
  __proto__: Tracker.prototype,

  _enabled: false,
  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "weave:engine:start-tracking":
        if (!this._enabled) {
          Svc.Bookmark.addObserver(this, true);
          Svc.Obs.add("bookmarks-restore-begin", this);
          Svc.Obs.add("bookmarks-restore-success", this);
          Svc.Obs.add("bookmarks-restore-failed", this);
          this._enabled = true;
        }
        break;
      case "weave:engine:stop-tracking":
        if (this._enabled) {
          Svc.Bookmark.removeObserver(this);
          Svc.Obs.remove("bookmarks-restore-begin", this);
          Svc.Obs.remove("bookmarks-restore-success", this);
          Svc.Obs.remove("bookmarks-restore-failed", this);
          this._enabled = false;
        }
        
      case "places-shutdown":
        
        
        this.__ls = null;
        this.__bms = null;
        break;
        
      case "bookmarks-restore-begin":
        this._log.debug("Ignoring changes from importing bookmarks.");
        this.ignoreAll = true;
        break;
      case "bookmarks-restore-success":
        this._log.debug("Tracking all items on successful import.");
        this.ignoreAll = false;
        
        this._log.debug("Restore succeeded: wiping server and other clients.");
        Weave.Service.resetClient([this.name]);
        Weave.Service.wipeServer([this.name]);
        Weave.Service.prepCommand("wipeEngine", [this.name]);
        break;
      case "bookmarks-restore-failed":
        this._log.debug("Tracking all items on failed import.");
        this.ignoreAll = false;
        break;
    }
  },

  __bms: null,
  get _bms() {
    if (!this.__bms)
      this.__bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                   getService(Ci.nsINavBookmarksService);
    return this.__bms;
  },

  __ls: null,
  get _ls() {
    if (!this.__ls)
      this.__ls = Cc["@mozilla.org/browser/livemark-service;2"].
                  getService(Ci.nsILivemarkService);
    return this.__ls;
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver,
    Ci.nsINavBookmarkObserver_MOZILLA_1_9_1_ADDITIONS,
    Ci.nsISupportsWeakReference
  ]),

  _idForGUID: function _idForGUID(item_id) {
    
    return Engines.get("bookmarks")._store.idForGUID(item_id);
  },

  _GUIDForId: function _GUIDForId(item_id) {
    
    return Engines.get("bookmarks")._store.GUIDForId(item_id);
  },

  





  _addId: function BMT__addId(itemId) {
    if (this.addChangedID(this._GUIDForId(itemId)))
      this._upScore();
  },

  
  _upScore: function BMT__upScore() {
    this.score += 10;
  },

  








  _ignore: function BMT__ignore(itemId, folder) {
    
    if (this.ignoreAll)
      return true;

    
    if (folder == null) {
      try {
        folder = this._bms.getFolderIdForItem(itemId);
      } catch (ex) {
        this._log.debug("getFolderIdForItem(" + itemId +
                        ") threw; calling _ensureMobileQuery.");
        
        
        this._ensureMobileQuery();
        folder = this._bms.getFolderIdForItem(itemId);
      }
    }

    
    if (this._ls.isLivemark(folder))
      return true;

    
    let tags = kSpecialIds.tags;
    if (folder == tags)
      return true;

    
    if (this._bms.getFolderIdForItem(folder) == tags)
      return true;

    
    if (Svc.Annos.itemHasAnnotation(itemId, EXCLUDEBACKUP_ANNO)) {
      this.removeChangedID(this._GUIDForId(itemId));
      return true;
    }

    return false;
  },

  onItemAdded: function BMT_onEndUpdateBatch(itemId, folder, index) {
    if (this._ignore(itemId, folder))
      return;

    this._log.trace("onItemAdded: " + itemId);
    this._addId(itemId);
    this._addId(folder);
  },

  onBeforeItemRemoved: function BMT_onBeforeItemRemoved(itemId) {
    if (this._ignore(itemId))
      return;

    this._log.trace("onBeforeItemRemoved: " + itemId);
    this._addId(itemId);
    let folder = Svc.Bookmark.getFolderIdForItem(itemId);
    this._addId(folder);
  },

  _ensureMobileQuery: function _ensureMobileQuery() {
    let anno = "PlacesOrganizer/OrganizerQuery";
    let find = function(val) Svc.Annos.getItemsWithAnnotation(anno, {}).filter(
      function(id) Svc.Annos.getItemAnnotation(id, anno) == val);

    
    let all = find(ALLBOOKMARKS_ANNO);
    if (all.length == 0)
      return;

    
    this.ignoreAll = true;

    let mobile = find(MOBILE_ANNO);
    let queryURI = Utils.makeURI("place:folder=" + kSpecialIds.mobile);
    let title = Str.sync.get("mobile.label");

    
    if (Svc.Bookmark.getIdForItemAt(kSpecialIds.mobile, 0) == -1) {
      if (mobile.length != 0)
        Svc.Bookmark.removeItem(mobile[0]);
    }
    
    else if (mobile.length == 0) {
      let query = Svc.Bookmark.insertBookmark(all[0], queryURI, -1, title);
      Svc.Annos.setItemAnnotation(query, anno, MOBILE_ANNO, 0,
                                  Svc.Annos.EXPIRE_NEVER);
      Svc.Annos.setItemAnnotation(query, EXCLUDEBACKUP_ANNO, 1, 0,
                                  Svc.Annos.EXPIRE_NEVER);
    }
    
    else if (Svc.Bookmark.getItemTitle(mobile[0]) != title)
      Svc.Bookmark.setItemTitle(mobile[0], title);

    this.ignoreAll = false;
  },

  
  
  
  
  onItemChanged: function BMT_onItemChanged(itemId, property, isAnno, value) {
    
    if (this.ignoreAll)
      return;

    if (isAnno && (ANNOS_TO_TRACK.indexOf(property) == -1))
      
      return;

    
    if (property == "favicon")
      return;

    if (this._ignore(itemId))
      return;

    this._log.trace("onItemChanged: " + itemId +
                    (", " + property + (isAnno? " (anno)" : "")) +
                    (value ? (" = \"" + value + "\"") : ""));
    this._addId(itemId);
  },

  onItemMoved: function BMT_onItemMoved(itemId, oldParent, oldIndex, newParent, newIndex) {
    if (this._ignore(itemId))
      return;

    this._log.trace("onItemMoved: " + itemId);
    this._addId(oldParent);
    if (oldParent != newParent) {
      this._addId(itemId);
      this._addId(newParent);
    }

    
    Svc.Annos.removeItemAnnotation(itemId, PARENT_ANNO);
  },

  onBeginUpdateBatch: function BMT_onBeginUpdateBatch() {},
  onEndUpdateBatch: function BMT_onEndUpdateBatch() {},
  onItemRemoved: function BMT_onItemRemoved(itemId, folder, index) {},
  onItemVisited: function BMT_onItemVisited(itemId, aVisitID, time) {}
};
