



"use strict";

function debug(str) {
  dump("CHROME PERMISSON HANDLER -- " + str + "\n");
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const { Services } = Cu.import("resource://gre/modules/Services.jsm");
const { SystemAppProxy } = Cu.import("resource://gre/modules/SystemAppProxy.jsm");

let eventHandler = function(evt) {
  if (!evt.detail || evt.detail.type !== "permission-prompt") {
    return;
  }

  sendAsyncMessage("permission-request", evt.detail);
};

SystemAppProxy.addEventListener("mozChromeEvent", eventHandler);


addMessageListener("teardown", function() {
  SystemAppProxy.removeEventListener("mozChromeEvent", eventHandler);
});

addMessageListener("permission-response", function(detail) {
  SystemAppProxy._sendCustomEvent('mozContentEvent', detail);
});

