




const TEST_DATA = [];
let {LineGraphWidget} = Cu.import("resource:///modules/devtools/Graphs.jsm", {});
let {DOMHelpers} = Cu.import("resource:///modules/devtools/DOMHelpers.jsm", {});
let {Promise} = devtools.require("resource://gre/modules/Promise.jsm");
let {Hosts} = devtools.require("devtools/framework/toolbox-hosts");

let test = Task.async(function*() {
  yield promiseTab("about:blank");
  yield performTest();
  gBrowser.removeCurrentTab();
  finish();
});

function* performTest() {
  let [host, win, doc] = yield createHost();
  let graph = new LineGraphWidget(doc.body, "fps");

  yield testGraph(graph);

  graph.destroy();
  host.destroy();
}

function* testGraph(graph) {
  yield graph.setDataWhenReady(TEST_DATA);

  is(graph._gutter.hidden, false,
    "The gutter should not be hidden.");
  is(graph._maxTooltip.hidden, true,
    "The max tooltip should be hidden.");
  is(graph._avgTooltip.hidden, true,
    "The avg tooltip should be hidden.");
  is(graph._minTooltip.hidden, true,
    "The min tooltip should be hidden.");
}
