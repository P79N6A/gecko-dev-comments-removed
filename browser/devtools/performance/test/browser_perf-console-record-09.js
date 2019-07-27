







function* spawnTest() {
  loadFrameScripts();
  let { target, toolbox, panel } = yield initPerformance(SIMPLE_URL);
  let win = panel.panelWin;
  let { gFront, PerformanceController } = win;

  info("Starting console.profile()...");
  yield consoleProfile(win);
  yield PerformanceController.clearRecordings();

  info("Ending console.profileEnd()...");
  consoleMethod("profileEnd");
  
  yield once(gFront, "recording-stopped");

  
  
  yield idleWait(100);
  ok(true, "Stopping an in-progress console profile after clearing recordings does not throw.");

  yield teardown(panel);
  finish();
}
