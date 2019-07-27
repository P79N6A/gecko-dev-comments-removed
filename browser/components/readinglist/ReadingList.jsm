



"use strict";

this.EXPORTED_SYMBOLS = ["ReadingList"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;


Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Log.jsm");


(function() {
  let parentLog = Log.repository.getLogger("readinglist");
  parentLog.level = Preferences.get("browser.readinglist.logLevel", Log.Level.Warn);
  Preferences.observe("browser.readinglist.logLevel", value => {
    parentLog.level = value;
  });
  let formatter = new Log.BasicFormatter();
  parentLog.addAppender(new Log.ConsoleAppender(formatter));
  parentLog.addAppender(new Log.DumpAppender(formatter));
})();

let log = Log.repository.getLogger("readinglist.api");






function Item(data) {
  this._data = data;
}

Item.prototype = {
  



  get id() {
    return this._data.id;
  },

  



  get lastModified() {
    return this._data.last_modified;
  },

  


  get originalUrl() {
    return Services.io.newURI(this._data.url, null, null);
  },

  


  get originalTitle() {
    return this._data.title || "";
  },

  


  get resolvedUrl() {
    return Services.io.newURI(this._data.resolved_url || this._data.url, null, null);
  },

  


  get resolvedTitle() {
    return this._data.resolved_title || this.originalTitle;
  },

  


  get excerpt() {
    return this._data.excerpt || "";
  },

  


  get state() {
    return ReadingList.ItemStates[this._data.state] || ReadingList.ItemStates.OK;
  },

  


  get isFavorite() {
    return !!this._data.favorite;
  },

  


  get isArticle() {
    return !!this._data.is_article;
  },

  


  get wordCount() {
    return this._data.word_count || 0;
  },

  


  get isUnread() {
    return !!this._data.unread;
  },

  



  get addedBy() {
    return this._data.added_by;
  },

  


  get addedOn() {
    return new Date(this._data.added_on);
  },

  


  get storedOn() {
    return new Date(this._data.stored_on);
  },

  



  get markedReadBy() {
    return this._data.marked_read_by;
  },

  


  get markedReadOn() {
    return new date(this._data.marked_read_on);
  },

  


  get readPosition() {
    return this._data.read_position;
  },

  

  




  get images() {
    return [];
  },

  




  get favicon() {
    return null;
  },

  

  



  get url() {
    return this.resolvedUrl;
  },
  


  get title() {
    return this.resolvedTitle;
  },

  



  get domain() {
    let host = this.resolvedUrl.host;
    if (host.startsWith("www.")) {
      host = host.slice(4);
    }
    return host;
  },

  


  toString() {
    return `[Item url=${this.url.spec}]`;
  },

  


  toJSON() {
    return this._data;
  },
};


let ItemStates = {
  OK: Symbol("ok"),
  ARCHIVED: Symbol("archived"),
  DELETED: Symbol("deleted"),
};


this.ReadingList = {
  Item: Item,
  ItemStates: ItemStates,

  _listeners: new Set(),
  _items: [],

  


  _init() {
    log.debug("Init");

    
    let mockData = JSON.parse(Preferences.get("browser.readinglist.mockData", "[]"));
    for (let itemData of mockData) {
      this._items.push(new Item(itemData));
    }
  },

  



  addListener(listener) {
    this._listeners.add(listener);
  },

  



  removeListener(listener) {
    this._listeners.delete(listener);
  },

  




  _notifyListeners(eventName, ...args) {
    for (let listener of this._listeners) {
      if (typeof listener[eventName] != "function") {
        continue;
      }

      try {
        listener[eventName](...args);
      } catch (e) {
        log.error(`Error calling listener.${eventName}`, e);
      }
    }
  },

  








  getNumItems(conditions = {unread: false}) {
    return new Promise((resolve, reject) => {
      resolve(this._items.length);
    });
  },

  






  getItems(options = {sort: "addedOn", conditions: {unread: false}}) {
    return new Promise((resolve, reject) => {
      resolve([...this._items]);
    });
  },

  






  getItemByID(url) {
    return new Promise((resolve, reject) => {
      resolve(null);
    });
  },

  









  getItemByURL(url) {
    return new Promise((resolve, reject) => {
      resolve(null);
    });
  },
};


ReadingList._init();
