

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const RecordingUtils = devtools.require("devtools/performance/recording-utils");

const PLATFORM_DATA_PREF = "devtools.performance.ui.show-platform-data";




function getFrameNodePath(root, path) {
  let calls = root.calls;
  let node;
  for (let key of path.split(" > ")) {
    node = calls.find((node) => node.key == key);
    if (!node) {
      break;
    }
    calls = node.calls;
  }
  return node;
}




function synthesizeProfileForTest(samples) {
  samples.unshift({
    time: 0,
    frames: [
      { location: "(root)" }
    ]
  });

  let uniqueStacks = new RecordingUtils.UniqueStacks();
  return RecordingUtils.deflateThread({
    samples: samples,
    markers: []
  }, uniqueStacks);
}
