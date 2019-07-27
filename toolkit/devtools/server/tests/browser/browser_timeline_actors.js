



"use strict";




const {TimelineFront} = require("devtools/server/actors/timeline");

let test = asyncTest(function*() {
  let doc = yield addTab("data:text/html;charset=utf-8,mop");

  initDebuggerServer();
  let client = new DebuggerClient(DebuggerServer.connectPipe());
  let form = yield connectDebuggerClient(client);
  let front = TimelineFront(client, form);

  info("Start timeline marker recording");
  yield front.start({ withMemory: true, withTicks: true });

  let updatedMemory = 0;
  let updatedTicks = 0;

  front.on("memory", (delta, measurement) => {
    ok(delta > 0, "The delta should be a timestamp.");
    ok(measurement, "The measurement should not be null.");
    ok(measurement.total > 0, "There should be a 'total' value in the measurement.");
    info("Received 'memory' event at " + delta + " with " + measurement.toSource());
    updatedMemory++;
  });

  front.on("ticks", (delta, ticks) => {
    ok(delta > 0, "The delta should be a timestamp.");
    ok(ticks, "The ticks should not be null.");
    info("Received 'ticks' event with " + ticks.toSource());
    updatedTicks++;
  });

  ok((yield waitUntil(() => updatedMemory > 1)),
    "Some memory measurements were emitted.");
  ok((yield waitUntil(() => updatedTicks > 1)),
    "Some refresh driver ticks were emitted.");

  info("Stop timeline marker recording");
  yield front.stop();
  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});









function waitUntil(predicate, interval = 10) {
  if (predicate()) {
    return Promise.resolve(true);
  }
  return new Promise(resolve =>
    setTimeout(function() {
      waitUntil(predicate).then(() => resolve(true));
    }, interval));
}
