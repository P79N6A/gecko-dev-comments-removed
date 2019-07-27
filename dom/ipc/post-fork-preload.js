










(function (global) {
  "use strict";

  Components.utils.import("resource://gre/modules/AppsServiceChild.jsm");
  Components.classes["@mozilla.org/network/protocol-proxy-service;1"].
    getService(Components.interfaces["nsIProtocolProxyService"]);

  DOMApplicationRegistry.resetList();
})(this);
