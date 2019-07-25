



"use strict";

const EXPORTED_SYMBOLS = ['ContactDB'];

let DEBUG = 0;

if (DEBUG) {
  debug = function (s) { dump("-*- ContactDB component: " + s + "\n"); }
} else {
  debug = function (s) {}
}

const Cu = Components.utils; 
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/IndexedDBHelper.jsm");

const DB_NAME = "contacts";
const DB_VERSION = 1;
const STORE_NAME = "contacts";

function ContactDB(aGlobal) {
  debug("Constructor");
  this._global = aGlobal;
}

ContactDB.prototype = {
  __proto__: IndexedDBHelper.prototype,
  










  createSchema: function createSchema(db) {
    let objectStore = db.createObjectStore(this.dbStoreName, {keyPath: "id"});

    
    objectStore.createIndex("published", "published", { unique: false });
    objectStore.createIndex("updated",   "updated",   { unique: false });

    
    objectStore.createIndex("nickname",   "properties.nickname",   { unique: false, multiEntry: true });
    objectStore.createIndex("name",       "properties.name",       { unique: false, multiEntry: true });
    objectStore.createIndex("familyName", "properties.familyName", { unique: false, multiEntry: true });
    objectStore.createIndex("givenName",  "properties.givenName",  { unique: false, multiEntry: true });
    objectStore.createIndex("tel",        "properties.tel",        { unique: false, multiEntry: true });
    objectStore.createIndex("email",      "properties.email",      { unique: false, multiEntry: true });
    objectStore.createIndex("note",       "properties.note",       { unique: false, multiEntry: true });

    objectStore.createIndex("nicknameLowerCase",   "search.nickname",   { unique: false, multiEntry: true });
    objectStore.createIndex("nameLowerCase",       "search.name",       { unique: false, multiEntry: true });
    objectStore.createIndex("familyNameLowerCase", "search.familyName", { unique: false, multiEntry: true });
    objectStore.createIndex("givenNameLowerCase",  "search.givenName",  { unique: false, multiEntry: true });
    objectStore.createIndex("telLowerCase",        "search.tel",        { unique: false, multiEntry: true });
    objectStore.createIndex("emailLowerCase",      "search.email",      { unique: false, multiEntry: true });
    objectStore.createIndex("noteLowerCase",       "search.note",       { unique: false, multiEntry: true });

    debug("Created object stores and indexes");
  },

  makeImport: function makeImport(aContact) {
    let contact = {};
    contact.properties = {
      name:            [],
      honorificPrefix: [],
      givenName:       [],
      additionalName:  [],
      familyName:      [],
      honorificSuffix: [],
      nickname:        [],
      email:           [],
      photo:           [],
      url:             [],
      category:        [],
      adr:             [],
      tel:             [],
      org:             [],
      bday:            null,
      note:            [],
      impp:            [],
      anniversary:     null,
      sex:             null,
      genderIdentity:  null
    };

    contact.search = {
      name:            [],
      honorificPrefix: [],
      givenName:       [],
      additionalName:  [],
      familyName:      [],
      honorificSuffix: [],
      nickname:        [],
      email:           [],
      category:        [],
      tel:             [],
      org:             [],
      note:            [],
      impp:            []
    };

    for (let field in aContact.properties) {
      contact.properties[field] = aContact.properties[field];
      
      if (aContact.properties[field] && contact.search[field]) {
        for (let i = 0; i <= aContact.properties[field].length; i++) {
          if (aContact.properties[field][i]) {
            if (field == "tel") {
              
              

              
              let number = aContact.properties[field][i];
              for(let i = 0; i < number.length; i++) {
                contact.search[field].push(number.substring(i, number.length));
              }
              
              let digits = number.match(/\d/g);
              if (digits && number.length != digits.length) {
                digits = digits.join('');
                for(let i = 0; i < digits.length; i++) {
                  contact.search[field].push(digits.substring(i, digits.length));
                }
              }
              debug("lookup: " + JSON.stringify(contact.search[field]));
            } else {
              contact.search[field].push(aContact.properties[field][i].toLowerCase());
            }
          }
        }
      }
    }
    debug("contact:" + JSON.stringify(contact));

    contact.updated = aContact.updated;
    contact.published = aContact.published;
    contact.id = aContact.id;

    return contact;
  },

  makeExport: function makeExport(aRecord) {
    let contact = {};
    contact.properties = aRecord.properties;

    for (let field in aRecord.properties)
      contact.properties[field] = aRecord.properties[field];

    contact.updated = aRecord.updated;
    contact.published = aRecord.published;
    contact.id = aRecord.id;
    return contact;
  },

  updateRecordMetadata: function updateRecordMetadata(record) {
    if (!record.id) {
      Cu.reportError("Contact without ID");
    }
    if (!record.published) {
      record.published = new Date();
    }
    record.updated = new Date();
  },

  saveContact: function saveContact(aContact, successCb, errorCb) {
    let contact = this.makeImport(aContact);
    this.newTxn("readwrite", function (txn, store) {
      debug("Going to update" + JSON.stringify(contact));

      
      
      let newRequest = store.get(contact.id);
      newRequest.onsuccess = function (event) {
        if (!event.target.result) {
          debug("new record!")
          this.updateRecordMetadata(contact);
          store.put(contact);
        } else {
          debug("old record!")
          if (new Date(typeof contact.updated === "undefined" ? 0 : contact.updated) < new Date(event.target.result.updated)) {
            debug("rev check fail!");
            txn.abort();
            return;
          } else {
            debug("rev check OK");
            contact.published = event.target.result.published;
            contact.updated = new Date();
            store.put(contact);
          }
        }
      }.bind(this);
    }.bind(this), successCb, errorCb);
  },

  removeContact: function removeContact(aId, aSuccessCb, aErrorCb) {
    this.newTxn("readwrite", function (txn, store) {
      debug("Going to delete" + aId);
      store.delete(aId);
    }, aSuccessCb, aErrorCb);
  },

  clear: function clear(aSuccessCb, aErrorCb) {
    this.newTxn("readwrite", function (txn, store) {
      debug("Going to clear all!");
      store.clear();
    }, aSuccessCb, aErrorCb);
  },

  
















  find: function find(aSuccessCb, aFailureCb, aOptions) {
    debug("ContactDB:find val:" + aOptions.filterValue + " by: " + aOptions.filterBy + " op: " + aOptions.filterOp + "\n");

    let self = this;
    this.newTxn("readonly", function (txn, store) {
      if (aOptions && (aOptions.filterOp == "equals" || aOptions.filterOp == "contains")) {
        self._findWithIndex(txn, store, aOptions);
      } else {
        self._findAll(txn, store, aOptions);
      }
    }, aSuccessCb, aFailureCb);
  },

  _findWithIndex: function _findWithIndex(txn, store, options) {
    debug("_findWithIndex: " + options.filterValue +" " + options.filterOp + " " + options.filterBy + " ");
    let fields = options.filterBy;
    for (let key in fields) {
      debug("key: " + fields[key]);
      if (!store.indexNames.contains(fields[key]) && !fields[key] == "id") {
        debug("Key not valid!" + fields[key] + ", " + store.indexNames);
        txn.abort();
        return;
      }
    }

    
    if (options.filterBy.length == 0) {
      debug("search in all fields!" + JSON.stringify(store.indexNames));
      for(let myIndex = 0; myIndex < store.indexNames.length; myIndex++) {
        fields = Array.concat(fields, store.indexNames[myIndex])
      }
    }

    
    let limit = options.sortBy === 'undefined' ? options.filterLimit : null;

    let filter_keys = fields.slice();
    for (let key = filter_keys.shift(); key; key = filter_keys.shift()) {
      let request;
      if (key == "id") {
        
        request = store.getAll(options.filterValue);
      } else if (options.filterOp == "equals") {
        debug("Getting index: " + key);
        
        let index = store.index(key);
        request = index.getAll(options.filterValue, limit);
      } else {
        
        let tmp = options.filterValue.toLowerCase();
        let range = this._global.IDBKeyRange.bound(tmp, tmp + "\uFFFF");
        let index = store.index(key + "LowerCase");
        request = index.getAll(range, limit);
      }
      if (!txn.result)
        txn.result = {};

      request.onsuccess = function (event) {
        debug("Request successful. Record count:" + event.target.result.length);
        for (let i in event.target.result)
          txn.result[event.target.result[i].id] = this.makeExport(event.target.result[i]);
      }.bind(this);
    }
  },

  _findAll: function _findAll(txn, store, options) {
    debug("ContactDB:_findAll:  " + JSON.stringify(options));
    if (!txn.result)
      txn.result = {};
    
    let limit = options.sortBy === 'undefined' ? options.filterLimit : null;
    store.getAll(null, limit).onsuccess = function (event) {
      debug("Request successful. Record count:", event.target.result.length);
      for (let i in event.target.result)
        txn.result[event.target.result[i].id] = this.makeExport(event.target.result[i]);
    }.bind(this);
  },

  init: function init(aGlobal) {
      this.initDBHelper(DB_NAME, DB_VERSION, STORE_NAME, aGlobal);
  }
};
