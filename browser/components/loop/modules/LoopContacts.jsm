


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoopStorage",
                                  "resource:///modules/loop/LoopStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CardDavImporter",
                                  "resource:///modules/loop/CardDavImporter.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "GoogleImporter",
                                  "resource:///modules/loop/GoogleImporter.jsm");
XPCOMUtils.defineLazyGetter(this, "eventEmitter", function() {
  const {EventEmitter} = Cu.import("resource://gre/modules/devtools/event-emitter.js", {});
  return new EventEmitter();
});

this.EXPORTED_SYMBOLS = ["LoopContacts"];

const kObjectStoreName = "contacts";












const kKeyPath = "_guid";
const kServiceIdIndex = "id";












const kFieldTypeString = "string";
const kFieldTypeNumber = "number";
const kFieldTypeNumberOrString = "number|string";
const kFieldTypeArray = "array";
const kFieldTypeBool = "boolean";
const kContactFields = {
  "id": {
    
    type: kFieldTypeNumberOrString
  },
  "published": {
    
    
    
    
    type: kFieldTypeNumberOrString
  },
  "updated": {
    
    
    
    
    type: kFieldTypeNumberOrString
  },
  "bday": {
    
    
    
    
    type: kFieldTypeNumberOrString
  },
  "blocked": {
    type: kFieldTypeBool
  },
  "adr": {
    type: kFieldTypeArray,
    contains: {
      "countryName": {
        type: kFieldTypeString
      },
      "locality": {
        type: kFieldTypeString
      },
      "postalCode": {
        
        type: kFieldTypeNumberOrString
      },
      "pref": {
        type: kFieldTypeBool
      },
      "region": {
        type: kFieldTypeString
      },
      "streetAddress": {
        type: kFieldTypeString
      },
      "type": {
        type: kFieldTypeArray,
        contains: kFieldTypeString
      }
    }
  },
  "email": {
    type: kFieldTypeArray,
    contains: {
      "pref": {
        type: kFieldTypeBool
      },
      "type": {
        type: kFieldTypeArray,
        contains: kFieldTypeString
      },
      "value": {
        type: kFieldTypeString
      }
    }
  },
  "tel": {
    type: kFieldTypeArray,
    contains: {
      "pref": {
        type: kFieldTypeBool
      },
      "type": {
        type: kFieldTypeArray,
        contains: kFieldTypeString
      },
      "value": {
        type: kFieldTypeString
      }
    }
  },
  "name": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "honorificPrefix": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "givenName": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "additionalName": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "familyName": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "honorificSuffix": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "category": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "org": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "jobTitle": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  },
  "note": {
    type: kFieldTypeArray,
    contains: kFieldTypeString
  }
};














const validateContact = function(obj, def = kContactFields) {
  for (let propName of Object.getOwnPropertyNames(obj)) {
    
    if (propName.startsWith("_")) {
      continue;
    }

    let propDef = def[propName];
    if (!propDef) {
      throw new Error("Field '" + propName + "' is not supported for contacts");
    }

    let val = obj[propName];

    switch (propDef.type) {
      case kFieldTypeString:
        if (typeof val != kFieldTypeString) {
          throw new Error("Field '" + propName + "' must be of type String");
        }
        break;
      case kFieldTypeNumberOrString:
        let type = typeof val;
        if (type != kFieldTypeNumber && type != kFieldTypeString) {
          throw new Error("Field '" + propName + "' must be of type Number or String");
        }
        break;
      case kFieldTypeBool:
        if (typeof val != kFieldTypeBool) {
          throw new Error("Field '" + propName + "' must be of type Boolean");
        }
        break;
      case kFieldTypeArray:
        if (!Array.isArray(val)) {
          throw new Error("Field '" + propName + "' must be an Array");
        }

        let contains = propDef.contains;
        
        
        let isScalarCheck = (typeof contains == kFieldTypeString);
        for (let arrayValue of val) {
          if (isScalarCheck) {
            if (typeof arrayValue != contains) {
              throw new Error("Field '" + propName + "' must be of type " + contains);
            }
          } else {
            validateContact(arrayValue, contains);
          }
        }
        break;
    }
  }
};














const batch = function(operation, data, callback) {
  let processed = [];
  if (!LoopContactsInternal.hasOwnProperty(operation) ||
    typeof LoopContactsInternal[operation] != 'function') {
    callback(new Error ("LoopContactsInternal does not contain a '" +
             operation + "' method"));
    return;
  }
  LoopStorage.asyncForEach(data, (item, next) => {
    LoopContactsInternal[operation](item, (err, result) => {
      if (err) {
        next(err);
        return;
      }
      processed.push(result);
      next();
    });
  }, err => {
    if (err) {
      callback(err, processed);
      return;
    }
    callback(null, processed);
  });
}







const extend = function(target, source) {
  for (let key of Object.getOwnPropertyNames(source)) {
    target[key] = source[key];
  }
  return target;
};

