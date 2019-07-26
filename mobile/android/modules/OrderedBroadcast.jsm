



"use strict";

this.EXPORTED_SYMBOLS = ["sendOrderedBroadcast"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;


Cu.import("resource://gre/modules/Services.jsm");

function sendMessageToJava(message) {
  return Cc["@mozilla.org/android/bridge;1"]
    .getService(Ci.nsIAndroidBridge)
    .handleGeckoMessage(JSON.stringify(message));
}

let _callbackId = 1;




















function sendOrderedBroadcast(action, token, callback, permission) {
  let callbackId = _callbackId++;
  let responseEvent = "OrderedBroadcast:Response:" + callbackId;

  let observer = {
    callbackId: callbackId,
    callback: callback,

    observe: function observe(subject, topic, data) {
      if (topic != responseEvent) {
        return;
      }

      
      Services.obs.removeObserver(observer, responseEvent);

      let msg = JSON.parse(data);
      if (!msg.action || !msg.token || !msg.token.callbackId)
        return;

      let theToken = msg.token.data;
      let theAction = msg.action;
      let theData = msg.data ? JSON.parse(msg.data) : null;

      let theCallback = this.callback;
      if (!theCallback)
        return;

      
      
      theCallback(theData, theToken, theAction);
    },
  };

  Services.obs.addObserver(observer, responseEvent, false);

  sendMessageToJava({
    type: "OrderedBroadcast:Send",
    action: action,
    responseEvent: responseEvent,
    token: { callbackId: callbackId, data: token || null },
    permission: permission || null,
  });
};
