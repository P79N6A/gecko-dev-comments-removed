



































const EXPORTED_SYMBOLS = ["Notifications", "Notification", "NotificationButton"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/ext/Observers.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/util.js");

let Notifications = {
  
  get PRIORITY_INFO()     1, 
  get PRIORITY_WARNING()  4, 
  get PRIORITY_ERROR()    7, 

  
  
  
  notifications: [],

  _observers: [],

  
  

  add: function Notifications_add(notification) {
    this.notifications.push(notification);
    Observers.notify("weave:notification:added", notification, null);
  },

  remove: function Notifications_remove(notification) {
    let index = this.notifications.indexOf(notification);

    if (index != -1) {
      this.notifications.splice(index, 1);
      Observers.notify("weave:notification:removed", notification, null);
    }
  },

  


  replace: function Notifications_replace(oldNotification, newNotification) {
    let index = this.notifications.indexOf(oldNotification);

    if (index != -1)
      this.notifications.splice(index, 1, newNotification);
    else {
      this.notifications.push(newNotification);
      
      
    }

    
  },

  






  removeAll: function Notifications_removeAll(title) {
    this.notifications.filter(function(old) old.title == title || !title).
      forEach(function(old) this.remove(old), this);
  },

  
  replaceTitle: function Notifications_replaceTitle(notification) {
    this.removeAll(notification.title);
    this.add(notification);
  }
};





function Notification(title, description, iconURL, priority, buttons) {
  this.title = title;
  this.description = description;

  if (iconURL)
    this.iconURL = iconURL;

  if (priority)
    this.priority = priority;

  if (buttons)
    this.buttons = buttons;
}





Notification.prototype.priority = Notifications.PRIORITY_INFO;
Notification.prototype.iconURL = null;
Notification.prototype.buttons = [];




function NotificationButton(label, accessKey, callback) {
  function callbackWrapper() {
    try {
      callback.apply(this, arguments);
    } catch (e) {
      let logger = Log4Moz.repository.getLogger("Sync.Notifications");
      logger.error("An exception occurred: " + Utils.exceptionStr(e));
      logger.info(Utils.stackTrace(e));
      throw e;
    }
  }

  this.label = label;
  this.accessKey = accessKey;
  this.callback = callbackWrapper;
}
