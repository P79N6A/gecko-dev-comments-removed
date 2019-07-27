








let test = Task.async(function*() {
  let [target, debuggee, panel] = yield initFrontend(SIMPLE_URL);
  let { $, EVENTS, RecordingsListView, ProfileView } = panel.panelWin;

  yield startRecording(panel);
  yield stopRecording(panel, { waitForDisplay: true });

  yield startRecording(panel);
  yield stopRecording(panel, { waitForDisplay: true });

  yield startRecording(panel);

  info("Selecting recording #0 and waiting for it to be displayed.");
  let selected = panel.panelWin.once(EVENTS.RECORDING_DISPLAYED);
  RecordingsListView.selectedIndex = 0;
  yield selected;

  ok($("#record-button").hasAttribute("checked"),
    "Button is still checked after selecting another item.");

  ok(!$("#record-button").hasAttribute("locked"),
    "Button is not locked after selecting another item.");

  yield teardown(panel);
  finish();
});
