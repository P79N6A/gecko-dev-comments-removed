



"use strict";

this.EXPORTED_SYMBOLS = [
  "ReadingList",
];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Log.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SQLiteStore",
  "resource:///modules/readinglist/SQLiteStore.jsm");



XPCOMUtils.defineLazyGetter(this, "SyncUtils", function() {
  const {Utils} = Cu.import("resource://services-sync/util.js", {});
  return Utils;
});

let log = Log.repository.getLogger("readinglist.api");









const ITEM_RECORD_PROPERTIES = `
  guid
  serverLastModified
  url
  preview
  title
  resolvedURL
  resolvedTitle
  excerpt
  archived
  deleted
  favorite
  isArticle
  wordCount
  unread
  addedBy
  addedOn
  storedOn
  markedReadBy
  markedReadOn
  readPosition
  syncStatus
`.trim().split(/\s+/);



const SYNC_STATUS_SYNCED = 0;
const SYNC_STATUS_NEW = 1;
const SYNC_STATUS_CHANGED_STATUS = 2;
const SYNC_STATUS_CHANGED_MATERIAL = 3;
const SYNC_STATUS_DELETED = 4;



const STORE_OPTIONS_IGNORE_DELETED = {
  syncStatus: [
    SYNC_STATUS_SYNCED,
    SYNC_STATUS_NEW,
    SYNC_STATUS_CHANGED_STATUS,
    SYNC_STATUS_CHANGED_MATERIAL,
  ],
};




const SYNC_STATUS_PROPERTIES_STATUS = `
  favorite
  markedReadBy
  markedReadOn
  readPosition
  unread
`.trim().split(/\s+/);

function ReadingListError(message) {
  this.message = message;
  this.name = this.constructor.name;
  this.stack = (new Error()).stack;

  
  this.originalError = null;
}
ReadingListError.prototype = new Error();
ReadingListError.prototype.constructor = ReadingListError;

function ReadingListExistsError(message) {
  message = message || "The item already exists";
  ReadingListError.call(this, message);
}
ReadingListExistsError.prototype = new ReadingListError();
ReadingListExistsError.prototype.constructor = ReadingListExistsError;

function ReadingListDeletedError(message) {
  message = message || "The item has been deleted";
  ReadingListError.call(this, message);
}
ReadingListDeletedError.prototype = new ReadingListError();
ReadingListDeletedError.prototype.constructor = ReadingListDeletedError;




















































function ReadingListImpl(store) {
  this._store = store;
  this._itemsByNormalizedURL = new Map();
  this._iterators = new Set();
  this._listeners = new Set();
}

