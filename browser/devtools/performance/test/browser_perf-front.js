






let WAIT = 1000;

function spawnTest () {
  let { target, front } = yield initBackend(SIMPLE_URL);

  let { startTime } = yield front.startRecording();

  ok(typeof startTime === "number", "front.startRecording() emits start time");

  yield busyWait(WAIT);

  let { endTime } = yield front.stopRecording();

  ok(typeof endTime === "number", "front.stopRecording() emits end time");
  ok(endTime > startTime, "endTime is after startTime");

  yield removeTab(target.tab);
  finish();

}
