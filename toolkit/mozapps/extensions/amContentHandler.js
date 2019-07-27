



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const XPI_CONTENT_TYPE = "application/x-xpinstall";
const MSG_INSTALL_ADDONS = "WebInstallerInstallAddonsFromWebpage";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function amContentHandler() {
}

amContentHandler.prototype = {
  









  handleContent: function XCH_handleContent(aMimetype, aContext, aRequest) {
    if (aMimetype != XPI_CONTENT_TYPE)
      throw Cr.NS_ERROR_WONT_HANDLE_CONTENT;

    if (!(aRequest instanceof Ci.nsIChannel))
      throw Cr.NS_ERROR_WONT_HANDLE_CONTENT;

    let uri = aRequest.URI;

    let window = null;
    let callbacks = aRequest.notificationCallbacks ?
                    aRequest.notificationCallbacks :
                    aRequest.loadGroup.notificationCallbacks;
    if (callbacks)
      window = callbacks.getInterface(Ci.nsIDOMWindow);

    let referer = null;
    if (aRequest instanceof Ci.nsIPropertyBag2) {
      referer = aRequest.getPropertyAsInterface("docshell.internalReferrer",
                                                Ci.nsIURI);
    }

    if (!referer && aRequest instanceof Ci.nsIHttpChannel)
      referer = aRequest.referrer;

    aRequest.cancel(Cr.NS_BINDING_ABORTED);

    let messageManager = window.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIDocShell)
                               .QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIContentFrameMessageManager);

    messageManager.sendAsyncMessage(MSG_INSTALL_ADDONS, {
      uris: [uri.spec],
      hashes: [null],
      names: [null],
      icons: [null],
      mimetype: XPI_CONTENT_TYPE,
      referer: referer ? referer.spec : null,
      callbackID: -1
    });
  },

  classID: Components.ID("{7beb3ba8-6ec3-41b4-b67c-da89b8518922}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentHandler]),

  log : function XCH_log(aMsg) {
    let msg = "amContentHandler.js: " + (aMsg.join ? aMsg.join("") : aMsg);
    Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
      logStringMessage(msg);
    dump(msg + "\n");
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([amContentHandler]);
