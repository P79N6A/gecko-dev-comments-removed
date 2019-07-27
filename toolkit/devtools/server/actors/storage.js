



"use strict";

const {Cu, Cc, Ci} = require("chrome");
const events = require("sdk/event/core");
const protocol = require("devtools/server/protocol");
try {
  const { indexedDB } = require("sdk/indexed-db");
} catch (e) {
  
  
}
const {async} = require("devtools/async-utils");
const {Arg, Option, method, RetVal, types} = protocol;
const {LongStringActor} = require("devtools/server/actors/string");
const {DebuggerServer} = require("devtools/server/main");
const Services = require("Services");
const promise = require("promise");

loader.lazyImporter(this, "OS", "resource://gre/modules/osfile.jsm");
loader.lazyImporter(this, "Sqlite", "resource://gre/modules/Sqlite.jsm");
loader.lazyImporter(this, "LayoutHelpers",
                    "resource://gre/modules/devtools/LayoutHelpers.jsm");

let gTrackedMessageManager = new Map();



const MAX_STORE_OBJECT_COUNT = 50;


const UPDATE_INTERVAL = 500;




let illegalFileNameCharacters = [
  "[",
  
  "\\x00-\\x25",
  
  "/:*?\\\"<>|\\\\",
  "]"
].join("");
let ILLEGAL_CHAR_REGEX = new RegExp(illegalFileNameCharacters, "g");


let storageTypePool = new Map();





function getRegisteredTypes() {
  let registeredTypes = {};
  for (let store of storageTypePool.keys()) {
    registeredTypes[store] = store;
  }
  return registeredTypes;
}







function sleep(time) {
  let wait = promise.defer();
  let updateTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  updateTimer.initWithCallback({
    notify: function() {
      updateTimer.cancel();
      updateTimer = null;
      wait.resolve(null);
    }
  }, time, Ci.nsITimer.TYPE_ONE_SHOT);
  return wait.promise;
}


types.addDictType("cookieobject", {
  name: "string",
  value: "longstring",
  path: "nullable:string",
  host: "string",
  isDomain: "boolean",
  isSecure: "boolean",
  isHttpOnly: "boolean",
  creationTime: "number",
  lastAccessed: "number",
  expires: "number"
});


types.addDictType("cookiestoreobject", {
  total: "number",
  offset: "number",
  data: "array:nullable:cookieobject"
});


types.addDictType("storageobject", {
  name: "string",
  value: "longstring"
});


types.addDictType("storagestoreobject", {
  total: "number",
  offset: "number",
  data: "array:nullable:storageobject"
});




types.addDictType("idbobject", {
  name: "nullable:string",
  db: "nullable:string",
  objectStore: "nullable:string",
  origin: "nullable:string",
  version: "nullable:number",
  objectStores: "nullable:number",
  keyPath: "nullable:string",
  autoIncrement: "nullable:boolean",
  indexes: "nullable:string",
  value: "nullable:longstring"
});


types.addDictType("idbstoreobject", {
  total: "number",
  offset: "number",
  data: "array:nullable:idbobject"
});


types.addDictType("storeUpdateObject", {
  changed: "nullable:json",
  deleted: "nullable:json",
  added: "nullable:json"
});


let StorageActors = {};
























