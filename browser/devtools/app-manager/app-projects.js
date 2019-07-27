const {Cc,Ci,Cu,Cr} = require("chrome");
const ObservableObject = require("devtools/shared/observable-object");
const promise = require("devtools/toolkit/deprecated-sync-thenables");

const {EventEmitter} = Cu.import("resource://gre/modules/devtools/event-emitter.js");
const {generateUUID} = Cc['@mozilla.org/uuid-generator;1'].getService(Ci.nsIUUIDGenerator);
const {FileUtils} = Cu.import("resource://gre/modules/FileUtils.jsm");
const { indexedDB } = require("sdk/indexed-db");








const IDB = {
  _db: null,

  open: function () {
    let deferred = promise.defer();

    let request = indexedDB.open("AppProjects", 5);
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
      let toRemove = [];
      objectStore.openCursor().onsuccess = function(event) {
        let cursor = event.target.result;
        if (cursor) {
          if (cursor.value.location) {

            
            
            
            
            


            
            

            try {
              let file = FileUtils.File(cursor.value.location);
              if (file.exists()) {
                projects.push(cursor.value);
              } else {
                toRemove.push(cursor.value.location);
              }
            } catch (e) {
              if (e.result == Cr.NS_ERROR_FILE_UNRECOGNIZED_PATH) {
                
                projects.push(cursor.value);
              }
            }
          }
          cursor.continue();
        } else {
          let removePromises = [];
          for (let location of toRemove) {
            removePromises.push(IDB.remove(location));
          }
          promise.all(removePromises).then(() => {
            deferred.resolve(projects);
          });
        }
      };
    };

    return deferred.promise;
  },

  add: function(project) {
    let deferred = promise.defer();

    if (!project.location) {
      
      deferred.reject("Missing location property on project object.");
    } else {
      let transaction = IDB._db.transaction(["projects"], "readwrite");
      let objectStore = transaction.objectStore("projects");
      let request = objectStore.add(project);
      request.onerror = function(event) {
        deferred.reject("Unable to add project to the AppProjects indexedDB: " +
                        this.error.name + " - " + this.error.message );
      };
      request.onsuccess = function() {
        deferred.resolve();
      };
    }

    return deferred.promise;
  },

  update: function(project) {
    let deferred = promise.defer();

    
    
    
    
    project = JSON.parse(JSON.stringify(project));

    var transaction = IDB._db.transaction(["projects"], "readwrite");
    var objectStore = transaction.objectStore("projects");
    var request = objectStore.put(project);
    request.onerror = function(event) {
      deferred.reject("Unable to update project to the AppProjects indexedDB: " +
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

let loadDeferred = promise.defer();

IDB.open().then(function (projects) {
  store.object.projects = projects;
  AppProjects.emit("ready", store.object.projects);
  loadDeferred.resolve();
});

const AppProjects = {
  load: function() {
    return loadDeferred.promise;
  },

  addPackaged: function(folder) {
    let file = FileUtils.File(folder.path);
    if (!file.exists()) {
      return promise.reject("path doesn't exist");
    }
    let existingProject = this.get(folder.path);
    if (existingProject) {
      return promise.reject("Already added");
    }
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
    let existingProject = this.get(manifestURL);
    if (existingProject) {
      return promise.reject("Already added");
    }
    let project = {
      type: "hosted",
      location: manifestURL
    };
    return IDB.add(project).then(function () {
      store.object.projects.push(project);
      
      return store.object.projects[store.object.projects.length - 1];
    });
  },

  update: function (project) {
    return IDB.update(project);
  },

  updateLocation: function(project, newLocation)Â {
    return IDB.remove(project.location)
              .then(() => {
                project.location = newLocation;
                return IDB.add(project);
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
