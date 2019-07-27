



"use strict";




if (!("Cu" in window)) {
  window.Cu = Components.utils;
}

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ResetProfile.jsm");

function onResetProfileAccepted() {
  let retVals = window.arguments[0];
  retVals.reset = true;
}