StorageActors.defaults = function(typeName, observationTopic, storeObjectType) {
  return {
    typeName: typeName,

    get conn() {
      return this.storageActor.conn;
    },

    



    get hosts() {
      let hosts = new Set();
      for (let {location} of this.storageActor.windows) {
        hosts.add(this.getHostName(location));
      }
      return hosts;
    },

    



    get windows() {
      return this.storageActor.windows;
    },

    


    getHostName: function(location) {
      return location.hostname || location.href;
    },

    initialize: function(storageActor) {
      protocol.Actor.prototype.initialize.call(this, null);

      this.storageActor = storageActor;

      this.populateStoresForHosts();
      if (observationTopic) {
        Services.obs.addObserver(this, observationTopic, false);
      }
      this.onWindowReady = this.onWindowReady.bind(this);
      this.onWindowDestroyed = this.onWindowDestroyed.bind(this);
      events.on(this.storageActor, "window-ready", this.onWindowReady);
      events.on(this.storageActor, "window-destroyed", this.onWindowDestroyed);
    },

    destroy: function() {
      this.hostVsStores = null;
      if (observationTopic) {
        Services.obs.removeObserver(this, observationTopic, false);
      }
      events.off(this.storageActor, "window-ready", this.onWindowReady);
      events.off(this.storageActor, "window-destroyed", this.onWindowDestroyed);
    },

    getNamesForHost: function(host) {
      return [...this.hostVsStores.get(host).keys()];
    },

    getValuesForHost: function(host, name) {
      if (name) {
        return [this.hostVsStores.get(host).get(name)];
      }
      return [...this.hostVsStores.get(host).values()];
    },

    getObjectsSize: function(host, names) {
      return names.length;
    },

    






    onWindowReady: async(function*(window) {
      let host = this.getHostName(window.location);
      if (!this.hostVsStores.has(host)) {
        yield this.populateStoresForHost(host, window);
        let data = {};
        data[host] = this.getNamesForHost(host);
        this.storageActor.update("added", typeName, data);
      }
    }),

    






    onWindowDestroyed: function(window) {
      if (!window.location) {
        
        return;
      }
      let host = this.getHostName(window.location);
      if (!this.hosts.has(host)) {
        this.hostVsStores.delete(host);
        let data = {};
        data[host] = [];
        this.storageActor.update("deleted", typeName, data);
      }
    },

    form: function(form, detail) {
      if (detail === "actorid") {
        return this.actorID;
      }

      let hosts = {};
      for (let host of this.hosts) {
        hosts[host] = [];
      }

      return {
        actor: this.actorID,
        hosts: hosts
      };
    },

    


    populateStoresForHosts: function() {
      this.hostVsStores = new Map();
      for (let host of this.hosts) {
        this.populateStoresForHost(host);
      }
    },

    


























    getStoreObjects: method(async(function*(host, names, options = {}) {
      let offset = options.offset || 0;
      let size = options.size || MAX_STORE_OBJECT_COUNT;
      if (size > MAX_STORE_OBJECT_COUNT) {
        size = MAX_STORE_OBJECT_COUNT;
      }
      let sortOn = options.sortOn || "name";

      let toReturn = {
        offset: offset,
        total: 0,
        data: []
      };

      if (names) {
        for (let name of names) {
          let values =
            yield this.getValuesForHost(host, name, options, this.hostVsStores);

          let {result, objectStores} = values;

          if (result && typeof result.objectsSize !== "undefined") {
            for (let {key, count} of result.objectsSize) {
              this.objectsSize[key] = count;
            }
          }

          if (result) {
            toReturn.data.push(...result.data);
          } else if (objectStores) {
            toReturn.data.push(...objectStores);
          } else {
            toReturn.data.push(...values);
          }
        }
        toReturn.total = this.getObjectsSize(host, names, options);
        if (offset > toReturn.total) {
          
          toReturn.offset = toReturn.total;
          toReturn.data = [];
        } else {
          toReturn.data = toReturn.data.sort((a, b) => {
            return a[sortOn] - b[sortOn];
          }).slice(offset, offset + size).map(a => this.toStoreObject(a));
        }
      } else {
        let obj = yield this.getValuesForHost(host, undefined, undefined,
                                              this.hostVsStores);
        if (obj.dbs) {
          obj = obj.dbs;
        }

        toReturn.total = obj.length;
        if (offset > toReturn.total) {
          
          toReturn.offset = offset = toReturn.total;
          toReturn.data = [];
        } else {
          toReturn.data = obj.sort((a, b) => {
            return a[sortOn] - b[sortOn];
          }).slice(offset, offset + size)
            .map(object => this.toStoreObject(object));
        }
      }

      return toReturn;
    }), {
      request: {
        host: Arg(0),
        names: Arg(1, "nullable:array:string"),
        options: Arg(2, "nullable:json")
      },
      response: RetVal(storeObjectType)
    })
  };
};




















StorageActors.createActor = function(options = {}, overrides = {}) {
  let actorObject = StorageActors.defaults(
    options.typeName,
    options.observationTopic || null,
    options.storeObjectType
  );
  for (let key in overrides) {
    actorObject[key] = overrides[key];
  }

  let actor = protocol.ActorClass(actorObject);
  let front = protocol.FrontClass(actor, {
    form: function(form, detail) {
      if (detail === "actorid") {
        this.actorID = form;
        return null;
      }

      this.actorID = form.actor;
      this.hosts = form.hosts;
      return null;
    }
  });
  storageTypePool.set(actorObject.typeName, actor);
};




