



const {Cc, Ci, Cu, CC} = require("chrome");
const protocol = require("devtools/server/protocol");
const {Arg, method, RetVal} = protocol;
const promise = require("sdk/core/promise");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/devtools/dbg-server.jsm');

exports.register = function(handle) {
  handle.addGlobalActor(PreferenceActor, "preferenceActor");
};

exports.unregister = function(handle) {
};

let PreferenceActor = protocol.ActorClass({
  typeName: "preference",

  _arePrefsAccessible: function() {
    
    
    
    return !Services.prefs.getBoolPref("devtools.debugger.forbid-certified-apps");
  },
  arePrefsAccessible: method(function() {
    return this._arePrefsAccessible();
  }, {
    request: {},
    response: { value: RetVal("boolean") },
  }),

  getBoolPref: method(function(name) {
    if (!this._arePrefsAccessible()) {
      throw new Error("Operation not permitted");
    }
    return Services.prefs.getBoolPref(name);
  }, {
    request: { value: Arg(0) },
    response: { value: RetVal("boolean") }
  }),

  getCharPref: method(function(name) {
    if (!this._arePrefsAccessible()) {
      throw new Error("Operation not permitted");
    }
    return Services.prefs.getCharPref(name);
  }, {
    request: { value: Arg(0) },
    response: { value: RetVal("string") }
  }),

  getIntPref: method(function(name) {
    if (!this._arePrefsAccessible()) {
      throw new Error("Operation not permitted");
    }
    return Services.prefs.getIntPref(name);
  }, {
    request: { value: Arg(0) },
    response: { value: RetVal("number") }
  }),

  getAllPrefs: method(function() {
    if (!this._arePrefsAccessible()) {
      throw new Error("Operation not permitted");
    }
    let prefs = {};
    Services.prefs.getChildList("").forEach(function(name, index) {
      
      try {
        let value;
        switch (Services.prefs.getPrefType(name)) {
          case Ci.nsIPrefBranch.PREF_STRING:
            value = Services.prefs.getCharPref(name);
            break;
          case Ci.nsIPrefBranch.PREF_INT:
            value = Services.prefs.getIntPref(name);
            break;
          case Ci.nsIPrefBranch.PREF_BOOL:
            value = Services.prefs.getBoolPref(name);
            break;
          default:
        }
        prefs[name] = {
          value: value,
          hasUserValue: Services.prefs.prefHasUserValue(name)
        };
      } catch (e) {
        
      }
    });
    return prefs;
  }, {
    request: {},
    response: { value: RetVal("json") }
  }),

  setBoolPref: method(function(name, value) {
    if (!this._arePrefsAccessible()) {
      throw new Error("Operation not permitted");
    }
    Services.prefs.setBoolPref(name, value);
    Services.prefs.savePrefFile(null);
  }, {
    request: { name: Arg(0), value: Arg(1) },
    response: {}
  }),

  setCharPref: method(function(name, value) {
    if (!this._arePrefsAccessible()) {
      throw new Error("Operation not permitted");
    }
    Services.prefs.setCharPref(name, value);
    Services.prefs.savePrefFile(null);
  }, {
    request: { name: Arg(0), value: Arg(1) },
    response: {}
  }),

  setIntPref: method(function(name, value) {
    if (!this._arePrefsAccessible()) {
      throw new Error("Operation not permitted");
    }
    Services.prefs.setIntPref(name, value);
    Services.prefs.savePrefFile(null);
  }, {
    request: { name: Arg(0), value: Arg(1) },
    response: {}
  }),

  clearUserPref: method(function(name) {
    if (!this._arePrefsAccessible()) {
      throw new Error("Operation not permitted");
    }
    Services.prefs.clearUserPref(name);
    Services.prefs.savePrefFile(null);
  }, {
    request: { name: Arg(0) },
    response: {}
  }),
});

let PreferenceFront = protocol.FrontClass(PreferenceActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = form.preferenceActor;
    client.addActorPool(this);
    this.manage(this);
  },
});

const _knownPreferenceFronts = new WeakMap();

exports.getPreferenceFront = function(client, form) {
  if (_knownPreferenceFronts.has(client))
    return _knownPreferenceFronts.get(client);

  let front = new PreferenceFront(client, form);
  _knownPreferenceFronts.set(client, front);
  return front;
};
