








let test = Task.async(function*() {
  let { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  let { $, EVENTS, PerformanceController, RecordingsView } = panel.panelWin;

  yield startRecording(panel);
  yield stopRecording(panel);

  yield startRecording(panel);

  info("Selecting recording #0 and waiting for it to be displayed.");
  let select = once(PerformanceController, EVENTS.RECORDING_SELECTED);
  RecordingsView.selectedIndex = 0;
  yield select;

  ok($("#main-record-button").hasAttribute("checked"),
    "Button is still checked after selecting another item.");

  ok(!$("#main-record-button").hasAttribute("locked"),
    "Button is not locked after selecting another item.");

  yield stopRecording(panel);
  yield teardown(panel);
  finish();
});