StorageActors.createActor({
  typeName: "cookies",
  storeObjectType: "cookiestoreobject"
}, {
  initialize: function(storageActor) {
    protocol.Actor.prototype.initialize.call(this, null);

    this.storageActor = storageActor;

    this.maybeSetupChildProcess();
    this.populateStoresForHosts();
    this.addCookieObservers();

    this.onWindowReady = this.onWindowReady.bind(this);
    this.onWindowDestroyed = this.onWindowDestroyed.bind(this);
    events.on(this.storageActor, "window-ready", this.onWindowReady);
    events.on(this.storageActor, "window-destroyed", this.onWindowDestroyed);
  },

  destroy: function() {
    this.hostVsStores = null;

    this.removeCookieObservers();

    
    let oldMM = gTrackedMessageManager.get("cookies");

    if (oldMM) {
      gTrackedMessageManager.delete("cookies");
      oldMM.removeMessageListener("storage:storage-cookie-request-parent",
                                  cookieHelpers.handleChildRequest);
    }

    events.off(this.storageActor, "window-ready", this.onWindowReady);
    events.off(this.storageActor, "window-destroyed", this.onWindowDestroyed);
  },

  



  getMatchingHosts: function(cookies) {
    if (!cookies.length) {
      cookies = [cookies];
    }
    let hosts = new Set();
    for (let host of this.hosts) {
      for (let cookie of cookies) {
        if (this.isCookieAtHost(cookie, host)) {
          hosts.add(host);
        }
      }
    }
    return [...hosts];
  },

  



  isCookieAtHost: function(cookie, host) {
    if (cookie.host == null) {
      return host == null;
    }
    if (cookie.host.startsWith(".")) {
      return host.endsWith(cookie.host);
    }
    return cookie.host == host;
  },

  toStoreObject: function(cookie) {
    if (!cookie) {
      return null;
    }

    return {
      name: cookie.name,
      path: cookie.path || "",
      host: cookie.host || "",

      
      expires: (cookie.expires || 0) * 1000,

      
      creationTime: cookie.creationTime / 1000,

      
      lastAccessed: cookie.lastAccessed / 1000,
      value: new LongStringActor(this.conn, cookie.value || ""),
      isDomain: cookie.isDomain,
      isSecure: cookie.isSecure,
      isHttpOnly: cookie.isHttpOnly
    };
  },

  populateStoresForHost: function(host) {
    this.hostVsStores.set(host, new Map());

    let cookies = this.getCookiesFromHost(host);

    for (let cookie of cookies) {
      if (this.isCookieAtHost(cookie, host)) {
        this.hostVsStores.get(host).set(cookie.name, cookie);
      }
    }
  },

  












  onCookieChanged: function(subject, topic, action) {
    if (topic != "cookie-changed" || !this.storageActor.windows) {
      return null;
    }

    let hosts = this.getMatchingHosts(subject);
    let data = {};

    switch (action) {
      case "added":
      case "changed":
        if (hosts.length) {
          for (let host of hosts) {
            this.hostVsStores.get(host).set(subject.name, subject);
            data[host] = [subject.name];
          }
          this.storageActor.update(action, "cookies", data);
        }
        break;

      case "deleted":
        if (hosts.length) {
          for (let host of hosts) {
            this.hostVsStores.get(host).delete(subject.name);
            data[host] = [subject.name];
          }
          this.storageActor.update("deleted", "cookies", data);
        }
        break;

      case "batch-deleted":
        if (hosts.length) {
          for (let host of hosts) {
            let stores = [];
            for (let cookie of subject) {
              this.hostVsStores.get(host).delete(cookie.name);
              stores.push(cookie.name);
            }
            data[host] = stores;
          }
          this.storageActor.update("deleted", "cookies", data);
        }
        break;

      case "cleared":
        this.storageActor.update("cleared", "cookies", hosts);
        break;

      case "reload":
        this.storageActor.update("reloaded", "cookies", hosts);
        break;
    }
    return null;
  },

  maybeSetupChildProcess: function() {
    cookieHelpers.onCookieChanged = this.onCookieChanged.bind(this);

    if (!DebuggerServer.isInChildProcess) {
      this.getCookiesFromHost = cookieHelpers.getCookiesFromHost;
      this.addCookieObservers = cookieHelpers.addCookieObservers;
      this.removeCookieObservers = cookieHelpers.removeCookieObservers;
      return;
    }

    const { sendSyncMessage, addMessageListener } =
      DebuggerServer.parentMessageManager;

    DebuggerServer.setupInParent({
      module: "devtools/server/actors/storage",
      setupParent: "setupParentProcessForCookies"
    });

    this.getCookiesFromHost =
      callParentProcess.bind(null, "getCookiesFromHost");
    this.addCookieObservers =
      callParentProcess.bind(null, "addCookieObservers");
    this.removeCookieObservers =
      callParentProcess.bind(null, "removeCookieObservers");

    addMessageListener("storage:storage-cookie-request-child",
                       cookieHelpers.handleParentRequest);

    function callParentProcess(methodName, ...args) {
      let reply = sendSyncMessage("storage:storage-cookie-request-parent", {
        method: methodName,
        args: args
      });

      if (reply.length === 0) {
        console.error("ERR_DIRECTOR_CHILD_NO_REPLY from " + methodName);
        throw Error("ERR_DIRECTOR_CHILD_NO_REPLY from " + methodName);
      } else if (reply.length > 1) {
        console.error("ERR_DIRECTOR_CHILD_MULTIPLE_REPLIES from " + methodName);
        throw Error("ERR_DIRECTOR_CHILD_MULTIPLE_REPLIES from " + methodName);
      }

      let result = reply[0];

      if (methodName === "getCookiesFromHost") {
        return JSON.parse(result);
      }

      return result;
    }
  },
});

let cookieHelpers = {
  getCookiesFromHost: function(host) {
    let cookies = Services.cookies.getCookiesFromHost(host);
    let store = [];

    while (cookies.hasMoreElements()) {
      let cookie = cookies.getNext().QueryInterface(Ci.nsICookie2);
      store.push(cookie);
    }

    return store;
  },

  addCookieObservers: function() {
    Services.obs.addObserver(cookieHelpers, "cookie-changed", false);
    return null;
  },

  removeCookieObservers: function() {
    Services.obs.removeObserver(cookieHelpers, "cookie-changed", false);
    return null;
  },

  observe: function(subject, topic, data) {
    switch (topic) {
      case "cookie-changed":
        let cookie = subject.QueryInterface(Ci.nsICookie2);
        cookieHelpers.onCookieChanged(cookie, topic, data);
      break;
    }
  },

  handleParentRequest: function(msg) {
    switch (msg.json.method) {
      case "onCookieChanged":
        let [cookie, topic, data] = msg.data.args;
        cookie = JSON.parse(cookie);
        cookieHelpers.onCookieChanged(cookie, topic, data);
      break;
    }
  },

  handleChildRequest: function(msg) {
    switch (msg.json.method) {
      case "getCookiesFromHost":
        let host = msg.data.args[0];
        let cookies = cookieHelpers.getCookiesFromHost(host);
        return JSON.stringify(cookies);
      case "addCookieObservers":
        return cookieHelpers.addCookieObservers();
        break;
      case "removeCookieObservers":
        return cookieHelpers.removeCookieObservers();
        return null;
      default:
        console.error("ERR_DIRECTOR_PARENT_UNKNOWN_METHOD", msg.json.method);
        throw new Error("ERR_DIRECTOR_PARENT_UNKNOWN_METHOD");
    }
  },
};





