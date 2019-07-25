

if (this.do_load_httpd_js == null) {
  Cc = Components.classes;
  Ci = Components.interfaces;
  Cr = Components.results;
  Cu = Components.utils;
  Cu.import("resource://harness/modules/httpd.js");
}
else {
  do_load_httpd_js();
  do_get_profile();
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

try {
  
  Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
}
catch(ex) {


let OS = "XPCShell";
if ("@mozilla.org/windows-registry-key;1" in Cc)
  OS = "WINNT";
else if ("nsILocalFileMac" in Ci)
  OS = "Darwin";
else
  OS = "Linux";

let XULAppInfo = {
  vendor: "Mozilla",
  name: "XPCShell",
  ID: "{3e3ba16c-1675-4e88-b9c8-afef81b3d2ef}",
  version: "1",
  appBuildID: "20100621",
  platformVersion: "",
  platformBuildID: "20100621",
  inSafeMode: false,
  logConsoleErrors: true,
  OS: OS,
  XPCOMABI: "noarch-spidermonkey",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIXULAppInfo, Ci.nsIXULRuntime])
};

let XULAppInfoFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return XULAppInfo.QueryInterface(iid);
  }
};

let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{fbfae60b-64a4-44ef-a911-08ceb70b9f31}"),
                          "XULAppInfo", "@mozilla.org/xre/app-info;1",
                          XULAppInfoFactory);

}
