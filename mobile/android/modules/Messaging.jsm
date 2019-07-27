


"use strict"

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

this.EXPORTED_SYMBOLS = ["sendMessageToJava", "Messaging"];

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

function sendMessageToJava(aMessage, aCallback) {
  Cu.reportError("sendMessageToJava is deprecated. Use Messaging API instead.");

  Messaging.sendRequest(aMessage, aCallback);
}

let Messaging = {
  































  addListener: function (aListener, aMessage) {
    requestHandler.addListener(aListener, aMessage);
  },

  




  removeListener: function (aMessage) {
    requestHandler.removeListener(aMessage);
  },

  





  sendRequest: function (aMessage, aCallback) {
    if (aCallback) {
      let id = uuidgen.generateUUID().toString();
      let obs = {
        observe: function(aSubject, aTopic, aData) {
          let data = JSON.parse(aData);
          if (data.__guid__ != id) {
            return;
          }

          Services.obs.removeObserver(obs, aMessage.type + ":Response", false);

          if (data.status === "cancel") {
            
            return;
          }

          aCallback(data.status === "success" ? data.response : null,
                    data.status === "error"   ? data.response : null);
        }
      }

      aMessage.__guid__ = id;
      Services.obs.addObserver(obs, aMessage.type + ":Response", false);
    }

    return Services.androidBridge.handleGeckoMessage(aMessage);
  },
};

let requestHandler = {
  _listeners: {},

  addListener: function (aListener, aMessage) {
    if (aMessage in this._listeners) {
      throw new Error("Error in addListener: A listener already exists for message " + aMessage);
    }

    if (typeof aListener !== "function") {
      throw new Error("Error in addListener: Listener must be a function for message " + aMessage);
    }

    this._listeners[aMessage] = aListener;
    Services.obs.addObserver(this, aMessage, false);
  },

  removeListener: function (aMessage) {
    if (!(aMessage in this._listeners)) {
      throw new Error("Error in removeListener: There is no listener for message " + aMessage);
    }

    delete this._listeners[aMessage];
    Services.obs.removeObserver(this, aMessage);
  },

  observe: Task.async(function* (aSubject, aTopic, aData) {
    let wrapper = JSON.parse(aData);
    let listener = this._listeners[aTopic];

    
    
    
    let response = null;

    try {
      let result = yield listener(wrapper.data);
      if (typeof result !== "object" || result === null) {
        throw new Error("Gecko request listener did not return an object");
      }
      response = result;
    } catch (e) {
      Cu.reportError(e);
    }

    Messaging.sendRequest({
      type: "Gecko:Request" + wrapper.id,
      response: response
    });
  })
};