exports.setupParentProcessForCookies = function({mm, childID}) {
  cookieHelpers.onCookieChanged =
    callChildProcess.bind(null, "onCookieChanged");

  
  mm.addMessageListener("storage:storage-cookie-request-parent",
                        cookieHelpers.handleChildRequest);

  DebuggerServer.once("disconnected-from-child:" + childID,
                      handleMessageManagerDisconnected);

  gTrackedMessageManager.set("cookies", mm);

  function handleMessageManagerDisconnected(evt, { mm: disconnected_mm }) {
    
    if (disconnected_mm !== mm || !gTrackedMessageManager.has(mm)) {
      return;
    }

    gTrackedMessageManager.delete("cookies");

    
    
    mm.removeMessageListener("storage:storage-cookie-request-parent",
                             cookieHelpers.handleChildRequest);
  }

  function callChildProcess(methodName, ...args) {
    if (methodName === "onCookieChanged") {
      args[0] = JSON.stringify(args[0]);
    }
    let reply = mm.sendAsyncMessage("storage:storage-cookie-request-child", {
      method: methodName,
      args: args
    });
  }
};







function getObjectForLocalOrSessionStorage(type) {
  return {
    getNamesForHost: function(host) {
      let storage = this.hostVsStores.get(host);
      return Object.keys(storage);
    },

    getValuesForHost: function(host, name) {
      let storage = this.hostVsStores.get(host);
      if (name) {
        return [{name: name, value: storage.getItem(name)}];
      }
      return Object.keys(storage).map(key => {
        return {
          name: key,
          value: storage.getItem(key)
        };
      });
    },

    getHostName: function(location) {
      if (!location.host) {
        return location.href;
      }
      return location.protocol + "//" + location.host;
    },

    populateStoresForHost: function(host, window) {
      try {
        this.hostVsStores.set(host, window[type]);
      } catch(ex) {
        
      }
      return null;
    },

    populateStoresForHosts: function() {
      this.hostVsStores = new Map();
      try {
        for (let window of this.windows) {
          this.hostVsStores.set(this.getHostName(window.location),
                                window[type]);
        }
      } catch(ex) {
        
      }
      return null;
    },

    observe: function(subject, topic, data) {
      if (topic != "dom-storage2-changed" || data != type) {
        return null;
      }

      let host = this.getSchemaAndHost(subject.url);

      if (!this.hostVsStores.has(host)) {
        return null;
      }

      let action = "changed";
      if (subject.key == null) {
        return this.storageActor.update("cleared", type, [host]);
      } else if (subject.oldValue == null) {
        action = "added";
      } else if (subject.newValue == null) {
        action = "deleted";
      }
      let updateData = {};
      updateData[host] = [subject.key];
      return this.storageActor.update(action, type, updateData);
    },

    


    getSchemaAndHost: function(url) {
      let uri = Services.io.newURI(url, null, null);
      if (!uri.host) {
        return uri.spec;
      }
      return uri.scheme + "://" + uri.hostPort;
    },

    toStoreObject: function(item) {
      if (!item) {
        return null;
      }

      return {
        name: item.name,
        value: new LongStringActor(this.conn, item.value || "")
      };
    },
  };
}




StorageActors.createActor({
  typeName: "localStorage",
  observationTopic: "dom-storage2-changed",
  storeObjectType: "storagestoreobject"
}, getObjectForLocalOrSessionStorage("localStorage"));




StorageActors.createActor({
  typeName: "sessionStorage",
  observationTopic: "dom-storage2-changed",
  storeObjectType: "storagestoreobject"
}, getObjectForLocalOrSessionStorage("sessionStorage"));













function IndexMetadata(index) {
  this._name = index.name;
  this._keyPath = index.keyPath;
  this._unique = index.unique;
  this._multiEntry = index.multiEntry;
}
IndexMetadata.prototype = {
  toObject: function() {
    return {
      name: this._name,
      keyPath: this._keyPath,
      unique: this._unique,
      multiEntry: this._multiEntry
    };
  }
};







function ObjectStoreMetadata(objectStore) {
  this._name = objectStore.name;
  this._keyPath = objectStore.keyPath;
  this._autoIncrement = objectStore.autoIncrement;
  this._indexes = [];

  for (let i = 0; i < objectStore.indexNames.length; i++) {
    let index = objectStore.index(objectStore.indexNames[i]);

    let newIndex = {
      keypath: index.keyPath,
      multiEntry: index.multiEntry,
      name: index.name,
      objectStore: {
        autoIncrement: index.objectStore.autoIncrement,
        indexNames: [...index.objectStore.indexNames],
        keyPath: index.objectStore.keyPath,
        name: index.objectStore.name,
      }
    };

    this._indexes.push([newIndex, new IndexMetadata(index)]);
  }
}
ObjectStoreMetadata.prototype = {
  toObject: function() {
    return {
      name: this._name,
      keyPath: this._keyPath,
      autoIncrement: this._autoIncrement,
      indexes: JSON.stringify(
        [...this._indexes.values()].map(index => index.toObject())
      )
    };
  }
};









function DatabaseMetadata(origin, db) {
  this._origin = origin;
  this._name = db.name;
  this._version = db.version;
  this._objectStores = [];

  if (db.objectStoreNames.length) {
    let transaction = db.transaction(db.objectStoreNames, "readonly");

    for (let i = 0; i < transaction.objectStoreNames.length; i++) {
      let objectStore =
        transaction.objectStore(transaction.objectStoreNames[i]);
      this._objectStores.push([transaction.objectStoreNames[i],
                              new ObjectStoreMetadata(objectStore)]);
    }
  }
}
DatabaseMetadata.prototype = {
  get objectStores() {
    return this._objectStores;
  },

  toObject: function() {
    return {
      name: this._name,
      origin: this._origin,
      version: this._version,
      objectStores: this._objectStores.size
    };
  }
};

