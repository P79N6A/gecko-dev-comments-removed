



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function ContentDispatchChooser() {}

ContentDispatchChooser.prototype =
{
  classDescription: "Used to handle different types of content",
  classID: Components.ID("5a072a22-1e66-4100-afc1-07aed8b62fc5"),
  contractID: "@mozilla.org/content-dispatch-chooser;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentDispatchChooser]),

  ask: function ask(aHandler, aWindowContext, aURI, aReason) {
    let window = null;
    try {
      if (aWindowContext)
        window = aWindowContext.getInterface(Ci.nsIDOMWindow);
    } catch (e) {  }

    let sbs = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
    let bundle = sbs.createBundle("chrome://mozapps/locale/handling/handling.properties");

    let prompter = Cc["@mozilla.org/embedcomp/prompt-service;1"].getService(Ci.nsIPromptService);
    let title = bundle.GetStringFromName("protocol.title");
    let message = bundle.GetStringFromName("protocol.description");

    let open = prompter.confirm(window, title, message);
    if (open)
      aHandler.launchWithURI(aURI, aWindowContext);
  }
};

function NSGetModule(aCompMgr, aFileSpec)
{
  return XPCOMUtils.generateModule([ContentDispatchChooser]);
}

