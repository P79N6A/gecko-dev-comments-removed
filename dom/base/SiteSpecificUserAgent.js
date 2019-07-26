



const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/UserAgentOverrides.jsm");

const DEFAULT_UA = Cc["@mozilla.org/network/protocol;1?name=http"]
                     .getService(Ci.nsIHttpProtocolHandler)
                     .userAgent;

function SiteSpecificUserAgent() {}

SiteSpecificUserAgent.prototype = {
  getUserAgentForURI: function ssua_getUserAgentForURI(aURI) {
    return UserAgentOverrides.getOverrideForURI(aURI) || DEFAULT_UA;
  },

  classID: Components.ID("{506c680f-3d1c-4954-b351-2c80afbc37d3}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISiteSpecificUserAgent])
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([SiteSpecificUserAgent]);
