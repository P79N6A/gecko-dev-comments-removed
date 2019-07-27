



"use strict";

this.EXPORTED_SYMBOLS = ["sendOrderedBroadcast"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;


Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

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

  Messaging.sendRequest({
    type: "OrderedBroadcast:Send",
    action: action,
    responseEvent: responseEvent,
    token: { callbackId: callbackId, data: token || null },
    permission: permission,
  });
};
