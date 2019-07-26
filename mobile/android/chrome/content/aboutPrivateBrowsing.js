



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

if (!PrivateBrowsingUtils.isWindowPrivate(window)) {
  document.addEventListener("DOMContentLoaded", function () {
    document.body.setAttribute("class", "normal");
  }, false);
}
