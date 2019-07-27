



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

if (!PrivateBrowsingUtils.isContentWindowPrivate(window)) {
  document.addEventListener("DOMContentLoaded", function () {
    document.body.setAttribute("class", "normal");
  }, false);
}
