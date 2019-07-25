





































const EXPORTED_SYMBOLS = ['BookmarksEngine', 'BookmarksSharingManager'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const PARENT_ANNO = "weave/parent";
const PREDECESSOR_ANNO = "weave/predecessor";
const SERVICE_NOT_SUPPORTED = "Service not supported on this platform";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/notifications.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/type_records/bookmark.js");


let kSpecialIds = {};
[["menu", "bookmarksMenuFolder"],
 ["places", "placesRoot"],
 ["tags", "tagsFolder"],
 ["toolbar", "toolbarFolder"],
 ["unfiled", "unfiledBookmarksFolder"],
].forEach(function([guid, placeName]) {
  Utils.lazy2(kSpecialIds, guid, function() Svc.Bookmark[placeName]);
});


function idForGUID(guid) {
  if (guid in kSpecialIds)
    return kSpecialIds[guid];
  return Svc.Bookmark.getItemIdForGUID(guid);
}
function GUIDForId(placeId) {
  for (let [guid, id] in Iterator(kSpecialIds))
    if (placeId == id)
      return guid;
  return Svc.Bookmark.getItemGUID(placeId);
}

function BookmarksEngine() {
  this._init();
}
BookmarksEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "bookmarks",
  displayName: "Bookmarks",
  logName: "Bookmarks",
  _recordObj: PlacesItem,
  _storeObj: BookmarksStore,
  _trackerObj: BookmarksTracker,

  _sync: function BookmarksEngine__sync() {
    Svc.Bookmark.runInBatchMode({
      runBatched: Utils.bind2(this, SyncEngine.prototype._sync)
    }, null);
  }
};

