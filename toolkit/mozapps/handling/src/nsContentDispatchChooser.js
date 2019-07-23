




































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CONTENT_HANDLING_URL = "chrome://mozapps/content/handling/dialog.xul";


const STRINGBUNDLE_URL = "chrome://mozapps/content/handling/handling.properties";




function nsContentDispatchChooser()
{
}

nsContentDispatchChooser.prototype =
{
  classDescription: "Used to handle different types of content",
  classID: Components.ID("e35d5067-95bc-4029-8432-e8f1e431148d"),
  contractID: "@mozilla.org/content-dispatch-chooser;1",

  
  

  ask: function ask(aHandler, aWindowContext, aURI, aReason)
  {
    let window = null;
    try {
      if (aWindowContext)
        window = aWindowContext.getInterface(Ci.nsIDOMWindow);
    } catch (e) {  }

    let sbs = Cc["@mozilla.org/intl/stringbundle;1"].
              getService(Ci.nsIStringBundleService);
    let bundle = sbs.createBundle(STRINGBUNDLE_URL);

    let xai = Cc["@mozilla.org/xre/app-info;1"].
              getService(Ci.nsIXULAppInfo);
    
    
    let arr = [bundle.GetStringFromName("protocol.title"),
               "",
               bundle.GetStringFromName("protocol.description"),
               bundle.GetStringFromName("protocol.choices.label"),
               bundle.formatStringFromName("protocol.checkbox.label",
                                           [aURI.scheme], 1),
               bundle.formatStringFromName("protocol.checkbox.extra",
                                           [xai.name], 1)];

    let params = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    for each (let text in arr) {
      let string = Cc["@mozilla.org/supports-string;1"].
                   createInstance(Ci.nsISupportsString);
      string.data = text;
      params.appendElement(string, false);
    }
    params.appendElement(aHandler, false);
    params.appendElement(aURI, false);

    let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);
    ww.openWindow(window,
                  CONTENT_HANDLING_URL,
                  null,
                  "chrome,dialog=yes,resizable,centerscreen",
                  params);
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentDispatchChooser])
};




let components = [nsContentDispatchChooser];

function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule(components);
}

