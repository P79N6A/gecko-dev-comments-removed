



































const EXPORTED_SYMBOLS = ["PlacesItem", "Bookmark", "BookmarkFolder",
  "BookmarkMicsum", "BookmarkQuery", "Livemark", "BookmarkSeparator"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function PlacesItem(uri, type) {
  CryptoWrapper.call(this, uri);
  this.type = type || "item";
}
PlacesItem.prototype = {
  decrypt: function PlacesItem_decrypt(passphrase) {
    
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

Utils.deferGetSet(PlacesItem, "cleartext", ["parentName", "predecessorid", "type"]);

function Bookmark(uri, type) {
  PlacesItem.call(this, uri, type || "bookmark");
}
Bookmark.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Bookmark",
};

Utils.deferGetSet(Bookmark, "cleartext", ["title", "bmkUri", "description",
  "loadInSidebar", "tags", "keyword"]);

function BookmarkMicsum(uri) {
  Bookmark.call(this, uri, "microsummary");
}
BookmarkMicsum.prototype = {
  __proto__: Bookmark.prototype,
  _logName: "Record.BookmarkMicsum",
};

Utils.deferGetSet(BookmarkMicsum, "cleartext", ["generatorUri", "staticTitle"]);

function BookmarkQuery(uri) {
  Bookmark.call(this, uri, "query");
}
BookmarkQuery.prototype = {
  __proto__: Bookmark.prototype,
  _logName: "Record.BookmarkQuery",
};

Utils.deferGetSet(BookmarkQuery, "cleartext", ["folderName"]);

function BookmarkFolder(uri, type) {
  PlacesItem.call(this, uri, type || "folder");
}
BookmarkFolder.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Folder",
};

Utils.deferGetSet(BookmarkFolder, "cleartext", "title");

function Livemark(uri) {
  BookmarkFolder.call(this, uri, "livemark");
}
Livemark.prototype = {
  __proto__: BookmarkFolder.prototype,
  _logName: "Record.Livemark",
};

Utils.deferGetSet(Livemark, "cleartext", ["siteUri", "feedUri"]);

function BookmarkSeparator(uri) {
  PlacesItem.call(this, uri, "separator");
}
BookmarkSeparator.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Separator",
};

Utils.deferGetSet(BookmarkSeparator, "cleartext", "pos");
