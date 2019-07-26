


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

