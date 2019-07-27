



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



const ITEM_BASIC_PROPERTY_NAMES = `
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
  this._itemsByURL = new Map();
  this._iterators = new Set();
  this._listeners = new Set();
}

ReadingListImpl.prototype = {

  ItemBasicPropertyNames: ITEM_BASIC_PROPERTY_NAMES,

  







  count: Task.async(function* (...optsList) {
    return (yield this._store.count(...optsList));
  }),

  






  containsURL: Task.async(function* (url) {
    url = normalizeURI(url).spec;

    
    

    
    if (this._itemsByURL.has(url)) {
      return true;
    }

    
    
    for (let itemWeakRef of this._itemsByURL.values()) {
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
    yield this._store.forEachItem(obj => {
      promiseChain = promiseChain.then(() => {
        return new Promise((resolve, reject) => {
          let promise = callback(this._itemFromObject(obj));
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

  














  addItem: Task.async(function* (obj) {
    obj = stripNonItemProperties(obj);
    normalizeReadingListProperties(obj);
    yield this._store.addItem(obj);
    this._invalidateIterators();
    let item = this._itemFromObject(obj);
    this._callListeners("onItemAdded", item);
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.broadcastAsyncMessage("Reader:Added", item);
    return item;
  }),

  













  updateItem: Task.async(function* (item) {
    this._ensureItemBelongsToList(item);
    yield this._store.updateItem(item._properties);
    this._invalidateIterators();
    this._callListeners("onItemUpdated", item);
  }),

  









  deleteItem: Task.async(function* (item) {
    this._ensureItemBelongsToList(item);
    yield this._store.deleteItemByURL(item.url);
    item.list = null;
    this._itemsByURL.delete(item.url);
    this._invalidateIterators();
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.broadcastAsyncMessage("Reader:Removed", item);
    this._callListeners("onItemDeleted", item);
  }),

  





  getItemForURL: Task.async(function* (uri) {
    let url = normalizeURI(uri).spec;
    let [item] = yield this.iterator({url: url}, {resolvedURL: url}).items(1);
    return item;
  }),

   







  addItemFromBrowser: Task.async(function* (browser, url) {
    let metadata = yield getMetadataFromBrowser(browser);
    let itemData = {
      url: url,
      title: metadata.title,
      resolvedURL: metadata.url,
      excerpt: metadata.description,
    };

    if (metadata.previews.length > 0) {
      itemData.preview = metadata.previews[0];
    }

    let item = yield ReadingList.addItem(itemData);
    return item;
  }),

  









  addListener(listener) {
    this._listeners.add(listener);
  },

  




  removeListener(listener) {
    this._listeners.delete(listener);
  },

  


  destroy: Task.async(function* () {
    yield this._store.destroy();
    for (let itemWeakRef of this._itemsByURL.values()) {
      let item = itemWeakRef.get();
      if (item) {
        item.list = null;
      }
    }
    this._itemsByURL.clear();
  }),

  
  _store: null,

  
  
  _itemsByURL: null,

  
  
  _iterators: null,

  
  _listeners: null,

  






  _itemFromObject(obj) {
    let itemWeakRef = this._itemsByURL.get(obj.url);
    let item = itemWeakRef ? itemWeakRef.get() : null;
    if (item) {
      item.setProperties(obj, false);
    }
    else {
      item = new ReadingListItem(obj);
      item.list = this;
      this._itemsByURL.set(obj.url, Cu.getWeakReference(item));
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




function normalizeReadingListProperties(obj) {
  if (obj.url) {
    obj.url = normalizeURI(obj.url).spec;
  }
  if (obj.resolvedURL) {
    obj.resolvedURL = normalizeURI(obj.resolvedURL).spec;
  }
}


let _unserializable = () => {}; 









function ReadingListItem(props={}) {
  this._properties = {};

  
  
  
  
  
  
  
  
  
  this._unserializable = _unserializable;

  this.setProperties(props, false);
}

ReadingListItem.prototype = {

  



  get id() {
    if (!this._id) {
      this._id = hash(this.url);
    }
    return this._id;
  },

  




  get guid() {
    return this._properties.guid || undefined;
  },
  set guid(val) {
    this._properties.guid = val;
  },

  



  get lastModified() {
    return this._properties.lastModified ?
           new Date(this._properties.lastModified) :
           undefined;
  },
  set lastModified(val) {
    this._properties.lastModified = val.valueOf();
  },

  



  get url() {
    return this._properties.url;
  },
  set url(val) {
    this._properties.url = normalizeURI(val).spec;
  },

  



  get uri() {
    return this._properties.url ?
           Services.io.newURI(this._properties.url, "", null) :
           undefined;
  },
  set uri(val) {
    this.url = normalizeURI(val).spec;
  },

  



  get domain() {
    try {
      return this.uri.host;
    }
    catch (err) {}
    return this.url;
  },

  



  get resolvedURL() {
    return this._properties.resolvedURL;
  },
  set resolvedURL(val) {
    this._properties.resolvedURL = normalizeURI(val).spec;
  },

  



  get resolvedURI() {
    return this._properties.resolvedURL ?
           Services.io.newURI(this._properties.resolvedURL, "", null) :
           undefined;
  },
  set resolvedURI(val) {
    this.resolvedURL = val.spec;
  },

  



  get title() {
    return this._properties.title;
  },
  set title(val) {
    this._properties.title = val;
  },

  



  get resolvedTitle() {
    return this._properties.resolvedTitle;
  },
  set resolvedTitle(val) {
    this._properties.resolvedTitle = val;
  },

  



  get excerpt() {
    return this._properties.excerpt;
  },
  set excerpt(val) {
    this._properties.excerpt = val;
  },

  



  get status() {
    return this._properties.status;
  },
  set status(val) {
    this._properties.status = val;
  },

  



  get favorite() {
    return !!this._properties.favorite;
  },
  set favorite(val) {
    this._properties.favorite = !!val;
  },

  



  get isArticle() {
    return !!this._properties.isArticle;
  },
  set isArticle(val) {
    this._properties.isArticle = !!val;
  },

  



  get wordCount() {
    return this._properties.wordCount;
  },
  set wordCount(val) {
    this._properties.wordCount = val;
  },

  



  get unread() {
    return !!this._properties.unread;
  },
  set unread(val) {
    this._properties.unread = !!val;
  },

  



  get addedOn() {
    return this._properties.addedOn ?
           new Date(this._properties.addedOn) :
           undefined;
  },
  set addedOn(val) {
    this._properties.addedOn = val.valueOf();
  },

  



  get storedOn() {
    return this._properties.storedOn ?
           new Date(this._properties.storedOn) :
           undefined;
  },
  set storedOn(val) {
    this._properties.storedOn = val.valueOf();
  },

  



  get markedReadBy() {
    return this._properties.markedReadBy;
  },
  set markedReadBy(val) {
    this._properties.markedReadBy = val;
  },

  



  get markedReadOn() {
    return this._properties.markedReadOn ?
           new Date(this._properties.markedReadOn) :
           undefined;
  },
  set markedReadOn(val) {
    this._properties.markedReadOn = val.valueOf();
  },

  



  get readPosition() {
    return this._properties.readPosition;
  },
  set readPosition(val) {
    this._properties.readPosition = val;
  },

  







  setProperties: Task.async(function* (props, update=true) {
    for (let name in props) {
      this._properties[name] = props[name];
    }
    
    normalizeReadingListProperties(this._properties);
    if (update) {
      yield this.list.updateItem(this);
    }
  }),

  




  delete: Task.async(function* () {
    this._ensureBelongsToList();
    yield this.list.deleteItem(this);
    this.delete = () => Promise.reject("The item has already been deleted");
  }),

  toJSON() {
    return this._properties;
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

function stripNonItemProperties(item) {
  let obj = {};
  for (let name of ITEM_BASIC_PROPERTY_NAMES) {
    if (name in item) {
      obj[name] = item[name];
    }
  }
  return obj;
}

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
