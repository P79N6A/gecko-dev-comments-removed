



































const EXPORTED_SYMBOLS = ['FormEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/type_records/forms.js");

let FormWrapper = {
  getAllEntries: function getAllEntries() {
    let entries = [];
    
    let query = this.createStatement(
      "SELECT fieldname, value FROM moz_formhistory " +
      "ORDER BY 1.0 * (lastUsed - (SELECT lastUsed FROM moz_formhistory ORDER BY lastUsed ASC LIMIT 1)) / " +
        "((SELECT lastUsed FROM moz_formhistory ORDER BY lastUsed DESC LIMIT 1) - (SELECT lastUsed FROM moz_formhistory ORDER BY lastUsed ASC LIMIT 1)) * " +
        "timesUsed / (SELECT timesUsed FROM moz_formhistory ORDER BY timesUsed DESC LIMIT 1) DESC " +
      "LIMIT 500");
    while (query.executeStep()) {
      entries.push({
        name: query.row.fieldname,
        value: query.row.value
      });
    }
    return entries;
  },

  getEntry: function getEntry(guid) {
    let query = this.createStatement(
      "SELECT fieldname, value FROM moz_formhistory WHERE guid = :guid");
    query.params.guid = guid;
    if (!query.executeStep())
      return;

    return {
      name: query.row.fieldname,
      value: query.row.value
    };
  },

  getGUID: function getGUID(name, value) {
    
    let getQuery = this.createStatement(
      "SELECT guid FROM moz_formhistory " +
      "WHERE fieldname = :name AND value = :value");
    getQuery.params.name = name;
    getQuery.params.value = value;
    getQuery.executeStep();

    
    if (getQuery.row.guid != null)
      return getQuery.row.guid;

    
    let setQuery = this.createStatement(
      "UPDATE moz_formhistory SET guid = :guid " +
      "WHERE fieldname = :name AND value = :value");
    let guid = Utils.makeGUID();
    setQuery.params.guid = guid;
    setQuery.params.name = name;
    setQuery.params.value = value;
    setQuery.execute();

    return guid;
  },

  hasGUID: function hasGUID(guid) {
    let query = this.createStatement(
      "SELECT 1 FROM moz_formhistory WHERE guid = :guid");
    query.params.guid = guid;
    return query.executeStep();
  },

  replaceGUID: function replaceGUID(oldGUID, newGUID) {
    let query = this.createStatement(
      "UPDATE moz_formhistory SET guid = :newGUID WHERE guid = :oldGUID");
    query.params.oldGUID = oldGUID;
    query.params.newGUID = newGUID;
    query.execute();
  },

  createStatement: function createStatement(query) {
    try {
      
      return Svc.Form.DBConnection.createStatement(query);
    }
    catch(ex) {
      
      Svc.Form.DBConnection.executeSimpleSQL(
        "ALTER TABLE moz_formhistory ADD COLUMN guid TEXT");
      Svc.Form.DBConnection.executeSimpleSQL(
        "CREATE INDEX IF NOT EXISTS moz_formhistory_guid_index " +
        "ON moz_formhistory (guid)");

      
      return Svc.Form.DBConnection.createStatement(query);
    }
  }
};

function FormEngine() {
  SyncEngine.call(this, "Forms");
}
FormEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: FormStore,
  _trackerObj: FormTracker,
  _recordObj: FormRec,
  get prefName() "history",

  _findDupe: function _findDupe(item) {
    if (Svc.Form.entryExists(item.name, item.value))
      return FormWrapper.getGUID(item.name, item.value);
  }
};

function FormStore(name) {
  Store.call(this, name);
}
FormStore.prototype = {
  __proto__: Store.prototype,

  getAllIDs: function FormStore_getAllIDs() {
    let guids = {};
    for each (let {name, value} in FormWrapper.getAllEntries())
      guids[FormWrapper.getGUID(name, value)] = true;
    return guids;
  },

  changeItemID: function FormStore_changeItemID(oldID, newID) {
    FormWrapper.replaceGUID(oldID, newID);
  },

  itemExists: function FormStore_itemExists(id) {
    return FormWrapper.hasGUID(id);
  },

  createRecord: function createRecord(guid) {
    let record = new FormRec();
    let entry = FormWrapper.getEntry(guid);
    if (entry != null) {
      record.name = entry.name;
      record.value = entry.value
    }
    else
      record.deleted = true;
    return record;
  },

  create: function FormStore_create(record) {
    this._log.trace("Adding form record for " + record.name);
    Svc.Form.addEntry(record.name, record.value);
  },

  remove: function FormStore_remove(record) {
    this._log.trace("Removing form record: " + record.id);

    
    let entry = FormWrapper.getEntry(record.id);
    if (entry == null)
      return;

    Svc.Form.removeEntry(entry.name, entry.value);
  },

  update: function FormStore_update(record) {
    this._log.warn("Ignoring form record update request!");
  },

  wipe: function FormStore_wipe() {
    Svc.Form.removeAllEntries();
  }
};

function FormTracker(name) {
  Tracker.call(this, name);
  Svc.Obs.add("form-notifier", this);
  Svc.Observer.addObserver(this, "earlyformsubmit", false);
}
FormTracker.prototype = {
  __proto__: Tracker.prototype,

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIFormSubmitObserver,
    Ci.nsIObserver]),

  trackEntry: function trackEntry(name, value) {
    this.addChangedID(FormWrapper.getGUID(name, value));
    this.score += 10;
  },

  observe: function observe(subject, topic, data) {
    let name, value;

    
    let formCall = JSON.parse(data);
    let func = formCall.func;
    if ((func == "addEntry" && formCall.type == "after") ||
        (func == "removeEntry" && formCall.type == "before"))
      [name, value] = formCall.args;

    
    if (name == null || value == null)
      return;

    this._log.trace("Logging form action: " + [func, name, value]);
    this.trackEntry(name, value);
  },

  notify: function FormTracker_notify(formElement, aWindow, actionURI) {
    if (this.ignoreAll)
      return;

    this._log.trace("Form submission notification for " + actionURI.spec);

    
    
    

    
    let completeOff = function(domNode) {
      let autocomplete = domNode.getAttribute("autocomplete");
      return autocomplete && autocomplete.search(/^off$/i) == 0;
    }

    if (completeOff(formElement)) {
      this._log.trace("Form autocomplete set to off");
      return;
    }

    
    let len = formElement.length;
    let elements = formElement.elements;
    for (let i = 0; i < len; i++) {
      let el = elements.item(i);

      
      let name = el.name;
      if (name === "")
        name = el.id;

      if (!(el instanceof Ci.nsIDOMHTMLInputElement)) {
        this._log.trace(name + " is not a DOMHTMLInputElement: " + el);
        continue;
      }

      if (el.type.search(/^text$/i) != 0) {
        this._log.trace(name + "'s type is not 'text': " + el.type);
        continue;
      }

      if (completeOff(el)) {
        this._log.trace(name + "'s autocomplete set to off");
        continue;
      }

      if (el.value === "") {
        this._log.trace(name + "'s value is empty");
        continue;
      }

      if (el.value == el.defaultValue) {
        this._log.trace(name + "'s value is the default");
        continue;
      }

      if (name === "") {
        this._log.trace("Text input element has no name or id");
        continue;
      }

      
      Utils.delay(function() {
        this._log.trace("Logging form element: " + [name, el.value]);
        this.trackEntry(name, el.value);
      }, 0, this);
    }
  }
};
