



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

addEventListener("load", function(event) {
  let subframe = event.target != content.document;
  sendAsyncMessage("browser-test-utils:loadEvent",
    {subframe: subframe, url: event.target.documentURI});
}, true);

