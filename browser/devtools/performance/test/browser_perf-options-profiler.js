






function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { gFront } = panel.panelWin;

  ok(!nsIProfilerModule.IsActive(),
    "ensure profiler is stopped.");

  Services.prefs.setIntPref(PROFILER_BUFFER_SIZE_PREF, 1000);
  Services.prefs.setIntPref(PROFILER_SAMPLE_RATE_PREF, 2);

  yield startRecording(panel);

  let { entries, interval } = yield gFront._request("profiler", "getStartOptions");

  yield stopRecording(panel);

  is(entries, 1000, "profiler entries option is set on profiler");
  is(interval, 0.5, "profiler interval option is set on profiler");

  yield teardown(panel);
  finish();
}
