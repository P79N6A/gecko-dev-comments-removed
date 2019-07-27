



"use strict";







const {TimelineFront} = require("devtools/server/actors/timeline");

add_task(function*() {
  let doc = yield addTab("data:text/html;charset=utf-8,mop");

  initDebuggerServer();
  let client = new DebuggerClient(DebuggerServer.connectPipe());
  let form = yield connectDebuggerClient(client);
  let front = TimelineFront(client, form);

  ok(front, "The TimelineFront was created");

  let isActive = yield front.isRecording();
  ok(!isActive, "The TimelineFront is not initially recording");

  info("Flush any pending reflows");
  let forceSyncReflow = doc.body.innerHeight;

  info("Start recording");
  yield front.start();

  isActive = yield front.isRecording();
  ok(isActive, "The TimelineFront is now recording");

  info("Change some style on the page to cause style/reflow/paint");
  let onMarkers = once(front, "markers");
  doc.body.style.padding = "10px";
  let markers = yield onMarkers;

  ok(true, "The markers event was fired");
  ok(markers.length > 0, "Markers were returned");

  info("Flush pending reflows again");
  forceSyncReflow = doc.body.innerHeight;

  info("Change some style on the page to cause style/paint");
  onMarkers = once(front, "markers");
  doc.body.style.backgroundColor = "red";
  markers = yield onMarkers;

  ok(markers.length > 0, "markers were returned");

  yield front.stop();

  isActive = yield front.isRecording();
  ok(!isActive, "Not recording after stop()");

  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});
