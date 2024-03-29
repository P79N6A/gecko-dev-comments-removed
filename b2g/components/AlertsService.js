



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

const kTopicAlertShow          = "alertshow";
const kTopicAlertFinished      = "alertfinished";
const kTopicAlertClickCallback = "alertclickcallback";

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
                                  aLang, aDataStr, aPrincipal,
                                  aInPrivateBrowsing) {
    cpmm.sendAsyncMessage(kMessageAlertNotificationSend, {
      imageURL: aImageUrl,
      title: aTitle,
      text: aText,
      clickable: aTextClickable,
      cookie: aCookie,
      listener: aAlertListener,
      id: aName,
      dir: aBidi,
      lang: aLang,
      dataStr: aDataStr,
      inPrivateBrowsing: aInPrivateBrowsing
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

    let dataObj = this.deserializeStructuredClone(aDetails.data);
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
      tag: aDetails.tag || undefined,
      timestamp: aDetails.timestamp || undefined,
      dataObj: dataObj || undefined
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
        if (topic !== kTopicAlertShow) {
          
          
          
          gSystemMessenger.sendMessage(kNotificationSystemMessageName, {
              clicked: (topic === kTopicAlertClickCallback),
              title: listener.title,
              body: listener.text,
              imageURL: listener.imageURL,
              lang: listener.lang,
              dir: listener.dir,
              id: listener.id,
              tag: listener.tag,
              timestamp: listener.timestamp,
              data: listener.dataObj || undefined,
            },
            Services.io.newURI(data.target, null, null),
            Services.io.newURI(listener.manifestURL, null, null)
          );
        }
      }
      if (topic === kTopicAlertFinished && listener.dbId) {
        notificationStorage.delete(listener.manifestURL, listener.dbId);
      }
    }

    
    if (topic === kTopicAlertFinished) {
      delete this._listeners[data.uid];
    }
  },

  deserializeStructuredClone: function(dataString) {
    if (!dataString) {
      return null;
    }
    let scContainer = Cc["@mozilla.org/docshell/structured-clone-container;1"].
      createInstance(Ci.nsIStructuredCloneContainer);

    
    
    let JS_STRUCTURED_CLONE_VERSION = 4;
    scContainer.initFromBase64(dataString, JS_STRUCTURED_CLONE_VERSION);
    let dataObj = scContainer.deserializeToVariant();

    
    
    
    
    try {
      let data = Cu.cloneInto(dataObj, {});
    } catch(e) { dataObj = null; }

    return dataObj;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([AlertsService]);
