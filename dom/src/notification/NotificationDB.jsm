



"use strict";

this.EXPORTED_SYMBOLS = [];

const DEBUG = false;
function debug(s) { dump("-*- NotificationDB component: " + s + "\n"); }

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyGetter(this, "gEncoder", function() {
  return new TextEncoder();
});

XPCOMUtils.defineLazyGetter(this, "gDecoder", function() {
  return new TextDecoder();
});


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
    this.runningTask = false;

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

  
  load: function(callback) {
    var promise = OS.File.read(NOTIFICATION_STORE_PATH);
    promise.then(
      function onSuccess(data) {
        try {
          this.notifications = JSON.parse(gDecoder.decode(data));
        } catch (e) {
          if (DEBUG) { debug("Unable to parse file data " + e); }
        }
        
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
        callback && callback();
      }.bind(this),

      
      function onFailure(reason) {
        this.loaded = true;
        this.createStore(callback);
      }.bind(this)
    );
  },

  
  createStore: function(callback) {
    var promise = OS.File.makeDir(NOTIFICATION_STORE_DIR, {
      ignoreExisting: true
    });
    promise.then(
      function onSuccess() {
        this.createFile(callback);
      }.bind(this),

      function onFailure(reason) {
        if (DEBUG) { debug("Directory creation failed:" + reason); }
        callback && callback();
      }
    );
  },

  
  createFile: function(callback) {
    var promise = OS.File.open(NOTIFICATION_STORE_PATH, {create: true});
    promise.then(
      function onSuccess(handle) {
        handle.close();
        callback && callback();
      },
      function onFailure(reason) {
        if (DEBUG) { debug("File creation failed:" + reason); }
        callback && callback();
      }
    );
  },

  
  save: function(callback) {
    var data = gEncoder.encode(JSON.stringify(this.notifications));
    var promise = OS.File.writeAtomic(NOTIFICATION_STORE_PATH, data);
    promise.then(
      function onSuccess() {
        callback && callback();
      },
      function onFailure(reason) {
        if (DEBUG) { debug("Save failed:" + reason); }
        callback && callback();
      }
    );
  },

  
  ensureLoaded: function(callback) {
    if (!this.loaded) {
      this.load(callback);
    } else {
      callback();
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
        this.queueTask("getall", message.data, function(notifications) {
          returnMessage("Notification:GetAll:Return:OK", {
            requestID: message.data.requestID,
            notifications: notifications
          });
        });
        break;

      case "Notification:GetAllCrossOrigin":
        this.queueTask("getallaccrossorigin", message.data,
          function(notifications) {
            returnMessage("Notification:GetAllCrossOrigin:Return:OK", {
              notifications: notifications
            });
          });
        break;

      case "Notification:Save":
        this.queueTask("save", message.data, function() {
          returnMessage("Notification:Save:Return:OK", {
            requestID: message.data.requestID
          });
        });
        break;

      case "Notification:Delete":
        this.queueTask("delete", message.data, function() {
          returnMessage("Notification:Delete:Return:OK", {
            requestID: message.data.requestID
          });
        });
        break;

      default:
        if (DEBUG) { debug("Invalid message name" + message.name); }
    }
  },

  
  
  queueTask: function(operation, data, callback) {
    if (DEBUG) { debug("Queueing task: " + operation); }
    this.tasks.push({
      operation: operation,
      data: data,
      callback: callback
    });

    
    if (!this.runningTask) {
      if (DEBUG) { debug("Task queue was not running, starting now..."); }
      this.runNextTask();
    }
  },

  runNextTask: function() {
    if (this.tasks.length === 0) {
      if (DEBUG) { debug("No more tasks to run, queue depleted"); }
      this.runningTask = false;
      return;
    }
    this.runningTask = true;

    
    this.ensureLoaded(function() {
      var task = this.tasks.shift();

      
      
      var wrappedCallback = function() {
        if (DEBUG) { debug("Finishing task: " + task.operation); }
        task.callback.apply(this, arguments);
        this.runNextTask();
      }.bind(this);

      switch (task.operation) {
        case "getall":
          this.taskGetAll(task.data, wrappedCallback);
          break;

        case "getallaccrossorigin":
          this.taskGetAllCrossOrigin(wrappedCallback);
          break;

        case "save":
          this.taskSave(task.data, wrappedCallback);
          break;

        case "delete":
          this.taskDelete(task.data, wrappedCallback);
          break;
      }
    }.bind(this));
  },

  taskGetAll: function(data, callback) {
    if (DEBUG) { debug("Task, getting all"); }
    var origin = data.origin;
    var notifications = [];
    
    for (var i in this.notifications[origin]) {
      notifications.push(this.notifications[origin][i]);
    }
    callback(notifications);
  },

  taskGetAllCrossOrigin: function(callback) {
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
    callback(notifications);
  },

  taskSave: function(data, callback) {
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
    this.save(callback);
  },

  taskDelete: function(data, callback) {
    if (DEBUG) { debug("Task, deleting"); }
    var origin = data.origin;
    var id = data.id;
    if (!this.notifications[origin]) {
      if (DEBUG) { debug("No notifications found for origin: " + origin); }
      callback();
      return;
    }

    
    var oldNotification = this.notifications[origin][id];
    if (!oldNotification) {
      if (DEBUG) { debug("No notification found with id: " + id); }
      callback();
      return;
    }

    if (oldNotification.tag) {
      delete this.byTag[origin][oldNotification.tag];
    }
    delete this.notifications[origin][id];
    this.save(callback);
  }
};

NotificationDB.init();