LoopStorage.on("upgrade", function(e, db) {
  if (db.objectStoreNames.contains(kObjectStoreName)) {
    return;
  }

  
  let store = db.createObjectStore(kObjectStoreName, {
    keyPath: kKeyPath,
    autoIncrement: true
  });
  store.createIndex(kServiceIdIndex, kServiceIdIndex, {unique: false});
});








let LoopContactsInternal = Object.freeze({
  


  _importServices: {
    "carddav": new CardDavImporter(),
    "google": new GoogleImporter()
  },

  










  add: function(details, callback) {
    if (!(kServiceIdIndex in details)) {
      callback(new Error("No '" + kServiceIdIndex + "' field present"));
      return;
    }
    try {
      validateContact(details);
    } catch (ex) {
      callback(ex);
      return;
    }

    LoopStorage.getStore(kObjectStoreName, (err, store) => {
      if (err) {
        callback(err);
        return;
      }

      let contact = extend({}, details);
      let now = Date.now();
      
      
      
      
      
      
      
      
      
      
      
      
      contact.published = contact.published ? new Date(contact.published).getTime() : now;
      contact.updated = contact.updated ? new Date(contact.updated).getTime() : now;
      contact._date_add = contact._date_lch = now;

      let request;
      try {
        request = store.add(contact);
      } catch (ex) {
        callback(ex);
        return;
      }

      request.onsuccess = event => {
        contact[kKeyPath] = event.target.result;
        eventEmitter.emit("add", contact);
        callback(null, contact);
      };

      request.onerror = event => callback(event.target.error);
    }, "readwrite");
  },

  








  addMany: function(contacts, callback) {
    batch("add", contacts, callback);
  },

  








  remove: function(guid, callback) {
    this.get(guid, (err, contact) => {
      if (err) {
        callback(err);
        return;
      }

      LoopStorage.getStore(kObjectStoreName, (err, store) => {
        if (err) {
          callback(err);
          return;
        }

        let request;
        try {
          request = store.delete(guid);
        } catch (ex) {
          callback(ex);
          return;
        }

        request.onsuccess = event => {
          if (contact) {
            eventEmitter.emit("remove", contact);
          }
          callback(null, event.target.result);
        };
        request.onerror = event => callback(event.target.error);
      }, "readwrite");
    });
  },

  








  removeMany: function(guids, callback) {
    batch("remove", guids, callback);
  },

  









  removeAll: function(callback) {
    LoopStorage.getStore(kObjectStoreName, (err, store) => {
      if (err) {
        callback(err);
        return;
      }

      let request;
      try {
        request = store.clear();
      } catch (ex) {
        callback(ex);
        return;
      }

      request.onsuccess = event => {
        eventEmitter.emit("removeAll", event.target.result);
        callback(null, event.target.result);
      };
      request.onerror = event => callback(event.target.error);
    }, "readwrite");
  },

  











  get: function(guid, callback) {
    LoopStorage.getStore(kObjectStoreName, (err, store) => {
      if (err) {
        callback(err);
        return;
      }

      let request;
      try {
        request = store.get(guid);
      } catch (ex) {
        callback(ex);
        return;
      }

      request.onsuccess = event => {
        if (!event.target.result) {
          callback(null, null);
          return;
        }
        let contact = extend({}, event.target.result);
        contact[kKeyPath] = guid;
        callback(null, contact);
      };
      request.onerror = event => callback(event.target.error);
    });
  },

  












  getByServiceId: function(serviceId, callback) {
    LoopStorage.getStore(kObjectStoreName, (err, store) => {
      if (err) {
        callback(err);
        return;
      }

      let index = store.index(kServiceIdIndex);
      let request;
      try {
        request = index.get(serviceId);
      } catch (ex) {
        callback(ex);
        return;
      }

      request.onsuccess = event => {
        if (!event.target.result) {
          callback(null, null);
          return;
        }

        let contact = extend({}, event.target.result);
        callback(null, contact);
      };
      request.onerror = event => callback(event.target.error);
    });
  },

  









  getAll: function(callback) {
    LoopStorage.getStore(kObjectStoreName, (err, store) => {
      if (err) {
        callback(err);
        return;
      }

      let cursorRequest = store.openCursor();
      let contactsList = [];

      cursorRequest.onsuccess = event => {
        let cursor = event.target.result;
        
        if (!cursor) {
          callback(null, contactsList);
          return;
        }

        let contact = extend({}, cursor.value);
        contact[kKeyPath] = cursor.key;
        contactsList.push(contact);

        cursor.continue();
      };

      cursorRequest.onerror = event => callback(event.target.error);
    });
  },

  










  getMany: function(guids, callback) {
    let contacts = [];
    LoopStorage.asyncParallel(guids, (guid, next) => {
      this.get(guid, (err, contact) => {
        if (err) {
          next(err);
          return;
        }
        contacts.push(contact);
        next();
      });
    }, err => {
      callback(err, !err ? contacts : null);
    });
  },

  












  update: function(details, callback) {
    if (!(kKeyPath in details)) {
      callback(new Error("No '" + kKeyPath + "' field present"));
      return;
    }
    try {
      validateContact(details);
    } catch (ex) {
      callback(ex);
      return;
    }

    let guid = details[kKeyPath];

    this.get(guid, (err, contact) => {
      if (err) {
        callback(err);
        return;
      }

      if (!contact) {
        callback(new Error("Contact with " + kKeyPath + " '" +
                           guid + "' could not be found"));
        return;
      }

      LoopStorage.getStore(kObjectStoreName, (err, store) => {
        if (err) {
          callback(err);
          return;
        }

        let previous = extend({}, contact);
        
        extend(contact, details);

        details._date_lch = Date.now();
        let request;
        try {
          request = store.put(contact);
        } catch (ex) {
          callback(ex);
          return;
        }

        request.onsuccess = event => {
          eventEmitter.emit("update", contact, previous);
          callback(null, event.target.result);
        };
        request.onerror = event => callback(event.target.error);
      }, "readwrite");
    });
  },

  








  block: function(guid, callback) {
    this.get(guid, (err, contact) => {
      if (err) {
        callback(err);
        return;
      }

      if (!contact) {
        callback(new Error("Contact with " + kKeyPath + " '" +
                           guid + "' could not be found"));
        return;
      }

      contact.blocked = true;
      this.update(contact, callback);
    });
  },

  








  unblock: function(guid, callback) {
    this.get(guid, (err, contact) => {
      if (err) {
        callback(err);
        return;
      }

      if (!contact) {
        callback(new Error("Contact with " + kKeyPath + " '" +
                           guid + "' could not be found"));
        return;
      }

      contact.blocked = false;
      this.update(contact, callback);
    });
  },

  








  startImport: function(options, windowRef, callback) {
    if (!("service" in options)) {
      callback(new Error("No import service specified in options"));
      return;
    }
    if (!(options.service in this._importServices)) {
      callback(new Error("Unknown import service specified: " + options.service));
      return;
    }
    this._importServices[options.service].startImport(options, callback,
                                                      LoopContacts, windowRef);
  },

  























  search: function(query, callback) {
    if (!("q" in query) || !query.q) {
      callback(new Error("Nothing to search for. 'q' is required."));
      return;
    }
    if (!("field" in query)) {
      query.field = "email";
    }
    let queryValue = query.q;
    if (query.field == "tel") {
      queryValue = queryValue.replace(/[\D]+/g, "");
    }

    const checkForMatch = function(fieldValue) {
      if (typeof fieldValue == "string") {
        if (query.field == "tel") {
          return fieldValue.replace(/[\D]+/g, "").endsWith(queryValue);
        }
        return fieldValue == queryValue;
      }
      if (typeof fieldValue == "number" || typeof fieldValue == "boolean") {
        return fieldValue == queryValue;
      }
      if ("value" in fieldValue) {
        return checkForMatch(fieldValue.value);
      }
      return false;
    };

    let foundContacts = [];
    this.getAll((err, contacts) => {
      if (err) {
        callback(err);
        return;
      }

      for (let contact of contacts) {
        let matchWith = contact[query.field];
        if (!matchWith) {
          continue;
        }

        
        if (Array.isArray(matchWith)) {
          for (let fieldValue of matchWith) {
            if (checkForMatch(fieldValue)) {
              foundContacts.push(contact);
              break;
            }
          }
        } else if (checkForMatch(matchWith)) {
          foundContacts.push(contact);
        }
      }

      callback(null, foundContacts);
    });
  }
});