StorageActors.createActor({
  typeName: "indexedDB",
  storeObjectType: "idbstoreobject"
}, {
  initialize: function(storageActor) {
    protocol.Actor.prototype.initialize.call(this, null);
    this.maybeSetupChildProcess();

    this.objectsSize = {};
    this.storageActor = storageActor;
    this.onWindowReady = this.onWindowReady.bind(this);
    this.onWindowDestroyed = this.onWindowDestroyed.bind(this);

    events.on(this.storageActor, "window-ready", this.onWindowReady);
    events.on(this.storageActor, "window-destroyed", this.onWindowDestroyed);
  },

  destroy: function() {
    this.hostVsStores = null;
    this.objectsSize = null;

    
    let oldMM = gTrackedMessageManager.get("indexedDB");

    if (oldMM) {
      gTrackedMessageManager.delete("indexedDB");
      oldMM.removeMessageListener("storage:storage-cookie-request-parent",
                                  indexedDBHelpers.handleChildRequest);
    }

    events.off(this.storageActor, "window-ready", this.onWindowReady);
    events.off(this.storageActor, "window-destroyed", this.onWindowDestroyed);
  },

  getHostName: function(location) {
    if (!location.host) {
      return location.href;
    }
    return location.protocol + "//" + location.host;
  },

  




  populateStoresForHosts: function() {},

  getNamesForHost: function(host) {
    let names = [];

    for (let [dbName, metaData] of this.hostVsStores.get(host)) {
      for (let objectStore of metaData.objectStores.keys()) {
        names.push(JSON.stringify([dbName, objectStore]));
      }
    }
    return names;
  },

  
















  getObjectsSize: function(host, names, options) {
    
    
    let name = names[0];
    let parsedName = JSON.parse(name);

    if (parsedName.length == 3) {
      
      
      return names.length;
    } else if (parsedName.length == 2) {
      
      let index = options.index;
      let [db, objectStore] = parsedName;
      if (this.objectsSize[host + db + objectStore + index]) {
        return this.objectsSize[host + db + objectStore + index];
      }
    } else if (parsedName.length == 1) {
      
      
      if (this.hostVsStores.has(host) &&
          this.hostVsStores.get(host).has(parsedName[0])) {
        return this.hostVsStores.get(host).get(parsedName[0]).objectStores.size;
      }
    } else if (!parsedName || !parsedName.length) {
      
      if (this.hostVsStores.has(host)) {
        return this.hostVsStores.get(host).size;
      }
    }
    return 0;
  },

  





  preListStores: async(function*() {
    this.hostVsStores = new Map();

    for (let host of this.hosts) {
      yield this.populateStoresForHost(host);
    }
  }),

  populateStoresForHost: async(function*(host) {
    let storeMap = new Map();
    let {names} = yield this.getDBNamesForHost(host);

    for (let name of names) {
      let metadata = yield this.getDBMetaData(host, name);

      indexedDBHelpers.patchMetadataMapsAndProtos(metadata);
      storeMap.set(name, metadata);
    }

    this.hostVsStores.set(host, storeMap);
  }),

  


  toStoreObject: function(item) {
    if (!item) {
      return null;
    }

    if (item.indexes) {
      
      return {
        objectStore: item.name,
        keyPath: item.keyPath,
        autoIncrement: item.autoIncrement,
        indexes: item.indexes
      };
    }
    if (item.objectStores) {
      
      return {
        db: item.name,
        origin: item.origin,
        version: item.version,
        objectStores: item.objectStores
      };
    }
    
    return {
      name: item.name,
      value: new LongStringActor(this.conn, JSON.stringify(item.value))
    };
  },

  form: function(form, detail) {
    if (detail === "actorid") {
      return this.actorID;
    }

    let hosts = {};
    for (let host of this.hosts) {
      hosts[host] = this.getNamesForHost(host);
    }

    return {
      actor: this.actorID,
      hosts: hosts
    };
  },

  maybeSetupChildProcess: function() {
    if (!DebuggerServer.isInChildProcess) {
      this.backToChild = function(...args) {
        return args[1];
      };
      this.getDBMetaData = indexedDBHelpers.getDBMetaData;
      this.openWithOrigin = indexedDBHelpers.openWithOrigin;
      this.getDBNamesForHost = indexedDBHelpers.getDBNamesForHost;
      this.getSanitizedHost = indexedDBHelpers.getSanitizedHost;
      this.getNameFromDatabaseFile = indexedDBHelpers.getNameFromDatabaseFile;
      this.getValuesForHost = indexedDBHelpers.getValuesForHost;
      this.getObjectStoreData = indexedDBHelpers.getObjectStoreData;
      this.patchMetadataMapsAndProtos =
        indexedDBHelpers.patchMetadataMapsAndProtos;
      return;
    }

    const { sendSyncMessage, addMessageListener } =
      DebuggerServer.parentMessageManager;

    DebuggerServer.setupInParent({
      module: "devtools/server/actors/storage",
      setupParent: "setupParentProcessForIndexedDB"
    });

    this.getDBMetaData =
      callParentProcessAsync.bind(null, "getDBMetaData");
    this.getDBNamesForHost =
      callParentProcessAsync.bind(null, "getDBNamesForHost");
    this.getValuesForHost =
      callParentProcessAsync.bind(null, "getValuesForHost");

    addMessageListener("storage:storage-indexedDB-request-child", msg => {
      switch (msg.json.method) {
        case "backToChild":
          let func = msg.json.args.shift();
          let deferred = unresolvedPromises.get(func);

          if (deferred) {
            unresolvedPromises.delete(func);
            deferred.resolve(msg.json.args[0]);
          }
        break;
      }
    });

    let unresolvedPromises = new Map();
    function callParentProcessAsync(methodName, ...args) {
      let deferred = promise.defer();

      unresolvedPromises.set(methodName, deferred);

      let reply = sendSyncMessage("storage:storage-indexedDB-request-parent", {
        method: methodName,
        args: args
      });

      return deferred.promise;
    }
  },
});

