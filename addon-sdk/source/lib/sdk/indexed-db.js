



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Cc, Ci } = require("chrome");
const { extend } = require("./core/heritage");
const { id } = require("self");


let sanitizeId = function(id){
  let uuidRe =
    /^\{([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\}$/;

  let domain = id.
    toLowerCase().
    replace(/@/g, "-at-").
    replace(/\./g, "-dot-").
    replace(uuidRe, "$1");

  return domain
};

const PSEUDOURI = "indexeddb://" + sanitizeId(id) 


Cc["@mozilla.org/dom/indexeddb/manager;1"].
	getService(Ci.nsIIndexedDatabaseManager).
	initWindowless(this);


if (typeof(indexedDB) === "undefined")
  this.indexedDB = mozIndexedDB;


let principaluri = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService).
              newURI(PSEUDOURI, null, null);

let principal = Cc["@mozilla.org/scriptsecuritymanager;1"].
	               getService(Ci.nsIScriptSecurityManager).
	               getCodebasePrincipal(principaluri);

exports.indexedDB = extend({}, {   
  open: indexedDB.openForPrincipal.bind(indexedDB, principal),
  deleteDatabase: indexedDB.deleteForPrincipal.bind(indexedDB, principal),
  cmp: indexedDB.cmp
});

exports.IDBKeyRange = IDBKeyRange;
exports.DOMException = Ci.nsIDOMDOMException;
exports.IDBCursor = Ci.nsIIDBCursor;
exports.IDBTransaction = Ci.nsIIDBTransaction;
exports.IDBOpenDBRequest = Ci.nsIIDBOpenDBRequest;
exports.IDBVersionChangeEvent = Ci.nsIIDBVersionChangeEvent;
exports.IDBDatabase = Ci.nsIIDBDatabase;
exports.IDBFactory = Ci.nsIIDBFactory;
exports.IDBIndex = Ci.nsIIDBIndex;
exports.IDBObjectStore = Ci.nsIIDBObjectStore;
exports.IDBRequest = Ci.nsIIDBRequest;
