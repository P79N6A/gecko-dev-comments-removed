



"use strict";

const EXPORTED_SYMBOLS = [
  "RotaryEngine",
  "RotaryRecord",
  "RotaryStore",
  "RotaryTracker",
];

const {utils: Cu} = Components;

Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/util.js");








function RotaryRecord(collection, id) {
  CryptoWrapper.call(this, collection, id);
}
RotaryRecord.prototype = {
  __proto__: CryptoWrapper.prototype
};
Utils.deferGetSet(RotaryRecord, "cleartext", ["denomination"]);

function RotaryStore(engine) {
  Store.call(this, "Rotary", engine);
  this.items = {};
}
RotaryStore.prototype = {
  __proto__: Store.prototype,

  create: function create(record) {
    this.items[record.id] = record.denomination;
  },

  remove: function remove(record) {
    delete this.items[record.id];
  },

  update: function update(record) {
    this.items[record.id] = record.denomination;
  },

  itemExists: function itemExists(id) {
    return (id in this.items);
  },

  createRecord: function createRecord(id, collection) {
    let record = new RotaryRecord(collection, id);

    if (!(id in this.items)) {
      record.deleted = true;
      return record;
    }

    record.denomination = this.items[id] || "Data for new record: " + id;
    return record;
  },

  changeItemID: function changeItemID(oldID, newID) {
    if (oldID in this.items) {
      this.items[newID] = this.items[oldID];
    }

    delete this.items[oldID];
  },

  getAllIDs: function getAllIDs() {
    let ids = {};
    for (let id in this.items) {
      ids[id] = true;
    }
    return ids;
  },

  wipe: function wipe() {
    this.items = {};
  }
};

function RotaryTracker(engine) {
  Tracker.call(this, "Rotary", engine);
}
RotaryTracker.prototype = {
  __proto__: Tracker.prototype
};


function RotaryEngine(service) {
  SyncEngine.call(this, "Rotary", service);
  
  this.toFetch        = [];
  this.previousFailed = [];
}
RotaryEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: RotaryStore,
  _trackerObj: RotaryTracker,
  _recordObj: RotaryRecord,

  _findDupe: function _findDupe(item) {
    
    
    if (item.id == "DUPE_INCOMING") {
      return "DUPE_LOCAL";
    }

    for (let [id, value] in Iterator(this._store.items)) {
      if (item.denomination == value) {
        return id;
      }
    }
  }
};
