





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, WaterfallView } = panel.panelWin;

  yield startRecording(panel);

  yield waitUntil(() => WaterfallView._markers.length);

  let rendered = once(WaterfallView, EVENTS.WATERFALL_RENDERED);

  yield stopRecording(panel);

  yield rendered;
  ok(true, "WaterfallView rendered after recording is stopped.");

  ok(WaterfallView._markers.length, "WaterfallView contains markers");

  yield teardown(panel);
  finish();
}
