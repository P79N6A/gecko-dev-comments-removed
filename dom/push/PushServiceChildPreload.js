



"use strict";

XPCOMUtils.defineLazyServiceGetter(this,
                                   "swm",
                                   "@mozilla.org/serviceworkers/manager;1",
                                   "nsIServiceWorkerManager");

addMessageListener("push", function (aMessage) {
  let principal = content.document.nodePrincipal;
  swm.sendPushEvent(principal, aMessage.data.scope, aMessage.data.payload);
});

addMessageListener("pushsubscriptionchange", function (aMessage) {
  let principal = content.document.nodePrincipal;
  swm.sendPushSubscriptionChangeEvent(principal, aMessage.data);
});
