







let test = Task.async(function*() {
  let { panel: firstPanel } = yield initPerformance(SIMPLE_URL);
  let firstFront = firstPanel.panelWin.gFront;

  let activated = firstFront.once("profiler-activated");
  yield firstFront.startRecording();
  yield activated;

  let { panel: secondPanel } = yield initPerformance(SIMPLE_URL);
  let secondFront = secondPanel.panelWin.gFront;
  loadFrameScripts();

  let alreadyActive = secondFront.once("profiler-already-active");
  yield secondFront.startRecording();
  yield alreadyActive;

  
  let tab1 = firstPanel.target.tab;
  let tab2 = secondPanel.target.tab;
  yield firstPanel._toolbox.destroy();
  yield removeTab(tab1);
  ok((yield PMM_isProfilerActive()),
    "The built-in profiler module should still be active.");

  yield secondPanel._toolbox.destroy();
  ok(!(yield PMM_isProfilerActive()),
    "The built-in profiler module should no longer be active.");
  yield removeTab(tab2);
  tab1 = tab2 = null;

  finish();
});
