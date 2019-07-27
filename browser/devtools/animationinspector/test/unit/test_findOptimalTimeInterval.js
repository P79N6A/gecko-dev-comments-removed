





"use strict";

const Cu = Components.utils;
const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const {require} = devtools;

const {findOptimalTimeInterval} = require("devtools/animationinspector/utils");












const TEST_DATA = [{
  desc: "With 1px being 1ms and no minSpacing, expect the interval to be the " +
        "default min spacing",
  timeScale: 1,
  minSpacing: undefined,
  expectedInterval: 10
}, {
  desc: "With 1px being 1ms and a custom minSpacing being a multiple of 10 " +
        "expect the interval to be the custom min spacing",
  timeScale: 1,
  minSpacing: 40,
  expectedInterval: 40
}, {
  desc: "With 1px being 1ms and a custom minSpacing not being multiple of 10 " +
        "expect the interval to be the next multiple of 10",
  timeScale: 1,
  minSpacing: 13,
  expectedInterval: 20
}, {
  desc: "If 1ms corresponds to a distance that is greater than the min " +
        "spacing then, expect the interval to be this distance",
  timeScale: 20,
  minSpacing: undefined,
  expectedInterval: 20
}, {
  desc: "If 1ms corresponds to a distance that is greater than the min " +
        "spacing then, expect the interval to be this distance, even if it " +
        "isn't a multiple of 10",
  timeScale: 33,
  minSpacing: undefined,
  expectedInterval: 33
}, {
  desc: "If 1ms is a very small distance, then expect this distance to be " +
        "multiplied by 10, 20, 40, 80, etc... until it goes over the min " +
        "spacing",
  timeScale: 0.001,
  minSpacing: undefined,
  expectedInterval: 10.24
}, {
  desc: "If the time scale is such that we need to iterate more than the " +
        "maximum allowed number of iterations, then expect an interval lower " +
        "than the minimum one",
  timeScale: 1e-31,
  minSpacing: undefined,
  expectedInterval: "interval < 10"
}];

function run_test() {
  for (let {timeScale, desc, minSpacing, expectedInterval} of TEST_DATA) {
    do_print("Testing timeScale: " + timeScale + " and minSpacing: " +
              minSpacing + ". Expecting " + expectedInterval + ".");

    let interval = findOptimalTimeInterval(timeScale, minSpacing);
    if (typeof expectedInterval == "string") {
      ok(eval(expectedInterval), desc);
    } else {
      equal(interval, expectedInterval, desc);
    }
  }
}
