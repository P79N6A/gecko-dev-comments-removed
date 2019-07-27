




let TEST_DATA = [{ color: "#f00", blocks: [{ x: 0, y: 0, width: 50, height: 20, text: "FOO" }, { x: 50, y: 0, width: 100, height: 20, text: "BAR" }] }, { color: "#00f", blocks: [{ x: 0, y: 30, width: 30, height: 20, text: "BAZ" }] }];
let TEST_WIDTH = 200;
let TEST_HEIGHT = 100;

let {FlameGraph} = Cu.import("resource:///modules/devtools/FlameGraph.jsm", {});
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
  doc.body.setAttribute("style", "position: fixed; width: 100%; height: 100%; margin: 0;");

  let graph = new FlameGraph(doc.body, 1);
  graph.fixedWidth = TEST_WIDTH;
  graph.fixedHeight = TEST_HEIGHT;

  yield graph.ready();

  testGraph(graph);

  graph.destroy();
  host.destroy();
}

function testGraph(graph) {
  graph.setData(TEST_DATA);

  is(graph.getDataWindowStart(), 0,
    "The selection start boundary is correct (1).");
  is(graph.getDataWindowEnd(), TEST_WIDTH,
    "The selection end boundary is correct (1).");

  scroll(graph, 200, HORIZONTAL_AXIS, 10);
  is(graph.getDataWindowStart() | 0, 100,
    "The selection start boundary is correct (2).");
  is(graph.getDataWindowEnd() | 0, 300,
    "The selection end boundary is correct (2).");

  scroll(graph, -200, HORIZONTAL_AXIS, 10);
  is(graph.getDataWindowStart() | 0, 0,
    "The selection start boundary is correct (3).");
  is(graph.getDataWindowEnd() | 0, 200,
    "The selection end boundary is correct (3).");

  scroll(graph, 200, VERTICAL_AXIS, TEST_WIDTH / 2);
  is(graph.getDataWindowStart() | 0, 0,
    "The selection start boundary is correct (4).");
  is(graph.getDataWindowEnd() | 0, 207,
    "The selection end boundary is correct (4).");

  scroll(graph, -200, VERTICAL_AXIS, TEST_WIDTH / 2);
  is(graph.getDataWindowStart() | 0, 7,
    "The selection start boundary is correct (5).");
  is(graph.getDataWindowEnd() | 0, 199,
    "The selection end boundary is correct (5).");

  dragStart(graph, TEST_WIDTH / 2);
  is(graph.getDataWindowStart() | 0, 7,
    "The selection start boundary is correct (6).");
  is(graph.getDataWindowEnd() | 0, 199,
    "The selection end boundary is correct (6).");

  hover(graph, TEST_WIDTH / 2 - 10);
  is(graph.getDataWindowStart() | 0, 16,
    "The selection start boundary is correct (7).");
  is(graph.getDataWindowEnd() | 0, 209,
    "The selection end boundary is correct (7).");

  dragStop(graph, 10);
  is(graph.getDataWindowStart() | 0, 93,
    "The selection start boundary is correct (8).");
  is(graph.getDataWindowEnd() | 0, 286,
    "The selection end boundary is correct (8).");
}



function hover(graph, x, y = 1) {
  x /= window.devicePixelRatio;
  y /= window.devicePixelRatio;
  graph._onMouseMove({ clientX: x, clientY: y });
}

function dragStart(graph, x, y = 1) {
  x /= window.devicePixelRatio;
  y /= window.devicePixelRatio;
  graph._onMouseMove({ clientX: x, clientY: y });
  graph._onMouseDown({ clientX: x, clientY: y });
}

function dragStop(graph, x, y = 1) {
  x /= window.devicePixelRatio;
  y /= window.devicePixelRatio;
  graph._onMouseMove({ clientX: x, clientY: y });
  graph._onMouseUp({ clientX: x, clientY: y });
}

let HORIZONTAL_AXIS = 1;
let VERTICAL_AXIS = 2;

function scroll(graph, wheel, axis, x, y = 1) {
  x /= window.devicePixelRatio;
  y /= window.devicePixelRatio;
  graph._onMouseMove({ clientX: x, clientY: y });
  graph._onMouseWheel({ clientX: x, clientY: y, axis, detail: wheel, axis,
    HORIZONTAL_AXIS,
    VERTICAL_AXIS
  });
}
