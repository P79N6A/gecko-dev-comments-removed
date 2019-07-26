



const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyServiceGetter(this, "uuidGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyServiceGetter(this, "notificationStorage",
                                   "@mozilla.org/notificationStorage;1",
                                   "nsINotificationStorage");


XPCOMUtils.defineLazyGetter(this, "cpmm", function() {
  return Cc["@mozilla.org/childprocessmessagemanager;1"]
           .getService(Ci.nsIMessageSender);
});

function debug(str) {
  dump("=*= AlertsService.js : " + str + "\n");
}





const kNotificationSystemMessageName = "notification";

const kMessageAppNotificationSend    = "app-notification-send";
const kMessageAppNotificationReturn  = "app-notification-return";
const kMessageAlertNotificationSend  = "alert-notification-send";
const kMessageAlertNotificationClose = "alert-notification-close";

const kTopicAlertFinished      = "alertfinished";

function AlertsService() {
  Services.obs.addObserver(this, "xpcom-shutdown", false);
  cpmm.addMessageListener(kMessageAppNotificationReturn, this);
}

AlertsService.prototype = {
  classID: Components.ID("{fe33c107-82a4-41d6-8c64-5353267e04c9}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAlertsService,
                                         Ci.nsIAppNotificationService,
                                         Ci.nsIObserver]),

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-shutdown":
        Services.obs.removeObserver(this, "xpcom-shutdown");
        cpmm.removeMessageListener(kMessageAppNotificationReturn, this);
        break;
    }
  },

  
  showAlertNotification: function(aImageUrl, aTitle, aText, aTextClickable,
                                  aCookie, aAlertListener, aName, aBidi,
                                  aLang) {
    cpmm.sendAsyncMessage(kMessageAlertNotificationSend, {
      imageURL: aImageUrl,
      title: aTitle,
      text: aText,
      clickable: aTextClickable,
      cookie: aCookie,
      listener: aAlertListener,
      id: aName,
      dir: aBidi,
      lang: aLang
    });
  },

  closeAlert: function(aName) {
    cpmm.sendAsyncMessage(kMessageAlertNotificationClose, {
      name: aName
    });
  },

  
  showAppNotification: function(aImageURL, aTitle, aText, aAlertListener,
                                aDetails) {
    let uid = (aDetails.id == "") ?
          "app-notif-" + uuidGenerator.generateUUID() : aDetails.id;

    this._listeners[uid] = {
      observer: aAlertListener,
      title: aTitle,
      text: aText,
      manifestURL: aDetails.manifestURL,
      imageURL: aImageURL,
      lang: aDetails.lang || undefined,
      id: aDetails.id || undefined,
      dbId: aDetails.dbId || undefined,
      dir: aDetails.dir || undefined,
      tag: aDetails.tag || undefined
    };

    cpmm.sendAsyncMessage(kMessageAppNotificationSend, {
      imageURL: aImageURL,
      title: aTitle,
      text: aText,
      uid: uid,
      details: aDetails
    });
  },

  
  _listeners: [],

  receiveMessage: function(aMessage) {
    let data = aMessage.data;
    let listener = this._listeners[data.uid];
    if (aMessage.name !== kMessageAppNotificationReturn || !listener) {
      return;
    }

    let topic = data.topic;

    try {
      listener.observer.observe(null, topic, null);
    } catch (e) {
      
      
      
      if (data.target) {
        gSystemMessenger.sendMessage(kNotificationSystemMessageName, {
            title: listener.title,
            body: listener.text,
            imageURL: listener.imageURL,
            lang: listener.lang,
            dir: listener.dir,
            id: listener.id,
            tag: listener.tag,
            dbId: listener.dbId
          },
          Services.io.newURI(data.target, null, null),
          Services.io.newURI(listener.manifestURL, null, null)
        );
      }
    }

    
    if (topic === kTopicAlertFinished) {
      if (listener.dbId) {
        notificationStorage.delete(listener.manifestURL, listener.dbId);
      }
      delete this._listeners[data.uid];
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([AlertsService]);
