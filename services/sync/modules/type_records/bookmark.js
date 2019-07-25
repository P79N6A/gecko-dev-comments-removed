



































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

function PlacesItem(uri, authenticator) {
  this._PlacesItem_init(uri, authenticator);
}
PlacesItem.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.PlacesItem",

  _PlacesItem_init: function BmkItemRec_init(uri, authenticator) {
    this._CryptoWrap_init(uri, authenticator);
    this.cleartext = {
    };
  },

  get type() this.cleartext.type,
  set type(value) {
    
    this.cleartext.type = value;
  }
};

function Bookmark(uri, authenticator) {
  this._Bookmark_init(uri, authenticator);
}
Bookmark.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Bookmark",

  _Bookmark_init: function BmkRec_init(uri, authenticator) {
    this._PlacesItem_init(uri, authenticator);
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

function BookmarkMicsum(uri, authenticator) {
  this._BookmarkMicsum_init(uri, authenticator);
}
BookmarkMicsum.prototype = {
  __proto__: Bookmark.prototype,
  _logName: "Record.BookmarkMicsum",

  _BookmarkMicsum_init: function BmkMicsumRec_init(uri, authenticator) {
    this._Bookmark_init(uri, authenticator);
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

function BookmarkFolder(uri, authenticator) {
  this._BookmarkFolder_init(uri, authenticator);
}
BookmarkFolder.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Folder",

  _BookmarkFolder_init: function FolderRec_init(uri, authenticator) {
    this._PlacesItem_init(uri, authenticator);
    this.cleartext.type = "folder";
  },

  get title() this.cleartext.title,
  set title(value) {
    this.cleartext.title = value;
  }
};

function Livemark(uri, authenticator) {
  this._Livemark_init(uri, authenticator);
}
Livemark.prototype = {
  __proto__: BookmarkFolder.prototype,
  _logName: "Record.Livemark",

  _Livemark_init: function LvmkRec_init(uri, authenticator) {
    this._BookmarkFolder_init(uri, authenticator);
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

function BookmarkSeparator(uri, authenticator) {
  this._BookmarkSeparator_init(uri, authenticator);
}
BookmarkSeparator.prototype = {
  __proto__: PlacesItem.prototype,
  _logName: "Record.Separator",

  _BookmarkSeparator_init: function SepRec_init(uri, authenticator) {
    this._PlacesItem_init(uri, authenticator);
    this.cleartext.type = "separator";
  }
};
