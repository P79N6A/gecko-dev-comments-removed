



"use strict";

this.EXPORTED_SYMBOLS = [];

const DEBUG = false;
function debug(s) { dump("-*- NotificationDB component: " + s + "\n"); }

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

const NOTIFICATION_STORE_DIR = OS.Constants.Path.profileDir;
const NOTIFICATION_STORE_PATH =
        OS.Path.join(NOTIFICATION_STORE_DIR, "notificationstore.json");

const kMessages = [
  "Notification:Save",
  "Notification:Delete",
  "Notification:GetAll",
  "Notification:GetAllCrossOrigin"
];

let NotificationDB = {

  
  _shutdownInProgress: false,

  init: function() {
    if (this._shutdownInProgress) {
      return;
    }

    this.notifications = {};
    this.byTag = {};
    this.loaded = false;

    this.tasks = []; 
    this.runningTask = null;

    Services.obs.addObserver(this, "xpcom-shutdown", false);
    this.registerListeners();
  },

  registerListeners: function() {
    for (let message of kMessages) {
      ppmm.addMessageListener(message, this);
    }
  },

  unregisterListeners: function() {
    for (let message of kMessages) {
      ppmm.removeMessageListener(message, this);
    }
  },

  observe: function(aSubject, aTopic, aData) {
    if (DEBUG) debug("Topic: " + aTopic);
    if (aTopic == "xpcom-shutdown") {
      this._shutdownInProgress = true;
      Services.obs.removeObserver(this, "xpcom-shutdown");
      this.unregisterListeners();
    }
  },

  
  load: function() {
    var promise = OS.File.read(NOTIFICATION_STORE_PATH, { encoding: "utf-8"});
    return promise.then(
      function onSuccess(data) {
        
        
        this.notifications = JSON.parse(data);
        
        if (this.notifications) {
          for (var origin in this.notifications) {
            this.byTag[origin] = {};
            for (var id in this.notifications[origin]) {
              var curNotification = this.notifications[origin][id];
              if (curNotification.tag) {
                this.byTag[origin][curNotification.tag] = curNotification;
              }
            }
          }
        }
        this.loaded = true;
      }.bind(this),

      
      function onFailure(reason) {
        this.loaded = true;
        return this.createStore();
      }.bind(this)
    );
  },

  
  createStore: function() {
    var promise = OS.File.makeDir(NOTIFICATION_STORE_DIR, {
      ignoreExisting: true
    });
    return promise.then(
      this.createFile.bind(this)
    );
  },

  
  createFile: function() {
    return OS.File.writeAtomic(NOTIFICATION_STORE_PATH, "");
  },

  
  save: function() {
    var data = JSON.stringify(this.notifications);
    return OS.File.writeAtomic(NOTIFICATION_STORE_PATH, data, { encoding: "utf-8"});
  },

  
  ensureLoaded: function() {
    if (!this.loaded) {
      return this.load();
    } else {
      return Promise.resolve();
    }
  },

  receiveMessage: function(message) {
    if (DEBUG) { debug("Received message:" + message.name); }

    
    
    function returnMessage(name, data) {
      try {
        message.target.sendAsyncMessage(name, data);
      } catch (e) {
        if (DEBUG) { debug("Return message failed, " + name); }
      }
    }

    switch (message.name) {
      case "Notification:GetAll":
        this.queueTask("getall", message.data).then(function(notifications) {
          returnMessage("Notification:GetAll:Return:OK", {
            requestID: message.data.requestID,
            origin: message.data.origin,
            notifications: notifications
          });
        }).catch(function(error) {
          returnMessage("Notification:GetAll:Return:KO", {
            requestID: message.data.requestID,
            origin: message.data.origin,
            errorMsg: error
          });
        });
        break;

      case "Notification:GetAllCrossOrigin":
        this.queueTask("getallaccrossorigin", message.data).then(
          function(notifications) {
            returnMessage("Notification:GetAllCrossOrigin:Return:OK", {
              notifications: notifications
            });
          }).catch(function(error) {
            returnMessage("Notification:GetAllCrossOrigin:Return:KO", {
              errorMsg: error
            });
          });
        break;

      case "Notification:Save":
        this.queueTask("save", message.data).then(function() {
          returnMessage("Notification:Save:Return:OK", {
            requestID: message.data.requestID
          });
        }).catch(function(error) {
          returnMessage("Notification:Save:Return:KO", {
            requestID: message.data.requestID,
            errorMsg: error
          });
        });
        break;

      case "Notification:Delete":
        this.queueTask("delete", message.data).then(function() {
          returnMessage("Notification:Delete:Return:OK", {
            requestID: message.data.requestID
          });
        }).catch(function(error) {
          returnMessage("Notification:Delete:Return:KO", {
            requestID: message.data.requestID,
            errorMsg: error
          });
        });
        break;

      default:
        if (DEBUG) { debug("Invalid message name" + message.name); }
    }
  },

  
  
  queueTask: function(operation, data) {
    if (DEBUG) { debug("Queueing task: " + operation); }

    var defer = {};

    this.tasks.push({
      operation: operation,
      data: data,
      defer: defer
    });

    var promise = new Promise(function(resolve, reject) {
      defer.resolve = resolve;
      defer.reject = reject;
    });

    
    if (!this.runningTask) {
      if (DEBUG) { debug("Task queue was not running, starting now..."); }
      this.runNextTask();
    }

    return promise;
  },

  runNextTask: function() {
    if (this.tasks.length === 0) {
      if (DEBUG) { debug("No more tasks to run, queue depleted"); }
      this.runningTask = null;
      return;
    }
    this.runningTask = this.tasks.shift();

    
    this.ensureLoaded()
    .then(function() {
      var task = this.runningTask;

      switch (task.operation) {
        case "getall":
          return this.taskGetAll(task.data);
          break;

        case "getallaccrossorigin":
          return this.taskGetAllCrossOrigin();
          break;

        case "save":
          return this.taskSave(task.data);
          break;

        case "delete":
          return this.taskDelete(task.data);
          break;
      }

    }.bind(this))
    .then(function(payload) {
      if (DEBUG) {
        debug("Finishing task: " + this.runningTask.operation);
      }
      this.runningTask.defer.resolve(payload);
    }.bind(this))
    .catch(function(err) {
      if (DEBUG) {
        debug("Error while running " + this.runningTask.operation + ": " + err);
      }
      this.runningTask.defer.reject(new String(err));
    }.bind(this))
    .then(function() {
      this.runNextTask();
    }.bind(this));
  },

  taskGetAll: function(data) {
    if (DEBUG) { debug("Task, getting all"); }
    var origin = data.origin;
    var notifications = [];
    
    for (var i in this.notifications[origin]) {
      notifications.push(this.notifications[origin][i]);
    }
    return Promise.resolve(notifications);
  },

  taskGetAllCrossOrigin: function() {
    if (DEBUG) { debug("Task, getting all whatever origin"); }
    var notifications = [];
    for (var origin in this.notifications) {
      for (var i in this.notifications[origin]) {
        var notification = this.notifications[origin][i];

        
        
        
        if (!('alertName' in notification)) {
          continue;
        }

        notification.origin = origin;
        notifications.push(notification);
      }
    }
    return Promise.resolve(notifications);
  },

  taskSave: function(data) {
    if (DEBUG) { debug("Task, saving"); }
    var origin = data.origin;
    var notification = data.notification;
    if (!this.notifications[origin]) {
      this.notifications[origin] = {};
      this.byTag[origin] = {};
    }

    
    
    if (notification.tag && this.byTag[origin][notification.tag]) {
      var oldNotification = this.byTag[origin][notification.tag];
      delete this.notifications[origin][oldNotification.id];
      this.byTag[origin][notification.tag] = notification;
    }

    this.notifications[origin][notification.id] = notification;
    return this.save();
  },

  taskDelete: function(data) {
    if (DEBUG) { debug("Task, deleting"); }
    var origin = data.origin;
    var id = data.id;
    if (!this.notifications[origin]) {
      if (DEBUG) { debug("No notifications found for origin: " + origin); }
      return Promise.resolve();
    }

    
    var oldNotification = this.notifications[origin][id];
    if (!oldNotification) {
      if (DEBUG) { debug("No notification found with id: " + id); }
      return Promise.resolve();
    }

    if (oldNotification.tag) {
      delete this.byTag[origin][oldNotification.tag];
    }
    delete this.notifications[origin][id];
    return this.save();
  }
};

NotificationDB.init();
