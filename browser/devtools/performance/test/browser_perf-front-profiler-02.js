








let test = Task.async(function*() {
  let { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  let front = panel.panelWin.gFront;
  loadFrameScripts();

  ok(!(yield PMM_isProfilerActive()),
    "The built-in profiler module should not have been automatically started.");

  let activated = front.once("profiler-activated");
  let rec = yield front.startRecording();
  yield activated;
  yield front.stopRecording(rec);
  ok((yield PMM_isProfilerActive()),
    "The built-in profiler module should still be active (1).");

  let alreadyActive = front.once("profiler-already-active");
  rec = yield front.startRecording();
  yield alreadyActive;
  yield front.stopRecording(rec);
  ok((yield PMM_isProfilerActive()),
    "The built-in profiler module should still be active (2).");

  
  let tab = panel.target.tab;
  yield panel._toolbox.destroy();
  ok(!(yield PMM_isProfilerActive()),
    "The built-in profiler module should no longer be active.");
  yield removeTab(tab);
  tab = null;

  finish();
});