let indexedDBHelpers = {
  backToChild: function(...args) {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"]
               .getService(Ci.nsIMessageListenerManager);

    mm.broadcastAsyncMessage("storage:storage-indexedDB-request-child", {
      method: "backToChild",
      args: args
    });
  },

  




  getDBMetaData: async(function*(host, name) {
    let request = this.openWithOrigin(host, name);
    let success = promise.defer();

    request.onsuccess = event => {
      let db = event.target.result;

      let dbData = new DatabaseMetadata(host, db);
      db.close();

      this.backToChild("getDBMetaData", dbData);
      success.resolve(dbData);
    };
    request.onerror = () => {
      console.error("Error opening indexeddb database " + name + " for host " +
                    host);
      this.backToChild("getDBMetaData", null);
      success.resolve(null);
    };
    return success.promise;
  }),

  


  openWithOrigin: function(host, name) {
    let principal;

    if (/^(about:|chrome:)/.test(host)) {
      principal = Services.scriptSecurityManager.getSystemPrincipal();
    } else {
      let uri = Services.io.newURI(host, null, null);
      principal = Services.scriptSecurityManager.getCodebasePrincipal(uri);
    }

    return require("indexedDB").openForPrincipal(principal, name);
  },

    


  getDBNamesForHost: async(function*(host) {
    let sanitizedHost = this.getSanitizedHost(host);
    let directory = OS.Path.join(OS.Constants.Path.profileDir, "storage",
                                 "default", sanitizedHost, "idb");

    let exists = yield OS.File.exists(directory);
    if (!exists && host.startsWith("about:")) {
      
      sanitizedHost = this.getSanitizedHost("moz-safe-" + host);
      directory = OS.Path.join(OS.Constants.Path.profileDir, "storage",
                               "permanent", sanitizedHost, "idb");
      exists = yield OS.File.exists(directory);
    }
    if (!exists) {
      return this.backToChild("getDBNamesForHost", {names: []});
    }

    let names = [];
    let dirIterator = new OS.File.DirectoryIterator(directory);
    try {
      yield dirIterator.forEach(file => {
        
        if (file.isDir) {
          return null;
        }

        
        if (!file.name.endsWith(".sqlite")) {
          return null;
        }

        return this.getNameFromDatabaseFile(file.path).then(name => {
          if (name) {
            names.push(name);
          }
          return null;
        });
      });
    } finally {
      dirIterator.close();
    }
    return this.backToChild("getDBNamesForHost", {names: names});
  }),

  



  getSanitizedHost: function(host) {
    return host.replace(ILLEGAL_CHAR_REGEX, "+");
  },

  



  getNameFromDatabaseFile: async(function*(path) {
    let connection = null;
    let retryCount = 0;

    
    
    
    while (!connection && retryCount++ < 25) {
      try {
        connection = yield Sqlite.openConnection({ path: path });
      } catch (ex) {
        
        yield sleep(100);
      }
    }

    if (!connection) {
      return null;
    }

    let rows = yield connection.execute("SELECT name FROM database");
    if (rows.length != 1) {
      return null;
    }

    let name = rows[0].getResultByName("name");

    yield connection.close();

    return name;
  }),

  getValuesForHost:
  async(function*(host, name = "null", options, hostVsStores) {
    name = JSON.parse(name);
    if (!name || !name.length) {
      
      
      let dbs = [];
      if (hostVsStores.has(host)) {
        for (let [, db] of hostVsStores.get(host)) {
          indexedDBHelpers.patchMetadataMapsAndProtos(db);
          dbs.push(db.toObject());
        }
      }
      return this.backToChild("getValuesForHost", {dbs: dbs});
    }

    let [db2, objectStore, id] = name;
    if (!objectStore) {
      
      
      let objectStores = [];
      if (hostVsStores.has(host) && hostVsStores.get(host).has(db2)) {
        let db = hostVsStores.get(host).get(db2);

        indexedDBHelpers.patchMetadataMapsAndProtos(db);

        let objectStores2 = db.objectStores;

        for (let objectStore2 of objectStores2) {
          objectStores.push(objectStore2[1].toObject());
        }
      }
      return this.backToChild("getValuesForHost", {objectStores: objectStores});
    }
    
    let result = yield this.getObjectStoreData(host, db2, objectStore, id,
                                               options.index, options.size);
    return this.backToChild("getValuesForHost", {result: result});
  }),

  




















  getObjectStoreData:
  function(host, dbName, objectStore, id, index, offset, size) {
    let request = this.openWithOrigin(host, dbName);
    let success = promise.defer();
    let data = [];
    let db;

    if (!size || size > MAX_STORE_OBJECT_COUNT) {
      size = MAX_STORE_OBJECT_COUNT;
    }

    request.onsuccess = event => {
      db = event.target.result;

      let transaction = db.transaction(objectStore, "readonly");
      let source = transaction.objectStore(objectStore);
      if (index && index != "name") {
        source = source.index(index);
      }

      source.count().onsuccess = event2 => {
        let objectsSize = [];
        let count = event2.target.result;
        objectsSize.push({
          key: host + dbName + objectStore + index,
          count: count
        });

        if (!offset) {
          offset = 0;
        } else if (offset > count) {
          db.close();
          success.resolve([]);
          return;
        }

        if (id) {
          source.get(id).onsuccess = event3 => {
            db.close();
            success.resolve([{name: id, value: event3.target.result}]);
          };
        } else {
          source.openCursor().onsuccess = event4 => {
            let cursor = event4.target.result;

            if (!cursor || data.length >= size) {
              db.close();
              success.resolve({
                data: data,
                objectsSize: objectsSize
              });
              return;
            }
            if (offset-- <= 0) {
              data.push({name: cursor.key, value: cursor.value});
            }
            cursor.continue();
          };
        }
      };
    };
    request.onerror = () => {
      db.close();
      success.resolve([]);
    };
    return success.promise;
  },

  patchMetadataMapsAndProtos: function(metadata) {
    for (let [, store] of metadata._objectStores) {
      store.__proto__ = ObjectStoreMetadata.prototype;

      if (typeof store._indexes.length !== "undefined") {
        store._indexes = new Map(store._indexes);
      }

      for (let [, value] of store._indexes) {
        value.__proto__ = IndexMetadata.prototype;
      }
    }

    metadata._objectStores = new Map(metadata._objectStores);
    metadata.__proto__ = DatabaseMetadata.prototype;
  },

  handleChildRequest: function(msg) {
    let host;
    let name;
    let args = msg.data.args;

    switch (msg.json.method) {
      case "getDBMetaData":
        host = args[0];
        name = args[1];
        return indexedDBHelpers.getDBMetaData(host, name);
      case "getDBNamesForHost":
        host = args[0];
        return indexedDBHelpers.getDBNamesForHost(host);
      case "getValuesForHost":
        host = args[0];
        name = args[1];
        let options = args[2];
        let hostVsStores = args[3];
        return indexedDBHelpers.getValuesForHost(host, name, options,
                                                 hostVsStores);
      default:
        console.error("ERR_DIRECTOR_PARENT_UNKNOWN_METHOD", msg.json.method);
        throw new Error("ERR_DIRECTOR_PARENT_UNKNOWN_METHOD");
    }
  }
};





