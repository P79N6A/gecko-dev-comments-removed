



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CONTENT_HANDLING_URL = "chrome://mozapps/content/handling/dialog.xul";
const STRINGBUNDLE_URL = "chrome://mozapps/locale/handling/handling.properties";




function nsContentDispatchChooser()
{
}

nsContentDispatchChooser.prototype =
{
  classID: Components.ID("e35d5067-95bc-4029-8432-e8f1e431148d"),

  
  

  ask: function ask(aHandler, aWindowContext, aURI, aReason)
  {
    var window = null;
    try {
      if (aWindowContext)
        window = aWindowContext.getInterface(Ci.nsIDOMWindow);
    } catch (e) {  }

    var sbs = Cc["@mozilla.org/intl/stringbundle;1"].
              getService(Ci.nsIStringBundleService);
    var bundle = sbs.createBundle(STRINGBUNDLE_URL);

    var xai = Cc["@mozilla.org/xre/app-info;1"].
              getService(Ci.nsIXULAppInfo);
    
    
    var arr = [bundle.GetStringFromName("protocol.title"),
               "",
               bundle.GetStringFromName("protocol.description"),
               bundle.GetStringFromName("protocol.choices.label"),
               bundle.formatStringFromName("protocol.checkbox.label",
                                           [aURI.scheme], 1),
               bundle.GetStringFromName("protocol.checkbox.accesskey"),
               bundle.formatStringFromName("protocol.checkbox.extra",
                                           [xai.name], 1)];

    var params = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    let SupportsString = Components.Constructor(
                           "@mozilla.org/supports-string;1",
                           "nsISupportsString");
    for each (let text in arr) {
      let string = new SupportsString;
      string.data = text;
      params.appendElement(string, false);
    }
    params.appendElement(aHandler, false);
    params.appendElement(aURI, false);
    params.appendElement(aWindowContext, false);
    
    var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
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

var NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
