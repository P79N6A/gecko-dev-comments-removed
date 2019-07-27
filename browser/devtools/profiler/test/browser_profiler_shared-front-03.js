







let test = Task.async(function*() {
  let [,, firstPanel] = yield initFrontend(SIMPLE_URL);
  let firstFront = firstPanel.panelWin.gFront;

  let activated = firstFront.once("profiler-activated");
  yield firstFront.startRecording();
  yield activated;

  let [,, secondPanel] = yield initFrontend(SIMPLE_URL);
  let secondFront = secondPanel.panelWin.gFront;

  let alreadyActive = secondFront.once("profiler-already-active");
  yield secondFront.startRecording();
  yield alreadyActive;

  yield teardown(firstPanel);
  ok(nsIProfilerModule.IsActive(),
    "The built-in profiler module should still be active.");

  yield teardown(secondPanel);
  ok(!nsIProfilerModule.IsActive(),
    "The built-in profiler module should have been automatically stoped.");

  finish();
});
