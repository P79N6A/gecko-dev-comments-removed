



































const EXPORTED_SYMBOLS = ['Weave'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

addModuleAlias("weave", "{340c2bbc-ce74-4362-90b5-7c26312808ef}");
Cu.import("resource://weave/service.js");

let Weave = {};

Weave.Service = new WeaveSyncService();

function addModuleAlias(alias, extensionId) {
  let ioSvc = Cc["@mozilla.org/network/io-service;1"]
    .getService(Ci.nsIIOService);
  let resProt = ioSvc.getProtocolHandler("resource")
    .QueryInterface(Ci.nsIResProtocolHandler);

  if (!resProt.hasSubstitution(alias)) {
    let extMgr = Cc["@mozilla.org/extensions/manager;1"]
      .getService(Ci.nsIExtensionManager);
    let loc = extMgr.getInstallLocation(extensionId);
    let extD = loc.getItemLocation(extensionId);
    extD.append("modules");
    resProt.setSubstitution(alias, ioSvc.newFileURI(extD));
  }
}
