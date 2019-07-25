






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const XPI_CONTENT_TYPE = "application/x-xpinstall";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function amContentHandler() {
}

amContentHandler.prototype = {
  







  handleContent: function XCH_handleContent(mimetype, context, request) {
    if (mimetype != XPI_CONTENT_TYPE)
      throw Cr.NS_ERROR_WONT_HANDLE_CONTENT;

    if (!(request instanceof Ci.nsIChannel))
      throw Cr.NS_ERROR_WONT_HANDLE_CONTENT;

    let uri = request.URI;

    let referer = null;
    if (request instanceof Ci.nsIPropertyBag2) {
      referer = request.getPropertyAsInterface("docshell.internalReferrer",
                                               Ci.nsIURI);
    }

    let window = null;
    let callbacks = request.notificationCallbacks ?
                    request.notificationCallbacks :
                    request.loadGroup.notificationCallbacks;
    if (callbacks)
      window = callbacks.getInterface(Ci.nsIDOMWindow);

    request.cancel(Cr.NS_BINDING_ABORTED);

    let manager = Cc["@mozilla.org/addons/integration;1"].
                  getService(Ci.amIWebInstaller);
    manager.installAddonsFromWebpage(mimetype, window, referer, [uri.spec],
                                     [null], [null], [null], null, 1);
  },

  classDescription: "XPI Content Handler",
  contractID: "@mozilla.org/uriloader/content-handler;1?type=" + XPI_CONTENT_TYPE,
  classID: Components.ID("{7beb3ba8-6ec3-41b4-b67c-da89b8518922}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentHandler])
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([amContentHandler]);
