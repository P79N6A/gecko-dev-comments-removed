



































const EXPORTED_SYMBOLS = ["PlacesItem", "Bookmark", "BookmarkFolder",
  "BookmarkMicsum", "BookmarkQuery", "Livemark", "BookmarkSeparator"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/util.js");

function PlacesItem(collection, id, type) {
  CryptoWrapper.call(this, collection, id);
  this.type = type || "item";
}
PlacesItem.prototype = {
  decrypt: function PlacesItem_decrypt() {
    
    let clear = CryptoWrapper.prototype.decrypt.apply(this, arguments);

    
    if (!this.deleted)
      this.__proto__ = this.getTypeObject(this.type).prototype;

    return clear;
  },

  getTypeObject: function PlacesItem_getTypeObject(type) {
    switch (type) {
      case "bookmark":
        return Bookmark;
      case "microsummary":
        return BookmarkMicsum;
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

function BookmarkMicsum(collection, id) {
  Bookmark.call(this, collection, id, "microsummary");
}
BookmarkMicsum.prototype = {
  __proto__: Bookmark.prototype,
  _logName: "Record.BookmarkMicsum",
};

Utils.deferGetSet(BookmarkMicsum, "cleartext", ["generatorUri", "staticTitle"]);

function BookmarkQuery(collection, id) {
  Bookmark.call(this, collection, id, "query");
}
BookmarkQuery.prototype = {
  __proto__: Bookmark.prototype,
  _logName: "Record.BookmarkQuery",
};

Utils.deferGetSet(BookmarkQuery, "cleartext", ["folderName"]);

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
