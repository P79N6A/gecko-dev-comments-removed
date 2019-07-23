






































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const SCHEME = "place";
const URL = "chrome://browser/content/places/content-ui/controller.xhtml";

function PlacesProtocolHandler() {}

PlacesProtocolHandler.prototype = {
  scheme: SCHEME,
  defaultPort: -1,
  protocolFlags: Ci.nsIProtocolHandler.URI_DANGEROUS_TO_LOAD |
                 Ci.nsIProtocolHandler.URI_IS_LOCAL_RESOURCE |
                 Ci.nsIProtocolHandler.URI_NORELATIVE |
                 Ci.nsIProtocolHandler.URI_NOAUTH,

  newURI: function PPH_newURI(aSpec, aOriginCharset, aBaseUri) {
    let uri = Cc["@mozilla.org/network/simple-uri;1"].createInstance(Ci.nsIURI);
    uri.spec = aSpec;
    return uri;
  },

  newChannel: function PPH_newChannel(aUri) {
    let chan = NetUtil.newChannel(URL);
    chan.originalURI = aUri;
    return chan;
  },

  allowPort: function PPH_allowPort(aPort, aScheme) {
    return false;
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIProtocolHandler
  ]),

  classDescription: "Places Protocol Handler",
  contractID: "@mozilla.org/network/protocol;1?name=" + SCHEME,
  classID: Components.ID("{6bcb9bde-9018-4443-a071-c32653469597}")
};

function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([
    PlacesProtocolHandler
  ]);
}
