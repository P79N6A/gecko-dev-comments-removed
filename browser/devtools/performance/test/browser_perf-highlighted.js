







let { getPerformanceActorsConnection } = devtools.require("devtools/performance/front");

function spawnTest () {
  let profilerConnected = waitForProfilerConnection();
  let { target, toolbox, console } = yield initConsole(SIMPLE_URL);
  yield profilerConnected;
  let connection = getPerformanceActorsConnection(target);
  let tab = toolbox.doc.getElementById("toolbox-tab-performance");

  let profileStart = once(connection, "console-profile-start");
  console.profile("rust");
  yield profileStart;

  ok(tab.hasAttribute("highlighted"),
    "performance tab is highlighted during recording from console.profile when unloaded");

  let profileEnd = once(connection, "console-profile-end");
  console.profileEnd("rust");
  yield profileEnd;

  ok(!tab.hasAttribute("highlighted"),
    "performance tab is no longer highlighted when console.profile recording finishes");

  yield gDevTools.showToolbox(target, "performance");
  let panel = toolbox.getCurrentPanel();
  let { panelWin: { PerformanceController, RecordingsView }} = panel;

  yield startRecording(panel);

  ok(tab.hasAttribute("highlighted"),
    "performance tab is highlighted during recording while in performance tool");

  yield stopRecording(panel);

  ok(!tab.hasAttribute("highlighted"),
    "performance tab is no longer highlighted when recording finishes");

  yield teardown(panel);
  finish();
}