this.LoopContacts = Object.freeze({
  add: function(details, callback) {
    return LoopContactsInternal.add(details, callback);
  },

  addMany: function(contacts, callback) {
    return LoopContactsInternal.addMany(contacts, callback);
  },

  remove: function(guid, callback) {
    return LoopContactsInternal.remove(guid, callback);
  },

  removeMany: function(guids, callback) {
    return LoopContactsInternal.removeMany(guids, callback);
  },

  removeAll: function(callback) {
    return LoopContactsInternal.removeAll(callback);
  },

  get: function(guid, callback) {
    return LoopContactsInternal.get(guid, callback);
  },

  getByServiceId: function(serviceId, callback) {
    return LoopContactsInternal.getByServiceId(serviceId, callback);
  },

  getAll: function(callback) {
    return LoopContactsInternal.getAll(callback);
  },

  getMany: function(guids, callback) {
    return LoopContactsInternal.getMany(guids, callback);
  },

  update: function(details, callback) {
    return LoopContactsInternal.update(details, callback);
  },

  block: function(guid, callback) {
    return LoopContactsInternal.block(guid, callback);
  },

  unblock: function(guid, callback) {
    return LoopContactsInternal.unblock(guid, callback);
  },

  startImport: function(options, windowRef, callback) {
    return LoopContactsInternal.startImport(options, windowRef, callback);
  },

  search: function(query, callback) {
    return LoopContactsInternal.search(query, callback);
  },

  promise: function(method, ...params) {
    return new Promise((resolve, reject) => {
      this[method](...params, (error, result) => {
        if (error) {
          reject(error);
        } else {
          resolve(result);
        }
      });
    });
  },

  on: (...params) => eventEmitter.on(...params),

  once: (...params) => eventEmitter.once(...params),

  off: (...params) => eventEmitter.off(...params)
});