function BookmarksStore() {
  this._init();
}
BookmarksStore.prototype = {
  __proto__: Store.prototype,
  name: "bookmarks",
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


  itemExists: function BStore_itemExists(id) {
    return idForGUID(id) > 0;
  },

  applyIncoming: function BStore_applyIncoming(record) {
    
    if (record.id in kSpecialIds) {
      this._log.debug("Skipping change to root node: " + record.id);
      return;
    }

    
    switch (record.type) {
      case "query": {
        
        if (record.bmkUri == null || record.folderName == null)
          break;

        
        let tag = record.folderName;
        let dummyURI = Utils.makeURI("about:weave#BStore_preprocess");
        this._ts.tagURI(dummyURI, [tag]);

        
        let tags = this._getNode(this._bms.tagsFolder);
        if (!(tags instanceof Ci.nsINavHistoryQueryResultNode))
          break;

        tags.containerOpen = true;
        for (let i = 0; i < tags.childCount; i++) {
          let child = tags.getChild(i);
          
          if (child.title == tag) {
            this._log.debug("query folder: " + tag + " = " + child.itemId);
            record.bmkUri = record.bmkUri.replace(/([:&]folder=)\d+/, "$1" +
              child.itemId);
            break;
          }
        }
        break;
      }
    }

    
    let parentGUID = record.parentid;
    record._orphan = false;
    if (parentGUID != null) {
      let parentId = idForGUID(parentGUID);

      
      if (parentId <= 0) {
        this._log.trace("Reparenting to unfiled until parent is synced");
        record._orphan = true;
        parentId = kSpecialIds.unfiled;
      }

      
      record._parent = parentId;
    }

    
    let predGUID = record.predecessorid;
    record._insertPos = Svc.Bookmark.DEFAULT_INDEX;
    if (!record._orphan) {
      
      if (predGUID == null)
        record._insertPos = 0;
      else {
        
        let predId = idForGUID(predGUID);
        if (predId != -1 && this._getParentGUIDForId(predId) == parentGUID) {
          record._insertPos = Svc.Bookmark.getItemIndex(predId) + 1;
          record._predId = predId;
        }
        else
          this._log.trace("Appending to end until predecessor is synced");
      }
    }

    
    Store.prototype.applyIncoming.apply(this, arguments);

    
    let itemId = idForGUID(record.id);
    if (itemId > 0) {
      
      
      if (record._orphan)
        Utils.anno(itemId, PARENT_ANNO, "T" + parentGUID);

      
      
      if (predGUID != null && record._insertPos == Svc.Bookmark.DEFAULT_INDEX)
        Utils.anno(itemId, PREDECESSOR_ANNO, "R" + predGUID);
    }
  },

  


  _findAnnoItems: function BStore__findAnnoItems(anno, val) {
    
    if (anno == PARENT_ANNO)
      val = "T" + val;
    
    else if (anno == PREDECESSOR_ANNO)
      val = "R" + val;

    return Svc.Annos.getItemsWithAnnotation(anno, {}).filter(function(id)
      Utils.anno(id, anno) == val);
  },

  



  _moveItemChain: function BStore__moveItemChain(itemId, insertPos, stopId) {
    let parentId = Svc.Bookmark.getFolderIdForItem(itemId);

    
    do {
      
      let itemPos = Svc.Bookmark.getItemIndex(itemId);
      let nextId = Svc.Bookmark.getIdForItemAt(parentId, itemPos + 1);

      Svc.Bookmark.moveItem(itemId, parentId, insertPos);
      this._log.trace("Moved " + itemId + " to " + insertPos);

      
      insertPos = Svc.Bookmark.getItemIndex(itemId) + 1;
      itemId = nextId;

      
      if (itemId == -1 || Svc.Annos.itemHasAnnotation(itemId, PREDECESSOR_ANNO))
        break;
    } while (itemId != stopId);
  },

  create: function BStore_create(record) {
    let newId;
    switch (record.type) {
    case "bookmark":
    case "query":
    case "microsummary": {
      let uri = Utils.makeURI(record.bmkUri);
      newId = this._bms.insertBookmark(record._parent, uri, record._insertPos,
        record.title);
      this._log.debug(["created bookmark", newId, "under", record._parent, "at",
        record._insertPos, "as", record.title, record.bmkUri].join(" "));

      this._tagURI(uri, record.tags);
      this._bms.setKeywordForBookmark(newId, record.keyword);
      if (record.description)
        Utils.anno(newId, "bookmarkProperties/description", record.description);

      if (record.loadInSidebar)
        Utils.anno(newId, "bookmarkProperties/loadInSidebar", true);

      if (record.type == "microsummary") {
        this._log.debug("   \-> is a microsummary");
        Utils.anno(newId, "bookmarks/staticTitle", record.staticTitle || "");
        let genURI = Utils.makeURI(record.generatorUri);
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
      newId = this._bms.createFolder(record._parent, record.title,
        record._insertPos);
      this._log.debug(["created folder", newId, "under", record._parent, "at",
        record._insertPos, "as", record.title].join(" "));
      break;
    case "livemark":
      newId = this._ls.createLivemark(record._parent, record.title,
        Utils.makeURI(record.siteUri), Utils.makeURI(record.feedUri),
        record._insertPos);
      this._log.debug(["created livemark", newId, "under", record._parent, "at",
        record._insertPos, "as", record.title, record.siteUri, record.feedUri].
        join(" "));
      break;
    case "separator":
      newId = this._bms.insertSeparator(record._parent, record._insertPos);
      this._log.debug(["created separator", newId, "under", record._parent,
        "at", record._insertPos].join(" "));
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

    
    let parented = [];
    if (!record._orphan)
      parented.push(newId);

    
    if (record.type == "folder") {
      let orphans = this._findAnnoItems(PARENT_ANNO, record.id);
      this._log.debug("Reparenting orphans " + orphans + " to " + record.title);
      orphans.forEach(function(orphan) {
        
        let insertPos = Svc.Bookmark.DEFAULT_INDEX;
        if (!Svc.Annos.itemHasAnnotation(orphan, PREDECESSOR_ANNO))
          insertPos = 0;

        
        Svc.Bookmark.moveItem(orphan, newId, insertPos);
        Svc.Annos.removeItemAnnotation(orphan, PARENT_ANNO);
        parented.push(orphan);
      });
    }

    
    parented.forEach(function(predId) {
      let predGUID = GUIDForId(predId);
      let followers = this._findAnnoItems(PREDECESSOR_ANNO, predGUID);
      if (followers.length > 1)
        this._log.warn(predId + " has more than one followers: " + followers);

      
      let parent = Svc.Bookmark.getFolderIdForItem(predId);
      followers.forEach(function(follow) {
        this._log.debug("Repositioning " + follow + " behind " + predId);
        if (Svc.Bookmark.getFolderIdForItem(follow) != parent) {
          this._log.warn("Follower doesn't have the same parent: " + parent);
          return;
        }

        
        let insertPos = Svc.Bookmark.getItemIndex(predId) + 1;
        this._moveItemChain(follow, insertPos, predId);

        
        Svc.Annos.removeItemAnnotation(follow, PREDECESSOR_ANNO);
      }, this);
    }, this);
  },

  remove: function BStore_remove(record) {
    let itemId = idForGUID(record.id);
    if (itemId <= 0) {
      this._log.debug("Item " + record.id + " already removed");
      return;
    }
    var type = this._bms.getItemType(itemId);

    switch (type) {
    case this._bms.TYPE_BOOKMARK:
      this._log.debug("  -> removing bookmark " + record.id);
      this._ts.untagURI(this._bms.getBookmarkURI(itemId), null);
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
    let itemId = idForGUID(record.id);

    if (itemId <= 0) {
      this._log.debug("Skipping update for unknown item: " + record.id);
      return;
    }

    this._log.trace("Updating " + record.id + " (" + itemId + ")");

    
    if (Svc.Bookmark.getFolderIdForItem(itemId) != record._parent) {
      this._log.trace("Moving item to a new parent");
      Svc.Bookmark.moveItem(itemId, record._parent, record._insertPos);
    }
    
    else if (Svc.Bookmark.getItemIndex(itemId) != record._insertPos &&
             !record._orphan) {
      this._log.trace("Moving item and followers to a new position");

      
      this._moveItemChain(itemId, record._insertPos, record._predId || itemId);
    }

    for (let [key, val] in Iterator(record.cleartext)) {
      switch (key) {
      case "title":
        this._bms.setItemTitle(itemId, val);
        break;
      case "bmkUri":
        this._bms.changeBookmarkURI(itemId, Utils.makeURI(val));
        break;
      case "tags":
        this._tagURI(this._bms.getBookmarkURI(itemId), val);
        break;
      case "keyword":
        this._bms.setKeywordForBookmark(itemId, val);
        break;
      case "description":
        Utils.anno(itemId, "bookmarkProperties/description", val);
        break;
      case "loadInSidebar":
        if (val)
          Utils.anno(itemId, "bookmarkProperties/loadInSidebar", true);
        else
          Svc.Annos.removeItemAnnotation(itemId, "bookmarkProperties/loadInSidebar");
        break;
      case "generatorUri": {
        try {
          let micsumURI = this._bms.getBookmarkURI(itemId);
          let genURI = Utils.makeURI(val);
	  if (this._ms == SERVICE_NOT_SUPPORTED) {
	    this._log.warn("Can't create microsummary -- not supported.");
	  } else {
            let micsum = this._ms.createMicrosummary(micsumURI, genURI);
            this._ms.setMicrosummary(itemId, micsum);
	  }
        } catch (e) {
          this._log.debug("Could not set microsummary generator URI: " + e);
        }
      } break;
      case "siteUri":
        this._ls.setSiteURI(itemId, Utils.makeURI(val));
        break;
      case "feedUri":
        this._ls.setFeedURI(itemId, Utils.makeURI(val));
        break;
      }
    }
  },

  changeItemID: function BStore_changeItemID(oldID, newID) {
    let itemId = idForGUID(oldID);
    if (itemId == null) 
      return;
    if (itemId <= 0) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                      newID + ": Item does not exist");
      return;
    }

    this._log.debug("Changing GUID " + oldID + " to " + newID);
    this._setGUID(itemId, newID);
  },

  _setGUID: function BStore__setGUID(itemId, guid) {
    let collision = idForGUID(guid);
    if (collision != -1) {
      this._log.warn("Freeing up GUID " + guid  + " used by " + collision);
      Svc.Annos.removeItemAnnotation(collision, "placesInternal/GUID");
    }
    Svc.Bookmark.setItemGUID(itemId, guid);
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
      return Utils.anno(id, "bookmarkProperties/description");
    } catch (e) {
      return undefined;
    }
  },

  _isLoadInSidebar: function BStore__isLoadInSidebar(id) {
    return Svc.Annos.itemHasAnnotation(id, "bookmarkProperties/loadInSidebar");
  },

  _getStaticTitle: function BStore__getStaticTitle(id) {
    try {
      return Utils.anno(id, "bookmarks/staticTitle");
    } catch (e) {
      return "";
    }
  },

  
  createRecord: function BStore_createRecord(guid, cryptoMetaURL) {
    let record = this.cache.get(guid);
    if (record)
      return record;

    let placeId = idForGUID(guid);
    if (placeId <= 0) { 
      record = new PlacesItem();
      record.id = guid;
      record.deleted = true;
      this.cache.put(guid, record);
      return record;
    }

    switch (this._bms.getItemType(placeId)) {
    case this._bms.TYPE_BOOKMARK:
      let bmkUri = this._bms.getBookmarkURI(placeId).spec;
      if (this._ms && this._ms.hasMicrosummary(placeId)) {
        record = new BookmarkMicsum();
        let micsum = this._ms.getMicrosummary(placeId);
        record.generatorUri = micsum.generator.uri.spec; 
        record.staticTitle = this._getStaticTitle(placeId);
      }
      else {
        if (bmkUri.search(/^place:/) == 0) {
          record = new BookmarkQuery();

          
          let folder = bmkUri.match(/[:&]folder=(\d+)/);
          try {
            
            if (folder != null) {
              folder = folder[1];
              record.folderName = this._bms.getItemTitle(folder);
              this._log.debug("query id: " + folder + " = " + record.folderName);
            }
          }
          catch(ex) {}
        }
        else
          record = new Bookmark();
        record.title = this._bms.getItemTitle(placeId);
      }

      record.bmkUri = bmkUri;
      record.tags = this._getTags(record.bmkUri);
      record.keyword = this._bms.getKeywordForBookmark(placeId);
      record.description = this._getDescription(placeId);
      record.loadInSidebar = this._isLoadInSidebar(placeId);
      break;

    case this._bms.TYPE_FOLDER:
      if (this._ls.isLivemark(placeId)) {
        record = new Livemark();
        record.siteUri = this._ls.getSiteURI(placeId).spec;
        record.feedUri = this._ls.getFeedURI(placeId).spec;

      } else {
        record = new BookmarkFolder();
      }

      record.title = this._bms.getItemTitle(placeId);
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

    record.id = guid;
    record.parentid = this._getParentGUIDForId(placeId);
    record.predecessorid = this._getPredecessorGUIDForId(placeId);
    record.encryption = cryptoMetaURL;

    this.cache.put(guid, record);
    return record;
  },

  _getParentGUIDForId: function BStore__getParentGUIDForId(itemId) {
    let parentid = this._bms.getFolderIdForItem(itemId);
    if (parentid == -1) {
      this._log.debug("Found orphan bookmark, reparenting to unfiled");
      parentid = this._bms.unfiledBookmarksFolder;
      this._bms.moveItem(itemId, parentid, -1);
    }
    return GUIDForId(parentid);
  },

  _getPredecessorGUIDForId: function BStore__getPredecessorGUIDForId(itemId) {
    
    let itemPos = Svc.Bookmark.getItemIndex(itemId);
    if (itemPos == 0)
      return;

    let parentId = Svc.Bookmark.getFolderIdForItem(itemId);
    let predecessorId = Svc.Bookmark.getIdForItemAt(parentId, itemPos - 1);
    return GUIDForId(predecessorId);
  },

  _getChildren: function BStore_getChildren(guid, items) {
    let node = guid; 
    if (typeof(node) == "string") 
      node = this._getNode(idForGUID(guid));

    if (node.type == node.RESULT_TYPE_FOLDER &&
        !this._ls.isLivemark(node.itemId)) {
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;

      
      for (var i = 0; i < node.childCount; i++) {
        let child = node.getChild(i);
        items[GUIDForId(child.itemId)] = true;
        this._getChildren(child, items);
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
    let items = {};
    for (let [guid, id] in Iterator(kSpecialIds))
      if (guid != "places" && guid != "tags")
        this._getChildren(guid, items);
    return items;
  },

  wipe: function BStore_wipe() {
    for (let [guid, id] in Iterator(kSpecialIds))
      if (guid != "places")
        this._bms.removeFolderChildren(id);
  }
};

function BookmarksTracker() {
  this._init();
}
BookmarksTracker.prototype = {
  __proto__: Tracker.prototype,
  name: "bookmarks",
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

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver,
    Ci.nsINavBookmarkObserver_MOZILLA_1_9_1_ADDITIONS
  ]),

  _init: function BMT__init() {
    this.__proto__.__proto__._init.call(this);

    
    for (let guid in kSpecialIds)
      this.ignoreID(guid);

    this._bms.addObserver(this, false);
  },

  





  _addId: function BMT__addId(itemId) {
    if (this.addChangedID(GUIDForId(itemId)))
      this._upScore();
  },

  
  _upScore: function BMT__upScore() {
    this._score += 10;
  },

  








  _ignore: function BMT__ignore(itemId, folder) {
    
    if (this.ignoreAll)
      return true;

    
    if (folder == null)
      folder = this._bms.getFolderIdForItem(itemId);

    let tags = kSpecialIds.tags;
    
    if (folder == tags)
      return true;

    
    if (this._bms.getFolderIdForItem(folder) == tags)
      return true;

    
    return this._ls.isLivemark(folder);
  },

  onItemAdded: function BMT_onEndUpdateBatch(itemId, folder, index) {
    if (this._ignore(itemId, folder))
      return;

    this._log.trace("onItemAdded: " + itemId);
    this._addId(itemId);
  },

  onBeforeItemRemoved: function BMT_onBeforeItemRemoved(itemId) {
    if (this._ignore(itemId))
      return;

    this._log.trace("onBeforeItemRemoved: " + itemId);
    this._addId(itemId);
  },

  onItemChanged: function BMT_onItemChanged(itemId, property, isAnno, value) {
    if (this._ignore(itemId))
      return;

    
    let annos = ["bookmarkProperties/description",
      "bookmarkProperties/loadInSidebar", "bookmarks/staticTitle",
      "livemark/feedURI", "livemark/siteURI", "microsummary/generatorURI"];
    if (isAnno && annos.indexOf(property) == -1)
      return;

    this._log.trace("onItemChanged: " + itemId +
                    (", " + property + (isAnno? " (anno)" : "")) +
                    (value? (" = \"" + value + "\"") : ""));
    this._addId(itemId);
  },

  onItemMoved: function BMT_onItemMoved(itemId, oldParent, oldIndex, newParent, newIndex) {
    if (this._ignore(itemId))
      return;

    this._log.trace("onItemMoved: " + itemId);
    this._addId(itemId);
  },

  onBeginUpdateBatch: function BMT_onBeginUpdateBatch() {},
  onEndUpdateBatch: function BMT_onEndUpdateBatch() {},
  onItemRemoved: function BMT_onItemRemoved(itemId, folder, index) {},
  onItemVisited: function BMT_onItemVisited(itemId, aVisitID, time) {}
};