ReadingListImpl.prototype = {

  Error: {
    Error: ReadingListError,
    Exists: ReadingListExistsError,
    Deleted: ReadingListDeletedError,
  },

  ItemRecordProperties: ITEM_RECORD_PROPERTIES,

  SyncStatus: {
    SYNCED: SYNC_STATUS_SYNCED,
    NEW: SYNC_STATUS_NEW,
    CHANGED_STATUS: SYNC_STATUS_CHANGED_STATUS,
    CHANGED_MATERIAL: SYNC_STATUS_CHANGED_MATERIAL,
    DELETED: SYNC_STATUS_DELETED,
  },

  SyncStatusProperties: {
    STATUS: SYNC_STATUS_PROPERTIES_STATUS,
  },

  







  count: Task.async(function* (...optsList) {
    return (yield this._store.count(optsList, STORE_OPTIONS_IGNORE_DELETED));
  }),

  






  hasItemForURL: Task.async(function* (url) {
    url = normalizeURI(url);

    
    

    
    if (this._itemsByNormalizedURL.has(url)) {
      return true;
    }

    
    
    for (let itemWeakRef of this._itemsByNormalizedURL.values()) {
      let item = itemWeakRef.get();
      if (item && item.resolvedURL == url) {
        return true;
      }
    }

    
    let count = yield this.count({url: url}, {resolvedURL: url});
    return (count > 0);
  }),

  












  forEachItem: Task.async(function* (callback, ...optsList) {
    let thisCallback = record => callback(this._itemFromRecord(record));
    yield this._forEachRecord(thisCallback, optsList, STORE_OPTIONS_IGNORE_DELETED);
  }),

  



  forEachSyncedDeletedGUID: Task.async(function* (callback, ...optsList) {
    let thisCallback = record => callback(record.guid);
    yield this._forEachRecord(thisCallback, optsList, {
      syncStatus: SYNC_STATUS_DELETED,
    });
  }),

  





  _forEachRecord: Task.async(function* (callback, optsList, storeOptions) {
    let promiseChain = Promise.resolve();
    yield this._store.forEachItem(record => {
      promiseChain = promiseChain.then(() => {
        return new Promise((resolve, reject) => {
          let promise = callback(record);
          if (promise instanceof Promise) {
            return promise.then(resolve, reject);
          }
          resolve();
          return undefined;
        });
      });
    }, optsList, storeOptions);
    yield promiseChain;
  }),

  







  iterator(...optsList) {
    let iter = new ReadingListItemIterator(this, ...optsList);
    this._iterators.add(Cu.getWeakReference(iter));
    return iter;
  },

  














  addItem: Task.async(function* (record) {
    record = normalizeRecord(record);
    if (!record.url) {
      throw new ReadingListError("The item to be added must have a url");
    }
    if (!("addedOn" in record)) {
      record.addedOn = Date.now();
    }
    if (!("addedBy" in record)) {
      try {
        record.addedBy = Services.prefs.getCharPref("services.sync.client.name");
      } catch (ex) {
        record.addedBy = SyncUtils.getDefaultDeviceName();
      }
    }
    if (!("syncStatus" in record)) {
      record.syncStatus = SYNC_STATUS_NEW;
    }

    log.debug("Adding item with guid: ${guid}, url: ${url}", record);
    yield this._store.addItem(record);
    log.trace("Added item with guid: ${guid}, url: ${url}", record);
    this._invalidateIterators();
    let item = this._itemFromRecord(record);
    this._callListeners("onItemAdded", item);
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.broadcastAsyncMessage("Reader:Added", item.toJSON());
    return item;
  }),

  













  updateItem: Task.async(function* (item) {
    if (item._deleted) {
      throw new ReadingListDeletedError("The item to be updated has been deleted");
    }
    if (!item._record.url) {
      throw new ReadingListError("The item to be updated must have a url");
    }
    this._ensureItemBelongsToList(item);
    log.debug("Updating item with guid: ${guid}, url: ${url}", item._record);
    yield this._store.updateItem(item._record);
    log.trace("Finished updating item with guid: ${guid}, url: ${url}", item._record);
    this._invalidateIterators();
    this._callListeners("onItemUpdated", item);
  }),

  









  deleteItem: Task.async(function* (item) {
    if (item._deleted) {
      throw new ReadingListDeletedError("The item has already been deleted");
    }
    this._ensureItemBelongsToList(item);

    log.debug("Deleting item with guid: ${guid}, url: ${url}");

    
    
    
    if (item._record.syncStatus == SYNC_STATUS_NEW) {
      log.debug("Item is new, truly deleting it", item._record);
      yield this._store.deleteItemByURL(item.url);
    }
    else {
      log.debug("Item has been synced, only marking it as deleted",
                item._record);
      
      
      let newRecord = {};
      for (let prop of ITEM_RECORD_PROPERTIES) {
        newRecord[prop] = null;
      }
      newRecord.guid = item._record.guid;
      newRecord.syncStatus = SYNC_STATUS_DELETED;
      yield this._store.updateItemByGUID(newRecord);
    }

    log.trace("Finished deleting item with guid: ${guid}, url: ${url}", item._record);
    item.list = null;
    item._deleted = true;
    
    if (!this._itemsByNormalizedURL.delete(item.url)) {
      log.error("Failed to remove item from the map", item);
    }
    this._invalidateIterators();
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.broadcastAsyncMessage("Reader:Removed", item.toJSON());
    this._callListeners("onItemDeleted", item);
  }),

  





  item: Task.async(function* (...optsList) {
    return (yield this.iterator(...optsList).items(1))[0] || null;
  }),

  






  itemForURL: Task.async(function* (uri) {
    let url = normalizeURI(uri);
    return (yield this.item({ url: url }, { resolvedURL: url }));
  }),

  







  addItemFromBrowser: Task.async(function* (browser, url) {
    let metadata = yield this.getMetadataFromBrowser(browser);
    let record = {
      url: url,
      title: metadata.title,
      resolvedURL: metadata.url,
      excerpt: metadata.description,
    };

    if (metadata.previews.length > 0) {
      record.preview = metadata.previews[0];
    }

    return (yield this.addItem(record));
  }),

  






  getMetadataFromBrowser(browser) {
    let mm = browser.messageManager;
    return new Promise(resolve => {
      function handleResult(msg) {
        mm.removeMessageListener("PageMetadata:PageDataResult", handleResult);
        resolve(msg.json);
      }
      mm.addMessageListener("PageMetadata:PageDataResult", handleResult);
      mm.sendAsyncMessage("PageMetadata:GetPageData");
    });
  },

  









  addListener(listener) {
    this._listeners.add(listener);
  },

  




  removeListener(listener) {
    this._listeners.delete(listener);
  },

  


  destroy: Task.async(function* () {
    yield this._store.destroy();
    for (let itemWeakRef of this._itemsByNormalizedURL.values()) {
      let item = itemWeakRef.get();
      if (item) {
        item.list = null;
      }
    }
    this._itemsByNormalizedURL.clear();
  }),

  
  _store: null,

  
  
  _itemsByNormalizedURL: null,

  
  
  _iterators: null,

  
  _listeners: null,

  






  _itemFromRecord(record) {
    if (!record.url) {
      throw new Error("record must have a URL");
    }
    let itemWeakRef = this._itemsByNormalizedURL.get(record.url);
    let item = itemWeakRef ? itemWeakRef.get() : null;
    if (item) {
      item._record = record;
    }
    else {
      item = new ReadingListItem(record);
      item.list = this;
      this._itemsByNormalizedURL.set(record.url, Cu.getWeakReference(item));
    }
    return item;
  },

  



  _invalidateIterators() {
    for (let iterWeakRef of this._iterators) {
      let iter = iterWeakRef.get();
      if (iter) {
        iter.invalidate();
      }
    }
    this._iterators.clear();
  },

  





  _callListeners(methodName, item) {
    for (let listener of this._listeners) {
      if (methodName in listener) {
        try {
          listener[methodName](item);
        }
        catch (err) {
          Cu.reportError(err);
        }
      }
    }
  },

  _ensureItemBelongsToList(item) {
    if (!item || !item._ensureBelongsToList) {
      throw new ReadingListError("The item is not a ReadingListItem");
    }
    item._ensureBelongsToList();
  },
};