exports.setupParentProcessForIndexedDB = function({mm, childID}) {
  
  mm.addMessageListener("storage:storage-indexedDB-request-parent",
                        indexedDBHelpers.handleChildRequest);

  DebuggerServer.once("disconnected-from-child:" + childID,
                      handleMessageManagerDisconnected);

  gTrackedMessageManager.set("indexedDB", mm);

  function handleMessageManagerDisconnected(evt, { mm: disconnected_mm }) {
    
    if (disconnected_mm !== mm || !gTrackedMessageManager.has(mm)) {
      return;
    }

    gTrackedMessageManager.delete("indexedDB");

    
    
    mm.removeMessageListener("storage:storage-indexedDB-request-parent",
                             indexedDBHelpers.handleChildRequest);
  }
};




let StorageActor = exports.StorageActor = protocol.ActorClass({
  typeName: "storage",

  get window() {
    return this.parentActor.window;
  },

  get document() {
    return this.parentActor.window.document;
  },

  get windows() {
    return this.childWindowPool;
  },

  







  events: {
    "stores-update": {
      type: "storesUpdate",
      data: Arg(0, "storeUpdateObject")
    },
    "stores-cleared": {
      type: "storesCleared",
      data: Arg(0, "json")
    },
    "stores-reloaded": {
      type: "storesRelaoded",
      data: Arg(0, "json")
    }
  },

  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, null);

    this.conn = conn;
    this.parentActor = tabActor;

    this.childActorPool = new Map();
    this.childWindowPool = new Set();

    
    this.fetchChildWindows(this.parentActor.docShell);

    
    for (let [store, actor] of storageTypePool) {
      this.childActorPool.set(store, new actor(this));
    }

    
    
    Services.obs.addObserver(this, "content-document-global-created", false);
    Services.obs.addObserver(this, "inner-window-destroyed", false);
    this.onPageChange = this.onPageChange.bind(this);

    let handler = tabActor.chromeEventHandler;
    handler.addEventListener("pageshow", this.onPageChange, true);
    handler.addEventListener("pagehide", this.onPageChange, true);

    this.destroyed = false;
    this.boundUpdate = {};
    
    
    this.updateTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this.updateTimer.initWithCallback(this, UPDATE_INTERVAL,
      Ci.nsITimer.TYPE_REPEATING_PRECISE_CAN_SKIP);

    
    
    this.layoutHelper = new LayoutHelpers(this.window);
  },

  destroy: function() {
    this.updateTimer.cancel();
    this.updateTimer = null;
    this.layoutHelper = null;
    
    Services.obs.removeObserver(this, "content-document-global-created", false);
    Services.obs.removeObserver(this, "inner-window-destroyed", false);
    this.destroyed = true;
    if (this.parentActor.browser) {
      this.parentActor.browser.removeEventListener(
        "pageshow", this.onPageChange, true);
      this.parentActor.browser.removeEventListener(
        "pagehide", this.onPageChange, true);
    }
    
    for (let actor of this.childActorPool.values()) {
      actor.destroy();
    }
    this.childActorPool.clear();
    this.childWindowPool.clear();
    this.childWindowPool = this.childActorPool = null;
  },

  





  fetchChildWindows: function(item) {
    let docShell = item.QueryInterface(Ci.nsIDocShell)
                       .QueryInterface(Ci.nsIDocShellTreeItem);
    if (!docShell.contentViewer) {
      return null;
    }
    let window = docShell.contentViewer.DOMDocument.defaultView;
    if (window.location.href == "about:blank") {
      
      
      return null;
    }
    this.childWindowPool.add(window);
    for (let i = 0; i < docShell.childCount; i++) {
      let child = docShell.getChildAt(i);
      this.fetchChildWindows(child);
    }
    return null;
  },

  isIncludedInTopLevelWindow: function(window) {
    return this.layoutHelper.isIncludedInTopLevelWindow(window);
  },

  getWindowFromInnerWindowID: function(innerID) {
    innerID = innerID.QueryInterface(Ci.nsISupportsPRUint64).data;
    for (let win of this.childWindowPool.values()) {
      let id = win.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindowUtils)
                   .currentInnerWindowID;
      if (id == innerID) {
        return win;
      }
    }
    return null;
  },

  



  observe: function(subject, topic) {
    if (subject.location &&
        (!subject.location.href || subject.location.href == "about:blank")) {
      return null;
    }

    if (topic == "content-document-global-created" &&
        this.isIncludedInTopLevelWindow(subject)) {
      this.childWindowPool.add(subject);
      events.emit(this, "window-ready", subject);
    } else if (topic == "inner-window-destroyed") {
      let window = this.getWindowFromInnerWindowID(subject);
      if (window) {
        this.childWindowPool.delete(window);
        events.emit(this, "window-destroyed", window);
      }
    }
    return null;
  },

  











  onPageChange: function({target, type, persisted}) {
    if (this.destroyed) {
      return;
    }

    let window = target.defaultView;

    if (type == "pagehide" && this.childWindowPool.delete(window)) {
      events.emit(this, "window-destroyed", window);
    } else if (type == "pageshow" && persisted && window.location.href &&
               window.location.href != "about:blank" &&
               this.isIncludedInTopLevelWindow(window)) {
      this.childWindowPool.add(window);
      events.emit(this, "window-ready", window);
    }
  },

  








  listStores: method(async(function*() {
    let toReturn = {};

    for (let [name, value] of this.childActorPool) {
      if (value.preListStores) {
        yield value.preListStores();
      }
      toReturn[name] = value;
    }

    return toReturn;
  }), {
    response: RetVal(types.addDictType("storelist", getRegisteredTypes()))
  }),

  


  notify: function() {
    if (!this.updatePending || this.updatingUpdateObject) {
      return null;
    }

    events.emit(this, "stores-update", this.boundUpdate);
    this.boundUpdate = {};
    this.updatePending = false;
    return null;
  },

  





















  update: function(action, storeType, data) {
    if (action == "cleared" || action == "reloaded") {
      let toSend = {};
      toSend[storeType] = data;
      events.emit(this, "stores-" + action, toSend);
      return null;
    }

    this.updatingUpdateObject = true;
    if (!this.boundUpdate[action]) {
      this.boundUpdate[action] = {};
    }
    if (!this.boundUpdate[action][storeType]) {
      this.boundUpdate[action][storeType] = {};
    }
    this.updatePending = true;
    for (let host in data) {
      if (!this.boundUpdate[action][storeType][host] || action == "deleted") {
        this.boundUpdate[action][storeType][host] = data[host];
      } else {
        this.boundUpdate[action][storeType][host] =
        this.boundUpdate[action][storeType][host].concat(data[host]);
      }
    }
    if (action == "added") {
      
      
      this.removeNamesFromUpdateList("deleted", storeType, data);
      this.removeNamesFromUpdateList("changed", storeType, data);
    } else if (action == "changed" && this.boundUpdate.added &&
             this.boundUpdate.added[storeType]) {
      
      
      this.removeNamesFromUpdateList("changed", storeType,
                                     this.boundUpdate.added[storeType]);
    } else if (action == "deleted") {
      
      
      this.removeNamesFromUpdateList("added", storeType, data);
      this.removeNamesFromUpdateList("changed", storeType, data);
      for (let host in data) {
        if (data[host].length == 0 && this.boundUpdate.added &&
            this.boundUpdate.added[storeType] &&
            this.boundUpdate.added[storeType][host]) {
          delete this.boundUpdate.added[storeType][host];
        }
        if (data[host].length == 0 && this.boundUpdate.changed &&
            this.boundUpdate.changed[storeType] &&
            this.boundUpdate.changed[storeType][host]) {
          delete this.boundUpdate.changed[storeType][host];
        }
      }
    }
    this.updatingUpdateObject = false;
    return null;
  },

  
















  removeNamesFromUpdateList: function(action, storeType, data) {
    for (let host in data) {
      if (this.boundUpdate[action] && this.boundUpdate[action][storeType] &&
          this.boundUpdate[action][storeType][host]) {
        for (let name in data[host]) {
          let index = this.boundUpdate[action][storeType][host].indexOf(name);
          if (index > -1) {
            this.boundUpdate[action][storeType][host].splice(index, 1);
          }
        }
        if (!this.boundUpdate[action][storeType][host].length) {
          delete this.boundUpdate[action][storeType][host];
        }
      }
    }
    return null;
  }
});




let StorageFront = exports.StorageFront = protocol.FrontClass(StorageActor, {
  initialize: function(client, tabForm) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = tabForm.storageActor;
    this.manage(this);
  }
});
