


"use strict";

const modules = [
];

const test_modules = [
];

function run_test() {
  for (let m of modules) {
    let resource = "resource://gre/modules/services/metrics/" + m;
    Components.utils.import(resource, {});
  }

  for (let m of test_modules) {
    let resource = "resource://testing-common/services/metrics/" + m;
    Components.utils.import(resource, {});
  }
}

