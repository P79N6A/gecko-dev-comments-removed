const {Cc,Ci,Cu} = require("chrome");
const ObservableObject = require("devtools/shared/observable-object");
const promise = require("sdk/core/promise");

const {EventEmitter} = Cu.import("resource:///modules/devtools/shared/event-emitter.js");
const {generateUUID} = Cc['@mozilla.org/uuid-generator;1'].getService(Ci.nsIUUIDGenerator);








const global = this;
const IDB = {
  _db: null,

  open: function () {
    let deferred = promise.defer();

    var idbManager = Cc["@mozilla.org/dom/indexeddb/manager;1"]
                       .getService(Ci.nsIIndexedDatabaseManager);
    idbManager.initWindowless(global);

    let request = global.indexedDB.open("AppProjects", 5);
    request.onerror = function(event) {
      deferred.reject("Unable to open AppProjects indexedDB. " +
                      "Error code: " + event.target.errorCode);
    };
    request.onupgradeneeded = function(event) {
      let db = event.target.result;
      db.createObjectStore("projects", { keyPath: "location" });
    };

    request.onsuccess = function() {
      let db = IDB._db = request.result;
      let objectStore = db.transaction("projects").objectStore("projects");
      let projects = []
      objectStore.openCursor().onsuccess = function(event) {
        let cursor = event.target.result;
        if (cursor) {
          projects.push(cursor.value);
          cursor.continue();
        } else {
          deferred.resolve(projects);
        }
      };
    };

    return deferred.promise;
  },

  add: function(project) {
    let deferred = promise.defer();

    var transaction = IDB._db.transaction(["projects"], "readwrite");
    var objectStore = transaction.objectStore("projects");
    var request = objectStore.add(project);
    request.onerror = function(event) {
      deferred.reject("Unable to add project to the AppProjects indexedDB: " +
                      this.error.name + " - " + this.error.message );
    };
    request.onsuccess = function() {
      deferred.resolve();
    };

    return deferred.promise;
  },

  remove: function(location) {
    let deferred = promise.defer();

    let request = IDB._db.transaction(["projects"], "readwrite")
                    .objectStore("projects")
                    .delete(location);
    request.onsuccess = function(event) {
      deferred.resolve();
    };
    request.onerror = function() {
      deferred.reject("Unable to delete project to the AppProjects indexedDB: " +
                      this.error.name + " - " + this.error.message );
    };

    return deferred.promise;
  }
};

const store = new ObservableObject({ projects:[] });

IDB.open().then(function (projects) {
  store.object.projects = projects;
  AppProjects.emit("ready", store.object.projects);
});

const AppProjects = {
  addPackaged: function(folder) {
    let project = {
      type: "packaged",
      location: folder.path,
      
      
      
      
      
      packagedAppOrigin: generateUUID().toString().slice(1, -1)
    };
    return IDB.add(project).then(function () {
      store.object.projects.push(project);
      
      return store.object.projects[store.object.projects.length - 1];
    });
  },

  addHosted: function(manifestURL) {
    let project = {
      type: "hosted",
      location: manifestURL
    };
    return IDB.add(project).then(function () {
      store.object.projects.push(project);
      
      return store.object.projects[store.object.projects.length - 1];
    });
  },

  remove: function(location) {
    return IDB.remove(location).then(function () {
      let projects = store.object.projects;
      for (let i = 0; i < projects.length; i++) {
        if (projects[i].location == location) {
          projects.splice(i, 1);
          return;
        }
      }
      throw new Error("Unable to find project in AppProjects store");
    });
  },

  get: function(location) {
    let projects = store.object.projects;
    for (let i = 0; i < projects.length; i++) {
      if (projects[i].location == location) {
        return projects[i];
      }
    }
    return null;
  },

  store: store
};

EventEmitter.decorate(AppProjects);

exports.AppProjects = AppProjects;

