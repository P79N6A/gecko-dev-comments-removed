


const { interfaces: Ci, classes: Cc } = Components;

function startup(aParams, aReason) {
  Components.utils.import("resource://gre/modules/Services.jsm");
  let res = Services.io.getProtocolHandler("resource")
                       .QueryInterface(Ci.nsIResProtocolHandler);
  res.setSubstitution("browser_dbg_addon5", aParams.resourceURI);

  
  Components.utils.import("resource://browser_dbg_addon5/test.jsm");
}

function shutdown(aParams, aReason) {
  
  Components.utils.unload("resource://browser_dbg_addon5/test.jsm");

  let res = Services.io.getProtocolHandler("resource")
                       .QueryInterface(Ci.nsIResProtocolHandler);
  res.setSubstitution("browser_dbg_addon5", null);
}
