








function spawnTest () {
  let { target, front } = yield initBackend(SIMPLE_URL, { TEST_MOCK_BUFFER_CHECK_TIMER: 10 });
  let frontBufferStatusCalled = false;

  
  
  let isActive = front._connection._profiler.isActive;
  front._connection._profiler.isActive = function () {
    return isActive.apply(front._connection._profiler, arguments).then(res => {
      return { isActive: res.isActive, currentTime: res.currentTime };
    });
  };

  front.on("buffer-status", () => frontBufferStatusCalled = true);
  let model = yield front.startRecording();
  let [_, stats] = yield onceSpread(front._connection._profiler, "buffer-status");
  is(stats.generation, void 0, "buffer-status has void `generation`");
  is(stats.totalSize, void 0, "buffer-status has void `totalSize`");
  is(stats.position, void 0, "buffer-status has void `position`");

  let count = 0;
  while (count < 5) {
    let [_, stats] = yield onceSpread(front._connection._profiler, "buffer-status");
    is(stats.generation, void 0, "buffer-status has void `generation`");
    is(stats.totalSize, void 0, "buffer-status has void `totalSize`");
    is(stats.position, void 0, "buffer-status has void `position`");
    count++;
  }

  is(model.getBufferUsage(), null, "model should have `null` for its buffer usage");
  yield front.stopRecording(model);
  is(model.getBufferUsage(), null, "after recording, model should still have `null` for its buffer usage");
  ok(!frontBufferStatusCalled, "the front should never emit a buffer-status event when not supported.");

  yield removeTab(target.tab);
  finish();
}
