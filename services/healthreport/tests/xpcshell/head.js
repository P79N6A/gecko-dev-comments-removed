


"use strict";


do_get_profile();

(function initMetricsTestingInfrastructure() {
  let ns = {};
  Components.utils.import("resource://testing-common/services-common/logging.js",
                          ns);

  ns.initTestLogging();
}).call(this);

