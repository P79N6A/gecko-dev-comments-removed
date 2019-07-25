



































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const CATEGORY_WAKEUP_REQUEST = "wakeup-request";

function MessageWakeupService() { };

MessageWakeupService.prototype =
{
  classID:          Components.ID("{f9798742-4f7b-4188-86ba-48b116412b29}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIMessageWakeupService, Ci.nsISupports, Ci.nsIObserver]),

  messagesData: [],

  get messageManager() {
    if (!this._messageManager)
      this._messageManager = Cc["@mozilla.org/parentprocessmessagemanager;1"].
                             getService(Ci.nsIFrameMessageManager);
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
    var data = this.messagesData[aMessage.name];
    delete this.messagesData[aMessage.name];
    var service = Cc[data.cid][data.method](Ci[data.iid]).
                  wrappedJSObject;



    this.messageManager.addMessageListener(aMessage.name, service);
    this.messageManager.removeMessageListener(aMessage.name, this);
    service.receiveMessage(aMessage);
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
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);