let _unserializable = () => {}; 










function ReadingListItem(record={}) {
  this._record = record;
  this._deleted = false;

  
  
  
  
  
  
  
  
  
  this._unserializable = _unserializable;
}

ReadingListItem.prototype = {

  
  
  

  



  get id() {
    if (!this._id) {
      this._id = hash(this.url);
    }
    return this._id;
  },

  




  get guid() {
    return this._record.guid || undefined;
  },

  



  get url() {
    return this._record.url || undefined;
  },

  



  get uri() {
    if (!this._uri) {
      this._uri = this._record.url ?
                  Services.io.newURI(this._record.url, "", null) :
                  undefined;
    }
    return this._uri;
  },

  



  get resolvedURL() {
    return this._record.resolvedURL || undefined;
  },
  set resolvedURL(val) {
    this._updateRecord({ resolvedURL: val });
  },

  




  get resolvedURI() {
    return this._record.resolvedURL ?
           Services.io.newURI(this._record.resolvedURL, "", null) :
           undefined;
  },
  set resolvedURI(val) {
    this._updateRecord({ resolvedURL: val });
  },

  



  get title() {
    return this._record.title || undefined;
  },
  set title(val) {
    this._updateRecord({ title: val });
  },

  



  get resolvedTitle() {
    return this._record.resolvedTitle || undefined;
  },
  set resolvedTitle(val) {
    this._updateRecord({ resolvedTitle: val });
  },

  



  get excerpt() {
    return this._record.excerpt || undefined;
  },
  set excerpt(val) {
    this._updateRecord({ excerpt: val });
  },

  



  get archived() {
    return !!this._record.archived;
  },
  set archived(val) {
    this._updateRecord({ archived: !!val });
  },

  



  get favorite() {
    return !!this._record.favorite;
  },
  set favorite(val) {
    this._updateRecord({ favorite: !!val });
  },

  



  get isArticle() {
    return !!this._record.isArticle;
  },
  set isArticle(val) {
    this._updateRecord({ isArticle: !!val });
  },

  



  get wordCount() {
    return this._record.wordCount || undefined;
  },
  set wordCount(val) {
    this._updateRecord({ wordCount: val });
  },

  



  get unread() {
    return !!this._record.unread;
  },
  set unread(val) {
    this._updateRecord({ unread: !!val });
  },

  



  get addedOn() {
    return this._record.addedOn ?
           new Date(this._record.addedOn) :
           undefined;
  },
  set addedOn(val) {
    this._updateRecord({ addedOn: val.valueOf() });
  },

  



  get storedOn() {
    return this._record.storedOn ?
           new Date(this._record.storedOn) :
           undefined;
  },
  set storedOn(val) {
    this._updateRecord({ storedOn: val.valueOf() });
  },

  



  get markedReadBy() {
    return this._record.markedReadBy || undefined;
  },
  set markedReadBy(val) {
    this._updateRecord({ markedReadBy: val });
  },

  



  get markedReadOn() {
    return this._record.markedReadOn ?
           new Date(this._record.markedReadOn) :
           undefined;
  },
  set markedReadOn(val) {
    this._updateRecord({ markedReadOn: val.valueOf() });
  },

  



  get readPosition() {
    return this._record.readPosition || undefined;
  },
  set readPosition(val) {
    this._updateRecord({ readPosition: val });
  },

  



   get preview() {
     return this._record.preview || undefined;
   },

  




  delete: Task.async(function* () {
    if (this._deleted) {
      throw new ReadingListDeletedError("The item has already been deleted");
    }
    this._ensureBelongsToList();
    yield this.list.deleteItem(this);
  }),

  toJSON() {
    return this._record;
  },

  














  get _record() {
    return this.__record;
  },
  set _record(val) {
    this.__record = normalizeRecord(val);
  },

  





  _updateRecord(partialRecord) {
    let record = this._record;

    
    
    if (record.syncStatus == SYNC_STATUS_SYNCED ||
        record.syncStatus == SYNC_STATUS_CHANGED_STATUS) {
      let allStatusChanges = Object.keys(partialRecord).every(prop => {
        return SYNC_STATUS_PROPERTIES_STATUS.indexOf(prop) >= 0;
      });
      record.syncStatus = allStatusChanges ? SYNC_STATUS_CHANGED_STATUS :
                          SYNC_STATUS_CHANGED_MATERIAL;
    }

    for (let prop in partialRecord) {
      record[prop] = partialRecord[prop];
    }
    this._record = record;
  },

  _ensureBelongsToList() {
    if (!this.list) {
      throw new ReadingListError("The item must belong to a list");
    }
  },
};



















