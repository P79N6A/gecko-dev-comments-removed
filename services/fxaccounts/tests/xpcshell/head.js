


"use strict";

(function initFxAccountsTestingInfrastructure() {
  do_get_profile();

  let ns = {};
  Components.utils.import("resource://testing-common/services-common/logging.js",
                          ns);

  ns.initTestLogging("Trace");
}).call(this);


