




const FAST_FPS = 60;
const SLOW_FPS = 10;

const FRAMES= [FAST_FPS, FAST_FPS, FAST_FPS, SLOW_FPS, FAST_FPS];
const TEST_DATA = [];
const INTERVAL = 100;
const DURATION = 5000; 
let t = 0;
for (let frameRate of FRAMES) {
  for (let i = 0; i < frameRate; i++) {
    let delta = Math.floor(1000 / frameRate); 
    t += delta;
    TEST_DATA.push(t);
  }
}

let {LineGraphWidget} = Cu.import("resource:///modules/devtools/Graphs.jsm", {});
let {Promise} = devtools.require("resource://gre/modules/Promise.jsm");

add_task(function*() {
  yield promiseTab("about:blank");
  yield performTest();
  gBrowser.removeCurrentTab();
});

function* performTest() {
  let [host, win, doc] = yield createHost();
  let graph = new LineGraphWidget(doc.body, "fps");

  yield testGraph(graph);

  yield graph.destroy();
  host.destroy();
}

function* testGraph(graph) {

  console.log("test data", TEST_DATA);
  yield graph.setDataFromTimestamps(TEST_DATA, INTERVAL, DURATION);
  is(graph._avgTooltip.querySelector("[text=value]").textContent, "50",
    "The average tooltip displays the correct value.");
}
