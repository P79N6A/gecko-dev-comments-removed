



"use strict";

this.EXPORTED_SYMBOLS = [
  "getAppInfo",
  "updateAppInfo",
];


const {interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let APP_INFO = {
  vendor: "Mozilla",
  name: "xpcshell",
  ID: "xpcshell@tests.mozilla.org",
  version: "1",
  appBuildID: "20121107",
  platformVersion: "p-ver",
  platformBuildID: "20121106",
  inSafeMode: false,
  logConsoleErrors: true,
  OS: "XPCShell",
  XPCOMABI: "noarch-spidermonkey",

  invalidateCachesOnRestart() {},

  
  get userCanElevate() false,

  QueryInterface(iid) {
    let interfaces = [ Ci.nsIXULAppInfo, Ci.nsIXULRuntime ];
    if ("nsIWinAppHelper" in Ci)
      interfaces.push(Ci.nsIWinAppHelper);
    if (!interfaces.some(v => iid.equals(v)))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};





this.getAppInfo = function () { return APP_INFO; }










this.updateAppInfo = function (obj) {
  obj = obj || APP_INFO;
  APP_INFO = obj;

  let id = Components.ID("{fbfae60b-64a4-44ef-a911-08ceb70b9f31}");
  let cid = "@mozilla.org/xre/app-info;1";
  let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

  
  try {
    let existing = Components.manager.getClassObjectByContractID(cid, Ci.nsIFactory);
    registrar.unregisterFactory(id, existing);
  } catch (ex) {}

  let factory = {
    createInstance: function (outer, iid) {
      if (outer != null) {
        throw Cr.NS_ERROR_NO_AGGREGATION;
      }

      return obj.QueryInterface(iid);
    },
  };

  registrar.registerFactory(id, "XULAppInfo", cid, factory);
};

