


"use strict";


do_get_profile();

(function initMetricsTestingInfrastructure() {
  let ns = {};
  Components.utils.import("resource://testing-common/services-common/logging.js",
                          ns);

  ns.initTestLogging();
}).call(this);

(function createAppInfo() {
  let ns = {};
  Components.utils.import("resource://testing-common/services/healthreport/utils.jsm", ns);
  ns.updateAppInfo();
}).call(this);




let gGlobalScope = this;
function loadAddonManager() {
  let ns = {};
  Components.utils.import("resource://gre/modules/Services.jsm", ns);
  let head = "../../../../toolkit/mozapps/extensions/test/xpcshell/head_addons.js";
  let file = do_get_file(head);
  let uri = ns.Services.io.newFileURI(file);
  ns.Services.scriptloader.loadSubScript(uri.spec, gGlobalScope);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");
  startupManager();
}

