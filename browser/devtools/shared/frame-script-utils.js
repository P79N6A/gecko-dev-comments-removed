



"use strict";

let { utils: Cu, interfaces: Ci } = Components;

addMessageListener("devtools:test:history", function ({ data }) {
  content.history[data.direction]();
});

addMessageListener("devtools:test:navigate", function ({ data }) {
  content.location = data.location;
});

addMessageListener("devtools:test:reload", function ({ data }) {
  data = data || {};
  content.location.reload(data.forceget);
});

addMessageListener("devtools:test:forceCC", function () {
  let DOMWindowUtils = content.QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindowUtils)
  DOMWindowUtils.cycleCollect();
  DOMWindowUtils.garbageCollect();
  DOMWindowUtils.garbageCollect();
});
