






let gProfilerConnections = 0;
Services.obs.addObserver(profilerConnectionObserver, "profiler-connection-created", false);

let test = Task.async(function*() {
  let firstTab = yield addTab(SIMPLE_URL);
  let firstTarget = TargetFactory.forTab(firstTab);
  yield firstTarget.makeRemote();

  let toolboxFirstTab;
  yield gDevTools.showToolbox(firstTarget, "webconsole").then((aToolbox) => {
    toolboxFirstTab = aToolbox;
  });

  is(gProfilerConnections, 1,
    "A shared profiler connection should have been created.");

  yield gDevTools.showToolbox(firstTarget, "jsprofiler");
  is(gProfilerConnections, 1,
    "No new profiler connections should have been created.");

  let secondTab = yield addTab(SIMPLE_URL);
  let secondTarget = TargetFactory.forTab(secondTab);
  yield secondTarget.makeRemote();

  let toolboxSecondTab;
  yield gDevTools.showToolbox(secondTarget, "jsprofiler").then((aToolbox) => {
    toolboxSecondTab = aToolbox;
  });

  is(gProfilerConnections, 2,
    "Only one new profiler connection should have been created.");

  yield toolboxFirstTab.destroy().then(() => {
    removeTab(firstTab);
  });
  yield toolboxSecondTab.destroy().then(() => {
    removeTab(secondTab);
    finish();
  });
});

function profilerConnectionObserver(subject, topic, data) {
  is(topic, "profiler-connection-created", "The correct topic was observed.");
  gProfilerConnections++;
}

registerCleanupFunction(() => {
  Services.obs.removeObserver(profilerConnectionObserver, "profiler-connection-created");
});
