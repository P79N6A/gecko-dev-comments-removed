



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Cc, Ci } = require("chrome");
const { id } = require("./self");


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



if (typeof(indexedDB) === "undefined") {
  Cc["@mozilla.org/dom/indexeddb/manager;1"].
    getService(Ci.nsIIndexedDatabaseManager).
    initWindowless(this);

  
  if (typeof(indexedDB) === "undefined")
    this.indexedDB = mozIndexedDB;
}


let principaluri = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService).
              newURI(PSEUDOURI, null, null);

let principal = Cc["@mozilla.org/scriptsecuritymanager;1"].
	               getService(Ci.nsIScriptSecurityManager).
	               getCodebasePrincipal(principaluri);

exports.indexedDB = Object.freeze({
  open: indexedDB.openForPrincipal.bind(indexedDB, principal),
  deleteDatabase: indexedDB.deleteForPrincipal.bind(indexedDB, principal),
  cmp: indexedDB.cmp
});

exports.IDBKeyRange = IDBKeyRange;
exports.DOMException = Ci.nsIDOMDOMException;
