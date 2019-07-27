



"use strict";

XPCOMUtils.defineLazyServiceGetter(this,
                                   "swm",
                                   "@mozilla.org/serviceworkers/manager;1",
                                   "nsIServiceWorkerManager");

addMessageListener("push", function (aMessage) {
  swm.sendPushEvent(aMessage.data.scope, aMessage.data.payload);
});

addMessageListener("pushsubscriptionchanged", function (aMessage) {
  swm.sendPushSubscriptionChangedEvent(aMessage.data);
});