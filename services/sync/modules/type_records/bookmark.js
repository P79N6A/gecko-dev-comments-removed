



































const EXPORTED_SYMBOLS = ['PlacesItem', 'Bookmark', 'BookmarkFolder', 'BookmarkMicsum',
                          'Livemark', 'BookmarkSeparator'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");

Function.prototype.async = Async.sugar;

function PlacesItem(uri) {
  this._PlacesItem_init(uri);
}
PlacesItem.prototype = {
  decrypt: function PlacesItem_decrypt(onComplete, passphrase) {
    CryptoWrapper.prototype.decrypt.call(this, Utils.bind2(this, function(ret) {
      
      if (!this.deleted)
        this.__proto__ = this.getTypeObject(this.type).prototype;

      
      onComplete(ret);
    }), passphrase);
  },

  getTypeObject: function PlacesItem_getTypeObject(type) {
    switch (type) {
      case "bookmark":
        return Bookmark;
      case "microsummary":
        return BookmarkMicsum;
      case "folder":
        return BookmarkFolder;
      case "livemark":
        return Livemark;
      case "separator":
        return BookmarkSeparator;
    }
    throw "Unknown places item object type";
  },

  __proto__: CryptoWrapper.prototype,
  _logName: "Record.PlacesItem",

  _PlacesItem_init: function BmkItemRec_init(uri) {
    this._CryptoWrap_init(uri);
    this.cleartext = {
    };
  },

  get type() this.cleartext.type,
  set type(value) {
    
    this.cleartext.type = value;
  }
};

function Bookmark(uri) {
  this._Bookmark_init(uri);
}
Bookmark.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Bookmark",

  _Bookmark_init: function BmkRec_init(uri) {
    this._PlacesItem_init(uri);
    this.cleartext.type = "bookmark";
  },

  get title() this.cleartext.title,
  set title(value) {
    this.cleartext.title = value;
  },

  get bmkUri() this.cleartext.uri,
  set bmkUri(value) {
    if (typeof(value) == "string")
      this.cleartext.uri = value;
    else
      this.cleartext.uri = value.spec;
  },

  get description() this.cleartext.description,
  set description(value) {
    this.cleartext.description = value;
  },

  get tags() this.cleartext.tags,
  set tags(value) {
    this.cleartext.tags = value;
  },

  get keyword() this.cleartext.keyword,
  set keyword(value) {
    this.cleartext.keyword = value;
  }
};

function BookmarkMicsum(uri) {
  this._BookmarkMicsum_init(uri);
}
BookmarkMicsum.prototype = {
  __proto__: Bookmark.prototype,
  _logName: "Record.BookmarkMicsum",

  _BookmarkMicsum_init: function BmkMicsumRec_init(uri) {
    this._Bookmark_init(uri);
    this.cleartext.type = "microsummary";
  },

  get generatorURI() this.cleartext.generatorURI,
  set generatorURI(value) {
    if (typeof(value) == "string")
      this.cleartext.generatorURI = value;
    else
      this.cleartext.generatorURI = value? value.spec : "";
  },

  get staticTitle() this.cleartext.staticTitle,
  set staticTitle(value) {
    this.cleartext.staticTitle = value;
  }
};

function BookmarkFolder(uri) {
  this._BookmarkFolder_init(uri);
}
BookmarkFolder.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Folder",

  _BookmarkFolder_init: function FolderRec_init(uri) {
    this._PlacesItem_init(uri);
    this.cleartext.type = "folder";
  },

  get title() this.cleartext.title,
  set title(value) {
    this.cleartext.title = value;
  }
};

function Livemark(uri) {
  this._Livemark_init(uri);
}
Livemark.prototype = {
  __proto__: BookmarkFolder.prototype,
  _logName: "Record.Livemark",

  _Livemark_init: function LvmkRec_init(uri) {
    this._BookmarkFolder_init(uri);
    this.cleartext.type = "livemark";
  },

  get siteURI() this.cleartext.siteURI,
  set siteURI(value) {
    if (typeof(value) == "string")
      this.cleartext.siteURI = value;
    else
      this.cleartext.siteURI = value? value.spec : "";
  },

  get feedURI() this.cleartext.feedURI,
  set feedURI(value) {
    if (typeof(value) == "string")
      this.cleartext.feedURI = value;
    else
      this.cleartext.feedURI = value? value.spec : "";
  }
};

function BookmarkSeparator(uri) {
  this._BookmarkSeparator_init(uri);
}
BookmarkSeparator.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Separator",

  _BookmarkSeparator_init: function SepRec_init(uri) {
    this._PlacesItem_init(uri);
    this.cleartext.type = "separator";
  }
};
