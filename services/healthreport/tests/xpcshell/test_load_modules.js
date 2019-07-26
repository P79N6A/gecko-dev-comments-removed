


"use strict";

const modules = [
  "healthreporter.jsm",
  "profile.jsm",
  "providers.jsm",
];

function run_test() {
  for (let m of modules) {
    let resource = "resource://gre/modules/services/healthreport/" + m;
    Components.utils.import(resource, {});
  }

  Components.utils.import("resource://gre/modules/HealthReport.jsm", {});
}

