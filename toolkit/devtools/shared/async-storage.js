








































const {Cc, Ci, Cu, Cr} = require("chrome");
const {indexedDB} = require("sdk/indexed-db");
const {Promise} = Cu.import("resource://gre/modules/Promise.jsm", {});

module.exports = (function() {
  "use strict";

  var DBNAME = "devtools-async-storage";
  var DBVERSION = 1;
  var STORENAME = "keyvaluepairs";
  var db = null;

  function withStore(type, onsuccess, onerror) {
    if (db) {
      var transaction = db.transaction(STORENAME, type);
      var store = transaction.objectStore(STORENAME);
      onsuccess(store);
    } else {
      var openreq = indexedDB.open(DBNAME, DBVERSION);
      openreq.onerror = function withStoreOnError() {
        onerror();
      };
      openreq.onupgradeneeded = function withStoreOnUpgradeNeeded() {
        
        openreq.result.createObjectStore(STORENAME);
      };
      openreq.onsuccess = function withStoreOnSuccess() {
        db = openreq.result;
        var transaction = db.transaction(STORENAME, type);
        var store = transaction.objectStore(STORENAME);
        onsuccess(store);
      };
    }
  }

  function getItem(key) {
    return new Promise((resolve, reject) => {
      var req;
      withStore("readonly", (store) => {
        store.transaction.oncomplete = function onComplete() {
          var value = req.result;
          if (value === undefined) {
            value = null;
          }
          resolve(value);
        };
        req = store.get(key);
        req.onerror = function getItemOnError() {
          reject("Error in asyncStorage.getItem(): ", req.error.name);
        };
      }, reject);
    });
  }

  function setItem(key, value) {
    return new Promise((resolve, reject) => {
      withStore("readwrite", (store) => {
        store.transaction.oncomplete = resolve;
        var req = store.put(value, key);
        req.onerror = function setItemOnError() {
          reject("Error in asyncStorage.setItem(): ", req.error.name);
        };
      }, reject);
    });
  }

  function removeItem(key) {
    return new Promise((resolve, reject) => {
      withStore("readwrite", (store) => {
        store.transaction.oncomplete = resolve;
        var req = store.delete(key);
        req.onerror = function removeItemOnError() {
          reject("Error in asyncStorage.removeItem(): ", req.error.name);
        };
      }, reject);
    });
  }

  function clear() {
    return new Promise((resolve, reject) => {
      withStore("readwrite", (store) => {
        store.transaction.oncomplete = resolve;
        var req = store.clear();
        req.onerror = function clearOnError() {
          reject("Error in asyncStorage.clear(): ", req.error.name);
        };
      }, reject);
    });
  }

  function length() {
    return new Promise((resolve, reject) => {
      var req;
      withStore("readonly", (store) => {
        store.transaction.oncomplete = function onComplete() {
          resolve(req.result);
        }
        req = store.count();
        req.onerror = function lengthOnError() {
          reject("Error in asyncStorage.length(): ", req.error.name);
        };
      }, reject);
    });
  }

  function key(n) {
    return new Promise((resolve, reject) => {
      if (n < 0) {
        resolve(null);
        return;
      }

      var req;
      withStore("readonly", (store) => {
        store.transaction.oncomplete = function onComplete() {
          var cursor = req.result;
          resolve(cursor ? cursor.key : null);
        };
        var advanced = false;
        req = store.openCursor();
        req.onsuccess = function keyOnSuccess() {
          var cursor = req.result;
          if (!cursor) {
            
            return;
          }
          if (n === 0 || advanced) {
            
            
            return;
          }

          
          advanced = true;
          cursor.advance(n);
        };
        req.onerror = function keyOnError() {
          reject("Error in asyncStorage.key(): ", req.error.name);
        };
      }, reject);
    });
  }

  return {
    getItem: getItem,
    setItem: setItem,
    removeItem: removeItem,
    clear: clear,
    length: length,
    key: key
  };
}());