function ReadingListItemIterator(list, ...optsList) {
  this.list = list;
  this.index = 0;
  this.optsList = optsList;
}

ReadingListItemIterator.prototype = {

  



  invalid: false,

  












  forEach: Task.async(function* (callback, count=-1) {
    this._ensureValid();
    let optsList = clone(this.optsList);
    optsList.push({
      offset: this.index,
      limit: count,
    });
    yield this.list.forEachItem(item => {
      this.index++;
      return callback(item);
    }, ...optsList);
  }),

  






  items: Task.async(function* (count) {
    this._ensureValid();
    let optsList = clone(this.optsList);
    optsList.push({
      offset: this.index,
      limit: count,
    });
    let items = [];
    yield this.list.forEachItem(item => items.push(item), ...optsList);
    this.index += items.length;
    return items;
  }),

  



  invalidate() {
    this.invalid = true;
  },

  _ensureValid() {
    if (this.invalid) {
      throw new ReadingListError("The iterator has been invalidated");
    }
  },
};










function normalizeRecord(nonNormalizedRecord) {
  let record = {};
  for (let prop in nonNormalizedRecord) {
    if (ITEM_RECORD_PROPERTIES.indexOf(prop) < 0) {
      throw new ReadingListError("Unrecognized item property: " + prop);
    }
    switch (prop) {
    case "url":
    case "resolvedURL":
      if (nonNormalizedRecord[prop]) {
        record[prop] = normalizeURI(nonNormalizedRecord[prop]);
      }
      else {
        record[prop] = nonNormalizedRecord[prop];
      }
      break;
    default:
      record[prop] = nonNormalizedRecord[prop];
      break;
    }
  }
  return record;
}









function normalizeURI(uri) {
  if (typeof uri == "string") {
    try {
      uri = Services.io.newURI(uri, "", null);
    } catch (ex) {
      return uri;
    }
  }
  uri = uri.cloneIgnoringRef();
  try {
    uri.userPass = "";
  } catch (ex) {} 
  return uri.spec;
};

function hash(str) {
  let hasher = Cc["@mozilla.org/security/hash;1"].
               createInstance(Ci.nsICryptoHash);
  hasher.init(Ci.nsICryptoHash.MD5);
  let stream = Cc["@mozilla.org/io/string-input-stream;1"].
               createInstance(Ci.nsIStringInputStream);
  stream.data = str;
  hasher.updateFromStream(stream, -1);
  let binaryStr = hasher.finish(false);
  let hexStr =
    [("0" + binaryStr.charCodeAt(i).toString(16)).slice(-2) for (i in binaryStr)].
    join("");
  return hexStr;
}

function clone(obj) {
  return Cu.cloneInto(obj, {}, { cloneFunctions: false });
}

Object.defineProperty(this, "ReadingList", {
  get() {
    if (!this._singleton) {
      let store = new SQLiteStore("reading-list.sqlite");
      this._singleton = new ReadingListImpl(store);
    }
    return this._singleton;
  },
});
