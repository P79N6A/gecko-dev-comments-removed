








function* spawnTest() {
  let { target, front } = yield initBackend(SIMPLE_URL, {
    TEST_MOCK_PROFILER_CHECK_TIMER: 10,
    TEST_PROFILER_FILTER_STATUS: ["position", "totalSize", "generation"]
  });

  let model = yield front.startRecording();
  let count = 0;
  while (count < 5) {
    let [_, stats] = yield onceSpread(front._profiler, "profiler-status");
    is(stats.generation, void 0, "profiler-status has void `generation`");
    is(stats.totalSize, void 0, "profiler-status has void `totalSize`");
    is(stats.position, void 0, "profiler-status has void `position`");
    count++;
  }

  is(model.getBufferUsage(), null, "model should have `null` for its buffer usage");
  yield front.stopRecording(model);
  is(model.getBufferUsage(), null, "after recording, model should still have `null` for its buffer usage");

  yield removeTab(target.tab);
  finish();
}
