const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function executeSoon(f)
{
  Services.tm.mainThread.dispatch(f, Ci.nsIThread.DISPATCH_NORMAL);
}

var urlSuffix = "/this/is/the/test/url";


var policyID = Components.ID("{6aadacff-f1f2-46f4-a6db-6d429f884a30}");
var policyName = "@mozilla.org/simpletestpolicy;1";
var policy = {
  
  QueryInterface:
    XPCOMUtils.generateQI([
      Ci.nsISupports,
      Ci.nsIFactory,
      Ci.nsISimpleContentPolicy]),

  
  createInstance: function(outer, iid) {
    return this.QueryInterface(iid);
  },

  
  shouldLoad: function(contentType,
                       contentLocation,
                       requestOrigin,
                       frame,
                       isTopLevel,
                       mimeTypeGuess,
                       extra)
  {
    
    if (contentLocation.spec.endsWith(urlSuffix)) {
      assert.ok(frame === browserElement, "correct <browser> element");
      sendAsyncMessage("shouldLoad", {contentType: contentType, isTopLevel: isTopLevel});
      return Ci.nsIContentPolicy.REJECT_REQUEST;
    }

    return Ci.nsIContentPolicy.ACCEPT;
  },

  shouldProcess: function() {
    return Ci.nsIContentPolicy.ACCEPT;
  }
}


var componentManager = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
componentManager.registerFactory(policyID, "Test simple content policy", policyName, policy);

var categoryManager = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
categoryManager.addCategoryEntry("simple-content-policy", policyName, policyName, false, true);

addMessageListener("finished", () => {
  
  categoryManager.deleteCategoryEntry("simple-content-policy", policyName, false);

  executeSoon(function() {
    
    
    componentManager.unregisterFactory(policyID, policy);
  });
});

sendAsyncMessage("ready");
