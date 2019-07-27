


"use strict";




const { devtools } = Components.utils.import("resource://gre/modules/devtools/Loader.jsm", {});

Object.defineProperty(this, "NetworkHelper", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/network-helper");
  },
  configurable: true,
  writeable: false,
  enumerable: true
});

const Ci = Components.interfaces;
const wpl = Ci.nsIWebProgressListener;
const TEST_CASES = [
  {
    description: "weak cipher",
    input: wpl.STATE_IS_BROKEN | wpl.STATE_USES_WEAK_CRYPTO,
    expected: ["cipher"]
  }, {
    description: "only STATE_IS_BROKEN flag",
    input: wpl.STATE_IS_BROKEN,
    expected: []
  }, {
    description: "only STATE_IS_SECURE flag",
    input: wpl.STATE_IS_SECURE,
    expected: []
  },
];

function run_test() {
  do_print("Testing NetworkHelper.getReasonsForWeakness.");

  for (let {description, input, expected} of TEST_CASES) {
    do_print("Testing " + description);

    deepEqual(NetworkHelper.getReasonsForWeakness(input), expected,
      "Got the expected reasons for weakness.");
  }
}
