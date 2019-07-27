







let test = Task.async(function*() {
  let [target, debuggee, panel] = yield initFrontend(SIMPLE_URL);
  let { EVENTS } = panel.panelWin;

  yield startRecording(panel);

  let loadingNoticeDisplayed = panel.panelWin.once(EVENTS.LOADING_NOTICE_SHOWN);
  let recordingDisplayed = panel.panelWin.once(EVENTS.RECORDING_DISPLAYED);
  yield stopRecording(panel, { waitForDisplay: false });

  yield loadingNoticeDisplayed;
  ok(true, "The loading noticed was briefly displayed.");

  yield recordingDisplayed;
  ok(true, "The recording was finally displayed.");

  yield teardown(panel);
  finish();
});
