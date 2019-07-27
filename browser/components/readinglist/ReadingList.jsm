



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


{ 
  let parentLog = Log.repository.getLogger("readinglist");
  parentLog.level = Preferences.get("browser.readinglist.logLevel", Log.Level.Warn);
  Preferences.observe("browser.readinglist.logLevel", value => {
    parentLog.level = value;
  });
  let formatter = new Log.BasicFormatter();
  parentLog.addAppender(new Log.ConsoleAppender(formatter));
  parentLog.addAppender(new Log.DumpAppender(formatter));
}
let log = Log.repository.getLogger("readinglist.api");





const ITEM_RECORD_PROPERTIES = `
  guid
  lastModified
  url
  title
  resolvedURL
  resolvedTitle
  excerpt
  preview
  status
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
`.trim().split(/\s+/);




















































function ReadingListImpl(store) {
  this._store = store;
  this._itemsByNormalizedURL = new Map();
  this._iterators = new Set();
  this._listeners = new Set();
}

ReadingListImpl.prototype = {

  ItemRecordProperties: ITEM_RECORD_PROPERTIES,

  







  count: Task.async(function* (...optsList) {
    return (yield this._store.count(...optsList));
  }),

  






  hasItemForURL: Task.async(function* (url) {
    url = normalizeURI(url).spec;

    
    

    
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
    let promiseChain = Promise.resolve();
    yield this._store.forEachItem(record => {
      promiseChain = promiseChain.then(() => {
        return new Promise((resolve, reject) => {
          let promise = callback(this._itemFromRecord(record));
          if (promise instanceof Promise) {
            return promise.then(resolve, reject);
          }
          resolve();
          return undefined;
        });
      });
    }, ...optsList);
    yield promiseChain;
  }),

  







  iterator(...optsList) {
    let iter = new ReadingListItemIterator(this, ...optsList);
    this._iterators.add(Cu.getWeakReference(iter));
    return iter;
  },

  














  addItem: Task.async(function* (record) {
    record = normalizeRecord(record);
    record.addedOn = Date.now();
    if (Services.prefs.prefHasUserValue("services.sync.client.name")) {
      record.addedBy = Services.prefs.getCharPref("services.sync.client.name");
    }
    yield this._store.addItem(record);
    this._invalidateIterators();
    let item = this._itemFromRecord(record);
    this._callListeners("onItemAdded", item);
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.broadcastAsyncMessage("Reader:Added", item);
    return item;
  }),

  













  updateItem: Task.async(function* (item) {
    this._ensureItemBelongsToList(item);
    yield this._store.updateItem(item._record);
    this._invalidateIterators();
    this._callListeners("onItemUpdated", item);
  }),

  









  deleteItem: Task.async(function* (item) {
    this._ensureItemBelongsToList(item);
    yield this._store.deleteItemByURL(item.url);
    item.list = null;
    this._itemsByNormalizedURL.delete(item.url);
    this._invalidateIterators();
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.broadcastAsyncMessage("Reader:Removed", item);
    this._callListeners("onItemDeleted", item);
  }),

  





  item: Task.async(function* (...optsList) {
    return (yield this.iterator(...optsList).items(1))[0] || null;
  }),

  






  itemForURL: Task.async(function* (uri) {
    let url = normalizeURI(uri).spec;
    return (yield this.item({ url: url }, { resolvedURL: url }));
  }),

  







  addItemFromBrowser: Task.async(function* (browser, url) {
    let metadata = yield getMetadataFromBrowser(browser);
    let record = {
      url: url,
      title: metadata.title,
      resolvedURL: metadata.url,
      excerpt: metadata.description,
    };

    if (metadata.previews.length > 0) {
      itemData.preview = metadata.previews[0];
    }

    return (yield this.addItem(record));
  }),

  









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
      throw new Error("The item is not a ReadingListItem");
    }
    item._ensureBelongsToList();
  },
};


let _unserializable = () => {}; 










function ReadingListItem(record={}) {
  this._record = record;

  
  
  
  
  
  
  
  
  
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
    return this._record.url;
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
    return this._record.resolvedURL;
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
    return this._record.title;
  },
  set title(val) {
    this._updateRecord({ title: val });
  },

  



  get resolvedTitle() {
    return this._record.resolvedTitle;
  },
  set resolvedTitle(val) {
    this._updateRecord({ resolvedTitle: val });
  },

  



  get excerpt() {
    return this._record.excerpt;
  },
  set excerpt(val) {
    this._updateRecord({ excerpt: val });
  },

  



  get status() {
    return this._record.status;
  },
  set status(val) {
    this._updateRecord({ status: val });
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
    return this._record.wordCount;
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
    return this._record.markedReadBy;
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
    return this._record.readPosition;
  },
  set readPosition(val) {
    this._updateRecord({ readPosition: val });
  },

  



   get preview() {
     return this._record.preview;
   },

  




  delete: Task.async(function* () {
    this._ensureBelongsToList();
    yield this.list.deleteItem(this);
    this.delete = () => Promise.reject("The item has already been deleted");
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
    for (let prop in partialRecord) {
      record[prop] = partialRecord[prop];
    }
    this._record = record;
  },

  _ensureBelongsToList() {
    if (!this.list) {
      throw new Error("The item must belong to a reading list");
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
      throw new Error("The iterator has been invalidated");
    }
  },
};










function normalizeRecord(nonNormalizedRecord) {
  for (let prop in nonNormalizedRecord) {
    if (!ITEM_RECORD_PROPERTIES.includes(prop)) {
      throw new Error("Unrecognized item property: " + prop);
    }
  }

  let record = clone(nonNormalizedRecord);
  if (record.url) {
    record.url = normalizeURI(record.url).spec;
  }
  if (record.resolvedURL) {
    record.resolvedURL = normalizeURI(record.resolvedURL).spec;
  }
  return record;
}








function normalizeURI(uri) {
  if (typeof uri == "string") {
    uri = Services.io.newURI(uri, "", null);
  }
  uri = uri.cloneIgnoringRef();
  try {
    uri.userPass = "";
  } catch (ex) {} 
  return uri;
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








function getMetadataFromBrowser(browser) {
  let mm = browser.messageManager;
  return new Promise(resolve => {
    function handleResult(msg) {
      mm.removeMessageListener("PageMetadata:PageDataResult", handleResult);
      resolve(msg.json);
    }
    mm.addMessageListener("PageMetadata:PageDataResult", handleResult);
    mm.sendAsyncMessage("PageMetadata:GetPageData");
  });
}

Object.defineProperty(this, "ReadingList", {
  get() {
    if (!this._singleton) {
      let store = new SQLiteStore("reading-list-temp2.sqlite");
      this._singleton = new ReadingListImpl(store);
    }
    return this._singleton;
  },
});
