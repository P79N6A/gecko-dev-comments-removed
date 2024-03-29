



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const CATEGORY_WAKEUP_REQUEST = "wakeup-request";

function MessageWakeupService() { };

MessageWakeupService.prototype =
{
  classID:          Components.ID("{f9798742-4f7b-4188-86ba-48b116412b29}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIObserver]),

  messagesData: [],

  get messageManager() {
    if (!this._messageManager)
      this._messageManager = Cc["@mozilla.org/parentprocessmessagemanager;1"].
                             getService(Ci.nsIMessageListenerManager);
    return this._messageManager;
  },

  requestWakeup: function(aMessageName, aCid, aIid, aMethod) {
    this.messagesData[aMessageName] = {
      cid: aCid,
      iid: aIid,
      method: aMethod,
    };

    this.messageManager.addMessageListener(aMessageName, this);
  },

  receiveMessage: function(aMessage) {
    let data = this.messagesData[aMessage.name];
    
    
    
    let service = Cc[data.cid][data.method](Ci[data.iid]).
                  wrappedJSObject;

    
    
    
    
    let ret = service.receiveMessage(aMessage);

    if (data.timer) {
      
      data.timer.cancel();
      data.timer = null;
    }

    data.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let self = this;
    data.timer.initWithCallback(function() {
      self.messageManager.addMessageListener(aMessage.name, service);
      self.messageManager.removeMessageListener(aMessage.name, self);
      delete self.messagesData[aMessage.name];
    }, 0, Ci.nsITimer.TYPE_ONE_SHOT);

    return ret;
  },

  observe: function TM_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "profile-after-change":
        {
          var catMan = Cc["@mozilla.org/categorymanager;1"].
                           getService(Ci.nsICategoryManager);
          var entries = catMan.enumerateCategory(CATEGORY_WAKEUP_REQUEST);
          while (entries.hasMoreElements()) {
            var entry = entries.getNext().QueryInterface(Ci.nsISupportsCString).data;
            var value = catMan.getCategoryEntry(CATEGORY_WAKEUP_REQUEST, entry);
            var parts = value.split(",");
            var cid = parts[0];
            var iid = parts[1];
            var method = parts[2];
            var messages = parts.slice(3);
            messages.forEach(function(messageName) {
              this.requestWakeup(messageName, cid, iid, method);
            }, this);
          }
        }
        break;
    }
  },
};

var components = [MessageWakeupService];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(components);

